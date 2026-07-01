#ifndef POWER_MANAGEMENT_H
#define POWER_MANAGEMENT_H

#include "config.h"

// ===== BATTERY MANAGEMENT CLASS =====
class BatteryManager {
private:
    float currentVoltage;
    int currentPercent;
    bool isCharging;
    unsigned long lastCheckTime;
    float voltageReadings[10];
    int readingIndex;

public:
    BatteryManager() : currentVoltage(0.0), currentPercent(100), 
                      isCharging(false), lastCheckTime(0), readingIndex(0) {
        for (int i = 0; i < 10; i++) {
            voltageReadings[i] = 4.2; // Start at max
        }
    }

    /**
     * Read battery voltage from ADC
     * Note: Voltage divider reduces voltage by BATTERY_ADC_DIVIDER
     */
    float readBatteryVoltage() {
        int rawADC = analogRead(BATTERY_ADC_PIN);
        float adcVoltage = (rawADC / ADC_MAX_VALUE) * REFERENCE_VOLTAGE;
        float batteryVoltage = adcVoltage * BATTERY_ADC_DIVIDER;
        
        return batteryVoltage;
    }

    /**
     * Update battery status with smoothing
     */
    void update() {
        // Update every 1 second to avoid excessive ADC reads
        if (millis() - lastCheckTime < 1000) return;
        lastCheckTime = millis();

        // Read voltage and apply smoothing (moving average)
        float newVoltage = readBatteryVoltage();
        voltageReadings[readingIndex] = newVoltage;
        readingIndex = (readingIndex + 1) % 10;

        // Calculate average voltage
        float avgVoltage = 0.0;
        for (int i = 0; i < 10; i++) {
            avgVoltage += voltageReadings[i];
        }
        currentVoltage = avgVoltage / 10.0;

        // Update charge percentage
        updateChargePercentage();

        // Check charging status (optional: requires charge complete pin)
        checkChargingStatus();
    }

    /**
     * Convert voltage to percentage
     * Li-ion: 3.0V = 0%, 4.2V = 100%
     */
    void updateChargePercentage() {
        float minVolt = BATTERY_MIN_VOLTAGE;
        float maxVolt = BATTERY_MAX_VOLTAGE;

        if (currentVoltage <= minVolt) {
            currentPercent = 0;
        } else if (currentVoltage >= maxVolt) {
            currentPercent = 100;
        } else {
            // Linear approximation
            currentPercent = (int)(((currentVoltage - minVolt) / (maxVolt - minVolt)) * 100);
        }

        // Constrain to 0-100
        currentPercent = constrain(currentPercent, 0, 100);
    }

    /**
     * Check if device is charging
     * Optional: Requires TP4056 charge complete pin connection
     */
    void checkChargingStatus() {
        // If TP4056_CHARGE_COMPLETE pin is defined, read it
        // TP4056 outputs LOW when charging, HIGH when complete/not charging
        // This is a simplified implementation - adjust based on your TP4056 setup
        
        if (currentVoltage >= 4.1) {
            isCharging = false; // Battery nearly full
        }
    }

    /**
     * Get current battery voltage
     */
    float getVoltage() {
        return currentVoltage;
    }

    /**
     * Get battery charge percentage
     */
    int getChargePercent() {
        return currentPercent;
    }

    /**
     * Check if battery is low
     */
    bool isLow() {
        return currentVoltage < LOW_BATTERY_THRESHOLD;
    }

    /**
     * Check if battery is critical (device should shutdown)
     */
    bool isCritical() {
        return currentVoltage < BATTERY_MIN_VOLTAGE;
    }

    /**
     * Get charging status
     */
    bool getChargingStatus() {
        return isCharging;
    }

    /**
     * Print battery information
     */
    void printStatus() {
        Serial.print("Battery Voltage: ");
        Serial.print(currentVoltage, 2);
        Serial.print("V | Charge: ");
        Serial.print(currentPercent);
        Serial.print("% | Charging: ");
        Serial.println(isCharging ? "YES" : "NO");
    }

