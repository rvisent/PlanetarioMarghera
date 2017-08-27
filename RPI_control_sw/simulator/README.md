# planetario simulator
## version talking to a true serial port
### planetario.py
see line 499 for port configuration

## version talking to named pipe, to support "host pipe" mode in VirtualBox
### planetario_np.py
port name is \\.\pipe\planetario

_Both versions tested with Python 2.7.10 and Visual 5.74_
