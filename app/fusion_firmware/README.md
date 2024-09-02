# Fusion board MCU firmware

This folder contains the MCU firmware sources, it relies on [modm](https://modm.io/). 
To build:

```Bash
lbuild  # only the first time to generate modm source tree

scons build  # to build
scons program # to upload the firmware
```

This firmware exposes two CDC USB devices, one for small command packets and one for big data packets. 
The command packets are described into [commands_handler.hpp](commands_handler.hpp).