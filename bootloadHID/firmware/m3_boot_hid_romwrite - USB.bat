avrdude -c avrispmkII -P USB -p atmega32 -U hfuse:w:0xC0:m -U lfuse:w:0xcf:m

avrdude -c avrispmkII -P USB -p atmega32 -U flash:w:main.hex:i
pause;