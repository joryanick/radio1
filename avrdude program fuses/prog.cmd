@ECHO OFF
ECHO GoodPrototyping radio1 ISP fuse setting
ECHO .
ECHO Connect your ISP programmer and

pause

rem preserve eprom
avrdude -B 25 -c usbtiny -p m2560 -u -U hfuse:w:0xD8:m

pause

avrdude -B 25 -c usbtiny -p m2560 -u -U efuse:w:0xFD:m

pause

rem this is 16mhz
avrdude -B 25 -c usbtiny -p m2560 -u -U lfuse:w:0xFF:m