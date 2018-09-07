# Setup on Windows PC

## Installing the Requirements

### Oracle VM VirtualBox and Extension Pack

![VirtualBoxDownload](/doc/assets/Win00VirtualBoxDownload.PNG)

* Download Oracle VM VirtualBox and Extension Pack from [here](https://www.virtualbox.org/wiki/Downloads).
* Install VirtualBox.

![VirtualBoxPreferences](/doc/assets/Win01VirtualBoxPreferences.PNG)
![VirtualBoxInstallExtension](/doc/assets/Win02VirtualBoxInstallExtension.PNG)

* Install VirtualBox Extension Pack.

### PuTTY

Download PuTTY from [here](http://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html) and install it. You may install all the PuTTY utilities using MSI package, or download `putty.exe` standalone binary.

## Setting up Virtual Machine

You need to create a new virtual machine where you can compile kernel soruce and flash the device with your kernel images.

### Creating a Virtual Machine

On VirtualBox, create a new virtual machine instance.

* You need to allocate at least one virtual CPU core and 1GB of RAM.
    * Adding up more resources may help speeding up the build process.
* You need at least 3GB free space after installing Ubuntu on your VM.
    * Creating virtual hard drive with more than 12GB space is recommended if you cannot estimate free space after installing Ubuntu.

### Adding USB Forwarding Rule

![VirtualBoxAddUsbRule](/doc/assets/Win05VirtualBoxAddUsbRule.PNG)

Open properties of your VM and enable USB 3.0 controller. Add a new USB filter rule.

![VirtualBoxUsbRule](/doc/assets/Win06VirtualBoxUsbRule.PNG)

Edit the filter with the following parameters.

* Name: lthor
* Vendor ID: 04e8
* Product ID: 685d
* Remote: No

Create another filtering rule with the following parameters.

* Name: sdb
* Vendor ID: 04e8
* Product ID: 6860
* Remote: No

### Installing Ubuntu

First, download Ubuntu release from [ubuntu.com](https://www.ubuntu.com/download) or [release archive](http://kr.archive.ubuntu.com/ubuntu-releases).

We recommend you to download Ubuntu 16.04 LTS or its point releases.

* [ubuntu-16.04.2-desktop-amd64.iso](http://kr.archive.ubuntu.com/ubuntu-releases/xenial/ubuntu-16.04.2-desktop-amd64.iso)
* [ubuntu-16.04.2-server-amd64.iso](http://kr.archive.ubuntu.com/ubuntu-releases/xenial/ubuntu-16.04.2-server-amd64.iso) (Server edition may provide better performance, but GUI is not enabled out-of-the-box and it's a little bit harder to install.)

Start your VM and insert the ISO image into the virtual disk drive. Follow the on-screen instructions to install Ubuntu.

### Setting up Development Environment

On terminal in the virtual machine, execute the following commands in sequence.

```bash
sudo add-apt-repository -y ppa:cmssnu/os-env
sudo apt-get update
sudo apt-get install -y cmslab-os-env
```

## Accessing Serial Console

You can access your device using serial console on ARTIK10. Connect the device to your PC and follow the procedure below.

### Identifying the Port

Make sure the device has turned and connected to the PC.

![IdentifyingDevice](/doc/assets/Win07IdentifyingDevice.PNG)

Open start menu, click run, type `devmgmt.msc` and hit <kbd>Enter</kbd>. On the Device Manager, lookup for USB Serial Port. In the example picture above, `COM4` is the debug console device.

### Using PuTTY to Open the Console

Open PuTTY and configure the destination with the following parameters.

* Connection type: Serial
* Serial line: `COM4` (you may have to modify this)
* Speed: 115200

And click Open.

![Thordown](/doc/assets/Win09Thordown.PNG)

Press reset button on the device, and then press start button for one second. If you can see boot messages from ARTIK, you're on the right way.

## What's Next?

Read the [post-installation guide](/doc/OSEnvVMPostInstall.md).
