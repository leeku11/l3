avrdude -c usbasp -P USB -p atmega32 -U flash:r:main_read.hex:i
pause;