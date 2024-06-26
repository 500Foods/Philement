## hydro - System Status
This is a small self-contained test project that simply (?!) outputs a bunch of system status information. Things like memory usage, current network connections, free disk space and so on. The idea is
to have it output JSON that can then be fed via a WebSocket connection to a client system for display. Something like this.

```
{
    "memory": {
        "high_watermark": {
            "value": 7245824,
            "units": "bytes"
        },
        "current": {
            "value": 7245824,
            "units": "bytes"
        },
        "heap": {
            "value": 0,
            "units": "bytes"
        },
        "stack": {
            "value": 0,
            "units": "bytes"
        }
    },
    "time": {
        "timestamp": 1719407170,
        "local": "2024-06-26T06:06:10-0700",
        "UTC": "2024-06-26T13:06:10Z",
        "Elapsed Total (ms)": 7.7762450000000003,
        "Elapsed Memory (ms)": 3.3060809999999998,
        "Elapsed Network (ms)": 1.9865820000000001,
        "Elapsed Filesystem (ms)": 0.63804099999999997
    },
    "network": {
        "interfaces": [
            {
                "name": "wlan0",
                "addresses": [
                    "192.168.0.142",
                    "fd8f:a4ad:b5b8:e348:ec75:ec90:3f2:e38e",
                    "fe80::9968:b271:b506:9c08"
                ],
                "tcp_connections": 4,
                "udp_connections": 2,
                "mac_address": "48:8F:4C:61:93:58",
                "ssid": "BurgundyNet",
                "rx_bytes": 5567617053,
                "rx_bytes_per_second": 1638.0,
                "tx_bytes": 159332299866,
                "tx_bytes_per_second": 7526.8999999999996,
                "rx_packets": 50375097,
                "rx_packets_per_second": 14.800000000000001,
                "tx_packets": 105454543,
                "tx_packets_per_second": 12.4
            },
            {
                "name": "Unidentified Adapter",
                "tcp_connections": 12,
                "udp_connections": 4
            }
        ],
        "_debug_elapsed": 10.0,
        "_debug_previous_values": {
            "interfaces": [
                {
                    "name": "wlan0",
                    "addresses": [
                        "192.168.0.142",
                        "fd8f:a4ad:b5b8:e348:ec75:ec90:3f2:e38e",
                        "fe80::9968:b271:b506:9c08"
                    ],
                    "tcp_connections": 4,
                    "udp_connections": 2,
                    "mac_address": "48:8F:4C:61:93:58",
                    "ssid": "BurgundyNet",
                    "rx_bytes": 5567617053,
                    "rx_bytes_per_second": 1638.0,
                    "tx_bytes": 159332299866,
                    "tx_bytes_per_second": 7526.8999999999996,
                    "rx_packets": 50375097,
                    "rx_packets_per_second": 14.800000000000001,
                    "tx_packets": 105454543,
                    "tx_packets_per_second": 12.4
                },
                {
                    "name": "Unidentified Adapter",
                    "tcp_connections": 12,
                    "udp_connections": 4
                }
            ]
        },
        "_debug_last_network_check_time": 1719407170
    },
    "filesystems": {
        "/dev/mmcblk1p2": {
            "device": "/dev/mmcblk1p2",
            "mount_point": "/",
            "total_space": 31077969920,
            "used_space": 14779486208,
            "available_space": 16298483712,
            "used_percent": 47.556150694671892,
            "available_percent": 52.443849305328115,
            "read_operations": 2019432,
            "write_operations": 3031392,
            "read_bytes": 15758987264,
            "written_bytes": 12666484224,
            "read_operations_per_second": 0.0,
            "write_operations_per_second": 6.7999999999999998,
            "read_bytes_per_second": 43827.199999999997,
            "written_bytes_per_second": 18227.200000000001
        },
        "/dev/mmcblk1p1": {
            "device": "/dev/mmcblk1p1",
            "mount_point": "/boot",
            "total_space": 268148736,
            "used_space": 65941504,
            "available_space": 202207232,
            "used_percent": 24.591390951028014,
            "available_percent": 75.408609048971982,
            "read_operations": 7654,
            "write_operations": 1512,
            "read_bytes": 9216,
            "written_bytes": 780288,
            "read_operations_per_second": 0.0,
            "write_operations_per_second": 0.0,
            "read_bytes_per_second": 0.0,
            "written_bytes_per_second": 0.0
        },
        "_debug_elapsed": 10.0,
        "_debug_previous_values": {
            "/dev/mmcblk1p2": {
                "device": "/dev/mmcblk1p2",
                "mount_point": "/",
                "total_space": 31077969920,
                "used_space": 14779486208,
                "available_space": 16298483712,
                "used_percent": 47.556150694671892,
                "available_percent": 52.443849305328115,
                "read_operations": 2019432,
                "write_operations": 3031392,
                "read_bytes": 15758987264,
                "written_bytes": 12666484224,
                "read_operations_per_second": 0.0,
                "write_operations_per_second": 6.7999999999999998,
                "read_bytes_per_second": 43827.199999999997,
                "written_bytes_per_second": 18227.200000000001
            },
            "/dev/mmcblk1p1": {
                "device": "/dev/mmcblk1p1",
                "mount_point": "/boot",
                "total_space": 268148736,
                "used_space": 65941504,
                "available_space": 202207232,
                "used_percent": 24.591390951028014,
                "available_percent": 75.408609048971982,
                "read_operations": 7654,
                "write_operations": 1512,
                "read_bytes": 9216,
                "written_bytes": 780288,
                "read_operations_per_second": 0.0,
                "write_operations_per_second": 0.0,
                "read_bytes_per_second": 0.0,
                "written_bytes_per_second": 0.0
            }
        },
        "_debug_last_filesystem_check_time": 1719407170
    },
    "system": {
        "stats_counter": 2,
        "load_1min": 0.64000000000000001,
        "load_5min": 0.44,
        "load_15min": 0.46000000000000002,
        "cpu_cores": 4,
        "cpu_usage": {
            "total": 12.672746427960133
        },
        "cpu_usage_per_core": {
            "cpu0": 12.318229453328932,
            "cpu1": 13.239335411083623,
            "cpu2": 12.511925605231992,
            "cpu3": 12.626376054957529
        },
        "temperatures": {
            "thermal_zone2": 48.094999999999999,
            "thermal_zone0": 47.122999999999998,
            "thermal_zone3": 46.960999999999999,
            "thermal_zone1": 46.637
        },
        "uptime_seconds": 197481,
        "boot_time": 1719209689,
        "total_physical_ram": 1034891264,
        "total_swap": 517443584,
        "used_swap": 38273024,
        "free_swap": 479170560,
        "swap_used_percent": 7.3965597764567121,
        "swap_free_percent": 92.603440223543288,
        "os_name": "Linux",
        "os_version": "#2.3.3 SMP Wed Jul 12 11:07:49 CST 2023",
        "kernel_version": "5.16.17-sun50iw9",
        "os_full_name": "Debian GNU/Linux 11 (bullseye)",
        "open_file_descriptors_system": 1792,
        "max_file_descriptors_system": 9223372036854775807,
        "open_file_descriptors_process": 9,
        "open_files": [
            "/dev/pts/2",
            "/dev/pts/2",
            "/dev/pts/2",
            "anon_inode:[eventfd]",
            "anon_inode:[eventfd]",
            "anon_inode:[eventfd]",
            "socket:[10066593]",
            "anon_inode:[eventfd]",
            "/proc/666872/fd"
        ],
        "logged_in_users": [
            {
                "username": "biqu",
                "tty": "pts/2",
                "host": "192.168.0.44",
                "login_time": 1719407114
            }
        ],
        "total_processes": 149,
        "current_process_threads": 5
    }
}
```
