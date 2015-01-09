BLEneocom
----------

BLEneocom is for ttyUSB0 (like PL2303) which connect to cheap BLE module (BM-S02, TI CC2540)
```
Ubuntu --usb --> PL2302 --uart--> CC2540  <== wireless ==> BT usb dongle
  ^                                                               ^
  |------------------------ usb ----------------------------------|
```
neocom
======

"neocon" is a simple serial console utility that tries to open a
ttys that may exist on a system until one such open succeeds. It
then passes terminal input and output, until there is a read or
write failure on the tty, in which case it disconnects, and the
process restarts.

This is mainly intended for serial over USB interfaces that
disappear when the Neo or debug board is restarted. E.g.,
neocon /dev/ttyUSB0 /dev/ttyUSB1

The option  -t delay_ms  throttles keyboard input to a rate of
one character every "delay_ms" milliseconds. This can be used to
prevent buffer overruns on the remote end.

"neocon" can log to a file with the option "-l logfile". Non-ASCII
and non-printable characters are converted to hash signs (#). To
append to an existing logfile, add the option "-a". To add a
timestamp before each line, use the option "-T".

To leave neocon, type "~.". The escape character (~) can be changed
with the option "-e escape".


Ubuntu BLE operation
=====================

Use gatttool to operate BLE device on ubuntu 12.04.
Need to update gatttool as below.
Get bluez version
```
 dpkg --status bluez | grep '^Version:'
```
A kernel version higher then 3.5
So check with
```
# uname -r

get a recent bluez version from http://www.bluez.org/
# wget https://www.kernel.org/pub/linux/bluetooth/bluez-5.4.tar.xz
extract
# tar xvf bluez-5.4.tar.xz

get the necessary libs
# apt-get install libusb-dev libdbus-1-dev libglib2.0-dev automake libudev-dev libical-dev libreadline-dev

systemd is not needed, see later

configure and build SW:
# cd bluez-5.4
# ./configure --disable-systemd
# make
# make install

The I even had to copy gatttool manually into the /usr/local/bin dir

# cp attrib/gatttool /usr/local/bin/

# sudo gatttool -b B4:99:4C:3A:C0:6A --interactive
# sudo: unable to resolve host ben-ubuntu
[   ][B4:99:4C:3A:C0:6A][LE]> connect
[CON][B4:99:4C:3A:C0:6A][LE]> help
[CON][B4:99:4C:3A:C0:6A][LE]> characteristics
[CON][B4:99:4C:3A:C0:6A][LE]> 
handle: 0x0002, char properties: 0x02, char value handle: 0x0003, uuid: 00002a00-0000-1000-8000-00805f9b34fb
handle: 0x0004, char properties: 0x02, char value handle: 0x0005, uuid: 00002a01-0000-1000-8000-00805f9b34fb
handle: 0x0006, char properties: 0x0a, char value handle: 0x0007, uuid: 00002a02-0000-1000-8000-00805f9b34fb
handle: 0x0008, char properties: 0x08, char value handle: 0x0009, uuid: 00002a03-0000-1000-8000-00805f9b34fb
handle: 0x000a, char properties: 0x02, char value handle: 0x000b, uuid: 00002a04-0000-1000-8000-00805f9b34fb
handle: 0x000d, char properties: 0x20, char value handle: 0x000e, uuid: 00002a05-0000-1000-8000-00805f9b34fb
handle: 0x0011, char properties: 0x02, char value handle: 0x0012, uuid: 00002a23-0000-1000-8000-00805f9b34fb
handle: 0x0013, char properties: 0x02, char value handle: 0x0014, uuid: 00002a26-0000-1000-8000-00805f9b34fb
handle: 0x0017, char properties: 0x12, char value handle: 0x0018, uuid: 00002a19-0000-1000-8000-00805f9b34fb
handle: 0x001c, char properties: 0x10, char value handle: 0x001d, uuid: 0000ffe4-0000-1000-8000-00805f9b34fb
handle: 0x0021, char properties: 0x08, char value handle: 0x0022, uuid: 0000ffe9-0000-1000-8000-00805f9b34fb
handle: 0x0025, char properties: 0x0a, char value handle: 0x0026, uuid: 0000ff91-0000-1000-8000-00805f9b34fb
handle: 0x0028, char properties: 0x0a, char value handle: 0x0029, uuid: 0000ff92-0000-1000-8000-00805f9b34fb
handle: 0x002b, char properties: 0x0a, char value handle: 0x002c, uuid: 0000ff93-0000-1000-8000-00805f9b34fb
handle: 0x002e, char properties: 0x08, char value handle: 0x002f, uuid: 0000ff94-0000-1000-8000-00805f9b34fb
handle: 0x0031, char properties: 0x0a, char value handle: 0x0032, uuid: 0000ff95-0000-1000-8000-00805f9b34fb
handle: 0x0034, char properties: 0x0a, char value handle: 0x0035, uuid: 0000ff96-0000-1000-8000-00805f9b34fb
handle: 0x0037, char properties: 0x0a, char value handle: 0x0038, uuid: 0000ff97-0000-1000-8000-00805f9b34fb
handle: 0x003a, char properties: 0x0a, char value handle: 0x003b, uuid: 0000ff98-0000-1000-8000-00805f9b34fb
handle: 0x003d, char properties: 0x08, char value handle: 0x003e, uuid: 0000ff99-0000-1000-8000-00805f9b34fb
handle: 0x0040, char properties: 0x0a, char value handle: 0x0041, uuid: 0000ff9a-0000-1000-8000-00805f9b34fb
handle: 0x0044, char properties: 0x08, char value handle: 0x0045, uuid: 0000ffc1-0000-1000-8000-00805f9b34fb
handle: 0x0047, char properties: 0x10, char value handle: 0x0048, uuid: 0000ffc2-0000-1000-8000-00805f9b34fb
handle: 0x004c, char properties: 0x0a, char value handle: 0x004d, uuid: 0000ffa2-0000-1000-8000-00805f9b34fb
handle: 0x004f, char properties: 0x12, char value handle: 0x0050, uuid: 0000ffa1-0000-1000-8000-00805f9b34fb
[CON][B4:99:4C:3A:C0:6A][LE]> char-write-cmd 0x22 41
[CON][B4:99:4C:3A:C0:6A][LE]> char-read-hnd 0x1d
```
