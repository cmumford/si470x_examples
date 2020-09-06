# The Si470X library example programs.

## Overview

These are the example programs for the
[Si470X library](http://github.com/cmumford/si470x). This project
has samples for both the Linux/UNIX, and Mongoose OS platforms.

Contributions are welcome, please see [CONTRIBUTING.md](CONTRIBUTING.md)
for more information.

The Mongoose OS sample uses an LCD screen to display some of the
current radio state. This can be disabled by the following `mos.yml`
file change:

```yaml
config_schema:
  - ["ssd1306.enable", false]
```

If disabled, the current radio state will be periodically logged
and can be read on the mos console.

## Building for Linux/UNIX

```sh
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
```

These example programs (and the Si470X library) support a fairly
generic Linux/UNIX port, but really only run on Raspberry Pi
using the wiringPi library. It should be fairly straigtforward
to support a different platform by creating a new port.
