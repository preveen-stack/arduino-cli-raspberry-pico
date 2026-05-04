This README provides a comprehensive guide for using the **Pico Control Menu**, a command-line interface tool developed for the Raspberry Pi Pico using the `arduino-cli`. It covers system monitoring, LED control, and I2S audio generation.

---

# Pico Control Menu (RP2040)

A terminal-based diagnostic and control system for the Raspberry Pi Pico. This project allows you to manage hardware features through a Serial interface, including a custom I2S sine wave generator and real-time system clock monitoring.

## 🚀 Quick Start (CLI)

To compile and upload this project using the **Arduino CLI**, use the following command. Note the specific frequency flag to ensure the system runs at your desired underclocked speed.

```bash
# Compile and Upload (Targeting 125MHz)
arduino-cli compile --fqbn rp2040:rp2040:rpipico:freq=125 --upload -p /dev/cu.usbmodem14101 PicoBlink

# Connect to the Menu
arduino-cli monitor -p /dev/cu.usbmodem14101
```

---

## 🛠 Features

### 1. System Diagnostics
*   **Clock Monitoring:** View real-time System, USB, and ADC clock frequencies.
*   **Temperature Sensing:** Read the internal RP2040 CPU temperature using the onboard ADC.
*   **Pinout Reference:** Instant ASCII pinout map displayed in the terminal for easy wiring.

### 2. LED Control
*   **Blink Control:** Toggle the onboard LED (GP25).
*   **Variable Frequency:** Set blink rates from $1\text{ms}$ up to $10\text{s}$.

### 3. I2S Audio Engine
*   **Programmable I/O (PIO):** Uses the RP2040 PIO state machines for jitter-free audio.
*   **Adjustable Tone:** Generate sine waves from $100\text{Hz}$ to $18,000\text{Hz}$.
*   **Clock Verification:** Logical and hardware-based verification of $BCLK$ and $LRCLK$ signals.

---

## 📋 Command Reference

| Command | Description |
| :--- | :--- |
| `help` | Lists all available commands. |
| `clock` | Shows system clock speeds (e.g., 125MHz or 200MHz). |
| `temp` | Displays internal CPU temperature in Celsius. |
| `blink freq <ms>` | Sets the onboard LED interval. |
| `i2s init <sr> <bw> <ch>` | Configures I2S (Sample Rate, Bit Width, Channels). |
| `i2s start / stop` | Toggles the audio sine wave output. |
| `i2s tone <freq>` | Changes the audio frequency in Hz. |
| `i2s detail` | Shows theoretical PIO dividers and target clock values. |
| `reset` | Triggers a hardware watchdog reboot. |

---

## 🔌 Hardware Setup (I2S)

To use the audio features, connect an external I2S DAC (e.g., PCM5102A) to the following Pico pins:

| Pico Pin | I2S Signal | Function |
| :--- | :--- | :--- |
| **GP26** | **BCLK** | Bit Clock |
| **GP27** | **LRCLK** | Word Select (Left/Right) |
| **GP28** | **DOUT** | Serial Data Out |
| **GND** | **GND** | Common Ground |
| **VBUS (5V)** | **VIN** | Power Supply |



---

## ⚠️ Notes on Clock Speeds
By default, the modern RP2040 Arduino core (v5.x+) may run the Pico at **200MHz**. If your measurements or I2S timings require the original **125MHz** specification, ensure you use the `:freq=125` suffix during the `arduino-cli compile` step.

## 📄 Dependencies
*   **Core:** Earle Philhower's `rp2040` core (v5.6.0 recommended).
*   **Libraries:** `I2S` (built-in), `hardware/clocks`, `hardware/adc`, `hardware/watchdog`.
