/**
 * @file handlers.c
 *
 * @brief Implementation of signal and crash handling functions
 *
 * This file implements the signal handler functions for crash handling,
 * testing, and configuration dumping in the Hydrogen server.
 */

// Global includes
#include <src/hydrogen.h>
#include "handlers.h"

// System headers - must come first after feature test macros
#include <locale.h>       /* Locale settings */
#include <features.h>     /* GNU/glibc features */
#include <ucontext.h>     /* Context handling */
#include <string.h>       /* String functions */

// Extended POSIX headers
#include <sys/ucontext.h> /* Context handling */
#include <sys/procfs.h>   /* Process info */
#include <sys/prctl.h>    /* Process control */

// Process and threading
#include <elf.h>         /* ELF format */

#ifdef HYDROGEN_COVERAGE_BUILD
    #include <gcov.h>
#endif

// External declarations
extern AppConfig *app_config;

/*
 * ELF Core Dump Structures
 * These structures define the format of the core dump file generated
 * during crash handling. They follow the ELF specification for core dumps.
 */

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
// cppcheck-suppress[constParameterCallback] - Signal handler callback signature requires void*
// cppcheck-suppress[constParameterPointer] - Signal handler API requires non-const void* parameter
void crash_handler(int sig, siginfo_t *info, void *ucontext) {
    /* Step 1: Get executable path and create core file name */
    char exe_path[PATH_MAX];
    char core_name[PATH_MAX];

    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        log_this(SR_CRASH, "Failed to read /proc/self/exe: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
        _exit(128 + sig);
    }
    exe_path[len] = '\0';

    int pid = getpid();
    if (strnlen(exe_path, PATH_MAX) + 32 >= sizeof(core_name)) {
        log_this(SR_CRASH, "Path too long for core filename", LOG_LEVEL_ERROR, 0);
        _exit(128 + sig);
    }
    size_t needed = strlen(exe_path) + strlen(".core.") + 20; // 20 for PID digits + null terminator
    if (needed >= sizeof(core_name)) {
        log_this(SR_CRASH, "Path too long for core filename", LOG_LEVEL_ERROR, 0);
        _exit(128 + sig);
    }
    snprintf(core_name, sizeof(core_name), "%.*s.core.%d", (int)(sizeof(core_name) - 20), exe_path, pid);

    log_this(SR_CRASH, "Signal %d received (cause: %d), generating core dump at %s", LOG_LEVEL_ERROR, 3, sig, info->si_code, core_name);
    
    // Get config file path from stored program arguments (if any)
    char** program_args = get_program_args();
    const char* config_path = (program_args && program_args[1]) ? program_args[1] : "";
    
    // Output GDB commands IMMEDIATELY for debugging (before core dump generation)
    log_this(SR_CRASH, "Automated analysis: gdb -batch -ex \"set pagination off\" -ex \"info sharedlibrary\" -ex \"thread apply all bt full\" -ex \"info registers\" %s %s", LOG_LEVEL_ERROR, 2, exe_path, core_name);
    log_this(SR_CRASH, "Interactive debug: gdb -q -ex \"thread apply all bt\" -ex \"info sharedlibrary\" -ex \"info locals\" %s %s", LOG_LEVEL_ERROR, 2, exe_path, core_name);
    log_this(SR_CRASH, "Independent run: gdb -ex \"set environment MALLOC_CHECK_=3\" -ex \"catch syscall abort\" -ex \"run\" --args %s %s", LOG_LEVEL_ERROR, 2, exe_path, config_path);

    FILE *out = fopen(core_name, "w");
    if (!out) {
        log_this(SR_CRASH, "Failed to open %s: %s", LOG_LEVEL_ERROR, 2, core_name, strerror(errno));
        _exit(128 + sig);
    }

    /* Step 2: Open process memory maps for analysis */
    FILE *maps = fopen("/proc/self/maps", "r");
    FILE *mem = fopen("/proc/self/mem", "r");
    if (!maps || !mem) {
        log_this(SR_CRASH, "Failed to open /proc/self/{maps,mem}: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
        if (maps) fclose(maps);
        if (mem) fclose(mem);
        fclose(out);
        _exit(128 + sig);
    }

    /* Step 3: Locate stack and code segments in memory map */
    unsigned long stack_start = 0, stack_end = 0;
    unsigned long code_start = 0, code_end = 0;
    char line[256];
    const char *base = basename(exe_path);
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
        .e_ehsize = (Elf64_Half)(sizeof(Elf64_Ehdr) & 0xFFFF),
        .e_phentsize = (Elf64_Half)(sizeof(Elf64_Phdr) & 0xFFFF),
        .e_phnum = 3,
    };
    fwrite(&ehdr, sizeof(ehdr), 1, out);

    size_t prstatus_sz = sizeof(Elf64_Nhdr) + 4 + sizeof(struct elf_prstatus);
    size_t prpsinfo_sz = sizeof(Elf64_Nhdr) + 4 + sizeof(struct elf_prpsinfo);
    size_t note_size = prstatus_sz + prpsinfo_sz;

    Elf64_Off offset = sizeof(Elf64_Ehdr) + 3 * sizeof(Elf64_Phdr);
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

    fseek(out, (long)note_phdr.p_offset, SEEK_SET);

    /* Step 5: Capture process state and CPU context
     * This section captures:
     * - CPU register values at time of crash
     * - Signal information and pending signals
     * - Process hierarchy information (PID, PPID, etc.)
     */
    const ucontext_t *uc = (const ucontext_t *)ucontext;
    if (!uc) {
        log_this(SR_CRASH, "Invalid ucontext in crash handler", LOG_LEVEL_ERROR, 0);
        fclose(mem);
        fclose(out);
        _exit(128 + sig);
    }
    sigset_t pending_signals;
    sigpending(&pending_signals);

    struct elf_prstatus prstatus;
    memset(&prstatus, 0, sizeof(prstatus));
    prstatus.pr_info.si_signo = sig;
    prstatus.pr_info.si_code = info ? info->si_code : 0;
    prstatus.pr_info.si_errno = errno;
    prstatus.pr_cursig = (short)sig;
    prstatus.pr_sigpend = pending_signals.__val[0];
    prstatus.pr_pid = pid;
    prstatus.pr_ppid = getppid();
    prstatus.pr_pgrp = getpgrp();
    prstatus.pr_sid = getsid(0);
    prstatus.pr_fpvalid = 0;
    if (uc) {
        memcpy(&prstatus.pr_reg, uc->uc_mcontext.gregs, sizeof(prstatus.pr_reg));
    }

    Elf64_Nhdr nhdr1 = {.n_namesz = 4, .n_descsz = sizeof(prstatus), .n_type = NT_PRSTATUS};
    fwrite(&nhdr1, sizeof(nhdr1), 1, out);
    fwrite("CORE", 4, 1, out);
    fwrite(&prstatus, sizeof(prstatus), 1, out);

    struct elf_prpsinfo prpsinfo = {
        .pr_state = 0,
        .pr_sname = 'R',
        .pr_pid = (pid_t)pid,
        .pr_ppid = (pid_t)getppid(),
        .pr_pgrp = (pid_t)getpgrp(),
        .pr_sid = (pid_t)getsid(0),
        .pr_uid = (uid_t)getuid(),
        .pr_gid = (gid_t)getgid()
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
    if (stack_start && stack_end) {
        fseek(out, (long)stack_phdr.p_offset, SEEK_SET);
        fseek(mem, (long)stack_start, SEEK_SET);
        char *buf = malloc(stack_size);
        if (buf) {
            size_t stack_written = fread(buf, 1, stack_size, mem);
            fwrite(buf, 1, stack_written, out);
            free(buf);
        }
    }

    // Write code
    if (code_start && code_end) {
        fseek(out, (long)code_phdr.p_offset, SEEK_SET);
        fseek(mem, (long)code_start, SEEK_SET);
        char *buf = malloc(code_size);
        if (buf) {
            size_t code_written = fread(buf, 1, code_size, mem);
            fwrite(buf, 1, code_written, out);
            free(buf);
        }
    }

    fclose(mem);
    fclose(out);

#ifdef HYDROGEN_COVERAGE_BUILD
    __gcov_dump();
#endif

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
void test_crash_handler(int sig) {
    (void)sig;
    log_this(SR_CRASH, "Received SIGUSR1, simulating crash via null dereference", LOG_LEVEL_ERROR, 0);
    volatile int *ptr = NULL;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
    // cppcheck-suppress[nullPointer] Intentional null dereference to trigger test crash
    *ptr = 42; // Triggers SIGSEGV, which your crash_handler will catch
#pragma GCC diagnostic pop
}

/**
 * @brief Signal handler to dump current configuration
 *
 * This function is triggered by SIGUSR2 and dumps the current application
 * configuration to the log. It provides a way to inspect the running
 * configuration without restarting the server.
 *
 * @param sig Signal number (unused, but required for signal handler signature)
 */
void config_dump_handler(int sig) {
    (void)sig;
    log_this(SR_CONFIG_CURRENT, "Received SIGUSR2, dumping current configuration", LOG_LEVEL_STATE, 0);
    if (app_config) {
        dumpAppConfig(app_config, NULL);
    } else {
        log_this(SR_CONFIG_CURRENT, "No configuration available to dump", LOG_LEVEL_ERROR, 0);
    }
}