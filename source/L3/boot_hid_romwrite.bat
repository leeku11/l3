avrdude -c stk500v2 -P com3 -p atmega128 -U efuse:w:0xFE:m -U hfuse:w:0xDB:m -U lfuse:w:0xEF:m

avrdude -c stk500v2 -P com3 -p atmega128 -U flash:w:kbdmodmx.hex:i

avrdude -c stk500v2 -P com3 -p atmega128 -U flash:w:keymap.hex:i

;avrdude -c stk500v2 -P com3 -p atmega128 -U lfuse:r:low:r -U hfuse:r:high:r -U lock:r:lock:r
pause;