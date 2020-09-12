# Si470X example program utilities.

This library contains utilities used by the si470x library sample programs.
These are:

* **Ports**: (port_noop/port_unix). These are used by the the Raspberry Pi
  version of the si470x library. These interface with hardware specifics such
  as I2C and GPIO pins, which differ based upon the platform.
* **ODA decoding**: The si470x library does basic RDS decoding, but relies on
  the host application to decode RDS ODA extensions.
* **utils**: Mostly functions to convert RDS values to displayable strings.
  These are intended to be used to be shown to the user on a display/etc.
