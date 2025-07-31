# Tritium: Process Monitoring and Profiling Tool

## Summary

Tritium is a lightweight C program designed to monitor and profile the process tree spawned by the `test_00_all.sh` orchestration script, part of the Hydrogen project ecosystem. It replaces the slow `strace` tool by tracking process creation/termination (via Netlink Connector) and collecting CPU/memory usage (via `/proc`) for ~12,124 processes (~20-30 active at a time, including `cmake` subprocesses) over a 20-second run. Tritium aggregates resource usage per top-level script (~25 scripts, 200-300ms to 7s runtimes) and produces:

- A `strace`-like summary of forks and command invocations (e.g., `bash: 2410`, `cmake: 34`) per script.
- JSON output for D3.js charting, detailing CPU/memory samples per script at 0.01s precision.
- The JSON is processed by a Node.js script using D3.js to generate SVG files (`cpu_script_<script_pid>.svg`, `mem_script_<script_pid>.svg`) for CPU and memory usage over time.

The project uses `uthash` for efficient PID tracking, Jansson for JSON output, and minimizes overhead (<0.5% CPU) compared to `strace`’s 10-100x slowdown. Development proceeds in ~20 incremental steps, each producing a testable version, followed by a D3.js-based visualization step.

## Implementation Steps

### Step 1: Basic Program Launch and Termination

**Description**: Create a minimal C program (`Tritium`) that accepts the parent PID as a command-line argument, runs in the background, and exits cleanly on SIGTERM from `test_00_all.sh`.
**Implementation**:

- Parse `argv[1]` as `parent_pid`.
- Set up a SIGTERM handler to exit with status 0.
- Run an infinite loop to keep the program alive.
- Output a simple log to `/dev/shm/tritium.log` confirming start/stop.
**Code Outline**:

```c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t running = 1;

void handle_sigterm(int sig) {
    running = 0;
    FILE *log = fopen("/dev/shm/tritium.log", "a");
    fprintf(log, "Tritium stopped\n");
    fclose(log);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) { fprintf(stderr, "Usage: %s <parent_pid>\n", argv[0]); exit(1); }
    long parent_pid = atol(argv[1]);
    signal(SIGTERM, handle_sigterm);
    FILE *log = fopen("/dev/shm/tritium.log", "w");
    fprintf(log, "Tritium started with parent_pid %ld\n", parent_pid);
    fclose(log);
    while (running) sleep(1);
    return 0;
}
```

**Testing**:

- Compile: `gcc -o Tritium Tritium.c`.
- Run from bash:

  ```bash
  ./Tritium $$ & PID=$!; sleep 5; kill -TERM $PID
  ```

- Check `/dev/shm/tritium.log`: Should show “Tritium started with parent_pid <pid>” and “Tritium stopped”.

### Step 2: Integrate with Orchestration Script

**Description**: Modify `test_00_all.sh` to launch `Tritium` with its PID and ensure it stops after the script completes.
**Implementation**:

- Add launch/stop logic in `test_00_all.sh`.
- Use `timeout` for safety (25s for 20s run).
**Bash Code**:

```bash
#!/bin/bash
./Tritium $$ & MONITOR_PID=$!
# Simulate orchestration (replace with actual xargs/scripts)
sleep 10
kill -TERM $MONITOR_PID
```

**Testing**:

- Compile `Tritium` (from Step 1).
- Run: `timeout 25s ./test_00_all.sh`.
- Check `/dev/shm/tritium.log` for start/stop messages.
- Verify `Tritium` exits after `test_00_all.sh`.

### Step 3: Initialize Netlink Connector

**Description**: Set up Netlink Connector to receive `PROC_EVENT_FORK` and `PROC_EVENT_EXIT` events, logging events to `/dev/shm/tritium.log`.
**Implementation**:

- Open `NETLINK_CONNECTOR` socket, bind to `CN_IDX_PROC`.
- Send `PROC_CN_MCAST_LISTEN`.
- Non-blocking `recv` to log fork/exit events.
**Code Outline**:

