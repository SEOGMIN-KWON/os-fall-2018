# Using Prebuilt Packages for Ubuntu

This is the most recommended way to setup the development environment if you own an Ubuntu machine.

## Requirements

This method requires Ubuntu Desktop or Ubuntu Server to be installed. The following releases are supported:

* 16.04 LTS Xenial Xerus (and its point releases) (**recommended**)
* 14.04 LTS Trusty Tahr (and its point releases)
* 17.10.1 Artful Aardvark
* 18.04 LTS Bionic Beaver (*unstable*)

To check release information of your system, just type `lsb_release -a` in terminal. For those who are using other releases of Ubuntu, we recommend you to upgrade your system to one of the supported releases.

You need at least 3GB free space for development environment and kernel source tree.

## Installation

In terminal, type the following commands in sequence.

```bash
sudo add-apt-repository -y ppa:snuspl/os-env
sudo apt-get update
sudo apt-get install -y snuspl-os-env
```

And you're done!

## What's Next?
Read the [post-installation guide](/doc/OSEnvPostInstall.md).
