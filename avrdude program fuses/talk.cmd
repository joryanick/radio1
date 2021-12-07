@ECHO OFF
ECHO GoodPrototyping radio1 ISP connectivity checker
ECHO .
ECHO Connect your ISP programmer and

pause

avrdude -B 25 -c usbtiny -p m2560
pause
