avrdude -c usbasp -P USB -p atmega32 -U hfuse:w:0xC0:m -U lfuse:w:0x0E:m
avrdude -c usbasp -P USB -p atmega32 -U flash:w:main87kin.hex:i