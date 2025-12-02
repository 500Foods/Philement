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
#include <unistd.h>       /* sysconf for page size */
#include <libgen.h>       /* basename() function */
#include <limits.h>       /* PATH_MAX constant */
#include <errno.h>        /* Error handling */
#include <stdlib.h>       /* malloc/free */
#include <stdio.h>        /* FILE operations */
#include <fcntl.h>        /* File control */
#include <sys/mman.h>     /* Memory mapping */
#include <sys/stat.h>     /* File status */
#include <sys/types.h>    /* Type definitions */
#include <signal.h>       /* Signal handling */
#include <stdint.h>       /* Fixed-width integer types */
#include <stddef.h>       /* Standard definitions */

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
 * ELF Note Alignment Constants
 * These constants define the alignment requirements for ELF notes
 */
#define NOTE_ALIGN 4

/*
 * Memory Mapping Constants
 */
#define MAX_MAPPINGS 256
#define MAX_LOAD_SEGMENTS 256
#define COPY_BUF_SIZE (64 * 1024)

/*
 * Core Mapping Structure
 * Represents a memory mapping from /proc/self/maps
 */
typedef struct {
    unsigned long start;
    unsigned long end;
    unsigned long offset;   // file offset from /proc/self/maps
    char perms[5];          // "r-xp", "rw-p", etc.
    char path[PATH_MAX];    // file path or "" for anon
    int is_stack;           // 1 if "[stack]"
    int is_vdso;            // 1 if "[vdso]"
} CoreMapping;

/*
 * Load Segment Structure
 * Represents a PT_LOAD segment for the core dump
 */
typedef struct {
    CoreMapping *m;
    Elf64_Phdr phdr;
} LoadSegment;

/*
 * File Entry Structure
 * Represents a file-backed memory mapping for NT_FILE
 */
typedef struct {
    unsigned long start;
    unsigned long end;
    unsigned long file_offset; // raw offset from maps
    const char *path;          // pointer into CoreMapping.path
} FileEntry;

/*
 * Forward declarations for helper functions
 */
static size_t read_proc_maps(CoreMapping *out, size_t max);
static void dump_segment(FILE *mem, FILE *out, const LoadSegment *seg);
static size_t read_auxv_data(unsigned char *buf, size_t max_size);

/**
 * @brief Parse /proc/self/maps into an array of memory mappings
 *
 * This function reads the process memory map and parses each line into
 * a structured CoreMapping entry. It captures all memory regions including
 * executable segments, stack, heap, shared libraries, and anonymous mappings.
 *
 * @param out Array to store parsed mappings
 * @param max Maximum number of mappings to store
 * @return Number of mappings successfully parsed
 */
