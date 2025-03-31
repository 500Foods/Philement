/**
 * @file hydrogen.c
 * @brief Main entry point for the Hydrogen 3D Printer Control Server
 *
 * This file implements the core server functionality including:
 * - Server initialization and shutdown
 * - Signal handling for graceful termination
 * - Advanced crash handling with detailed core dump generation
 * - Main event loop management
 *
 * The crash handling system captures detailed state information when crashes occur,
 * generating an ELF format core dump with stack traces and register states to aid
 * in post-mortem debugging.
 *
 * @note Thread Safety: This module handles process-wide signals and must be
 * initialized before any threads are created. The crash handler is async-signal-safe.
 */

/* Standard C Library */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

/* POSIX & System */
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <libgen.h>

/* Signal Handling & Debug */
#include <signal.h>
#include <sys/signal.h>
#include <ucontext.h>
#include <sys/ucontext.h>
#include <elf.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <linux/limits.h>

/* Internal Headers */
#include "logging/logging.h"
#include "state/state.h"
#include "state/startup/startup.h"
#include "state/shutdown/shutdown.h"
#include "threads/threads.h"

/* Global Variables */
extern ServiceThreads logging_threads;
pthread_t main_thread_id;

/*
 * ELF Core Dump Structures
 * These structures define the format of the core dump file generated
 * during crash handling. They follow the ELF specification for core dumps.
 */

/**
 * @brief Process status information for core dump
 * 
 * Contains detailed process state including:
 * - Signal information that caused the crash
 * - Process/thread identifiers
 * - CPU register state at time of crash
 * - Resource usage statistics
 */
struct elf_prstatus {
     struct {
         int si_signo;
         int si_code;
         int si_errno;
     } pr_info;
     short pr_cursig;
     unsigned long pr_sigpend;
     unsigned long pr_sighold;
     pid_t pr_pid;
     pid_t pr_ppid;
     pid_t pr_pgrp;
     pid_t pr_sid;
     struct timeval pr_utime, pr_stime, pr_cutime, pr_cstime;
     struct user_regs_struct pr_reg;
     int pr_fpvalid;
 };

 struct elf_prpsinfo {
    char pr_state;         // Numeric process state
    char pr_sname;         // Char representation of pr_state
    char pr_zomb;          // Zombie status
    char pr_nice;          // Nice value
    unsigned long pr_flag; // Process flags
    uint32_t pr_uid;       // Real user ID
    uint32_t pr_gid;       // Real group ID
    int pr_pid;            // Process ID
    int pr_ppid;           // Parent process ID
    int pr_pgrp;           // Process group ID
    int pr_sid;            // Session ID
    char pr_fname[16];     // Short command name
    char pr_psargs[80];    // Full command line
};

/**
 * @brief Handles critical program crashes by generating a detailed core dump
 *
 * This handler is triggered by fatal signals (SIGSEGV, SIGABRT, SIGFPE) and creates
 * a comprehensive core dump file that can be analyzed with GDB. The core dump includes:
 * - Process status (registers, signal info, etc.)
 * - Memory maps (stack and code segments)
 * - Process information
 *
 * The handler follows these steps:
 * 1. Identifies the executable path and creates a unique core file name
 * 2. Opens necessary proc files (/proc/self/maps, /proc/self/mem)
 * 3. Locates stack and code segments in memory
 * 4. Writes ELF headers and program segments
 * 5. Captures CPU register state and process info
 * 6. Dumps memory contents (stack and code)
 *
 * @param sig Signal number that triggered the handler
 * @param info Detailed signal information
 * @param ucontext CPU context at time of crash
 *
 * @note This handler is async-signal-safe and uses only async-signal-safe functions
 * @note The generated core file can be analyzed with: gdb -q executable corefile
 */
