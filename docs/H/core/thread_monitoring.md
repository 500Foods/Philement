# Thread Monitoring

This document provides a bash script for detailed monitoring of the Hydrogen server's threads, memory usage, and file descriptors. The script provides a more detailed view of the information available through the `/api/system/info` endpoint.

## Script Usage

```bash
threadinfo.sh <executable_name>
```

For example:

```bash
threadinfo.sh hydrogen
```

## The Script

Save this script as `threadinfo.sh` and make it executable (`chmod +x threadinfo.sh`):

```bash
#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <executable_name>"
    exit 1
fi

EXEC_NAME=$1
PID=$(pgrep -x "$EXEC_NAME")

if [ -z "$PID" ]; then
    echo "Process $EXEC_NAME not found"
    exit 1
fi

# Get process-level memory information
echo "=== Memory Footprint for $EXEC_NAME (PID: $PID) ==="
ps -o pid,vsz,rss,pmem -p $PID --no-headers | awk '{
    printf "Total Virtual Memory: %d kB\n", $2;
    printf "Memory In Use: %d kB (RAM: %d kB, Swap: %d kB)\n", $3, $3, 0;
    printf "Memory Usage: %.1f%% of system RAM\n", $4;
}'

# Get thread information
echo -e "\n=== Thread Information ==="
if [ -d "/proc/$PID/task" ]; then
    echo "TID     CPU%   Stack Size  Runtime (s)   State"
    echo "---------------------------------------------"
    
    for TID in /proc/$PID/task/*; do
        THREAD_ID=$(basename $TID)
        
        # Get thread state
        STATE=$(grep -E "^State:" $TID/status 2>/dev/null | awk '{print $2$3}')
        
        # Get thread CPU usage
        CPU=$(ps -L -o lwp,pcpu -p $PID --no-headers | grep "\\b$THREAD_ID\\b" | awk '{print $2}')
        
        # Get thread stack size
        STACK=$(grep -E "^VmStk:" $TID/status 2>/dev/null | awk '{print $2" "$3}')
        
        # Get thread runtime
        if [ -f "$TID/stat" ]; then
            STAT=$(cat $TID/stat 2>/dev/null)
            UTIME=$(echo $STAT | awk '{print $14}')
            STIME=$(echo $STAT | awk '{print $15}')
            TOTAL_TIME=$((UTIME + STIME))
            HERTZ=$(getconf CLK_TCK)
            RUNTIME=$(echo "scale=2; $TOTAL_TIME / $HERTZ" | bc 2>/dev/null || echo "N/A")
        else
            RUNTIME="N/A"
        fi
        
        printf "%-7s %-6s %-11s %-13s %-s\n" \
            "$THREAD_ID" "$CPU" "$STACK" "$RUNTIME" "$STATE"
    done
fi

# Show memory distribution
echo -e "\n=== Memory Distribution ==="
if [ -f "/proc/$PID/smaps" ]; then
    grep -E "^(Rss|Pss|Swap):" "/proc/$PID/smaps_rollup" 2>/dev/null || \
    grep -E "^(Rss|Pss|Swap):" "/proc/$PID/smaps" | awk '
        BEGIN {rss=0; pss=0; swap=0}
        /^Rss:/ {rss+=$2}
        /^Pss:/ {pss+=$2}
        /^Swap:/ {swap+=$2}
        END {
            printf "Rss:               %d kB\n", rss;
            printf "Pss:               %d kB\n", pss;
            printf "Swap:              %d kB\n", swap;
        }'
fi

# Function to get socket information from /proc/net
get_socket_info() {
    local pid=$1
    local result=""
    declare -A socket_inodes
    
    # First, collect all socket inodes from process FDs
    for fd in /proc/$pid/fd/*; do
        if [ -L "$fd" ]; then
            fd_num=$(basename $fd)
            target=$(readlink $fd)
            if [[ $target =~ socket:\[([0-9]+)\] ]]; then
                inode="${BASH_REMATCH[1]}"
                socket_inodes[$inode]=$fd_num
            fi
        fi
    done
    
    # Function to process network file
    process_netfile() {
        local proto=$1
        local file=$2
        while read local_addr remote_addr state inode rest; do
            if [ -n "${socket_inodes[$inode]}" ]; then
                local hexport=$(echo $local_addr | cut -d: -f2)
                local port=$((16#$hexport))
                result+="${socket_inodes[$inode]} $proto $port\n"
            fi
        done < <(awk 'NR>1 {print $2" "$3" "$4" "$10}' "$file")
    }
    
    # Process each network file once
    [ -f "/proc/net/tcp" ] && process_netfile "TCP" "/proc/net/tcp"
    [ -f "/proc/net/tcp6" ] && process_netfile "TCP6" "/proc/net/tcp6"
    [ -f "/proc/net/udp" ] && process_netfile "UDP" "/proc/net/udp"
    [ -f "/proc/net/udp6" ] && process_netfile "UDP6" "/proc/net/udp6"
    
    echo -e $result
}

# Check if this process has any websocket sockets
has_websocket_server() {
    echo "$1" | grep -q "websocket server"
    return $?
}

# Show file descriptor information
echo -e "\n=== File Descriptor Information ==="
if [ -d "/proc/$PID/fd" ]; then
    FD_COUNT=$(ls -1 /proc/$PID/fd | wc -l)
    echo "Count: $FD_COUNT file descriptors"
    echo -e "\nDetailed FD List:"
    echo "FD     Type       Description"
    echo "--------------------------------"
    
    # Get socket information directly from /proc/net
    SOCKET_INFO=$(get_socket_info $PID)
    
    # Temporary file to store sorted output
    TEMP_FD_LIST=$(mktemp)
    
    for FD in /proc/$PID/fd/*; do
        if [ -L "$FD" ]; then
            FD_NUM=$(basename $FD)
            FD_TARGET=$(readlink $FD 2>/dev/null)
            
            # Determine the actual resource type
            if [[ $FD_NUM -le 2 ]]; then
                FD_TYPE="stdio"
            elif [[ $FD_TARGET == socket:* ]]; then
                FD_TYPE="socket"
            elif [[ $FD_TARGET == anon_inode:* ]]; then
                FD_TYPE="anon_inode"
            elif [[ $FD_TARGET == /dev/* ]]; then
                FD_TYPE="device"
            elif [[ -f $FD_TARGET ]]; then
                FD_TYPE="file"
            else
                FD_TYPE="other"
            fi
            
            DESCRIPTION=""
            
            # Handle standard descriptors
            case $FD_NUM in
                0) DESCRIPTION="stdin: " ;;
                1) DESCRIPTION="stdout: " ;;
                2) DESCRIPTION="stderr: " ;;
            esac
            
            # Handle different descriptor types
            if [[ $FD_TARGET == /dev/pts/* ]]; then
                DESCRIPTION+="terminal"
            elif [[ $FD_TARGET == socket:* ]]; then
                # Extract socket inode
                if [[ $FD_TARGET =~ socket:\[([0-9]+)\] ]]; then
                    SOCKET_INODE="${BASH_REMATCH[1]}"
                    
                    # First check our network socket mapping
                    PORT_INFO=$(echo "$SOCKET_INFO" | grep -w "^$FD_NUM")
                    if [ -n "$PORT_INFO" ]; then
                        PROTO=$(echo $PORT_INFO | awk '{print $2}')
                        PORT=$(echo $PORT_INFO | awk '{print $3}')
                        case $PORT in
                            5000) DESCRIPTION+="socket ($PROTO port $PORT - web server)" ;;
                            5001) DESCRIPTION+="socket ($PROTO port $PORT - websocket server)" ;;
                            5002) DESCRIPTION+="socket ($PROTO port $PORT - websocket server)" ;;
                            5353) DESCRIPTION+="socket ($PROTO port $PORT - mDNS)" ;;
                            *) DESCRIPTION+="socket ($PROTO port $PORT)" ;;
                        esac
                    else
                        # Check if it's a Unix domain socket
                        UNIX_SOCKET_INFO=$(ss -x 2>/dev/null | grep -w "$SOCKET_INODE")
                        if [ -n "$UNIX_SOCKET_INFO" ]; then
                            SOCK_TYPE=$(echo "$UNIX_SOCKET_INFO" | awk '{print $1}')
                            SOCK_PATH=$(echo "$UNIX_SOCKET_INFO" | awk '{print $5}')
                            if [ -n "$SOCK_PATH" ] && [ "$SOCK_PATH" != "*" ]; then
                                DESCRIPTION+="Unix domain socket: $SOCK_PATH"
                            else
                                DESCRIPTION+="Unix domain socket ($SOCK_TYPE)"
                            fi
                        else
                            # If we have websocket servers and this is an unbound socket, it might be libwebsockets
                            if has_websocket_server "$SOCKET_INFO" && 
                               ! echo "$SOCKET_INFO" | grep -q "^$FD_NUM"; then
                                DESCRIPTION+="socket (possible libwebsockets internal socket)"
                            else
                                # Check for connected/listening TCP sockets in unusual states
                                TCP_INFO=$(ss -ntap 2>/dev/null | grep -w "$SOCKET_INODE")
                                if [ -n "$TCP_INFO" ]; then
                                    TCP_STATE=$(echo "$TCP_INFO" | awk '{print $2}')
                                    DESCRIPTION+="TCP socket ($TCP_STATE)"
                                else
                                    # Generic socket type
                                    DESCRIPTION+="socket (inode: $SOCKET_INODE)"
                                fi
                            fi
                        fi
                    fi
                else
                    DESCRIPTION+="socket"
                fi
            elif [[ $FD_TARGET == anon_inode:* ]]; then
                ANON_TYPE=$(echo "$FD_TARGET" | cut -d: -f2)
                case "$ANON_TYPE" in
                    "[eventfd]")
                        DESCRIPTION+="event notification channel"
                        ;;
                    "[eventpoll]")
                        DESCRIPTION+="epoll instance (I/O event notification)"
                        ;;
                    "[timerfd]")
                        DESCRIPTION+="timer notification"
                        ;;
                    "[signalfd]")
                        DESCRIPTION+="signal notification"
                        ;;
                    "[inotify]")
                        DESCRIPTION+="file system watcher"
                        ;;
                    "[memfd]")
                        DESCRIPTION+="anonymous memory"
                        ;;
                    "[pipe]")
                        DESCRIPTION+="anonymous pipe"
                        ;;
                    *)
                        DESCRIPTION+="anonymous inode: $ANON_TYPE"
                        ;;
                esac
            elif [[ $FD_TARGET == /dev/urandom ]]; then
                DESCRIPTION+="random number source"
            elif [[ $FD_TARGET == /dev/null ]]; then
                DESCRIPTION+="null device (bit bucket)"
            elif [[ $FD_TARGET == /dev/zero ]]; then
                DESCRIPTION+="zero device (null bytes source)"
            elif [[ -f $FD_TARGET ]]; then
                DESCRIPTION+="file: $FD_TARGET"
            else
                DESCRIPTION+="$FD_TARGET"
            fi
            
            printf "%-6s %-10s %s\n" "$FD_NUM" "$FD_TYPE" "$DESCRIPTION" >> $TEMP_FD_LIST
        fi
    done
    
    # Sort by FD number and display
    sort -n $TEMP_FD_LIST
    rm $TEMP_FD_LIST
fi

# Show open files limit
echo -e "\n=== Resource Limits ==="
grep -E "^Max open files" "/proc/$PID/limits"

echo -e "\nNote: In Linux, threads share most memory except for their stacks."
echo "The total memory footprint combines both in-RAM (RSS) and swapped pages."
```

