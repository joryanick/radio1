// GoodPrototyping radio1 avrdude program fuses
// (c) 2016-18 Jory Anick, jory@goodprototyping.com

talk.cmd
Function: Uses Avrdude to communicate with radio1 via ISP programmer
          Verifies that everything is OK (radio1 can be programmed)

prog.cmd
Function: Uses Avrdude to set the microcontroller fuses
          ONLY ADJUST FUSE VALUES IF YOU KNOW WHAT YOU ARE DOING
          IMPROPER SETTINGS CAN MAKE THE radio1 UNUSABLE