```c
#include <linux/connector.h>
#include <linux/netlink.h>
#include <sys/socket.h>
// ... (include previous headers)

int main(int argc, char *argv[]) {
    // ... (previous setup)
    int nl_sock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
    struct sockaddr_nl sa = { .nl_family = AF_NETLINK, .nl_groups = CN_IDX_PROC };
    bind(nl_sock, (struct sockaddr*)&sa, sizeof(sa));
    struct nlmsghdr *nlh = malloc(NLMSG_LENGTH(sizeof(struct cn_msg) + sizeof(struct proc_event)));
    struct cn_msg *cn = NLMSG_DATA(nlh);
    cn->id.idx = CN_IDX_PROC;
    cn->id.val = CN_VAL_PROC;
    cn->len = sizeof(struct proc_event);
    ((struct proc_event*)cn->data)->what = PROC_CN_MCAST_LISTEN;
    send(nl_sock, nlh, NLMSG_LENGTH(cn->len), 0);
    while (running) {
        if (recv(nl_sock, nlh, NLMSG_LENGTH(sizeof(struct cn_msg) + sizeof(struct proc_event)), MSG_DONTWAIT) > 0) {
            struct cn_msg *msg = NLMSG_DATA(nlh);
            struct proc_event *ev = (struct proc_event*)msg->data;
            FILE *log = fopen("/dev/shm/tritium.log", "a");
            if (ev->what == PROC_EVENT_FORK) {
                fprintf(log, "Fork: pid=%ld, ppid=%ld\n", ev->event_data.fork.child_pid, ev->event_data.fork.parent_pid);
            } else if (ev->what == PROC_EVENT_EXIT) {
                fprintf(log, "Exit: pid=%ld\n", ev->event_data.exit.pid);
            }
            fclose(log);
        }
        usleep(10000); // 10ms
    }
    close(nl_sock);
    free(nlh);
    return 0;
}
```

**Testing**:

- Compile: `gcc -o Tritium Tritium.c`.
- Run with test script: `./test_00_all.sh` (use a simple script spawning `bash`, `sleep`).
- Check `/dev/shm/tritium.log` for fork/exit events (e.g., `Fork: pid=12345, ppid=<parent_pid>`).

### Step 4: Initialize PID Tracking Structure

**Description**: Add a `uthash`-based hash table to track PIDs, storing `pid`, `ppid`, `script_pid`, and `active` flag.
**Implementation**:

- Include `uthash.h`.
- Define `pid_info` struct.
- Add PIDs on `PROC_EVENT_FORK` if `ppid` matches `parent_pid`.
**Code Outline**:

```c
#include "uthash.h"
// ... (previous headers)

struct pid_info {
    long pid;
    long ppid;
    long script_pid;
    int active;
    UT_hash_handle hh;
};

struct pid_info *pids = NULL;

int main(int argc, char *argv[]) {
    // ... (previous setup)
    while (running) {
        if (recv(nl_sock, nlh, NLMSG_LENGTH(sizeof(struct cn_msg) + sizeof(struct proc_event)), MSG_DONTWAIT) > 0) {
            struct cn_msg *msg = NLMSG_DATA(nlh);
            struct proc_event *ev = (struct proc_event*)msg->data;
            FILE *log = fopen("/dev/shm/tritium.log", "a");
            if (ev->what == PROC_EVENT_FORK && ev->event_data.fork.parent_pid == parent_pid) {
                struct pid_info *p = malloc(sizeof(struct pid_info));
                p->pid = ev->event_data.fork.child_pid;
                p->ppid = ev->event_data.fork.parent_pid;
                p->script_pid = p->pid;
                p->active = 1;
                HASH_ADD_INT(pids, pid, p);
                fprintf(log, "Added pid=%ld, script_pid=%ld\n", p->pid, p->script_pid);
            } else if (ev->what == PROC_EVENT_EXIT) {
                struct pid_info *p;
                HASH_FIND_INT(pids, &ev->event_data.exit.pid, p);
                if (p) {
                    p->active = 0;
                    fprintf(log, "Marked inactive: pid=%ld\n", p->pid);
                }
            }
            fclose(log);
        }
        usleep(10000);
    }
    // ... (cleanup)
}
```

**Testing**:

- Download `uthash.h`: `wget https://raw.githubusercontent.com/troydhanson/uthash/master/src/uthash.h`.
- Compile: `gcc -o Tritium Tritium.c`.
- Run with test script spawning processes.
- Check `/dev/shm/tritium.log` for added PIDs and inactive markings.

### Step 5: Track Subprocesses (e.g., `cmake`)

