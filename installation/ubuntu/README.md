# Installation on Ubuntu

## udev rules

Rules are needed for non-root access to USB devices:

```
cat /etc/udev/rules.d/99-glow.rules

SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", MODE="0666"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", MODE="0666"
SUBSYSTEM=="usb", ATTRS{idVendor}=="06cd", ATTRS{idProduct}=="0402", MODE="0666"
```

Tell the kernel to not create `/dev/ttyUSB*` for the GlowEngine USB device.

```
cat /etc/modprobe.d/blacklist-glow.conf

# glowcontrol need full access to device via libusb
blacklist ftdi_sio
```

Also, make sure the local user is in the `input` group.