static void crash_handler(int sig, siginfo_t *info, void *ucontext) {
    /* Step 1: Get executable path and create core file name */
    char exe_path[PATH_MAX];
    char core_name[PATH_MAX];

    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        log_this("Crash", "Failed to read /proc/self/exe: %s", LOG_LEVEL_ERROR, strerror(errno));
        _exit(128 + sig);
    }
    exe_path[len] = '\0';

    int pid = getpid();
    if (strnlen(exe_path, PATH_MAX) + 32 >= sizeof(core_name)) {
        log_this("Crash", "Path too long for core filename", LOG_LEVEL_ERROR);
        _exit(128 + sig);
    }
    size_t needed = strlen(exe_path) + strlen(".core.") + 20; // 20 for PID digits + null terminator
    if (needed >= sizeof(core_name)) {
        log_this("Crash", "Path too long for core filename", LOG_LEVEL_ERROR);
        _exit(128 + sig);
    }
    snprintf(core_name, sizeof(core_name), "%s.core.%d", exe_path, pid);

    log_this("Crash", "Signal %d received (cause: %d), generating core dump at %s",
             LOG_LEVEL_ERROR, sig, info->si_code, core_name);

    FILE *out = fopen(core_name, "w");
    if (!out) {
        log_this("Crash", "Failed to open %s: %s", LOG_LEVEL_ERROR, core_name, strerror(errno));
        _exit(128 + sig);
    }

    /* Step 2: Open process memory maps for analysis */
    FILE *maps = fopen("/proc/self/maps", "r");
    FILE *mem = fopen("/proc/self/mem", "r");
    if (!maps || !mem) {
        log_this("Crash", "Failed to open /proc/self/{maps,mem}: %s", LOG_LEVEL_ERROR, strerror(errno));
        if (maps) fclose(maps);
        if (mem) fclose(mem);
        fclose(out);
        _exit(128 + sig);
    }

    /* Step 3: Locate stack and code segments in memory map */
    unsigned long stack_start = 0, stack_end = 0;
    unsigned long code_start = 0, code_end = 0;
    char line[256], *base = basename(exe_path);
    while (fgets(line, sizeof(line), maps)) {
        unsigned long start, end;
        char perms[5], path[256] = "";
        if (sscanf(line, "%lx-%lx %4s %*x %*x:%*x %*d %255s", &start, &end, perms, path) < 3) continue;
        if (strstr(line, "[stack]")) {
            stack_start = start;
            stack_end = end;
        } else if (strstr(perms, "r-x") && strstr(path, base)) {
            code_start = start;
            code_end = end;
        }
        if (stack_start && code_start) break;
    }
    fclose(maps);

    size_t stack_size = stack_end - stack_start;
    size_t code_size = code_end - code_start;

    /* Step 4: Create ELF header and program segments
     * The ELF header identifies this as a core dump file and contains:
     * - ELF magic number and file class (64-bit)
     * - Target architecture (x86_64)
     * - Program header table information
     */
    Elf64_Ehdr ehdr = {
        .e_ident = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_SYSV},
        .e_type = ET_CORE,
        .e_machine = EM_X86_64,
        .e_version = EV_CURRENT,
        .e_phoff = sizeof(Elf64_Ehdr),
        .e_ehsize = sizeof(Elf64_Ehdr),
        .e_phentsize = sizeof(Elf64_Phdr),
        .e_phnum = 3,
    };
    fwrite(&ehdr, sizeof(ehdr), 1, out);

    size_t prstatus_sz = sizeof(Elf64_Nhdr) + 4 + sizeof(struct elf_prstatus);
    size_t prpsinfo_sz = sizeof(Elf64_Nhdr) + 4 + sizeof(struct elf_prpsinfo);
    size_t note_size = prstatus_sz + prpsinfo_sz;

    off_t offset = sizeof(Elf64_Ehdr) + 3 * sizeof(Elf64_Phdr);
    Elf64_Phdr note_phdr = {
        .p_type = PT_NOTE,
        .p_offset = offset,
        .p_filesz = note_size,
        .p_memsz = note_size,
        .p_align = 4,
    };
    offset += note_size;

    Elf64_Phdr stack_phdr = {
        .p_type = PT_LOAD,
        .p_flags = PF_R | PF_W,
        .p_offset = offset,
        .p_vaddr = stack_start,
        .p_paddr = stack_start,
        .p_memsz = stack_size,
        .p_align = 4096,
    };
    offset += stack_size;

    Elf64_Phdr code_phdr = {
        .p_type = PT_LOAD,
        .p_flags = PF_R | PF_X,
        .p_offset = offset,
        .p_vaddr = code_start,
        .p_paddr = code_start,
        .p_memsz = code_size,
        .p_align = 4096,
    };

    fwrite(&note_phdr, sizeof(note_phdr), 1, out);
    fwrite(&stack_phdr, sizeof(stack_phdr), 1, out);
    fwrite(&code_phdr, sizeof(code_phdr), 1, out);

    fseek(out, note_phdr.p_offset, SEEK_SET);

    /* Step 5: Capture process state and CPU context
     * This section captures:
     * - CPU register values at time of crash
     * - Signal information and pending signals
     * - Process hierarchy information (PID, PPID, etc.)
     */
    ucontext_t *uc = (ucontext_t *)ucontext;
    sigset_t pending_signals;
    sigpending(&pending_signals);

    struct elf_prstatus prstatus = {
        .pr_info = {sig, info->si_code, errno},
        .pr_cursig = sig,
        .pr_sigpend = pending_signals.__val[0],
        .pr_pid = pid,
        .pr_ppid = getppid(),
        .pr_pgrp = getpgrp(),
        .pr_sid = getsid(0),
        .pr_fpvalid = 0,
        .pr_reg = {
            .rip = uc->uc_mcontext.gregs[REG_RIP],
            .rsp = uc->uc_mcontext.gregs[REG_RSP],
            .rbp = uc->uc_mcontext.gregs[REG_RBP],
            .rax = uc->uc_mcontext.gregs[REG_RAX],
            .rbx = uc->uc_mcontext.gregs[REG_RBX],
            .rcx = uc->uc_mcontext.gregs[REG_RCX],
            .rdx = uc->uc_mcontext.gregs[REG_RDX],
            .rsi = uc->uc_mcontext.gregs[REG_RSI],
            .rdi = uc->uc_mcontext.gregs[REG_RDI],
            .r8  = uc->uc_mcontext.gregs[REG_R8],
            .r9  = uc->uc_mcontext.gregs[REG_R9],
            .r10 = uc->uc_mcontext.gregs[REG_R10],
            .r11 = uc->uc_mcontext.gregs[REG_R11],
            .r12 = uc->uc_mcontext.gregs[REG_R12],
            .r13 = uc->uc_mcontext.gregs[REG_R13],
            .r14 = uc->uc_mcontext.gregs[REG_R14],
            .r15 = uc->uc_mcontext.gregs[REG_R15],
        }
    };

    Elf64_Nhdr nhdr1 = {.n_namesz = 4, .n_descsz = sizeof(prstatus), .n_type = NT_PRSTATUS};
    fwrite(&nhdr1, sizeof(nhdr1), 1, out);
    fwrite("CORE", 4, 1, out);
    fwrite(&prstatus, sizeof(prstatus), 1, out);

    struct elf_prpsinfo prpsinfo = {
        .pr_state = 0,
        .pr_sname = 'R',
        .pr_pid = pid,
        .pr_ppid = getppid(),
        .pr_pgrp = getpgrp(),
        .pr_sid = getsid(0),
        .pr_uid = getuid(),
        .pr_gid = getgid()
    };
    snprintf(prpsinfo.pr_fname, sizeof(prpsinfo.pr_fname), "%s", base);
    snprintf(prpsinfo.pr_psargs, sizeof(prpsinfo.pr_psargs), "%s", base);

    Elf64_Nhdr nhdr2 = {.n_namesz = 4, .n_descsz = sizeof(prpsinfo), .n_type = NT_PRPSINFO};
    fwrite(&nhdr2, sizeof(nhdr2), 1, out);
    fwrite("CORE", 4, 1, out);
    fwrite(&prpsinfo, sizeof(prpsinfo), 1, out);

    /* Step 6: Write memory contents
     * Dumps the actual memory contents of the process:
     * - Stack segment: Contains local variables and call frames
     * - Code segment: Contains the executable instructions
     * Memory is read directly from process space using /proc/self/mem
     */
    size_t stack_written = 0;
    if (stack_start && stack_end) {
        fseek(out, stack_phdr.p_offset, SEEK_SET);
        fseek(mem, stack_start, SEEK_SET);
        char *buf = malloc(stack_size);
        if (buf) {
            stack_written = fread(buf, 1, stack_size, mem);
            fwrite(buf, 1, stack_written, out);
            free(buf);
        }
    }

    // Write code
    size_t code_written = 0;
    if (code_start && code_end) {
        fseek(out, code_phdr.p_offset, SEEK_SET);
        fseek(mem, code_start, SEEK_SET);
        char *buf = malloc(code_size);
        if (buf) {
            code_written = fread(buf, 1, code_size, mem);
            fwrite(buf, 1, code_written, out);
            free(buf);
        }
    }

    fclose(mem);
    fclose(out);

    log_this("Crash", "Run: gdb -q %s %s", LOG_LEVEL_ERROR, exe_path, core_name);

    _exit(128 + sig);
}
 
