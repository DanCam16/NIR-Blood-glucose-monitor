# NIR Blood Glucose Monitor - IR LED & Photodiode

A non-invasive blood glucose monitoring system using IR LED (940nm) and photodiode pair with Arduino Nano and 0.96" OLED display.

## Features

- **Real-time Sensor Reading**: IR LED (940nm) & photodiode pair with LM358 op-amp amplification
- **Signal Processing**: Low-pass and high-pass filtering for noise reduction
- **OLED Display**: 0.96" I2C OLED real-time visualization
- **Calibration**: Multi-point calibration algorithm for accurate glucose readings
- **Power Management**: TP4056 charging module integration with battery monitoring
- **Portable**: Arduino Nano-based compact design

## Hardware Components

| Component | Specifications |
|-----------|----------------|
| Microcontroller | Arduino Nano |
| IR LED | 940nm wavelength |
| Photodiode | PIN photodiode with LM358 amplifier |
| Op-Amp | LM358 (dual op-amp) |
| Display | 0.96" OLED I2C |
| Power Module | TP4056 Li-ion charging module |
| Support Components | Resistors, capacitors, and breadboard/PCB |

## Circuit Configuration

### LM358 Op-Amp Configuration
- **Non-inverting amplifier** for photodiode signal
- Gain = 1 + (Rf/Rin) - adjustable via resistor values
- Low-pass filter cutoff frequency: ~1kHz

### ADC Sampling
- Arduino Nano ADC: 10-bit resolution
- Sampling rate: 1000 Hz
- Analog reference: 5V

### OLED Display
- I2C Communication (A4=SDA, A5=SCL)
- Address: 0x3C

## Pin Configuration

| Arduino Nano | Connection |
|--------------|------------|
| A0 | Photodiode amplified signal (LM358 output) |
| A4 | OLED SDA |
| A5 | OLED SCL |
| D3 | IR LED PWM control (optional) |
| D2 | Battery level input (ADC) |

## Getting Started

1. Install Arduino IDE and required libraries:
   - `Adafruit_SSD1306`
   - `Adafruit_GFX`

2. Connect hardware according to circuit diagram

3. Upload firmware to Arduino Nano

4. Run calibration with known glucose reference values

5. Start monitoring!

## Project Structure

```
NIR-blood-glucose-monitor/
├── README.md
├── NIR_blood_glucose_monitor.ino   # Main firmware
├── config.h                        # Configuration constants
├── signal_processing.h             # Filtering algorithms
├── calibration.h                   # Calibration routines
├── display.h                       # OLED display functions
└── power_management.h              # Battery & power functions
```

## Safety Warning

⚠️ **This is a prototype/experimental device.** Do not rely on this for medical decisions without proper validation and FDA/regulatory approval. Always consult healthcare professionals for glucose monitoring.

## License

MIT License

## Author

DanCam16

---

For questions or contributions, please open an issue or pull request.
