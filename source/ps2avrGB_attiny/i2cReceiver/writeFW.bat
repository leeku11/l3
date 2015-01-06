avrdude -c stk500v2 -P com3 -p attiny85 -U flash:w:main.hex:i
pause;