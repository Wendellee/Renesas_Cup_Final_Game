# P406 LED change rollback snapshot

Created before adding remote control of the vehicle LED on P406.

## Source revisions

- Controller commit: `b6bbdd5e4ae6559e766911c7b7bde4d8cbb9fa78`
- Vehicle commit: `ffb89767a46db211f3d4f31180bcda210af807ff`

## SHA256

```text
5BEA8FC7AE90E2AC24485D4B81CA8FFB919CDBD6CD1DFCB625371ED33AE4C211  controller_events_init.c
B7C91F3E4A2D82711E3F6D1ED11CC43346209E5F1D8839EBE697E897C38EF052  vehicle_command_radio.c
C08EFC36FBF7201B515B547436ADC5F4066B7E8BAE372D354052870F68810F3A  Renesas_Cup_Vehicle_CPU1_before_p406_led.srec
F3DB4D2DCE388B0697D5DB08D8212FFEE307396EAE84AC09D8EA42C593EF965D  Renesas_Cup_Vehicle_CPU1_before_p406_led.elf
```

The controller project had no firmware image at snapshot time, so its complete pre-change `events_init.c` is retained here.
