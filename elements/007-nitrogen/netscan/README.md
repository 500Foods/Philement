# netscan - Network Discovery
This is a sample C program used to test how to do network discovery on a typical Linux system. This uses low-level NetworkManager APIs to talk to dbus to do its work. 
This should make it small and light enough for the smallest nitrogen elements running on similar platforms. 

A separate test version will presumably need to be created for the RP2040/ESP32 WiFi system given that it won't have access to Network Manager, to start with, and the coprocessor mechanism is
considerably more involved than what we're dealing with here.

A Makefile has been provided, so simply run 'make' with 'Makefile' and 'netscan.c' files in the same folder. NetworkManager development libraries as well as glib2 are required dependencis.

## Example: BTT-CB1 
Here is an example of the program's output when it is run on a BTT-CB1 found inside of a Troodon 2.0 Pro 3D printer. The WiFi connection has already been configured.
```
$ ./netscan
netscan v.11

3 Network Interfaces:
[Inactive | Wired    ] eth0            (               ):
[Active   | Wireless ] wlan0           (BurgundyNet    ): 192.168.0.142
[Inactive | Other    ] p2p-dev-wlan0   (               ):

Scanning Wi-Fi networks on device: /org/freedesktop/NetworkManager/Devices/3
Requesting Wi-Fi scan...
Scan requested successfully.

Wi-Fi Device Information:
  HwAddress: '48:8F:4C:61:93:58'
  PermHwAddress: '48:8F:4C:61:93:58'
  Mode: 2
  Bitrate: 0 Kbit/s
  Active Access Point: /org/freedesktop/NetworkManager/AccessPoint/50
  Wireless Capabilities: WEP40 WEP104 TKIP CCMP WPA RSN AP AD-HOC FREQ_VALID FREQ_2GHZ

15 Access Points:
 [ 100% | 2.4G ] BurgundyNet
 [ 100% | 2.4G ] MaroonNet
 [ 100% | 2.4G ] ESP_FEC5CD
 [  82% | 2.4G ] SHAW-9026-24
 [  77% | 2.4G ] Beba
 [  75% | 2.4G ] SPSETUP-6EDB
 [  75% | 2.4G ] SPSETUP-4B08
 [  70% | 2.4G ] Anarchy
 [  67% | 2.4G ] SPSETUP-E266
 [  65% | 2.4G ] TELUSA74D
 [  64% | 2.4G ] CBN-4499C
 [  64% | 2.4G ] DIRECT-e4-HP M227f LaserJet
 [  54% | 2.4G ] TELUS6583
 [  40% | 2.4G ] TELUS5496-2.4G
 [  37% | 2.4G ] komrad-2.4
```

## Example: Fedora Linux Desktop
Here, the program is run on a system without WiFi but with an array of (virtual) adapters used by KVM/QEMU for running various virtual machines.
```
$ ./netscan
netscan v.11

10 Network Interfaces:
[Active   | Other    ] lo              (lo             ): 127.0.0.1
[Active   | Wired    ] eno1            (eno1           ):
[Active   | Other    ] bridge0         (bridge0        ): 192.168.0.44
[Active   | Other    ] virbr0          (virbr0         ): 192.168.122.1
[Active   | Other    ] vnet2           (vnet2          ):
[Active   | Other    ] vnet3           (vnet3          ):
[Active   | Other    ] vnet4           (vnet4          ):
[Active   | Other    ] br-b5dea5ed9377 (br-b5dea5ed9377): 172.19.0.1
[Active   | Other    ] docker0         (docker0        ): 172.17.0.1
[Active   | Other    ] virbr1          (virbr1         ): 192.168.100.1

No Wi-Fi networks found.
```

