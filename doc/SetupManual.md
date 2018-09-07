# Manual Setup on GNU/Linux

## Dependencies

* GNU Coreutils, `bash`, `tar`, and `bc`
    * These basic utilities may be included in your distribution. Or you may install them using package managers.
* `gcc`, `git`, `make`, `ccache`, `util-linux` and `binutils`
    * You may install them using package managers.
* `screen`
    * `screen` is required for serial communication with the device.
    * An alternative is `minicom`
* `gcc-arm-linux-gnueabihf`
    * This is cross compiler for compiling kernel source.
    * On Debian family, just look for the packages named `gcc-arm-linux-gnueabihf`.
    * On Fedora, look for [this package](https://copr.fedorainfracloud.org/coprs/lantw44/arm-linux-gnueabihf-toolchain), though it's not tested well.
* `gcc-arm-linux-gnueabi`
    * This is for compiling user program.
* `android-tools-fsutils`
    * The executable `make_ext4fs` is needed to build kernel module image.
    * The build script should be modified to setup environment without this executable. Let us know when you need this.
* `lthor`
    * Check out [this page](https://source.tizen.org/documentation/reference/flash-device?langswitch=en) to install `lthor`.
* `sdb`
    * You need to install Tizen SDK to obtain `sdb` binary.
    * Download Tizen SDK from [here](https://developer.tizen.org/development/tools/download?langswitch=en).
    * `sdb` is located at `tizen-sdk/tools`. Add this to the `PATH` environment variable. Consult the documentation for your shell to do this.

## (Optional) Accessing Serial Device without sudo

### Adding udev Rule

To access debug console device without root privilege or `sudo`, you can add udev rule.

Execute the following command with root privilege.

```bash
cat >> /etc/udev/rules.d/99-artik10.rules
SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", MODE="0666"
```

And hit <kbd>Ctrl</kbd>+<kbd>D</kbd>

Reload the rule.

```bash
udevadm control --reload-rules
```

### Adding yourself to the Appropriate Group

Or adding yourself to `dialout` or `uucp` group may resolve the issue.

Check the appropriate group.

```bash
ls -l /dev/ttyUSB0
crw-rw-rw- 1 root uucp 188, 0 Mar  3 19:39 /dev/ttyUSB0
```

And add yourself to, for example, `uucp` group.

```bash
usermod -a -G uucp $USER
```

Changes will take place on your next login.

## What's Next?

Read the [post-installation guide](/doc/OSEnvPostInstall.md).
