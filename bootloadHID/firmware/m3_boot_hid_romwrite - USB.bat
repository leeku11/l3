avrdude -c avrispmkII -P USB -p atmega32 -U hfuse:w:0xD0:m -U lfuse:w:0x2f:m
avrdude -c avrispmkII -P USB -p atmega32 -U flash:w:main.hex:i
pause;