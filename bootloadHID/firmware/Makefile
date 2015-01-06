# Name: Makefile
# Project: bootloadHID
# Author: Christian Starkjohann
# Creation Date: 2007-03-19
# Tabsize: 4
# Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
# License: GNU GPL v2 (see License.txt)
# This Revision: $Id$

###############################################################################
# Configure the following variables according to your AVR. The example below
# is for an ATMega8. Program the device with
#     make fuse    # to set the clock generator, boot section size etc.
#     make flash   # to load the boot loader into flash
#     make lock    # to protect the boot loader from overwriting

KBD = KBDMOD_M3

#DEVICE = atmega128
#	F800 * 2
#BOOTLOADER_ADDRESS = 0x1F000
#JUMPSTART = 0x1FC00


DEVICE = atmega32
#	F800 * 2
BOOTLOADER_ADDRESS = 0x7000
JUMPSTART = 0x7C00


F_CPU = 12000000
#F_CPU = 20000000
#F_CPU = 16000000
FUSEE = 0xFF
FUSEH = 0xDA
FUSEL = 0xEF


# Fuse extention byte :
# 0xFF = 1 1 1 1   1 1 1 1
#        \           / ^ ^
#              |       | |
#			   |	   | +---- WDTON (watchdog timer always on)
#         don't care   +------ M103C (M103C compatibility mode) 1=NOT, 0=COMPATIBLE	   
# Fuse high byte:
# 0xD0 = 1 1 0 1   0 0 0 0 <-- BOOTRST (1=>starts @0x0000, 0=>starts @0xbootsz)
#        ^ ^ ^ ^   ^ ^ ^------ BOOTSZ0
#        | | | |   | +-------- BOOTSZ1 (11/512words(0xFE00x2), 10/1024words(0xFC00x2), 01/2048words(0xF800x2), 00/4096words(0xF000x2) 
#        | | | |   + --------- EESAVE (preserve EEPROM over chip erase)
#        | | | +-------------- CKOPT (0=>full output swing, 1=>normal)
#        | | +---------------- SPIEN (allow serial programming)
#        | +------------------ JTAGEN (0=>enable, 1=>disable)
#        +-------------------- OCDEN (0=>enable, 1=>disable)
# Fuse low byte:
# 0xEF = 1 1 1 0   1 1 1 1
#        ^ ^ \ /   \--+--/
#        | |  |       +------- CKSEL 3..0 (external >8M crystal)
#        | |  +--------------- SUT 1..0 (Fast rising power)
#        | +------------------ BODEN (0=>enable, 1=>disable)
#        +-------------------- BODLEVEL (2.7V)

###############################################################################

AVRDUDE = avrdude -c stk500v2 -P com2 -p $(DEVICE)

LDFLAGS += -Wl,--relax,-Ttext=$(BOOTLOADER_ADDRESS),--section-start=.appinboot=$(JUMPSTART),-Map=main.map

# Omit -fno-* options when using gcc 3, it does not support them.
COMPILE = avr-gcc -Wall -Os -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions -Iusbdrv -I. -mmcu=$(DEVICE) -DF_CPU=$(F_CPU) -D$(KBD) -DDEBUG_LEVEL=0 # -DTEST_MODE
# NEVER compile the final product with debugging! Any debug output will
# distort timing so that the specs can't be met.

OBJECTS =  usbdrv/usbdrvasm.o usbdrv/oddebug.o main.o


# symbolic targets:
all:	main.hex

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

.c.s:
	$(COMPILE) -S $< -o $@

flash:	all
	$(AVRDUDE) -U flash:w:main.hex:i

readflash:
	$(AVRDUDE) -U flash:r:read.hex:i

fuse:
	$(AVRDUDE) -U hfuse:w:$(FUSEH):m -U lfuse:w:$(FUSEL):m

lock:
	$(AVRDUDE) -U lock:w:0x2f:m

read_fuses:
	$(UISP) --rd_fuses

clean:
	rm -f main.hex main.bin *.o usbdrv/*.o main.s usbdrv/oddebug.s usbdrv/usbdrv.s

# file targets:
main.bin:	$(OBJECTS)
	$(COMPILE) -o main.bin $(OBJECTS) $(LDFLAGS)

main.hex:	main.bin
	rm -f main.hex main.eep.hex
	avr-objcopy -j .text -j .data -j .appinboot -O ihex main.bin main.hex
	avr-size main.hex

disasm:	main.bin
	avr-objdump -d main.bin

cpp:
	$(COMPILE) -E main.c
