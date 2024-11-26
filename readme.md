# Soft I2C
Features:
- various speed support (400 clock pulses per second and more on 288 MHz CPU)
- multi-bus support
- transmit/receive in blocking mode only
- no interrupts, no timers required
- concurrent access protected (mutex)
- support for reading without a register

Tested on FreeRTOS 10, Artery AT32f437, zero loss on 1000 samples

# Fork source
https://github.com/liyanboy74/soft-i2c