    /**
     * Estimate remaining runtime (simplified)
     */
    int estimateRuntimeMinutes() {
        // Assuming ~100mA average current draw
        // Runtime = (mAh / mA)
        // Li-ion 500mAh battery: 500mAh / 100mA = 5 hours
        int estimatedCapacity = 500; // mAh (adjust for your battery)
        int currentDrawn = 100; // mA (approximate)
        return (estimatedCapacity / currentDrawn) * 60; // minutes
    }

    /**
     * Enter low power mode (for future use)
     */
    void enterLowPowerMode() {
        Serial.println("Entering low power mode...");
        // Reduce sampling rate, disable display updates, etc.
        // Implementation depends on your specific needs
    }

    /**
     * Exit low power mode
     */
    void exitLowPowerMode() {
        Serial.println("Exiting low power mode...");
    }
};

// ===== POWER STATE MANAGER =====
class PowerStateManager {
private:
    enum PowerState {
        PS_NORMAL,
        PS_LOW_BATTERY_WARNING,
        PS_CRITICAL,
        PS_SLEEP
    };

    PowerState currentState;
    BatteryManager& batteryManager;
    unsigned long lastStateChangeTime;

public:
    PowerStateManager(BatteryManager& bm) 
        : currentState(PS_NORMAL), batteryManager(bm), lastStateChangeTime(0) {}

    /**
     * Update power state based on battery status
     */
    void update() {
        PowerState newState = currentState;

        if (batteryManager.isCritical()) {
            newState = PS_CRITICAL;
        } else if (batteryManager.isLow()) {
            newState = PS_LOW_BATTERY_WARNING;
        } else {
            newState = PS_NORMAL;
        }

        // If state changed, handle transition
        if (newState != currentState) {
            handleStateChange(currentState, newState);
            currentState = newState;
            lastStateChangeTime = millis();
        }
    }

    /**
     * Handle state transitions
     */
    void handleStateChange(PowerState oldState, PowerState newState) {
        Serial.print("Power State Changed: ");
        printState(oldState);
        Serial.print(" -> ");
        printState(newState);
        Serial.println();

        switch (newState) {
            case PS_NORMAL:
                Serial.println("System operating normally");
                break;

            case PS_LOW_BATTERY_WARNING:
                Serial.println("WARNING: Low battery! Consider charging.");
                // Could trigger display warning
                break;

            case PS_CRITICAL:
                Serial.println("CRITICAL: Battery critical! Entering shutdown...");
                // Perform emergency shutdown
                // Save any data to EEPROM
                enterEmergencyShutdown();
                break;

            case PS_SLEEP:
                Serial.println("Entering sleep mode");
                break;
        }
    }

    /**
     * Emergency shutdown routine
     */
    void enterEmergencyShutdown() {
        // Save current readings to EEPROM
        // Disable non-essential systems
        // Flash warning on OLED
        delay(2000);
        // Could trigger watchdog reset or actual shutdown
    }

    /**
     * Get current power state
     */
    PowerState getState() {
        return currentState;
    }

    /**
     * Check if device can continue normal operation
     */
    bool canContinueOperation() {
        return currentState != PS_CRITICAL;
    }

    /**
     * Get recommended action for user
     */
    const char* getRecommendedAction() {
        switch (currentState) {
            case PS_NORMAL:
                return "Normal operation";
            case PS_LOW_BATTERY_WARNING:
                return "Please charge device soon";
            case PS_CRITICAL:
                return "CHARGE NOW - Device shutting down";
            case PS_SLEEP:
                return "Device in sleep mode";
            default:
                return "Unknown";
        }
    }

private:
    void printState(PowerState state) {
        switch (state) {
            case PS_NORMAL:
                Serial.print("NORMAL");
                break;
            case PS_LOW_BATTERY_WARNING:
                Serial.print("LOW_BATTERY_WARNING");
                break;
            case PS_CRITICAL:
                Serial.print("CRITICAL");
                break;
            case PS_SLEEP:
                Serial.print("SLEEP");
                break;
        }
    }
};

#endif // POWER_MANAGEMENT_H
