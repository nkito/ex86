# ex86 - a x86 emulator

It is a personal project to understand real processors in detail. Please use it at your own risk.

## Description

It has 8086, 80186, and 80386 modes. Compatibility of 80386 is not 100% but Linux and 32-bit DOS apps run on it.

## Build

```
$ make
```

[build-ia16](https://gitlab.com/tkchia/build-ia16/)  is necessary to buld BIOS. Packages for Ununtu are available in [this page](https://launchpad.net/~tkchia/+archive/ubuntu/build-ia16/).

```
$ cd bios
$ make
```


## Usage

The command ex86 outputs the usame when no arguments are specified. 
At least, a BIOS file or a monitor file and its load address are necessary to boot up.

The emulator boots up with the BIOS by the following command line:
```
./ex86 -l f0000 bios/bios.bin -386
```
If driveA.img, driveB.img, or driveC.img exist in the same directory of the simulator, 
those files are treated as disks.

driveA.img in this repository is a custom Linux image built with Buildroot.

