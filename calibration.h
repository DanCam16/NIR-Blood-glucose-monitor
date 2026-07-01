#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "config.h"
#include <EEPROM.h>

// ===== CALIBRATION DATA STRUCTURE =====
struct CalibrationData {
    float slope;        // Calibration curve slope
    float intercept;    // Calibration curve intercept
    float r_squared;    // R² value (goodness of fit)
    bool isCalibrated;  // Calibration status flag
    uint32_t timestamp; // Last calibration timestamp
};

// ===== LINEAR REGRESSION CALIBRATION =====
class LinearCalibration {
private:
    float referenceGlucose[CALIBRATION_POINTS];
    float sensorReadings[CALIBRATION_POINTS];
    int calibrationCount;
    CalibrationData calData;

public:
    LinearCalibration() : calibrationCount(0) {
        calData.slope = 1.0;
        calData.intercept = 0.0;
        calData.r_squared = 0.0;
        calData.isCalibrated = false;
        calData.timestamp = 0;
    }

    /**
     * Add a calibration point
     * @param glucoseRef: Reference glucose value (mg/dL) from blood test
     * @param sensorValue: Sensor reading (voltage or ADC counts)
     */
    bool addCalibrationPoint(float glucoseRef, float sensorValue) {
        if (calibrationCount >= CALIBRATION_POINTS) {
            Serial.println("ERROR: Calibration buffer full!");
            return false;
        }

        referenceGlucose[calibrationCount] = glucoseRef;
        sensorReadings[calibrationCount] = sensorValue;
        calibrationCount++;

        Serial.print("Calibration Point ");
        Serial.print(calibrationCount);
        Serial.print(": Glucose = ");
        Serial.print(glucoseRef);
        Serial.print(" mg/dL, Sensor = ");
        Serial.println(sensorValue);

        return true;
    }

    /**
     * Calculate linear regression: Glucose = slope * SensorValue + intercept
     * Using least squares method: y = mx + b
     */
    bool performCalibration() {
        if (calibrationCount < 2) {
            Serial.println("ERROR: Need at least 2 calibration points!");
            return false;
        }

        // Calculate means
        float meanGlucose = 0.0, meanSensor = 0.0;
        for (int i = 0; i < calibrationCount; i++) {
            meanGlucose += referenceGlucose[i];
            meanSensor += sensorReadings[i];
        }
        meanGlucose /= calibrationCount;
        meanSensor /= calibrationCount;

        // Calculate slope (m) and intercept (b)
        float numerator = 0.0, denominator = 0.0;
        for (int i = 0; i < calibrationCount; i++) {
            numerator += (referenceGlucose[i] - meanGlucose) * (sensorReadings[i] - meanSensor);
            denominator += pow(sensorReadings[i] - meanSensor, 2);
        }

        if (denominator == 0.0) {
            Serial.println("ERROR: Calibration denominator is zero!");
            return false;
        }

        calData.slope = numerator / denominator;
        calData.intercept = meanGlucose - (calData.slope * meanSensor);

        // Calculate R² (coefficient of determination)
        float ssRes = 0.0, ssTot = 0.0;
        for (int i = 0; i < calibrationCount; i++) {
            float predicted = calData.slope * sensorReadings[i] + calData.intercept;
            ssRes += pow(referenceGlucose[i] - predicted, 2);
            ssTot += pow(referenceGlucose[i] - meanGlucose, 2);
        }
        calData.r_squared = 1.0 - (ssRes / ssTot);

        calData.isCalibrated = true;
        calData.timestamp = millis() / 1000; // Unix-like timestamp in seconds

        // Print calibration results
        printCalibrationResults();

        return true;
    }

    /**
     * Convert sensor reading to glucose value using calibration
     * @param sensorValue: Raw sensor reading
     * @return: Estimated glucose in mg/dL
     */
    float sensorToGlucose(float sensorValue) {
        if (!calData.isCalibrated) {
            Serial.println("WARNING: Device not calibrated! Returning raw value.");
            return sensorValue;
        }
        return calData.slope * sensorValue + calData.intercept;
    }