/**
 * @brief Test function to simulate a crash for testing the crash handler
 *
 * This function is triggered by SIGUSR1 and deliberately causes a segmentation fault
 * by dereferencing a null pointer. This allows testing of the crash handler in a
 * controlled manner.
 *
 * @param sig Signal number (unused, but required for signal handler signature)
 * @note This handler is only for testing and should not be enabled in production
 */
static void test_crash_handler(int sig) {
    (void)sig;
    log_this("Crash", "Received SIGUSR1, simulating crash via null dereference", LOG_LEVEL_ERROR);
    volatile int *ptr = NULL;
    *ptr = 42; // Triggers SIGSEGV, which your crash_handler will catch
}

/**
 * @brief Main entry point for the Hydrogen server
 *
 * Initializes the server in the following order:
 * 1. Enables core dump generation
 * 2. Sets up signal handlers for:
 *    - Normal termination (SIGINT, SIGTERM, SIGHUP)
 *    - Crash handling (SIGSEGV, SIGABRT, SIGFPE)
 *    - Testing (SIGUSR1)
 * 3. Starts the server with optional configuration
 * 4. Enters the main event loop
 * 5. Handles graceful shutdown
 *
 * @param argc Argument count
 * @param argv Argument vector, argv[1] may contain config path
 * @return 0 on success, 1 on initialization failure
 */
