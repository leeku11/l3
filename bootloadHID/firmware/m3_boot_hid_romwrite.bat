avrdude -c stk500v2 -P com3 -p atmega32 -U hfuse:w:0xD0:m -U lfuse:w:0xAE:m
avrdude -c stk500v2 -P com3 -p atmega32 -U flash:w:main.hex:i
