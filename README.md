[![GitHub Workflow Status (with event)](https://img.shields.io/github/actions/workflow/status/nkito/ex86/c-cpp.yml?label=ci:emu-build&logo=github&style=flat-square)](https://github.com/nkito/ex86/actions/workflows/c-cpp.yml)
[![GitHub Workflow Status (with event)](https://img.shields.io/github/actions/workflow/status/nkito/ex86/bios_ci.yml?label=ci:BIOS-build&logo=github&style=flat-square)](https://github.com/nkito/ex86/actions/workflows/bios_ci.yml)

# ex86 - a x86 emulator

It is a personal project to understand real processors in detail. Please use it at your own risk.

## Description

It has 8086, 80186, and 80386 modes. Compatibility of 80386 is not 100% but Linux and 32-bit DOS apps run on it.
An original BIOS suited to the emulator is available.

## Build

```
$ make
```

A prebuild BIOS file is available in this repository, i.e., bios/bios.bin. Usually, a build process for BIOS codes is not necessary.
If rebuild is necessary for reasons such as customizing BIOS, 
[build-ia16](https://gitlab.com/tkchia/build-ia16/)  is necessary as a tool chain. Packages for Ununtu are available in [this page](https://launchpad.net/~tkchia/+archive/ubuntu/build-ia16/).

```
$ cd bios
$ make
```


## Usage

The command ex86 outputs the usage when no arguments are specified. 
At least, a BIOS file or a raw binary file of a monitor program and its load address are necessary to boot up.

The emulator boots up with using the BIOS by the following command line:
```
./ex86 -l f0000 bios/bios.bin -386
```
If driveA.img, driveB.img, or driveC.img exist in the same directory of the simulator, 
those files are treated as disks.

driveA.img in this repository is a custom Linux image built with Buildroot.

