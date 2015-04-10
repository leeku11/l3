avrdude -c stk500v2 -P com3 -p atmega32 -U hfuse:w:0xD0:m -U lfuse:w:0x1F:m

avrdude -c stk500v2 -P com3 -p atmega32 -U flash:w:l3_finger_150325.hex:i
pause;