## Sample Output

```log
=== Memory Footprint for hydrogen (PID: 1265598) ===
Total Virtual Memory: 472328 kB
Memory In Use: 14028 kB (RAM: 14028 kB, Swap: 0 kB)
Memory Usage: 0.0% of system RAM

=== Thread Information ===
TID     CPU%   Stack Size  Runtime (s)   State
---------------------------------------------
1265598 0.0    136 kB      .01           S(sleeping)
1265599 0.0    136 kB      0             S(sleeping)
1265600 0.0    136 kB      0             S(sleeping)
1265602 0.0    136 kB      0             S(sleeping)
1265603 0.0    136 kB      0             S(sleeping)
1265604 0.0    136 kB      0             S(sleeping)

=== Memory Distribution ===
Rss:               14864 kB
Pss:               11490 kB
Swap:                  0 kB

=== File Descriptor Information ===
Count: 13 file descriptors

Detailed FD List:
FD     Type       Description
--------------------------------
0      stdio      stdin: terminal
1      stdio      stdout: terminal
2      stdio      stderr: terminal
3      file       file: /var/log/hydrogen.log
4      anon_inode event notification channel
5      socket     socket (TCP port 5000 - web server)
6      device     random number source
7      anon_inode event notification channel
8      socket     socket (inode: 95115985)
9      socket     socket (TCP port 5002 - websocket server)
10     socket     socket (TCP6 port 5002 - websocket server)
11     socket     socket (UDP port 5353 - mDNS)
43     socket     Unix domain socket: *

=== Resource Limits ===
Max open files            524288               524288               files

Note: In Linux, threads share most memory except for their stacks.
The total memory footprint combines both in-RAM (RSS) and swapped pages.
```

## Understanding the Output

The script provides detailed information about the Hydrogen server process in several sections:

### Memory Footprint

Shows the overall memory usage of the process, including:

- Total Virtual Memory allocated
- Memory actually in use (Resident Set Size)
- Percentage of system RAM used

### Thread Information

Lists all threads in the process with:

- Thread ID (TID)
- CPU usage percentage
- Stack size
- Runtime in seconds
- Current state

### Memory Distribution

Shows detailed memory statistics:

- Rss (Resident Set Size): Physical memory used
- Pss (Proportional Set Size): RSS adjusted for shared pages
- Swap: Memory in swap space

### File Descriptor Information

Lists all open file descriptors with:

- FD number
- Type (stdio, socket, file, etc.)
- Detailed description

### Resource Limits

Shows the maximum number of files the process can open.

## Relationship to /api/system/info

This script provides a more detailed view of the information available through the `/api/system/info` endpoint. While the API provides a high-level overview suitable for monitoring and automation, this script offers deeper insights useful for debugging and performance analysis.

The script is particularly useful for:

- Debugging memory issues
- Analyzing thread behavior
- Investigating file descriptor usage
- Understanding resource utilization

For the API endpoint documentation, see [System Info API Documentation](/docs/H/core//system_info.md).
