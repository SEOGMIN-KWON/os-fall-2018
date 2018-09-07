# Running Docker Container on GNU/Linux

![SYNOPSIS](/doc/assets/SetupDocker.png)

This guide illustrates how to create Docker container with development environment preloaded.

## Setting up

### Installing Docker Engine

* Docker for Linux
    * Docker Engine is supported on several GNU/Linux distributions. You can check distribution-specific installation instructions [here](https://docs.docker.com/engine/installation/linux/).
    * This [post-installation guide](https://docs.docker.com/engine/installation/linux/linux-postinstall/) is also useful if you don't want to use `sudo` for using `docker` command.

### Obtaining the Image

In your terminal, type in the following command to download the image.

```bash
docker pull cmssnu/os-env
```

### Creating a new Container

Type in the following command to create a new contianer.


```bash
docker create -i -t --privileged -v /dev/bus/usb:/dev/bus/usb --name os cmssnu/os-env /bin/bash
```

The option `--privileged -v /dev/bus/usb:/dev/bus/usb` is to give the container access to ARTIK device.

## Using the Contianer

Type in the following command to start the contianer and attach to it.

```bash
docker start -ai os
```

If you cannot see the shell prompt after attaching, try hitting <kbd>Enter</kbd> again.

## Something went wrong?

You can use the command `docker rm os` to remove the container and start over. Removing a container will destroy all data in the container, so please be careful.

## What's Next?

Read the [post-installation guide](/doc/OSEnvPostInstall.md).