**Description**: Extend PID tracking to include subprocesses (e.g., `cmake`, `make`) by checking `ppid` against active PIDs.
**Implementation**:

- On `PROC_EVENT_FORK`, if `ppid` is in `pids`, set `script_pid` to `ppid`’s `script_pid`.
**Code Outline**:

```c
// ... (previous code)
if (ev->what == PROC_EVENT_FORK) {
    struct pid_info *parent;
    HASH_FIND_INT(pids, &ev->event_data.fork.parent_pid, parent);
    if (ev->event_data.fork.parent_pid == parent_pid || parent) {
        struct pid_info *p = malloc(sizeof(struct pid_info));
        p->pid = ev->event_data.fork.child_pid;
        p->ppid = ev->event_data.fork.parent_pid;
        p->script_pid = (ev->event_data.fork.parent_pid == parent_pid) ? p->pid : parent->script_pid;
        p->active = 1;
        HASH_ADD_INT(pids, pid, p);
        fprintf(log, "Added pid=%ld, script_pid=%ld\n", p->pid, p->script_pid);
    }
}
```

**Testing**:

- Compile and run with a script spawning `cmake` (e.g., `bash -c "cmake ." &`).
- Check `/dev/shm/tritium.log` for PIDs with correct `script_pid` (e.g., `cmake` inherits script’s `script_pid`).

### Step 6: Add Start Time Tracking

**Description**: Store PID start time from `/proc/<pid>/stat` (field 22) for mapping to script start/end times.
**Implementation**:

- Read `/proc/<pid>/stat` on fork, calculate start time (jiffies ÷ `sysconf(_SC_CLK_TCK)`).
**Code Outline**:

```c
#include <unistd.h>
// ... (previous code)
struct pid_info {
    // ... (previous fields)
    double start_time;
};

if (ev->what == PROC_EVENT_FORK) {
    // ... (previous checks)
    char path[32];
    snprintf(path, sizeof(path), "/proc/%ld/stat", p->pid);
    FILE *f = fopen(path, "r");
    if (f) {
        long start_jiffies;
        fscanf(f, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %ld", &start_jiffies);
        fclose(f);
        p->start_time = start_jiffies / (double)sysconf(_SC_CLK_TCK);
    } else {
        p->start_time = 0;
    }
    fprintf(log, "Added pid=%ld, script_pid=%ld, start_time=%.3f\n", p->pid, p->script_pid, p->start_time);
}
```

**Testing**:

- Compile and run.
- Check `/dev/shm/tritium.log` for valid start times (seconds since boot).

### Step 7: Track Commands

**Description**: Read `/proc/<pid>/cmdline` to store command names (e.g., `bash`, `cmake`) and count invocations.
**Implementation**:

- Add `command` field to `pid_info`.
- Parse `cmdline` on fork, count per `script_pid`.
**Code Outline**:

```c
#define MAX_COMMANDS 50
struct pid_info {
    // ... (previous fields)
    char command[256];
};
struct script_data {
    long script_pid;
    int command_counts[MAX_COMMANDS];
    char *command_names[MAX_COMMANDS];
    UT_hash_handle hh;
};
struct script_data *scripts = NULL;
int command_count = 0;
char *command_names[MAX_COMMANDS] = {0};

int get_command_index(const char *cmd) {
    for (int i = 0; i < command_count; i++) {
        if (strcmp(cmd, command_names[i]) == 0) return i;
    }
    if (command_count < MAX_COMMANDS) {
        command_names[command_count] = strdup(cmd);
        return command_count++;
    }
    return -1;
}

if (ev->what == PROC_EVENT_FORK) {
    // ... (previous checks)
    snprintf(path, sizeof(path), "/proc/%ld/cmdline", p->pid);
    FILE *f = fopen(path, "r");
    if (f) {
        char cmd[256];
        fread(cmd, 1, sizeof(cmd)-1, f);
        cmd[255] = '\0';
        for (char *c = cmd; *c; c++) if (*c == '\0') *c = ' ';
        sscanf(cmd, "%255s", p->command);
        fclose(f);
    } else {
        strcpy(p->command, "unknown");
    }
    struct script_data *s;
    HASH_FIND_INT(scripts, &p->script_pid, s);
    if (!s) {
        s = malloc(sizeof(struct script_data));
        s->script_pid = p->script_pid;
        memset(s->command_counts, 0, sizeof(s->command_counts));
        HASH_ADD_INT(scripts, script_pid, s);
    }
    int cmd_idx = get_command_index(p->command);
    if (cmd_idx >= 0) s->command_counts[cmd_idx]++;
}
```