int main(int argc, char *argv[]) {
     if (prctl(PR_SET_DUMPABLE, 1) == -1) {
         log_this("Main", "Failed to set dumpable: %s", LOG_LEVEL_ERROR, strerror(errno));
     }
 
     struct rlimit core_limit = { 10 * 1024 * 1024, 10 * 1024 * 1024 };
     if (setrlimit(RLIMIT_CORE, &core_limit) == -1) {
         log_this("Main", "Failed to enable core dumps: %s", LOG_LEVEL_ERROR, strerror(errno));
     }
 
     main_thread_id = pthread_self();
 
     /* Initialize signal handlers
      * Three types of signals are handled:
      * 1. Normal termination signals (SIGINT, SIGTERM, SIGHUP)
      *    - Used for graceful shutdown
      *    - SA_RESTART: Automatically restart interrupted system calls
      *    - SA_NODEFER: Allow signal to be received again while in handler
      */
     struct sigaction sa;
     sa.sa_handler = signal_handler; // Assuming signal_handler is defined elsewhere
     sigfillset(&sa.sa_mask);
     sa.sa_flags = SA_RESTART | SA_NODEFER;
     if (sigaction(SIGINT, &sa, NULL) == -1 ||
         sigaction(SIGTERM, &sa, NULL) == -1 ||
         sigaction(SIGHUP, &sa, NULL) == -1) {
         fprintf(stderr, "Failed to set up signal handlers\n");
         return 1;
     }
 
     /* 2. Fatal crash signals (SIGSEGV, SIGABRT, SIGFPE)
      *    - Handled by crash_handler with detailed core dump generation
      *    - SA_SIGINFO: Provides extended signal information
      *    - SA_RESTART: Automatically restart interrupted system calls
      */
     struct sigaction sa_crash;
     sa_crash.sa_sigaction = crash_handler;
     sigemptyset(&sa_crash.sa_mask);
     sa_crash.sa_flags = SA_SIGINFO | SA_RESTART;
     if (sigaction(SIGSEGV, &sa_crash, NULL) == -1 ||
         sigaction(SIGABRT, &sa_crash, NULL) == -1 ||
         sigaction(SIGFPE, &sa_crash, NULL) == -1) {
         log_this("Main", "Failed to set crash signal handlers: %s", LOG_LEVEL_ERROR, strerror(errno));
         return 1;
     }

     /* 3. Test signal handler (SIGUSR1)
      *    - Used for development/testing of crash handler
      *    - Triggers a controlled crash via null pointer dereference
      *    - SA_RESTART: Automatically restart interrupted system calls
      */
     struct sigaction sa_test;
     sa_test.sa_handler = test_crash_handler;
     sigemptyset(&sa_test.sa_mask);
     sa_test.sa_flags = SA_RESTART;
     if (sigaction(SIGUSR1, &sa_test, NULL) == -1) {
         log_this("Main", "Failed to set test crash signal handler: %s", LOG_LEVEL_ERROR, strerror(errno));
     }

     char* config_path = (argc > 1) ? argv[1] : NULL;
     if (!startup_hydrogen(config_path)) {
         return 1;
     }
 
     struct timespec ts;
     while (server_running) {
         clock_gettime(CLOCK_REALTIME, &ts);
         ts.tv_sec += 1;
         pthread_mutex_lock(&terminate_mutex);
         int wait_result = pthread_cond_timedwait(&terminate_cond, &terminate_mutex, &ts);
         pthread_mutex_unlock(&terminate_mutex);
         if (wait_result != 0 && wait_result != ETIMEDOUT) {
             log_this("Main", "Unexpected error in main event loop: %d", LOG_LEVEL_ERROR, wait_result);
         }
     }
 
     add_service_thread(&logging_threads, main_thread_id);
     graceful_shutdown();
     remove_service_thread(&logging_threads, main_thread_id);
 
     return 0;
 }