static size_t read_proc_maps(CoreMapping *out, size_t max) {
    FILE *maps = fopen("/proc/self/maps", "r");
    if (!maps) {
        log_this(SR_CRASH, "Failed to open /proc/self/maps: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
        return 0;
    }

    char line[512];
    size_t count = 0;

    while (fgets(line, sizeof(line), maps) && count < max) {
        CoreMapping *m = &out[count];
        unsigned long start, end, offset;
        char perms[5] = "";

        // Parse the maps line format: address perms offset dev inode pathname
        int parsed = sscanf(line, "%lx-%lx %4s %lx %*x:%*x %*d %*s",
                           &start, &end, perms, &offset);

        if (parsed < 3) continue;

        m->start = start;
        m->end = end;
        m->offset = offset;
        strncpy(m->perms, perms, sizeof(m->perms) - 1);
        m->perms[sizeof(m->perms) - 1] = '\0';

        // Extract path if present
        char *path_start = strchr(line, '/');
        if (path_start) {
            // Copy from the first '/' to end of line (excluding newline)
            char *path_end = strchr(path_start, '\n');
            if (path_end) *path_end = '\0';
            strncpy(m->path, path_start, sizeof(m->path) - 1);
            m->path[sizeof(m->path) - 1] = '\0';
        } else {
            // Check for special mappings like [stack], [vdso], etc.
            if (strstr(line, "[stack]")) {
                strcpy(m->path, "[stack]");
                m->is_stack = 1;
            } else if (strstr(line, "[vdso]")) {
                strcpy(m->path, "[vdso]");
                m->is_vdso = 1;
            } else if (strstr(line, "[heap]")) {
                strcpy(m->path, "[heap]");
            } else {
                m->path[0] = '\0'; // Anonymous mapping
            }
        }

        count++;
    }

    fclose(maps);
    return count;
}


/**
 * @brief Dump a memory segment to the core file
 *
 * This function handles the robust copying of memory from /proc/self/mem
 * to the core dump file, using chunked I/O to handle large segments efficiently.
 *
 * @param mem FILE pointer to /proc/self/mem
 * @param out FILE pointer to the core dump file
 * @param seg LoadSegment containing the mapping and program header info
 */
static void dump_segment(FILE *mem, FILE *out, const LoadSegment *seg) {
    const CoreMapping *m = seg->m;
    const Elf64_Phdr *ph = &seg->phdr;
    unsigned long addr = m->start;
    size_t size = m->end - m->start;
    unsigned char buf[COPY_BUF_SIZE];

    if (fseeko(out, (off_t)ph->p_offset, SEEK_SET) != 0) {
        log_this(SR_CRASH, "Failed to seek in core file for segment %lx-%lx: %s",
                LOG_LEVEL_ERROR, 3, m->start, m->end, strerror(errno));
        return;
    }

    if (fseeko(mem, (off_t)addr, SEEK_SET) != 0) {
        log_this(SR_CRASH, "Failed to seek in /proc/self/mem for segment %lx-%lx: %s",
                LOG_LEVEL_ERROR, 3, m->start, m->end, strerror(errno));
        return;
    }

    while (size > 0) {
        size_t chunk = size < COPY_BUF_SIZE ? size : COPY_BUF_SIZE;
        size_t r = fread(buf, 1, chunk, mem);
        if (r == 0) {
            if (errno == 0) {
                // End of readable memory
                break;
            } else {
                log_this(SR_CRASH, "Failed to read memory for segment %lx-%lx: %s",
                        LOG_LEVEL_ERROR, 3, m->start, m->end, strerror(errno));
                break;
            }
        }

        size_t w = fwrite(buf, 1, r, out);
        if (w != r) {
            log_this(SR_CRASH, "Partial write for segment %lx-%lx: %zu/%zu bytes",
                    LOG_LEVEL_ALERT, 4, m->start, m->end, w, r);
            break;
        }

        size -= r;
    }
}

/**
 * @brief Read auxiliary vector data from /proc/self/auxv
 *
 * This function reads the auxiliary vector which contains information
 * about the process execution environment, including shared library paths,
 * platform information, and other runtime details.
 *
 * @param buf Buffer to store the auxiliary vector data
 * @param max_size Maximum size of the buffer
 * @return Number of bytes read, or 0 on failure
 */
static size_t read_auxv_data(unsigned char *buf, size_t max_size) {
    FILE *auxv = fopen("/proc/self/auxv", "rb");
    if (!auxv) {
        log_this(SR_CRASH, "Failed to open /proc/self/auxv: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
        return 0;
    }

    size_t bytes_read = fread(buf, 1, max_size, auxv);
    fclose(auxv);
    return bytes_read;
}

// Global includes
#include <src/hydrogen.h>
#include <src/config/config.h>

/**
 * @brief Test function to simulate a crash for testing the crash handler
 * @param sig Signal number (unused, but required for signal handler signature)
 * @note This handler is only for testing and should not be enabled in production
 */
void test_crash_handler(int sig) {
    (void)sig; // Suppress unused parameter warning
    // Simulate a crash by raising SIGSEGV
    raise(SIGSEGV);
}

/**
 * @brief Signal handler to dump current configuration
 * @param sig Signal number (unused, but required for signal handler signature)
 */
void config_dump_handler(int sig) {
    (void)sig; // Suppress unused parameter warning

    if (app_config == NULL) {
        log_this(SR_CONFIG, "Configuration dump requested but app_config is NULL", LOG_LEVEL_ERROR, 0);
        return;
    }

    // Dump the configuration
    dumpAppConfig(app_config, "signal-handler");
}

/**
 * @brief Handles critical program crashes by generating a detailed core dump
 *
 * This handler is triggered by fatal signals (SIGSEGV, SIGABRT, SIGFPE) and creates
 * a comprehensive core dump file that can be analyzed with GDB. The core dump includes:
 * - Process status (registers, signal info, etc.)
 * - Memory maps (all important segments including stack, executable, and RIP mapping)
 * - Process information
 * - Shared library information via NT_FILE and NT_AUXV notes
 *
 * The handler follows these steps:
 * 1. Identifies the executable path and creates a unique core file name
 * 2. Parses all memory mappings from /proc/self/maps
 * 3. Identifies important mappings (stack, executable, RIP location)
 * 4. Selects PT_LOAD segments dynamically
 * 5. Writes ELF headers with dynamic program header count
 * 6. Captures CPU register state and process info
 * 7. Generates comprehensive NT_FILE and NT_AUXV notes
 * 8. Dumps memory contents for all selected segments
 *
 * @param sig Signal number that triggered the handler
 * @param info Detailed signal information
 * @param ucontext CPU context at time of crash
 *
 * @note This handler uses many non-async-signal-safe functions. It is intended for
 *       best-effort debugging in a controlled environment (e.g., coverage builds),
 *       not for production-safe crash handling in fully async-signal-safe contexts.
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

    // Create core file in current working directory to avoid path length issues
    const char *exe_basename = basename(exe_path);
    snprintf(core_name, sizeof(core_name), "%s.core.%d", exe_basename, pid);

    // Also create absolute path version for logging
    char abs_core_name[PATH_MAX];
    if (getcwd(abs_core_name, sizeof(abs_core_name)) != NULL) {
        size_t dir_len = strlen(abs_core_name);
        if (dir_len + strlen("/") + strlen(core_name) + 1 < sizeof(abs_core_name)) {
            abs_core_name[dir_len] = '/';
            strcpy(abs_core_name + dir_len + 1, core_name);
        } else {
            // Fallback to relative name if absolute path is too long
            strcpy(abs_core_name, core_name);
        }
    } else {
        strcpy(abs_core_name, core_name);
    }

    log_this(SR_CRASH, "Signal %d received (cause: %d), generating core dump at %s", LOG_LEVEL_ERROR, 3, sig, info->si_code, abs_core_name);

    // Get config file path from stored program arguments (if any)
    char** program_args = get_program_args();
    const char* config_path = (program_args && program_args[1]) ? program_args[1] : "";

    // Output GDB commands IMMEDIATELY for debugging (before core dump generation)
    log_this(SR_CRASH, "Enhanced GDB analysis: gdb -batch -ex \"set pagination off\" -ex \"set print pretty on\" -ex \"set print static-members on\" -ex \"file %s\" -ex \"core-file %s\" -ex \"info sharedlibrary\" -ex \"thread apply all bt full\" -ex \"info registers\" -ex \"info locals\"", LOG_LEVEL_ERROR, 2, exe_path, abs_core_name);
    log_this(SR_CRASH, "Interactive debug: gdb -q -ex \"set print pretty on\" -ex \"file %s\" -ex \"core-file %s\" -ex \"thread apply all bt\" -ex \"info sharedlibrary\" -ex \"info locals\"", LOG_LEVEL_ERROR, 2, exe_path, abs_core_name);
    log_this(SR_CRASH, "Library analysis: gdb -batch -ex \"set pagination off\" -ex \"file %s\" -ex \"core-file %s\" -ex \"info sharedlibrary\" -ex \"info program\" -ex \"info threads\"", LOG_LEVEL_ERROR, 2, exe_path, abs_core_name);
    log_this(SR_CRASH, "Independent run: gdb -ex \"set environment MALLOC_CHECK_=3\" -ex \"catch syscall abort\" -ex \"run\" --args %s %s", LOG_LEVEL_ERROR, 2, exe_path, config_path);

    FILE *out = fopen(core_name, "w");
    if (!out) {
        log_this(SR_CRASH, "Failed to open %s: %s", LOG_LEVEL_ERROR, 2, core_name, strerror(errno));
        _exit(128 + sig);
    }

    /* Step 2: Parse all memory mappings */
    CoreMapping mappings[MAX_MAPPINGS];
    size_t mapping_count = read_proc_maps(mappings, MAX_MAPPINGS);
    if (mapping_count == 0) {
        log_this(SR_CRASH, "No memory mappings found - cannot generate core dump", LOG_LEVEL_ERROR, 0);
        fclose(out);
        _exit(128 + sig);
    }

    FILE *mem = fopen("/proc/self/mem", "r");
    if (!mem) {
        log_this(SR_CRASH, "Failed to open /proc/self/mem: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
        // cppcheck-suppress doubleFree - out is closed and set to NULL to prevent double free
        fclose(out);
        out = NULL; // Prevent double free
        _exit(128 + sig);
    }

    /* Step 3: No need to identify specific mappings since we include all readable ones */
    const ucontext_t *uc = (const ucontext_t *)ucontext;

    /* Step 4: Select PT_LOAD segments - include all readable mappings */
    LoadSegment load_segments[MAX_LOAD_SEGMENTS];
    size_t load_segment_count = 0;

    for (size_t i = 0; i < mapping_count; i++) {
        const CoreMapping *m = &mappings[i];
        if (m->perms[0] == 'r') {  // readable mapping
            load_segments[load_segment_count++].m = (CoreMapping *)m;
        }
    }

    /* Step 5: Build ELF header with dynamic program header count */
    size_t ph_count = 1 + load_segment_count; // 1 NOTE + N LOAD segments

    // Calculate note sizes - we'll add NT_AUXV if available
    size_t prstatus_desc_sz = sizeof(struct elf_prstatus);
    size_t prpsinfo_desc_sz = sizeof(struct elf_prpsinfo);

    // Calculate NT_FILE size for all file-backed mappings
    FileEntry file_entries[MAX_MAPPINGS];
    size_t file_count = 0;
    size_t filenames_total_len = 0;

    for (size_t i = 0; i < mapping_count; i++) {
        const CoreMapping *m = &mappings[i];
        if (m->path[0] == '\0' || m->path[0] == '[') continue;

        file_entries[file_count].start = m->start;
        file_entries[file_count].end = m->end;
        file_entries[file_count].file_offset = m->offset;
        file_entries[file_count].path = m->path;
        filenames_total_len += strlen(m->path) + 1;
        file_count++;
    }

    // Calculate NT_FILE desc size
    uint64_t page_size = (uint64_t)sysconf(_SC_PAGESIZE);
    size_t nt_file_desc_sz =
        sizeof(uint64_t) +                    // count
        sizeof(uint64_t) +                    // page_size
        file_count * 3 * sizeof(uint64_t) +  // triples
        filenames_total_len;                  // concatenated names with '\0'

    // Calculate note sizes with proper 4-byte alignment
    size_t prstatus_note_sz =
        sizeof(Elf64_Nhdr) + ((4 + 3) / 4 * 4) + ((prstatus_desc_sz + 3) / 4 * 4);

    size_t prpsinfo_note_sz =
        sizeof(Elf64_Nhdr) + ((4 + 3) / 4 * 4) + ((prpsinfo_desc_sz + 3) / 4 * 4);

    size_t file_note_sz =
        sizeof(Elf64_Nhdr) + ((4 + 3) / 4 * 4) + ((nt_file_desc_sz + 3) / 4 * 4);

    // Add NT_AUXV note if available
    unsigned char auxv_buf[8192];
    size_t auxv_size = read_auxv_data(auxv_buf, sizeof(auxv_buf));
    size_t auxv_note_sz = 0;

    if (auxv_size > 0) {
        size_t auxv_desc_sz = (auxv_size + 3) / 4 * 4; // Align to 4 bytes
        auxv_note_sz =
            sizeof(Elf64_Nhdr) + ((4 + 3) / 4 * 4) + auxv_desc_sz;
    }

    size_t note_size = prstatus_note_sz + prpsinfo_note_sz + file_note_sz + auxv_note_sz;

    /* Step 6: Write ELF header and program headers */
    Elf64_Ehdr ehdr = {
        .e_ident = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_SYSV},
        .e_type = ET_CORE,
        .e_machine = EM_X86_64,
        .e_version = EV_CURRENT,
        .e_phoff = sizeof(Elf64_Ehdr),
        .e_ehsize = (Elf64_Half)(sizeof(Elf64_Ehdr) & 0xFFFF),
        .e_phentsize = (Elf64_Half)(sizeof(Elf64_Phdr) & 0xFFFF),
        .e_phnum = (Elf64_Half)ph_count,
    };
    fwrite(&ehdr, sizeof(ehdr), 1, out);

    // Calculate offsets for program headers and data
    Elf64_Off current = sizeof(Elf64_Ehdr) + ph_count * sizeof(Elf64_Phdr);

    // Write program headers
    Elf64_Phdr note_phdr = {
        .p_type = PT_NOTE,
        .p_offset = current,
        .p_filesz = note_size,
        .p_memsz = note_size,
        .p_align = 4,
    };
    fwrite(&note_phdr, sizeof(note_phdr), 1, out);
    current += note_size;

    // Write PT_LOAD headers
    for (size_t i = 0; i < load_segment_count; i++) {
        const CoreMapping *m = load_segments[i].m;
        Elf64_Phdr *ph = &load_segments[i].phdr;

        // Align file offset to page size
        Elf64_Off aligned = (current + (Elf64_Off)4096 - 1) & ~((Elf64_Off)4096 - 1);
        current = aligned;

        ph->p_type = PT_LOAD;
        ph->p_offset = current;
        ph->p_vaddr = m->start;
        ph->p_paddr = m->start;
        ph->p_filesz = m->end - m->start;
        ph->p_memsz = m->end - m->start;
        ph->p_flags = 0;
        if (m->perms[0] == 'r') ph->p_flags |= PF_R;
        if (m->perms[1] == 'w') ph->p_flags |= PF_W;
        if (m->perms[2] == 'x') ph->p_flags |= PF_X;
        ph->p_align = 4096;

        fwrite(ph, sizeof(Elf64_Phdr), 1, out);
        current += ph->p_filesz;
    }

    /* Step 7: Write note data */
    fseek(out, (long)note_phdr.p_offset, SEEK_SET);

    // Write notes with proper 4-byte padding
    static const char zero_pad[NOTE_ALIGN] = {0};

    // PRSTATUS note with padding
    struct elf_prstatus prstatus;
    memset(&prstatus, 0, sizeof(prstatus));
    prstatus.pr_info.si_signo = sig;
    prstatus.pr_info.si_code = info ? info->si_code : 0;
    prstatus.pr_info.si_errno = errno;
    prstatus.pr_cursig = (short)sig;

    sigset_t pending_signals;
    sigpending(&pending_signals);
    prstatus.pr_sigpend = pending_signals.__val[0];
    prstatus.pr_pid = pid;
    prstatus.pr_ppid = getppid();
    prstatus.pr_pgrp = getpgrp();
    prstatus.pr_sid = getsid(0);
    prstatus.pr_fpvalid = 0;

    if (uc) {
        // Copy registers in the correct order for ELF core dump format (x86_64)
        prstatus.pr_reg[0] = (elf_greg_t)uc->uc_mcontext.gregs[REG_R15];
        prstatus.pr_reg[1] = (elf_greg_t)uc->uc_mcontext.gregs[REG_R14];
        prstatus.pr_reg[2] = (elf_greg_t)uc->uc_mcontext.gregs[REG_R13];
        prstatus.pr_reg[3] = (elf_greg_t)uc->uc_mcontext.gregs[REG_R12];
        prstatus.pr_reg[4] = (elf_greg_t)uc->uc_mcontext.gregs[REG_RBP];
        prstatus.pr_reg[5] = (elf_greg_t)uc->uc_mcontext.gregs[REG_RBX];
        prstatus.pr_reg[6] = (elf_greg_t)uc->uc_mcontext.gregs[REG_R11];
        prstatus.pr_reg[7] = (elf_greg_t)uc->uc_mcontext.gregs[REG_R10];
        prstatus.pr_reg[8] = (elf_greg_t)uc->uc_mcontext.gregs[REG_R9];
        prstatus.pr_reg[9] = (elf_greg_t)uc->uc_mcontext.gregs[REG_R8];
        prstatus.pr_reg[10] = (elf_greg_t)uc->uc_mcontext.gregs[REG_RAX];
        prstatus.pr_reg[11] = (elf_greg_t)uc->uc_mcontext.gregs[REG_RCX];
        prstatus.pr_reg[12] = (elf_greg_t)uc->uc_mcontext.gregs[REG_RDX];
        prstatus.pr_reg[13] = (elf_greg_t)uc->uc_mcontext.gregs[REG_RSI];
        prstatus.pr_reg[14] = (elf_greg_t)uc->uc_mcontext.gregs[REG_RDI];
        prstatus.pr_reg[15] = (elf_greg_t)uc->uc_mcontext.gregs[REG_RAX]; // orig_rax
        prstatus.pr_reg[16] = (elf_greg_t)uc->uc_mcontext.gregs[REG_RIP];
        prstatus.pr_reg[17] = (elf_greg_t)uc->uc_mcontext.gregs[REG_CSGSFS]; // CS
        prstatus.pr_reg[18] = (elf_greg_t)uc->uc_mcontext.gregs[REG_EFL];
        prstatus.pr_reg[19] = (elf_greg_t)uc->uc_mcontext.gregs[REG_RSP];
        prstatus.pr_reg[20] = (elf_greg_t)uc->uc_mcontext.gregs[REG_CSGSFS]; // SS
        prstatus.pr_reg[21] = 0; // FS_BASE
        prstatus.pr_reg[22] = 0; // GS_BASE
        prstatus.pr_reg[23] = (elf_greg_t)uc->uc_mcontext.gregs[REG_CSGSFS]; // DS
        prstatus.pr_reg[24] = (elf_greg_t)uc->uc_mcontext.gregs[REG_CSGSFS]; // ES
        prstatus.pr_reg[25] = (elf_greg_t)uc->uc_mcontext.gregs[REG_CSGSFS]; // FS
        prstatus.pr_reg[26] = (elf_greg_t)uc->uc_mcontext.gregs[REG_CSGSFS]; // GS
    }

    Elf64_Nhdr nhdr1 = {.n_namesz = 4, .n_descsz = (Elf64_Word)prstatus_desc_sz, .n_type = NT_PRSTATUS};
    fwrite(&nhdr1, sizeof(nhdr1), 1, out);
    fwrite("CORE", 4, 1, out);
    fwrite(&prstatus, sizeof(prstatus), 1, out);
    {
        size_t pad = ((prstatus_desc_sz + 3) / 4 * 4) - prstatus_desc_sz;
        if (pad) fwrite(zero_pad, pad, 1, out);
    }

    // PRPSINFO note
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
    snprintf(prpsinfo.pr_fname, sizeof(prpsinfo.pr_fname), "%s", exe_basename);
    snprintf(prpsinfo.pr_psargs, sizeof(prpsinfo.pr_psargs), "%s", exe_basename);

    Elf64_Nhdr nhdr2 = {.n_namesz = 4, .n_descsz = (Elf64_Word)prpsinfo_desc_sz, .n_type = NT_PRPSINFO};
    fwrite(&nhdr2, sizeof(nhdr2), 1, out);
    fwrite("CORE", 4, 1, out);
    fwrite(&prpsinfo, sizeof(prpsinfo), 1, out);
    {
        size_t pad = ((prpsinfo_desc_sz + 3) / 4 * 4) - prpsinfo_desc_sz;
        if (pad) fwrite(zero_pad, pad, 1, out);
    }

    // NT_FILE note with all file-backed mappings
    Elf64_Nhdr nhdr3 = {
        .n_namesz = 4,
        .n_descsz = (Elf64_Word)nt_file_desc_sz,
        .n_type = NT_FILE
    };
    fwrite(&nhdr3, sizeof(nhdr3), 1, out);
    fwrite("CORE", 4, 1, out);

    // Write NT_FILE desc: count, page_size, mappings, filename
    uint64_t count64 = (uint64_t)file_count;
    fwrite(&count64, sizeof(count64), 1, out);
    fwrite(&page_size, sizeof(page_size), 1, out);

    for (size_t i = 0; i < file_count; i++) {
        uint64_t start = (uint64_t)file_entries[i].start;
        uint64_t end = (uint64_t)file_entries[i].end;
        uint64_t poffs = (uint64_t)(file_entries[i].file_offset / page_size);
        fwrite(&start, sizeof(start), 1, out);
        fwrite(&end, sizeof(end), 1, out);
        fwrite(&poffs, sizeof(poffs), 1, out);
    }

    for (size_t i = 0; i < file_count; i++) {
        const char *name = file_entries[i].path;
        size_t name_len = strlen(name) + 1;
        fwrite(name, name_len, 1, out);
    }

    // Pad NT_FILE desc to 4-byte alignment
    {
        size_t pad = ((nt_file_desc_sz + 3) / 4 * 4) - nt_file_desc_sz;
        if (pad) fwrite(zero_pad, pad, 1, out);
    }

    // NT_AUXV note if available
    if (auxv_size > 0) {
        size_t auxv_desc_sz = (auxv_size + 3) / 4 * 4; // Align to 4 bytes
        Elf64_Nhdr nhdr_auxv = {
            .n_namesz = 4,
            .n_descsz = (Elf64_Word)auxv_desc_sz,
            .n_type = NT_AUXV
        };
        fwrite(&nhdr_auxv, sizeof(nhdr_auxv), 1, out);
        fwrite("CORE", 4, 1, out);
        fwrite(auxv_buf, auxv_size, 1, out);
        if (auxv_desc_sz > auxv_size) {
            fwrite(zero_pad, auxv_desc_sz - auxv_size, 1, out);
        }
    }

    /* Step 8: Dump memory for all PT_LOAD segments */
    for (size_t i = 0; i < load_segment_count; i++) {
        LoadSegment *seg = &load_segments[i];
        seg->phdr = *(Elf64_Phdr *)&load_segments[i].phdr; // Copy the header we wrote earlier
        dump_segment(mem, out, seg);
    }

    // Validate that we actually captured memory before closing
    long current_pos = ftell(out);
    if (current_pos <= (long)(sizeof(Elf64_Ehdr) + ph_count * sizeof(Elf64_Phdr) + note_size)) {
        log_this(SR_CRASH, "WARNING: Core dump appears to contain no memory data - investigation needed", LOG_LEVEL_ERROR, 0);
    }

    fclose(mem);
    fclose(out);

#ifdef HYDROGEN_COVERAGE_BUILD
    __gcov_dump();
#endif

    _exit(128 + sig);
}
