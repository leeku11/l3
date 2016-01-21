avrdude -c usbasp -P USB -p atmega32 -U hfuse:w:0xD0:m -U lfuse:w:0xAE:m
avrdude -c usbasp -P USB -p atmega32 -U flash:w:maincp.hex:i