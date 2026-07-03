# Lab 6 - 4×4 Keypad on the Port Expander

C exercises for the ATmega328PB reading a 4×4 matrix keypad via the PCA9555 expander over TWI.

## ex6.1 - Keypad Scanning
Builds a small keypad library: `scan_row` checks one row for pressed keys, `scan_keypad` calls it across all four rows, and `scan_keypad_rising_edge` detects newly pressed keys with debouncing, keeping state in a 16-bit `pressed_keys` variable. `keypad_to_ascii` returns the ASCII code of the pressed key (0 if none), and four keys ("4","2","3","B") are mapped to LEDs PB1-PB4.

## ex6.3 - Password System
Implements an electronic lock: entering the correct two-digit group code lights LEDs PB0-PB5 for 3 seconds, while a wrong code blinks them (500 ms on/off) for 6 seconds. Each key press counts once regardless of how long it is held, and no new digits are accepted for 5 seconds after a two-digit entry.
