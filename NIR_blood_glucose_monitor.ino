/**
 * NIR Blood Glucose Monitor - Main Firmware
 * Arduino Nano based non-invasive glucose monitoring system
 * 
 * Hardware:
 * - Arduino Nano microcontroller
 * - IR LED 940nm for non-invasive sensing
 * - Photodiode with LM358 op-amp amplification
 * - 0.96" OLED I2C display
 * - TP4056 Li-ion charging module
 * - Li-ion battery (3.7V)
 * 
 * Author: DanCam16
 * Date: 2024
 */

// ===== INCLUDE REQUIRED LIBRARIES =====
#include "config.h"
#include "signal_processing.h"
#include "calibration.h"
#include "display.h"
#include "power_management.h"
#include <Wire.h>
#include <EEPROM.h>

// ===== GLOBAL OBJECTS =====
DisplayManager display;
SignalProcessor signalProcessor;
CalibrationManager calibrationManager;
BatteryManager batteryManager;
PowerStateManager powerStateManager(batteryManager);

// ===== STATE VARIABLES =====
enum OperatingMode {
    MODE_NORMAL_READING,
    MODE_CALIBRATION,
    MODE_MENU,
    MODE_POWER_WARNING,
    MODE_ERROR
};

OperatingMode currentMode = MODE_NORMAL_READING;
unsigned long lastDisplayUpdate = 0;
unsigned long lastSensorRead = 0;
unsigned long lastCalibrationCheck = 0;

float currentGlucoseReading = 0.0;
int calibrationStep = 0;
float calibrationReference[CALIBRATION_POINTS];

// ===== DATA LOGGING VARIABLES =====
struct LogEntry {
    uint32_t timestamp;
    float glucoseValue;
    float batteryVoltage;
    int signalQuality;
};

LogEntry dataLog[MAX_LOG_ENTRIES];
int logIndex = 0;

// ===== SETUP FUNCTION =====
void setup() {
    // Initialize Serial communication for debugging
    Serial.begin(9600);
    delay(100);
    
    Serial.println("\n\n===== NIR BLOOD GLUCOSE MONITOR - STARTUP =====");
    Serial.println("Initializing systems...\n");

    // Initialize pins
    initializePins();

    // Initialize display
    if (!display.initialize()) {
        Serial.println("ERROR: Failed to initialize display!");
        currentMode = MODE_ERROR;
    } else {
        display.showSplashScreen();
    }

    // Initialize ADC
    initializeADC();

    // Load calibration data from EEPROM
    if (!calibrationManager.loadPreviousCalibration()) {
        Serial.println("No previous calibration found. Please calibrate device.");
        display.showError("Calibration Required!");
        delay(2000);
    }

    // Check battery status
    batteryManager.update();
    Serial.print("Initial Battery: ");
    batteryManager.printStatus();

    // Initialize signal processor
    signalProcessor.reset();

    Serial.println("\n===== SYSTEM READY =====\n");
    
    // Show initial screen
    if (calibrationManager.isCalibrated()) {
        currentMode = MODE_NORMAL_READING;
    } else {
        currentMode = MODE_CALIBRATION;
        startCalibrationSequence();
    }
}

// ===== MAIN LOOP =====
void loop() {
    // Update power state
    batteryManager.update();
    powerStateManager.update();

    // Check if device can continue operation
    if (!powerStateManager.canContinueOperation()) {
        currentMode = MODE_POWER_WARNING;
        display.showError("BATTERY CRITICAL!");
        delay(5000);
        return;
    }

    // Handle different operating modes
    switch (currentMode) {
        case MODE_NORMAL_READING:
            handleNormalReading();
            break;

        case MODE_CALIBRATION:
            handleCalibrationMode();
            break;

        case MODE_MENU:
            handleMenuMode();
            break;

        case MODE_POWER_WARNING:
            handlePowerWarning();
            break;

        case MODE_ERROR:
            handleErrorMode();
            break;

        default:
            currentMode = MODE_NORMAL_READING;
            break;
    }

    // Update display periodically
    if (millis() - lastDisplayUpdate >= OLED_UPDATE_RATE) {
        updateDisplay();
        lastDisplayUpdate = millis();
    }
}

// ===== HELPER FUNCTIONS =====

/**
 * Initialize Arduino pins
 */
void initializePins() {
    // Photodiode analog input (A0) - configured as input
    pinMode(PHOTODIODE_ANALOG_PIN, INPUT);

    // Battery ADC pin (A2) - configured as input
    pinMode(BATTERY_ADC_PIN, INPUT);

    // IR LED PWM output (D3) - configured as output
    pinMode(IR_LED_PWM_PIN, OUTPUT);
    digitalWrite(IR_LED_PWM_PIN, HIGH); // Keep IR LED on (you can add PWM modulation)

    Serial.println("Pins initialized successfully");
}

/**
 * Initialize ADC for optimal performance
 */
void initializeADC() {
    // Set ADC reference to DEFAULT (5V for Arduino Nano)
    analogReference(DEFAULT);

    // Set ADC prescaler for 1MHz ADC clock (fast sampling)
    // Prescaler = 16 for 16MHz CPU -> 1MHz ADC clock
    ADCSRA &= ~(0b111); // Clear prescaler bits
    ADCSRA |= 0b100;    // Set prescaler to 16

    // Enable ADC
    ADCSRA |= (1 << ADEN);

    // Read once to stabilize
    analogRead(PHOTODIODE_ANALOG_PIN);
    delay(10);

    Serial.println("ADC initialized (1MHz clock, 10-bit resolution)");
}

/**
 * Read photodiode signal from ADC
 */
int readPhotodiodeSignal() {
    int rawADC = analogRead(PHOTODIODE_ANALOG_PIN);
    return rawADC;
}