    /**
     * Get calibration coefficient
     */
    float getSlope() { return calData.slope; }
    float getIntercept() { return calData.intercept; }
    float getRSquared() { return calData.r_squared; }
    bool isCalibrated() { return calData.isCalibrated; }

    /**
     * Print calibration results to Serial
     */
    void printCalibrationResults() {
        Serial.println("\n===== CALIBRATION RESULTS =====");
        Serial.print("Slope: ");
        Serial.println(calData.slope, 6);
        Serial.print("Intercept: ");
        Serial.println(calData.intercept, 6);
        Serial.print("R² (Goodness of Fit): ");
        Serial.println(calData.r_squared, 6);
        Serial.print("Calibration Points: ");
        Serial.println(calibrationCount);
        Serial.println("==============================\n");
    }

    /**
     * Clear calibration data
     */
    void clearCalibration() {
        calibrationCount = 0;
        calData.slope = 1.0;
        calData.intercept = 0.0;
        calData.r_squared = 0.0;
        calData.isCalibrated = false;
        calData.timestamp = 0;
    }

    /**
     * Save calibration to EEPROM
     */
    bool saveToEEPROM() {
        byte* ptr = (byte*)&calData;
        for (size_t i = 0; i < sizeof(CalibrationData); i++) {
            EEPROM.write(CALIBRATION_EEPROM_ADDR + i, ptr[i]);
        }
        Serial.println("Calibration saved to EEPROM");
        return true;
    }

    /**
     * Load calibration from EEPROM
     */
    bool loadFromEEPROM() {
        byte* ptr = (byte*)&calData;
        for (size_t i = 0; i < sizeof(CalibrationData); i++) {
            ptr[i] = EEPROM.read(CALIBRATION_EEPROM_ADDR + i);
        }
        
        if (!calData.isCalibrated) {
            Serial.println("No calibration data found in EEPROM");
            return false;
        }
        
        Serial.println("Calibration loaded from EEPROM");
        printCalibrationResults();
        return true;
    }

    /**
     * Get measurement accuracy estimate
     */
    float getAccuracyEstimate() {
        if (!calData.isCalibrated) return 0.0;
        // RMSE = sqrt(1 - R²) * std_dev_of_reference
        return sqrt(1.0 - calData.r_squared) * 15.0; // ~15 mg/dL std dev
    }
};

// ===== MULTI-POINT CALIBRATION MANAGER =====
class CalibrationManager {
private:
    LinearCalibration calibrator;
    int currentCalibrationStep;
    float currentGlucoseReading;

public:
    CalibrationManager() : currentCalibrationStep(0), currentGlucoseReading(0) {}

    void startCalibration() {
        calibrator.clearCalibration();
        currentCalibrationStep = 0;
        Serial.println("\n===== STARTING CALIBRATION SEQUENCE =====");
        Serial.print("Please provide ");
        Serial.print(CALIBRATION_POINTS);
        Serial.println(" reference glucose measurements...");
    }

    bool addCalibrationReading(float glucoseRef, float sensorReading) {
        bool success = calibrator.addCalibrationPoint(glucoseRef, sensorReading);
        if (success) {
            currentCalibrationStep++;
        }
        return success;
    }

    bool finishCalibration() {
        if (currentCalibrationStep < CALIBRATION_POINTS) {
            Serial.print("ERROR: Need ");
            Serial.print(CALIBRATION_POINTS);
            Serial.print(" points, have ");
            Serial.println(currentCalibrationStep);
            return false;
        }
        
        bool result = calibrator.performCalibration();
        if (result) {
            calibrator.saveToEEPROM();
        }
        return result;
    }

    float getGlucoseEstimate(float sensorReading) {
        return calibrator.sensorToGlucose(sensorReading);
    }

    int getCalibrationProgress() {
        return (currentCalibrationStep * 100) / CALIBRATION_POINTS;
    }

    bool isCalibrated() {
        return calibrator.isCalibrated();
    }

    void loadPreviousCalibration() {
        calibrator.loadFromEEPROM();
    }

    LinearCalibration& getCalibrator() {
        return calibrator;
    }
};

#endif // CALIBRATION_H
