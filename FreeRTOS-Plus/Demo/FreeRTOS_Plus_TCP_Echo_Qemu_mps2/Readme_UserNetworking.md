# TCP Echo Client Demo for MPS2 Cortex-M3 AN385 emulated using QEMU with User Mode Networking

## Setup Description
The demo requires 2 components -
1. Echo Client - The demo in this repository.
1. Echo Server - An echo server.

```
+--------------------------------------------------------+
|  Host Machine                                          |
|  OS - Ubuntu                                           |
|  Runs - Echo Server                                    |
|                          +--------------------------+  |
|                          |                          |  |
|                          |    QEMU - mps2-an385     |  |
|                          |                          |  |
|                          |                          |  |
|  +----------------+      |    +----------------+    |  |
|  |                |      |    |                |    |  |
|  |                |      |    |                |    |  |
|  |  Echo Client   | <-------> |   Echo Server  |    |  |
|  |                |      |    |                |    |  |
|  |                |      |    |                |    |  |
|  |                |      |    |                |    |  |
|  +----------------+      |    +----------------+    |  |
|                          |                          |  |
|                          +--------------------------+  |
+--------------------------------------------------------+
```

## PreRequisites

1. Install the following tools in the Host Machine:
   * [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads).
   * [qemu-arm-system](https://www.qemu.org/download).
   * Make (Version 4.3):
     ```
     sudo apt install make
     sudo apt install gcc-arm-none-eabi
     ```
2.  Clone the source code:
     ```
      git clone https://github.com/FreeRTOS/FreeRTOS.git --recurse-submodules --depth 1
     ```
## Enable User Mode Networking in QEMU

The User Mode Networking is implemented using *slirp*, which provides a full
TCP/IP stack within QEMU and uses it to implement a virtual NAT network. It does
not require Administrator privileges.

User Mode Networking has the following limitations:

 - The performance is not very good because of the overhead involved..
 - ICMP does not work out of the box i.e. you cannot use ping within the guest.
   Linux hosts require one time setup by root to make ICMP work within the
   guest.
 - The guest is not directly accessible from the host or the external network.
1. Configure number of server tasks:
Update ```TCPServer server(&loop, addr, "TCPEcho", <number of slave event loops>);``` in evpp/example_tcp_echo.cpp

2. Build:
```shell
   make
```

3. Run:
```shell
   qemu-system-arm -machine mps2-an385 -cpu cortex-m3 \
   -kernel ./build/freertos_tcp_mps2_demo.axf \
   -monitor null -semihosting -semihosting-config enable=on,target=native -serial stdio -nographic \
   -netdev user,id=mynet0,hostfwd=tcp::2222-:9999,restrict=off \
   -net nic,model=lan9118,netdev=mynet0
```

## Connect to the Server
On host machine:
```shell
   netcat localhost 2222
```
Can start multiple netcat client to the server

## Debug
1. Build with debugging symbols:
```
   make DEBUG=1
```

2. Start QEMU in the paused state waiting for GDB connection:
```shell
   sudo qemu-system-arm -machine mps2-an385 -cpu cortex-m3 -s -S \
   -kernel ./build/freertos_tcp_mps2_demo.axf \
   -monitor null -semihosting -semihosting-config enable=on,target=native -serial stdio -nographic \
   -netdev user,id=mynet0,hostfwd=tcp::2222-:9999,restrict=off -net nic,model=lan9118,netdev=mynet0 \
   -object filter-dump,id=tap_dump,netdev=mynet0,file=/tmp/qemu_tap_dump
```

3. Run GDB:
```shell
sudo arm-none-eabi-gdb -q ./build/freertos_tcp_mps2_demo.axf

(gdb) target remote :1234
(gdb) break main
(gdb) c
```

4. The above QEMU command creates a network packet dump in the file
`/tmp/qemu_tap_dump` which you can examine using `tcpdump` or WireShark:
```shell
sudo tcpdump -r /tmp/qemu_tap_dump  | less
```
