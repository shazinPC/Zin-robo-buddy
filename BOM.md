# Bill of Materials

| # | Component | Qty | Specs / Notes |
|---|---|---|---|
| 1 | ESP32-C3 Super Mini | 1 | Main microcontroller, native USB |
| 2 | 0.96" OLED display | 1 | SSD1306, I2C, 4-pin, 128x64 |
| 3 | Li-ion / LiPo battery | 1 | 3.7 V, 300 mAh |
| 4 | USB Type-C charging module | 1 | Charges the 3.7 V cell; choose one with battery protection |
| 5 | General-purpose PCB (perfboard) | 1 | For mounting and wiring |

## Sourcing

No single shop stocked everything, so parts came from a mix of online stores and local markets. All of these are common parts and are available from most electronics retailers.

## Notes

- Header pins were soldered to the ESP32-C3 Super Mini for the breadboard-style bring-up tests.
- Always use a charging module designed for single-cell Li-ion/LiPo batteries, and check polarity before connecting the battery.
