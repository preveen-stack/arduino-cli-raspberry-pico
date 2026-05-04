```

--- Commands ---
i2s start/stop  - Toggle Audio
i2s sniff       - Read internal PIO registers
i2s tone <f>    - Change sine frequency
reset to <mhz>  - Reboot at specific speed
clock           - Show current SysClk
i2s config 8000 8 1
Configured: 8000Hz, 8-bit, 1-ch
New Hardware Divisor: 976.5625
i2 start
i2s sniff

--- Real-Time Hardware Verification ---
BCLK (Bit Clock):   0.064 MHz
LRCLK (Sample Rate): 2000.00 Hz
Bits Per Frame:     32
Status: TIMING MISMATCH
i2s stop   
Audio Stopped
i2s config 48000 16 2
Configured: 48000Hz, 16-bit, 2-ch
New Hardware Divisor: 40.6901
i2s start
Audio Started. Forced Divisor: 40.6901 (Reg: 0x0028B000)
i2s sniff

--- Real-Time Hardware Verification ---
BCLK (Bit Clock):   1.536 MHz
LRCLK (Sample Rate): 48003.07 Hz
Bits Per Frame:     32
Status: TIMING MATCHED
i2s stop
Audio Stopped
i2s config 22025 16 2 
Configured: 22025Hz, 16-bit, 2-ch
New Hardware Divisor: 88.6776
i2s start
Audio Started. Forced Divisor: 88.6776 (Reg: 0x0058AD00)
i2s sniff

--- Real-Time Hardware Verification ---
BCLK (Bit Clock):   0.705 MHz
LRCLK (Sample Rate): 22025.46 Hz
Bits Per Frame:     32
Status: TIMING MATCHED
i2s stop
Audio Stopped
i2s config 16000 8 1
Configured: 16000Hz, 8-bit, 1-ch
New Hardware Divisor: 488.2812
i2s start
Audio Started. Forced Divisor: 122.0703 (Reg: 0x007A1200)
i2s sniff

--- Real-Time Hardware Verification ---
BCLK (Bit Clock):   0.512 MHz
LRCLK (Sample Rate): 16000.00 Hz
Bits Per Frame:     32
Status: TIMING MATCHED
i2s stop
Audio Stopped
i2s config 8000 8 1
Configured: 8000Hz, 8-bit, 1-ch
New Hardware Divisor: 976.5625
i2s start
Audio Started. Forced Divisor: 244.1406 (Reg: 0x00F42400)
i2s sniff

--- Real-Time Hardware Verification ---
BCLK (Bit Clock):   0.256 MHz
LRCLK (Sample Rate): 8000.00 Hz
Bits Per Frame:     32
Status: TIMING MATCHED
i2s config 96000 16 2 
Configured: 96000Hz, 16-bit, 2-ch
New Hardware Divisor: 20.3451
i2s start
Audio Started. Forced Divisor: 20.3451 (Reg: 0x00145800)
i2s sniff

--- Real-Time Hardware Verification ---
BCLK (Bit Clock):   3.072 MHz
LRCLK (Sample Rate): 96006.14 Hz
Bits Per Frame:     32
Status: TIMING MATCHED
^C
Preveens-MacBook-Pro:arduino-cli preveen$ make monitor

i2s config 192000 16 2
Configured: 192000Hz, 16-bit, 2-ch
New Hardware Divisor: 10.1725
i2s start
Audio Started. Forced Divisor: 10.1725 (Reg: 0x000A2C00)
i2s sniff

--- Real-Time Hardware Verification ---
BCLK (Bit Clock):   6.144 MHz
LRCLK (Sample Rate): 192012.28 Hz
Bits Per Frame:     32
Status: TIMING MATCHED

i2s config 384000 16 2
Configured: 384000Hz, 16-bit, 2-ch
New Hardware Divisor: 5.0863
i2s sniff

--- Real-Time Hardware Verification ---
BCLK (Bit Clock):   12.289 MHz
LRCLK (Sample Rate): 384024.56 Hz
Bits Per Frame:     32
Status: TIMING MATCHED
i2s config 768000 16 2
Configured: 768000Hz, 16-bit, 2-ch
New Hardware Divisor: 2.5431
i2s sniff

--- Real-Time Hardware Verification ---
BCLK (Bit Clock):   24.578 MHz
LRCLK (Sample Rate): 768049.12 Hz
Bits Per Frame:     32
Status: TIMING MATCHED

```
