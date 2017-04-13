avrdude -c stk500v2 -P com3 -p atmega32 -U flash:r:main_read.hex:i
pause;