**Testing**:

- Compile and run with script spawning `bash`, `cmake`.
- Check `/dev/shm/tritium.log` for commands; verify counts in `scripts`.

### Step 8: Initialize CPU Sampling

**Description**: Sample CPU usage (`/proc/<pid>/stat`, fields 14+15) for active PIDs every 1s (for testing).
**Implementation**:

- Add `last_utime`, `last_stime` to `pid_info`.
- Calculate CPU % from tick delta.
**Code Outline**:

```c
#define SAMPLE_INTERVAL 1.0
struct pid_info {
    // ... (previous fields)
    unsigned long last_utime, last_stime;
};

while (running) {
    // ... (Netlink loop)
    struct timespec now, sleep = {0, (long)(SAMPLE_INTERVAL * 1e9)};
    clock_gettime(CLOCK_MONOTONIC, &now);
    struct pid_info *p, *tmp;
    HASH_ITER(hh, pids, p, tmp) {
        if (!p->active) continue;
        char path[32];
        snprintf(path, sizeof(path), "/proc/%ld/stat", p->pid);
        FILE *f = fopen(path, "r");
        if (!f) { p->active = 0; continue; }
        unsigned long utime, stime;
        fscanf(f, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu", &utime, &stime);
        fclose(f);
        double cpu = ((utime + stime) - (p->last_utime + p->last_stime)) * 100.0 / sysconf(_SC_CLK_TCK) / SAMPLE_INTERVAL;
        p->last_utime = utime;
        p->last_stime = stime;
        fprintf(log, "Sample: pid=%ld, cpu=%.1f\n", p->pid, cpu);
    }
    nanosleep(&sleep, NULL);
}
```

**Testing**:

- Compile and run.
- Check `/dev/shm/tritium.log` for CPU samples (0-100% per PID, >100% for multi-core).

### Step 9: Add Memory Sampling

**Description**: Sample memory (`/proc/<pid>/status`, `VmRSS`) every 1s.
**Implementation**:

- Read `VmRSS` for each active PID.
**Code Outline**:

```c
// ... (previous code)
HASH_ITER(hh, pids, p, tmp) {
    // ... (CPU sampling)
    long mem = 0;
    snprintf(path, sizeof(path), "/proc/%ld/status", p->pid);
    FILE *f = fopen(path, "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                sscanf(line, "VmRSS: %ld", &mem);
                break;
            }
        }
        fclose(f);
    }
    fprintf(log, "Sample: pid=%ld, cpu=%.1f, mem=%ld\n", p->pid, cpu, mem);
}
```

**Testing**:

- Compile and run.
- Check `/dev/shm/tritium.log` for CPU and memory samples (mem in KB).

### Step 10: Add Timing for Samples

**Description**: Add precise timing to samples using `clock_gettime(CLOCK_MONOTONIC)`.
**Implementation**:

- Calculate elapsed time since start.
**Code Outline**:

```c
struct timespec start;
clock_gettime(CLOCK_MONOTONIC, &start);
// ... (main loop)
clock_gettime(CLOCK_MONOTONIC, &now);
double elapsed = (now.tv_sec - start.tv_sec) + (now.tv_nsec - start.tv_nsec) / 1e9;
fprintf(log, "Sample: time=%.3f, pid=%ld, cpu=%.1f, mem=%ld\n", elapsed, p->pid, cpu, mem);
```

**Testing**:

- Compile and run.
- Check `/dev/shm/tritium.log` for timestamped samples.

### Step 11: Log Samples to File

**Description**: Log samples to `/dev/shm/usage.log` in CSV format.
**Implementation**:

- Open `log` globally, write CSV header.
**Code Outline**:

```c
FILE *log = NULL;
int main(int argc, char *argv[]) {
    // ... (previous setup)
    log = fopen("/dev/shm/usage.log", "w");
    fprintf(log, "time,pid,script_pid,cpu,mem,command\n");
    // ... (main loop)
    fprintf(log, "%.3f,%ld,%ld,%.1f,%ld,%s\n", elapsed, p->pid, p->script_pid, cpu, mem, p->command);
}
```

