#ifndef CONFIG_H
#define CONFIG_H

// ===== MICROCONTROLLER CONFIGURATION =====
#define ARDUINO_NANO

// ===== PIN DEFINITIONS =====
#define PHOTODIODE_ANALOG_PIN    A0    // Amplified photodiode signal from LM358
#define OLED_SDA_PIN             A4    // OLED I2C SDA
#define OLED_SCL_PIN             A5    // OLED I2C SCL
#define IR_LED_PWM_PIN           3     // Optional PWM control for IR LED
#define BATTERY_ADC_PIN          A2    // Battery voltage monitoring
#define REFERENCE_VOLTAGE        5.0   // Arduino reference voltage (5V)

// ===== ADC CONFIGURATION =====
#define ADC_RESOLUTION           10    // 10-bit ADC (0-1023)
#define ADC_MAX_VALUE            1023  // Max ADC value
#define SAMPLING_RATE            1000  // Samples per second (Hz)
#define SAMPLING_INTERVAL        1     // milliseconds (1000/SAMPLING_RATE)

// ===== SENSOR CONFIGURATION =====
#define IR_LED_WAVELENGTH        940   // IR LED wavelength in nanometers
#define PHOTODIODE_TYPE          "PIN" // PIN or APD photodiode
#define LM358_GAIN               10.0  // Op-amp gain (1 + Rf/Rin)
#define SIGNAL_THRESHOLD         50    // Minimum signal threshold (ADC counts)

// ===== FILTERING CONFIGURATION =====
#define FILTER_ENABLE            true
#define LOW_PASS_CUTOFF          1.0   // Hz (filters high-frequency noise)
#define HIGH_PASS_CUTOFF         0.05  // Hz (removes baseline drift)
#define MOVING_AVG_WINDOW        5     // Moving average window size (samples)
#define IIR_ALPHA                0.1   // IIR filter coefficient (0-1, lower = more filtering)

// ===== OLED DISPLAY CONFIGURATION =====
#define OLED_ADDRESS             0x3C  // I2C address for 0.96" OLED
#define OLED_WIDTH               128   // pixels
#define OLED_HEIGHT              64    // pixels
#define OLED_UPDATE_RATE         500   // milliseconds
#define OLED_BRIGHTNESS          255   // 0-255

// ===== CALIBRATION CONFIGURATION =====
#define CALIBRATION_POINTS       3     // Number of reference points for calibration
#define MIN_GLUCOSE_REFERENCE    70    // mg/dL (lower calibration point)
#define MAX_GLUCOSE_REFERENCE    300   // mg/dL (upper calibration point)
#define CALIBRATION_SAMPLES      50    // Samples to average per calibration point

// ===== POWER MANAGEMENT CONFIGURATION =====
#define BATTERY_MAX_VOLTAGE      4.2   // Li-ion max voltage
#define BATTERY_MIN_VOLTAGE      3.0   // Li-ion min voltage
#define BATTERY_ADC_DIVIDER      2     // Voltage divider ratio
#define LOW_BATTERY_THRESHOLD    3.2   // Voltage threshold for warning
#define TP4056_CHARGE_COMPLETE   2     // Digital pin for charge complete indicator (optional)

// ===== DATA LOGGING CONFIGURATION =====
#define DATA_LOGGING_ENABLE      true
#define LOG_INTERVAL             5000  // milliseconds (log every 5 seconds)
#define MAX_LOG_ENTRIES          1000  // Maximum entries in circular buffer

// ===== SIGNAL PROCESSING BUFFERS =====
#define BUFFER_SIZE              128   // Circular buffer size for real-time processing
#define FFT_SIZE                 128   // FFT size for frequency analysis (optional)

// ===== CALIBRATION DATA STORAGE =====
#define CALIBRATION_EEPROM_ADDR 0     // EEPROM starting address for calibration data
#define CALIBRATION_DATA_SIZE    32    // Bytes to store calibration coefficients

#endif // CONFIG_H
