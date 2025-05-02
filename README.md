kcshdproxy-nbdkit-plugin
========================
This package contains an ndbkit plugin that makes it possible to read from and
write to emulated MS-DOS hard drive partitions that are managed by the
[KCS Power PC board](https://amiga.resource.cx/exp/powerpc), a PC emulation
solution for the Commodore Amiga.

Prerequisites
=============
To run a server:

[nbdkit](https://gitlab.com/nbdkit/nbdkit)

To run a client:

[nbd](https://nbd.sourceforge.io)

To build from source code:

* [GCC](https://gcc.gnu.org)
* [GNU Make](https://www.gnu.org/software/make)
* [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config)

To access the filesystem you need FAT filesystem support in your kernel. Most
Linux distributions have this feature enabled by default.

Deploying from source code
==========================
Compiling this package from source code is straight forward:

```bash
$ make
$ make install
```

If you need the `searchmbr` utility, compile it with:

```bash
$ make searchmbr
```

The installation prefix can be changed by providing a modified `PREFIX`
environment variable.

It is also possible to use the [Nix package manager](http://nixos.org/nix) to
build the project. It takes care of automatically obtaining all required build
dependencies:

```bash
$ nix-build
```

An example usage scenario
=========================
There a variety of ways to use this plugin. The most common method to gain
access to an emulated PC hard-drive partition stored on an Amiga hard-drive is
by inserting the SD card that normally resides in a
[SCSI2SD](http://www.codesrc.com/mediawiki/index.php/SCSI2SD) device (a
replacement for a physical SCSI hard disk) into the card reader of your PC,
running an NBD server locally and mounting the partition from the SD card.

### Determining the offset of a KCS emulated PC hard drive partition

To be able to mount an emulated PC hard drive partition, we must know its
offset. On Linux, [GNU Parted](https://www.gnu.org/software/parted) is a tool
that can be helpful with this -- it knows how to interpret Amiga RDB partition
tables.

Running the following commands should suffice to get the required offset:

```bash
$ parted /dev/sdd
unit B
print
Model: Generic- USB3.0 CRW-SD (scsi)
Disk /dev/sdd: 15931539456B
Sector size (logical/physical): 512B/512B
Partition Table: amiga
Disk Flags: 

Number  Start       End          Size        File system  Name  Flags
 1      557056B     104726527B   104169472B  affs1        DH0   boot
 2      104726528B  209174527B   104448000B               KCS
 3      209174528B  628637695B   419463168B  affs1        DH1
 4      628637696B  1073725439B  445087744B  affs1        DH2
```

The above commands start GNU Parted, changes the unit to bytes and displays the
partition table.

In the above example, my emulated PC hard-drive is stored in the second
partition. Its offset is: `104726528`.

### Determining the position of the Master Boot Record (MBR)

In most of the cases, the master boot record (MBR) can be found at the beginning
of the emulated PC partition, but sometimes there are exceptions. For example,
on my Kickstart 1.3 drive, it is moved somewhat.

To cope with this problem, we can use a program named: `searchmbr` to scan for
the presence of the MBR so that we know the exact offset:

```bash
$ searchmbr /dev/sdd 104726528
MBR found at offset: 104726528
```

In the above example, we have instructed `searchmbr` to search for an MBR block
from the beginning of the PC partition (starting at offset: `104726528`). The
MBR's offset is identical to the Amiga partition offset confirming that the MBR
is at the beginning of the partition.

### Running a NBD server on a Unix domain socket

Although NBD can work with remote storage devices, mounting an emulated
partition is typically done locally. The following command starts an NDB server
that can be connected from by a client through a UNIX domain socket
(`$HOME/kcshdproxy.socket`):

```bash
$ nbdkit --unix $HOME/kcshdproxy.socket \
  --foreground \
  --verbose /usr/lib/kcshdproxy-plugin.so \
  offset=104726528 targetFile=/dev/sdd
```

The plugin accepts two parameters:

* `offset` specifies the offset of the emulated PC hard drive. In the example,
  the value corresponds to our previously discovered partition offset. If no
  offset is given, then it defaults to 0
* `targetFile` specifies the file that refers to the hard disk image. In the
  example, `/dev/sdd` refers to my card reader.

### Configure a NBD device

To connect to the server and configure a network block device, run the following
command:

```bash
$ nbd-client -unix $HOME/kcshdproxy.socket -block-size 512
```

### Mounting a KCS emulated PC partition

After the client has been started, `/dev/ndb0` is a network block device
representing our virtual PC hard drive. `/dev/ndb0p1` is the first partition.
On PCs, Linux should automatically detect all available MS-DOS partitions.

With the following commands, we can mount the first partition and inspect its
contents:

```bash
$ mount /dev/nbd0p1 /mnt/kcs-pc-partition
$ cd /mnt/kcs-pc-partition
$ ls
 COMMAND.COM    DRVSPACE.BIN  GAMES
 AUTOEXEC.BAT   CONFIG.SYS    IO.SYS
 AUTOEXEC.OLD   DOS           MSDOS.SYS
```

After the work is done, we should unmount the partition and disconnect the NDB
client:

```bash
$ umount /mnt/kcs-pc-partition
$ ndb-client -d /dev/ndb0
```

License
=======
The deployment recipes and configuration files in this repository are
[MIT licensed](./LICENSE.txt).