**Testing**:

- Compile and run.
- Check `/dev/shm/usage.log` for CSV rows (e.g., `0.123,1746510,1746510,10.2,5120,bash`).

### Step 12: Initialize Script Data Structure

**Description**: Add `script_data` struct to aggregate samples per `script_pid`.
**Implementation**:

- Define `sample` and `script_data` structs.
**Code Outline**:

```c
struct sample {
    double time, cpu;
    long mem;
};

struct script_data {
    long script_pid;
    struct sample *samples;
    int sample_count, sample_capacity;
    int command_counts[MAX_COMMANDS];
    char *command_names[MAX_COMMANDS];
    UT_hash_handle hh;
};
struct script_data *scripts = NULL;

if (ev->what == PROC_EVENT_FORK) {
    // ... (add pid)
    struct script_data *s;
    HASH_FIND_INT(scripts, &p->script_pid, s);
    if (!s) {
        s = malloc(sizeof(struct script_data));
        s->script_pid = p->script_pid;
        s->samples = malloc(1000 * sizeof(struct sample));
        s->sample_count = 0;
        s->sample_capacity = 1000;
        memset(s->command_counts, 0, sizeof(s->command_counts));
        HASH_ADD_INT(scripts, script_pid, s);
    }
}
```

**Testing**:

- Compile and run.
- Check `scripts` initialization in debugger or log.

### Step 13: Aggregate CPU/Memory per Script

**Description**: Sum CPU/memory for each `script_pid` during sampling.
**Implementation**:

- Add samples to `script_data`.
**Code Outline**:

```c
struct script_data *s;
HASH_FIND_INT(scripts, &p->script_pid, s);
if (s && s->sample_count < s->sample_capacity) {
    s->samples[s->sample_count++] = (struct sample){elapsed, cpu, mem};
}
```

**Testing**:

- Compile and run.
- Check `/dev/shm/usage.log` and verify `script_pid` grouping.

### Step 14: Test with 1s Sampling

**Description**: Run full program with 1s sampling to verify stability.
**Implementation**:

- Set `SAMPLE_INTERVAL = 1.0`.
**Testing**:
- Run with `test_00_all.sh`.
- Check `/dev/shm/usage.log` for consistent samples, correct `script_pid`.

### Step 15: Switch to 0.1s Sampling

**Description**: Reduce sampling interval to 0.1s.
**Implementation**:

- Set `SAMPLE_INTERVAL = 0.1`.
**Testing**:
- Run and check `/dev/shm/usage.log` for ~10x more samples, low CPU usage (<5%).

### Step 16: Initialize Jansson for JSON

**Description**: Add Jansson to prepare for JSON output.
**Implementation**:

- Include `jansson.h`, test JSON creation.
**Code Outline**:

```c
#include <jansson.h>
void test_json() {
    json_t *root = json_object();
    json_dump_file(root, "/dev/shm/test.json", JSON_INDENT(2));
    json_decref(root);
}
```

**Testing**:

- Compile: `gcc -o Tritium Tritium.c -ljansson`.
- Run and check `/dev/shm/test.json`.

### Step 17: JSON Output on SIGTERM

**Description**: Output JSON with `parent_pid`, `total_forks`, and `scripts` array (no samples yet).
**Implementation**:

- Build JSON in `finalize`.
**Code Outline**:

```c
void finalize(int sig) {
    running = 0;
    json_t *root = json_object();
    json_object_set_new(root, "parent_pid", json_integer(parent_pid));
    json_object_set_new(root, "total_forks", json_integer(total_forks));
    json_t *scripts_array = json_array();
    struct script_data *s, *tmp;
    HASH_ITER(hh, scripts, s, tmp) {
        json_t *script = json_object();
        json_object_set_new(script, "script_pid", json_integer(s->script_pid));
        json_array_append_new(scripts_array, script);
    }
    json_object_set_new(root, "scripts", scripts_array);
    json_dump_file(root, "/dev/shm/output.json", JSON_INDENT(2));
    fclose(log);
    json_decref(root);
    exit(0);
}
```

**Testing**:

- Compile and run.
- Check `/dev/shm/output.json` for `parent_pid`, `total_forks`, `scripts`.

### Step 18: Add Command Counts to JSON

**Description**: Include `command_counts` in JSON per `script_pid`.
**Implementation**:

