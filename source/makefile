# Makefile for AVR function library development and examples
# Author: Pascal Stang
#
# For those who have never heard of makefiles: a makefile is essentially a
# script for compiling your code.  Most C/C++ compilers in the world are
# command line programs and this is even true of programming environments
# which appear to be windows-based (like Microsoft Visual C++).  Although
# you could use AVR-GCC directly from the command line and try to remember
# the compiler options each time, using a makefile keeps you free of this
# tedious task and automates the process.
#
# For those just starting with AVR-GCC and not used to using makefiles,
# I've added some extra comments above several of the makefile fields which
# you will have to deal with.

AVRLIB = ./avrlib
AVRLIB_CONF = ./avrlib/conf

########### change this lines according to your project ##################
#put the name of the TRG mcu here (at90s8515, at90s8535, attiny22, atmega603 etc.)
	MCU = atmega32
#	MCU = atmega161
#	MCU = atmega128

F_CPU = 12000000
#F_CPU = 16000000
#F_CPU = 20000000

#JUMPSTART = 0x1FC00
JUMPSTART = 0x7C00
#put the name of the TRG file here (without extension)
#  Your "TRG" file is your C source file that is at the top level of your code.
#  In other words, this is the file which contains your main() function.

# 	TRG = L3_ALPhas
  TRG = L3_Finger

#put your C sourcefiles here 
#  Here you must list any C source files which are used by your TRG file.
#  They will be compiled in the order you list them, so it's probably best
#  to list $(TRG).c, your top-level TRG file, last.

	SRC =   usbdrv/usbdrv.c usbdrv/oddebug.c \
			matrix/matrix.c matrix/keymap.c \
			macro/macro.c \
			usbmain.c ps2main.c keysta.c \
			main.c \
			$(AVRLIB)/timer.c \
			led/led.c \
			$(AVRLIB)/i2c.c \
			tinycmd/tinycmdmain.c \
			
#put additional assembler source file here
#  The ASRC line allows you to list files which contain assembly code/routines that
#  you would like to use from within your C programs.  The assembly code must be
#  written in a special way to be usable as a function from your C code.

	ASRC = usbdrv/usbdrvasm.s

#additional libraries and object files to link
#  Libraries and object files are collections of functions which have already been
#  compiled.  If you have such files, list them here, and you will be able to use
#  use the functions they contain in your TRG program.

	LIB	=

#additional includes to compile
	INC	= -Iusbdrv -Imatrix -Iled -Imacro -Itinycmd

#assembler flags
	ASFLAGS = -Wa, -gstabs -DF_CPU=$(F_CPU)
#TRG out
	OUT = $(TRG)
#compiler flags
	CPFLAGS	= -g -Os -Wall -Wstrict-prototypes -I$(AVRLIB) -I$(AVRLIB_CONF) $(INC) -DF_CPU=$(F_CPU)UL -D$(TRG) -Wa,-ahlms=$(<:.c=.lst)

#linker flags
	LDFLAGS = -Wl,-Map=$(TRG).map,--cref,--section-start=.appinboot=$(JUMPSTART)

#	LDFLAGS = -Wl,-Map=$(TRG).map,--cref -lm

	
########### you should not need to change the following line #############
include $(AVRLIB)/make/avrproj_make
		  
###### dependecies, add any dependencies you need here ###################
#  Dependencies tell the compiler which files in your code depend on which
#  other files.  When you change a piece of code, the dependencies allow
#  the compiler to intelligently figure out which files are affected and
#  need to be recompiled.  You should only list the dependencies of *.o 
#  files.  For example: uart.o is the compiled output of uart.c and uart.h
#  and therefore, uart.o "depends" on uart.c and uart.h.  But the code in
#  uart.c also uses information from global.h, so that file should be listed
#  in the dependecies too.  That way, if you alter global.h, uart.o will be
#  recompiled to take into account the changes.

buffer.o	: buffer.c		buffer.h
uart.o		: uart.c		uart.h		global.h
uart2.o		: uart2.c		uart2.h		global.h
rprintf.o	: rprintf.c		rprintf.h
a2d.o		: a2d.c			a2d.h
timer.o		: timer.c		timer.h		global.h
pulse.o		: pulse.c		pulse.h		timer.h		global.h
lcd.o		: lcd.c			lcd.h		global.h
i2c.o		: i2c.c			i2c.h		global.h
spi.o		: spi.c			spi.h		global.h
swpwm.o		: swpwm.c		swpwm.h		global.h
servo.o		: servo.c		servo.h		global.h
swuart.o	: swuart.c		swuart.h	global.h
tsip.o		: tsip.c		tsip.h		global.h
nmea.o		: nmea.c		nmea.h		global.h
vt100.o		: vt100.c		vt100.h		global.h
gps.o		: gps.c			gps.h		global.h
main.o		: main.c		global.h	hwaddress.h	
keysta.o	: keysta.c		keysta.h	common_inc.h
matrix/keymap.o	: matrix/keymap.c	matrix/keymap.h		matrix/matrix.h		led/led.h	hwaddress.h
matrix/matrix.o	: matrix/matrix.c	matrix/matrix.h		usbmain.h			ps2main.h	macro/macro.h
macro/macro.o	: macro/macro.c		macro/macro.h
led/led.o	: led/led.c		led/led.h
usbmain.o 	: usbmain.c		usbmain.h 	usbconfig.h		matrix/matrix.h		usbdrv/usbdrv.h
ps2main.o	: ps2main.c		ps2main.h	keysta.h	matrix/keymap.h	matrix/matrix.h	
usbdrv/usbdrv.o	:	usbdrv/usbdrv.c		usbdrv/usbdrv.h	usbdrv/oddebug.h
usbdrv/oddebug.o :	usbdrv/oddebug.c 	usbdrv/oddebug.h
usbdrv/usbdrvasm.o : usbdrv/usbdrvasm.asm usbdrv/usbdrvasm.S usbdrv/usbportability.h	usbdrv/usbdrv.h  usbconfig.h

all:
	avr-size -C --mcu=atmega32 $(TRG).elf
	rm -rf $(TRG)
	mkdir $(TRG)
	mv *.lst $(TRG)
	mv *.bin $(TRG)
	mv *.cof $(TRG)
	mv *.eep $(TRG)
	cp *.elf $(TRG)
	mv *.hex $(TRG)
	mv *.map $(TRG)
	cp *.bat $(TRG)
	cp bootloadHID_l3.exe $(TRG)
	cp sendHIDCmd.exe $(TRG)
   