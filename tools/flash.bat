avrdude -c stk500 -P com2 -p atmega128 -U hfuse:w:0xDB:m -U lfuse:w:0xEF:m
avrdude -c stk500 -P com2 -p atmega128 -U flash:w:main.hex:i
pause;