/**
 * Handle normal glucose reading mode
 */
void handleNormalReading() {
    // Acquire sensor data
    if (millis() - lastSensorRead >= SAMPLING_INTERVAL) {
        int rawADC = readPhotodiodeSignal();
        float processedSignal = signalProcessor.processSignal(rawADC);

        // Every MOVING_AVG_WINDOW samples, update glucose estimate
        static int sampleCounter = 0;
        sampleCounter++;

        if (sampleCounter >= MOVING_AVG_WINDOW) {
            sampleCounter = 0;

            // Convert to glucose reading
            currentGlucoseReading = calibrationManager.getGlucoseEstimate(processedSignal);

            // Log data
            logData(currentGlucoseReading, batteryManager.getVoltage());

            // Print debug info
            Serial.print("Glucose: ");
            Serial.print(currentGlucoseReading, 1);
            Serial.print(" mg/dL | Signal: ");
            Serial.print(processedSignal, 2);
            Serial.print("V | ADC: ");
            Serial.println(rawADC);
        }

        lastSensorRead = millis();
    }

    // Check for user input (e.g., button press to enter menu) - add later
}

/**
 * Handle calibration mode
 */
void handleCalibrationMode() {
    static unsigned long calibrationTimeout = 0;
    static float calibrationSamples[CALIBRATION_SAMPLES];
    static int sampleCounter = 0;

    // Collect samples for current calibration point
    int rawADC = readPhotodiodeSignal();
    float processedSignal = signalProcessor.processSignal(rawADC);

    if (millis() - lastSensorRead >= SAMPLING_INTERVAL) {
        calibrationSamples[sampleCounter] = processedSignal;
        sampleCounter++;
        lastSensorRead = millis();

        if (sampleCounter >= CALIBRATION_SAMPLES) {
            // Average the samples
            float avgSignal = 0.0;
            for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
                avgSignal += calibrationSamples[i];
            }
            avgSignal /= CALIBRATION_SAMPLES;

            // Add to calibration
            calibrationManager.addCalibrationReading(calibrationReference[calibrationStep], avgSignal);

            calibrationStep++;
            sampleCounter = 0;

            if (calibrationStep >= CALIBRATION_POINTS) {
                // Finish calibration
                if (calibrationManager.finishCalibration()) {
                    Serial.println("Calibration completed successfully!");
                    display.showSplashScreen();
                    currentMode = MODE_NORMAL_READING;
                    signalProcessor.reset();
                } else {
                    Serial.println("Calibration failed!");
                    display.showError("Calibration Failed!");
                    delay(2000);
                    currentMode = MODE_CALIBRATION;
                    calibrationStep = 0;
                }
            }
        }
    }

    // Update display
    display.showCalibrationScreen(calibrationStep + 1, CALIBRATION_POINTS, 
                                   calibrationReference[calibrationStep]);
}

/**
 * Start calibration sequence
 */
void startCalibrationSequence() {
    Serial.println("\n===== STARTING CALIBRATION =====");
    calibrationManager.startCalibration();
    
    // Set reference glucose values (you would input these based on blood test)
    // Example values - adjust based on actual reference measurements
    calibrationReference[0] = 100.0;  // First reference point
    calibrationReference[1] = 200.0;  // Second reference point
    if (CALIBRATION_POINTS > 2) {
        calibrationReference[2] = 300.0; // Third reference point (if applicable)
    }

    calibrationStep = 0;
    currentMode = MODE_CALIBRATION;
}

/**
 * Handle menu mode
 */
void handleMenuMode() {
    // Implement menu navigation here
    // For now, just return to normal mode
    currentMode = MODE_NORMAL_READING;
}

/**
 * Handle power warning mode
 */
void handlePowerWarning() {
    display.showError(powerStateManager.getRecommendedAction());
    delay(1000);
    
    // Try to recover to normal operation
    if (!batteryManager.isLow()) {
        currentMode = MODE_NORMAL_READING;
    }
}

/**
 * Handle error mode
 */
void handleErrorMode() {
    display.showError("System Error!");
    delay(5000);
}

/**
 * Update OLED display
 */
void updateDisplay() {
    if (!display.isInit()) return;

    switch (currentMode) {
        case MODE_NORMAL_READING:
            display.showGlucoseReading(currentGlucoseReading, 
                                      batteryManager.getChargePercent(),
                                      calibrationManager.isCalibrated());
            break;

        case MODE_CALIBRATION:
            // Handled in handleCalibrationMode()
            break;

        case MODE_POWER_WARNING:
            display.showBatteryStatus(batteryManager.getVoltage(),
                                     batteryManager.getChargePercent(),
                                     batteryManager.getChargingStatus());
            break;

        default:
            break;
    }
}

/**
 * Log glucose reading to memory
 */
void logData(float glucose, float battery) {
    if (logIndex >= MAX_LOG_ENTRIES) {
        logIndex = 0; // Circular buffer
    }

    dataLog[logIndex].timestamp = millis() / 1000; // Convert to seconds
    dataLog[logIndex].glucoseValue = glucose;
    dataLog[logIndex].batteryVoltage = battery;
    dataLog[logIndex].signalQuality = 100; // Placeholder

    logIndex++;
}

/**
 * Print logged data to Serial
 */
void printLogData() {
    Serial.println("\n===== DATA LOG =====");
    Serial.println("Time(s)\tGlucose(mg/dL)\tBattery(V)");
    
    for (int i = 0; i < min(logIndex, MAX_LOG_ENTRIES); i++) {
        Serial.print(dataLog[i].timestamp);
        Serial.print("\t");
        Serial.print(dataLog[i].glucoseValue);
        Serial.print("\t");
        Serial.println(dataLog[i].batteryVoltage, 2);
    }
    Serial.println("====================\n");
}