- Add to `finalize`.
**Code Outline**:

```c
json_t *counts = json_object();
for (int i = 0; i < command_count; i++) {
    if (s->command_counts[i] > 0) {
        json_object_set_new(counts, command_names[i], json_integer(s->command_counts[i]));
    }
}
json_object_set_new(script, "command_counts", counts);
```

**Testing**:

- Run and check `/dev/shm/output.json` for `command_counts` (e.g., `"bash": 100`).

### Step 19: Add Samples to JSON

**Description**: Include CPU/memory samples in JSON.
**Implementation**:

- Add `samples` array in `finalize`.
**Code Outline**:

```c
json_t *samples = json_array();
for (int i = 0; i < s->sample_count; i++) {
    json_t *sample = json_object();
    json_object_set_new(sample, "time", json_real(s->samples[i].time));
    json_object_set_new(sample, "cpu", json_real(s->samples[i].cpu));
    json_object_set_new(sample, "mem", json_integer(s->samples[i].mem));
    json_array_append_new(samples, sample);
}
json_object_set_new(script, "samples", samples);
```

**Testing**:

- Run and check `/dev/shm/output.json` for `samples` arrays.

### Step 20: Strace-Like Summary

**Description**: Output `strace`-like summary per `script_pid` to `/dev/shm/summary.txt`.
**Implementation**:

- Add to `finalize`.
**Code Outline**:

```c
FILE *summary = fopen("/dev/shm/summary.txt", "w");
fprintf(summary, "Profiling Summary for test_00_all.sh (%s)\n", "Thu Jul 31 2025");
fprintf(summary, "-----------------------------------\n");
fprintf(summary, "Total forks: %ld\n", total_forks);
fprintf(summary, "Command invocations per script:\n");
HASH_ITER(hh, scripts, s, tmp) {
    fprintf(summary, "Script PID %ld:\n", s->script_pid);
    for (int i = 0; i < command_count; i++) {
        if (s->command_counts[i] > 0) {
            fprintf(summary, "  %s: %d\n", command_names[i], s->command_counts[i]);
        }
    }
}
fclose(summary);
```

**Testing**:

- Run and check `/dev/shm/summary.txt` for per-script command counts (e.g., `bash: 2410`).

### Step 21: Switch to 0.01s Sampling

**Description**: Finalize with 0.01s sampling.
**Implementation**:

- Set `SAMPLE_INTERVAL = 0.01`.
**Testing**:
- Run with `test_00_all.sh`.
- Check `/dev/shm/usage.log`, `/dev/shm/output.json`, `/dev/shm/summary.txt` for full data (~17,500 samples).

### Step 22: Generate SVG Charts with D3.js

**Description**: Create a Node.js script using D3.js and `jsdom` to generate SVG charts (`cpu_script_<script_pid>.svg`, `mem_script_<script_pid>.svg`) from `/dev/shm/output.json`, plotting CPU and memory usage over time per `script_pid`.
**Implementation**:

- Install dependencies: `npm install d3 jsdom`.
- Write `generate_charts.js` to produce line charts (extendable to stacked area charts or labels later).
- Output one SVG per `script_pid` for CPU and memory.
**Code Outline** (`generate_charts.js`):

