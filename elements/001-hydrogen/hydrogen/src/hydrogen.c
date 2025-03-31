/*
 * Hydrogen Server
 * 
 * This is the main entry point for the Hydrogen Server.
 * 
 */

 #include <unistd.h>
 #include <sys/types.h>
 #include <sys/time.h>
 #include <sys/signal.h>
 #include <signal.h>
 #include <pthread.h>
 #include <errno.h>
 #include <time.h>
 #include <stdbool.h>
 #include <string.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <sys/prctl.h>
 #include <sys/resource.h>
 #include <limits.h>
 #include <elf.h>
 #include <ucontext.h>
 
 #include "logging/logging.h"
 #include "state/state.h"
 #include "state/startup/startup.h"
 #include "state/shutdown/shutdown.h"
 #include "threads/threads.h"
 
 // External thread tracking structures
 extern ServiceThreads logging_threads;
 
 // Global main thread ID for thread tracking
 pthread_t main_thread_id;
 
 /*
  * crash_handler - Custom signal handler to generate an ELF core dump on crash
  *
  * This function catches fatal signals (SIGSEGV, SIGABRT, SIGFPE) and creates a
  * custom ELF core file named "<appname>.core.<pid>" (e.g., "hydrogen.core.12345")
  * in the current directory. It includes memory segments from /proc/self/maps,
  * a PT_NOTE with process info (PID, signal, app name), and NT_PRSTATUS with
  * register state for full debugging.
  *
  * Usage with Debugging Tools:
  * 1. GDB:
  *    - Automatic: `gdb ./hydrogen` then `core-file hydrogen.core.<pid>`
  *    - Manual: `gdb ./hydrogen hydrogen.core.<pid>`
  *    - If GDB doesn’t recognize it automatically, ensure the file is in the
  *      same directory as the binary and use the manual command.
  *    - Run `bt` for a full backtrace (includes registers like RIP, RSP).
  *
  * 2. Valgrind:
  *    - Valgrind doesn’t directly use core files, but analyze live crashes:
  *      `valgrind --vgdb=yes --vgdb-error=0 ./hydrogen`
  *    - Trigger the crash (e.g., kill -SEGV <pid>), then connect GDB:
  *      `gdb ./hydrogen -q -x /tmp/vgdb-pipe`
  *    - The core file supplements this with memory and register state.
  *
  * Notes:
  * - Includes PT_LOAD (memory), PT_NOTE (process info), and NT_PRSTATUS (registers).
  * - Some memory regions may fail to dump (logged as "Failed to read...").
  * - File size is capped by RLIMIT_CORE (10 MB default).
  */
 static void crash_handler(int sig, siginfo_t *info, void *ucontext) {
     char core_name[PATH_MAX];
     const char *base = strrchr(program_invocation_name, '/');
     base = base ? base + 1 : program_invocation_name;
     int pid = getpid();
     snprintf(core_name, sizeof(core_name), "./%s.core.%d", base, pid);
 
     // Use info to log signal cause
     log_this("Crash", "Signal %d received (cause: %d), generating core dump at %s", 
              LOG_LEVEL_ERROR, sig, info->si_code, core_name);
 
     FILE *out = fopen(core_name, "w");
     if (!out) {
         log_this("Crash", "Failed to open %s: %s", LOG_LEVEL_ERROR, core_name, strerror(errno));
         _exit(128 + sig);
     }
 
     FILE *maps = fopen("/proc/self/maps", "r");
     FILE *mem = fopen("/proc/self/mem", "r");
     if (!maps || !mem) {
         log_this("Crash", "Failed to open /proc/self/maps or /mem: %s", LOG_LEVEL_ERROR, strerror(errno));
         goto cleanup;
     }
 
     // Count memory segments for program headers
     int phnum = 0;
     char line[256];
     while (fgets(line, sizeof(line), maps)) {
         if (strstr(line, " r")) phnum++;  // Count readable segments
     }
     rewind(maps);
 
     // ELF header
     Elf64_Ehdr ehdr = {
         .e_ident = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS64, ELFDATA2LSB, 1, ELFOSABI_SYSV, 0},
         .e_type = ET_CORE,
         .e_machine = EM_X86_64,
         .e_version = EV_CURRENT,
         .e_phoff = sizeof(Elf64_Ehdr),
         .e_ehsize = sizeof(Elf64_Ehdr),
         .e_phentsize = sizeof(Elf64_Phdr),
         .e_phnum = phnum + 2,  // +2 for PT_NOTE (PRPSINFO + PRSTATUS)
         .e_shentsize = 0,
         .e_shnum = 0,
         .e_shstrndx = 0
     };
     fwrite(&ehdr, sizeof(ehdr), 1, out);
 
     // Program headers
     // PT_NOTE (PRPSINFO)
     off_t note_offset = sizeof(Elf64_Ehdr) + (phnum + 2) * sizeof(Elf64_Phdr);
     Elf64_Phdr prpsinfo_phdr = {
         .p_type = PT_NOTE,
         .p_flags = PF_R,
         .p_offset = note_offset,
         .p_vaddr = 0,
         .p_paddr = 0,
         .p_filesz = sizeof(Elf64_Nhdr) + 8 + strlen(base) + 1 + 16,
         .p_memsz = sizeof(Elf64_Nhdr) + 8 + strlen(base) + 1 + 16,
         .p_align = 4
     };
     fwrite(&prpsinfo_phdr, sizeof(prpsinfo_phdr), 1, out);
 
     // PT_NOTE (PRSTATUS)
     off_t prstatus_offset = note_offset + prpsinfo_phdr.p_filesz;
     Elf64_Phdr prstatus_phdr = {
         .p_type = PT_NOTE,
         .p_flags = PF_R,
         .p_offset = prstatus_offset,
         .p_vaddr = 0,
         .p_paddr = 0,
         .p_filesz = sizeof(Elf64_Nhdr) + 8 + sizeof(gregset_t),
         .p_memsz = sizeof(Elf64_Nhdr) + 8 + sizeof(gregset_t),
         .p_align = 4
     };
     fwrite(&prstatus_phdr, sizeof(prstatus_phdr), 1, out);
 
     // Memory segments
     off_t data_offset = prstatus_offset + prstatus_phdr.p_filesz;
     while (fgets(line, sizeof(line), maps)) {
         unsigned long start, end;
         char perms[5];
         if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3 || !strchr(perms, 'r')) continue;
 
         Elf64_Phdr phdr = {
             .p_type = PT_LOAD,
             .p_flags = (strchr(perms, 'r') ? PF_R : 0) | (strchr(perms, 'w') ? PF_W : 0) | (strchr(perms, 'x') ? PF_X : 0),
             .p_offset = data_offset,
             .p_vaddr = start,
             .p_paddr = start,
             .p_filesz = end - start,
             .p_memsz = end - start,
             .p_align = 0x1000
         };
         fwrite(&phdr, sizeof(phdr), 1, out);
 
         data_offset += phdr.p_filesz;
     }
 
     // Write PT_NOTE (PRPSINFO)
     fseek(out, prpsinfo_phdr.p_offset, SEEK_SET);
     Elf64_Nhdr nhdr_prpsinfo = {
         .n_namesz = strlen("CORE") + 1,
         .n_descsz = strlen(base) + 1 + 16,
         .n_type = NT_PRPSINFO
     };
     fwrite(&nhdr_prpsinfo, sizeof(nhdr_prpsinfo), 1, out);
     fwrite("CORE", nhdr_prpsinfo.n_namesz, 1, out);
     fwrite(base, strlen(base) + 1, 1, out);
     struct { int pid; int sig; char pad[8]; } prpsinfo = {pid, sig, {0}};
     fwrite(&prpsinfo, sizeof(prpsinfo), 1, out);
 
     // Write PT_NOTE (PRSTATUS)
     fseek(out, prstatus_phdr.p_offset, SEEK_SET);
     Elf64_Nhdr nhdr_prstatus = {
         .n_namesz = strlen("CORE") + 1,
         .n_descsz = sizeof(gregset_t),
         .n_type = NT_PRSTATUS
     };
     fwrite(&nhdr_prstatus, sizeof(nhdr_prstatus), 1, out);
     fwrite("CORE", nhdr_prstatus.n_namesz, 1, out);
     ucontext_t *uc = (ucontext_t *)ucontext;
     fwrite(&uc->uc_mcontext.gregs, sizeof(gregset_t), 1, out);
 
     // Write memory data
     rewind(maps);
     while (fgets(line, sizeof(line), maps)) {
         unsigned long start, end;
         char perms[5];
         if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3 || !strchr(perms, 'r')) continue;
 
         if (fseek(mem, start, SEEK_SET) == -1) {
             log_this("Crash", "Failed to seek to %lx: %s", LOG_LEVEL_ERROR, start, strerror(errno));
             continue;
         }
         size_t size = end - start;
         char *buf = malloc(size);
         if (!buf) {
             log_this("Crash", "Failed to alloc %zu bytes", LOG_LEVEL_ERROR, size);
             continue;
         }
         size_t read = fread(buf, 1, size, mem);
         if (read > 0) {
             fwrite(buf, 1, read, out);
         } else {
             log_this("Crash", "Failed to read %zu bytes at %lx: %s", LOG_LEVEL_ERROR, size, start, strerror(errno));
         }
         free(buf);
     }
 
     log_this("Crash", "Core dump written to %s", LOG_LEVEL_ERROR, core_name);
 
 cleanup:
     if (maps) fclose(maps);
     if (mem) fclose(mem);
     fclose(out);
     _exit(128 + sig);
 }
 
 int main(int argc, char *argv[]) {
     if (prctl(PR_SET_DUMPABLE, 1) == -1) {
         log_this("Main", "Failed to set dumpable: %s", LOG_LEVEL_ERROR, strerror(errno));
     }
 
     struct rlimit core_limit = { 10 * 1024 * 1024, 10 * 1024 * 1024 };
     if (setrlimit(RLIMIT_CORE, &core_limit) == -1) {
         log_this("Main", "Failed to enable core dumps: %s", LOG_LEVEL_ERROR, strerror(errno));
     }
 
     main_thread_id = pthread_self();
 
     struct sigaction sa;
     sa.sa_handler = signal_handler;
     sigfillset(&sa.sa_mask);
     sa.sa_flags = SA_RESTART | SA_NODEFER;
     if (sigaction(SIGINT, &sa, NULL) == -1 ||
         sigaction(SIGTERM, &sa, NULL) == -1 ||
         sigaction(SIGHUP, &sa, NULL) == -1) {
         fprintf(stderr, "Failed to set up signal handlers\n");
         return 1;
     }
 
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