```javascript
const fs = require('fs');
const { JSDOM } = require('jsdom');
const d3 = require('d3');

// Load JSON
const data = JSON.parse(fs.readFileSync('/dev/shm/output.json'));

// Chart dimensions
const width = 800, height = 400, margin = {top: 20, right: 20, bottom: 30, left: 50};

// Per script_pid
data.scripts.forEach((script, i) => {
    const samples = script.samples;
    if (!samples.length) return;

    // CPU chart
    const dom_cpu = new JSDOM('<!DOCTYPE html><body><svg></svg></body>');
    const svg_cpu = d3.select(dom_cpu.window.document.querySelector('svg'))
        .attr('width', width + margin.left + margin.right)
        .attr('height', height + margin.top + margin.bottom)
        .append('g')
        .attr('transform', `translate(${margin.left},${margin.top})`);
    const x = d3.scaleLinear().domain([0, 20]).range([0, width]);
    const y_cpu = d3.scaleLinear().domain([0, d3.max(samples, d => d.cpu)]).range([height, 0]);
    svg_cpu.append('path')
        .datum(samples)
        .attr('fill', 'none')
        .attr('stroke', 'blue')
        .attr('d', d3.line().x(d => x(d.time)).y(d => y_cpu(d.cpu)));
    svg_cpu.append('g')
        .attr('transform', `translate(0,${height})`)
        .call(d3.axisBottom(x).ticks(20));
    svg_cpu.append('g')
        .call(d3.axisLeft(y_cpu).ticks(10));
    fs.writeFileSync(`cpu_script_${script.script_pid}.svg`, dom_cpu.window.document.querySelector('svg').outerHTML);

    // Memory chart
    const dom_mem = new JSDOM('<!DOCTYPE html><body><svg></svg></body>');
    const svg_mem = d3.select(dom_mem.window.document.querySelector('svg'))
        .attr('width', width + margin.left + margin.right)
        .attr('height', height + margin.top + margin.bottom)
        .append('g')
        .attr('transform', `translate(${margin.left},${margin.top})`);
    const y_mem = d3.scaleLinear().domain([0, d3.max(samples, d => d.mem)]).range([height, 0]);
    svg_mem.append('path')
        .datum(samples)
        .attr('fill', 'none')
        .attr('stroke', 'green')
        .attr('d', d3.line().x(d => x(d.time)).y(d => y_mem(d.mem)));
    svg_mem.append('g')
        .attr('transform', `translate(0,${height})`)
        .call(d3.axisBottom(x).ticks(20));
    svg_mem.append('g')
        .call(d3.axisLeft(y_mem).ticks(10));
    fs.writeFileSync(`mem_script_${script.script_pid}.svg`, dom_mem.window.document.querySelector('svg').outerHTML);
});
```

**Testing**:

- Install: `npm install d3 jsdom`.
- Run: `node generate_charts.js` after `Tritium`.
- Check `cpu_script_<script_pid>.svg`, `mem_script_<script_pid>.svg` in a viewer (e.g., Inkscape, browser).
- Verify: Line charts show CPU/memory vs. time, ~700 samples per script (7s at 0.01s).

**Future Enhancements**:

- **Labels**: Add script names (from `test_00_all.sh` logs) via `svg.append('text')`, matching `script_pid` to start/end times.
- **Stacked charts**: Use `d3.stack` for aggregate CPU/memory across all scripts.
- **Metadata**: Add `command_counts` as SVG annotations (e.g., `<text>` showing `bash: 100`).
- **Interactivity**: Embed hover text via `<title>` elements for static SVGs.

**Alternative (Gnuplot)**:

- If using Gnuplot, modify `Tritium` to output CSV (`/dev/shm/usage.csv`) instead of JSON:

  ```c
  fprintf(log, "%.3f,%ld,%.1f,%ld\n", elapsed, p->script_pid, cpu, mem);
  ```

- Gnuplot script:

  ```gnuplot
  set terminal svg size 800,400
  set output 'cpu.svg'
  set xlabel 'Time (s)'
  set ylabel 'CPU (%)'
  plot 'usage.csv' using 1:3:2 with lines title columnheader(2)
  set output 'mem.svg'
  set ylabel 'Memory (KB)'
  plot 'usage.csv' using 1:4:2 with lines title columnheader(2)
  ```

- Run: `gnuplot plot.gp`.
- Note: Lacks D3.js’s flexibility for stacked charts, labels, or interactivity.

## Dependencies

- **Tritium**:
  - `libjansson-dev`: JSON output (`sudo apt-get install libjansson-dev`).
  - `uthash.h`: Hash tables (download: `wget https://raw.githubusercontent.com/troydhanson/uthash/master/src/uthash.h`).
  - Linux kernel with `CONFIG_PROC_EVENTS=y` (standard in most distros).
- **Charting**:
  - D3.js: `npm install d3 jsdom` (~20MB).
  - Optional (Gnuplot): `sudo apt-get install gnuplot` (~5MB), `jq` for JSON-to-CSV (~1MB).

## Notes

- **Scalability**: Handles ~12,124 PIDs (~20-30 active), ~17,500 samples, ~1-2MB memory, <0.5% CPU.
- **Mapping**: Use `start_time` in JSON to match `script_pid` to `test_00_all.sh`’s script start/end times.
- **Charting**: D3.js produces flexible SVG charts (line or stacked area), with potential for labels, hover text, or animations. Gnuplot is a simpler native alternative but less customizable.