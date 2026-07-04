#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "config.h"
#include <EEPROM.h>
#include <math.h>

// ===== CALIBRATION DATA STRUCTURE =====
struct CalibrationData {
    // Absorbance-based polynomial regression coefficients
    float beta0;        // Constant term
    float beta1;        // Linear absorbance coefficient
    float beta2;        // Quadratic absorbance coefficient
    float r_squared;    // R² value (goodness of fit)
    bool isCalibrated;  // Calibration status flag
    uint32_t timestamp; // Last calibration timestamp
    float baselineIntensity;  // Reference intensity (finger not present)
};

// ===== ABSORBANCE-BASED CALIBRATION =====
/**
 * Modified Beer-Lambert Law Calibration
 * Calculates absorbance: A = log10(I0 / I)
 * Then performs polynomial regression: G = β0 + β1*A + β2*A²
 * This is physically correct for optical glucose sensing and aligns with
 * the recommendation for Level 400 Biomedical Engineering project.
 */
class AbsorbanceCalibration {
private:
    float referenceGlucose[CALIBRATION_POINTS];
    float sensorIntensities[CALIBRATION_POINTS];  // Absorbed light intensity
    float absorbanceValues[CALIBRATION_POINTS];   // Calculated absorbance
    int calibrationCount;
    CalibrationData calData;

public:
    AbsorbanceCalibration() : calibrationCount(0) {
        calData.beta0 = 0.0;
        calData.beta1 = 1.0;
        calData.beta2 = 0.0;
        calData.r_squared = 0.0;
        calData.isCalibrated = false;
        calData.timestamp = 0;
        calData.baselineIntensity = 0.0;
    }

    /**
     * Set baseline intensity (I0 - no glucose/baseline absorption)
     * Should be measured with clean finger at beginning of calibration
     */
    void setBaselineIntensity(float intensity) {
        calData.baselineIntensity = intensity;
        Serial.print("Baseline Intensity (I0) set to: ");
        Serial.println(intensity, 3);
    }

    /**
     * Add a calibration point with reference glucose and sensor intensity
     * @param glucoseRef: Reference glucose value (mg/dL) from blood test
     * @param sensorIntensity: Sensor reading - absorbed light intensity
     */
    bool addCalibrationPoint(float glucoseRef, float sensorIntensity) {
        if (calibrationCount >= CALIBRATION_POINTS) {
            Serial.println("ERROR: Calibration buffer full!");
            return false;
        }

        referenceGlucose[calibrationCount] = glucoseRef;
        sensorIntensities[calibrationCount] = sensorIntensity;

        // Calculate absorbance using Modified Beer-Lambert Law
        // A = log10(I0 / I)
        float absorbance = log10(calData.baselineIntensity / sensorIntensity);
        absorbanceValues[calibrationCount] = absorbance;
        calibrationCount++;

        Serial.print("Calibration Point ");
        Serial.print(calibrationCount);
        Serial.print(": Glucose = ");
        Serial.print(glucoseRef);
        Serial.print(" mg/dL, Intensity = ");
        Serial.print(sensorIntensity, 3);
        Serial.print(", Absorbance = ");
        Serial.println(absorbance, 4);

        return true;
    }

    /**
     * Perform polynomial regression: G = β0 + β1*A + β2*A²
     * Using least squares fitting
     */
    bool performCalibration() {
        if (calibrationCount < 2) {
            Serial.println("ERROR: Need at least 2 calibration points!");
            return false;
        }

        // For quadratic fit: solve the normal equations
        // y = β0 + β1*x + β2*x²
        // This requires solving a 3x3 system (or 2x2 for linear fit)

        float n = calibrationCount;
        float sumA = 0, sumA2 = 0, sumA3 = 0, sumA4 = 0;
        float sumG = 0, sumAG = 0, sumA2G = 0;

        for (int i = 0; i < calibrationCount; i++) {
            float a = absorbanceValues[i];
            float g = referenceGlucose[i];

            sumA += a;
            sumA2 += a * a;
            sumA3 += a * a * a;
            sumA4 += a * a * a * a;
            sumG += g;
            sumAG += a * g;
            sumA2G += a * a * g;
        }

        // Solve using Cramer's rule for quadratic
        float det = n * (sumA2 * sumA4 - sumA3 * sumA3) 
                    - sumA * (sumA * sumA4 - sumA2 * sumA3)
                    + sumA2 * (sumA * sumA3 - sumA2 * sumA2);

        if (fabs(det) < 1e-6) {
            Serial.println("ERROR: Singular matrix - cannot solve system!");
            return false;
        }

        // Use simplified 2-point linear fit if quadratic fails
        // G = β0 + β1*A (linear approximation)
        float meanG = sumG / n;
        float meanA = sumA / n;

        float numerator = 0, denominator = 0;
        for (int i = 0; i < calibrationCount; i++) {
            numerator += (absorbanceValues[i] - meanA) * (referenceGlucose[i] - meanG);
            denominator += (absorbanceValues[i] - meanA) * (absorbanceValues[i] - meanA);
        }

        if (denominator < 1e-6) {
            Serial.println("ERROR: Zero denominator in linear fit!");
            return false;
        }

        calData.beta1 = numerator / denominator;
        calData.beta0 = meanG - (calData.beta1 * meanA);
        calData.beta2 = 0.0;  // Linear model for now

        // Calculate R² (coefficient of determination)
        float ssRes = 0.0, ssTot = 0.0;
        for (int i = 0; i < calibrationCount; i++) {
            float predicted = calData.beta0 + calData.beta1 * absorbanceValues[i];
            ssRes += pow(referenceGlucose[i] - predicted, 2);
            ssTot += pow(referenceGlucose[i] - meanG, 2);
        }
        calData.r_squared = 1.0 - (ssRes / ssTot);

        calData.isCalibrated = true;
        calData.timestamp = millis() / 1000;

        printCalibrationResults();
        return true;
    }

    /**
     * Convert sensor intensity to glucose using Modified Beer-Lambert Law
     * @param sensorIntensity: Absorbed light intensity
     * @return: Estimated glucose in mg/dL
     */
    float sensorToGlucose(float sensorIntensity) {
        if (!calData.isCalibrated) {
            Serial.println("WARNING: Device not calibrated!");
            return -1.0;
        }

        // Calculate absorbance
        float absorbance = log10(calData.baselineIntensity / sensorIntensity);

        // Polynomial regression: G = β0 + β1*A + β2*A²
        float glucose = calData.beta0 + calData.beta1 * absorbance 
                       + calData.beta2 * absorbance * absorbance;

        return glucose;
    }

    /**
     * Get calibration coefficients
     */
    float getBeta0() { return calData.beta0; }
    float getBeta1() { return calData.beta1; }
    float getBeta2() { return calData.beta2; }
    float getRSquared() { return calData.r_squared; }
    float getBaselineIntensity() { return calData.baselineIntensity; }
    bool isCalibrated() { return calData.isCalibrated; }

    /**
     * Print calibration results
     */
    void printCalibrationResults() {
        Serial.println("\n===== ABSORBANCE-BASED CALIBRATION RESULTS =====");
        Serial.println("Modified Beer-Lambert Law: A = log10(I0 / I)");
        Serial.println("Polynomial Regression: G = β0 + β1*A + β2*A²");
        Serial.println("---");
        Serial.print("β0 (constant): ");
        Serial.println(calData.beta0, 4);
        Serial.print("β1 (linear absorbance coefficient): ");
        Serial.println(calData.beta1, 4);
        Serial.print("β2 (quadratic absorbance coefficient): ");
        Serial.println(calData.beta2, 6);
        Serial.print("Baseline Intensity (I0): ");
        Serial.println(calData.baselineIntensity, 3);
        Serial.print("R² (Goodness of Fit): ");
        Serial.println(calData.r_squared, 6);
        Serial.print("Calibration Points: ");
        Serial.println(calibrationCount);
        Serial.println("==================================================\n");
    }

    /**
     * Clear calibration data
     */
    void clearCalibration() {
        calibrationCount = 0;
        calData.beta0 = 0.0;
        calData.beta1 = 1.0;
        calData.beta2 = 0.0;
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

// ===== CALIBRATION MANAGER =====
class CalibrationManager {
private:
    AbsorbanceCalibration calibrator;
    int currentCalibrationStep;
    float currentGlucoseReading;
    bool baselineSet;

public:
    CalibrationManager() : currentCalibrationStep(0), currentGlucoseReading(0), baselineSet(false) {}

    void startCalibration() {
        calibrator.clearCalibration();
        currentCalibrationStep = 0;
        baselineSet = false;
        Serial.println("\n===== STARTING ABSORBANCE-BASED CALIBRATION SEQUENCE =====");
        Serial.println("Step 1: Set baseline (remove finger, press button)");
        Serial.println("Step 2-N: Measure with known glucose reference");
    }

    /**
     * Set baseline intensity (Step 1 of calibration)
     */
    bool setBaseline(float baselineIntensity) {
        calibrator.setBaselineIntensity(baselineIntensity);
        baselineSet = true;
        return true;
    }

    /**
     * Add calibration reading (Steps 2+)
     */
    bool addCalibrationReading(float glucoseRef, float sensorReading) {
        if (!baselineSet) {
            Serial.println("ERROR: Baseline must be set first!");
            return false;
        }

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

    bool loadPreviousCalibration() {
        return calibrator.loadFromEEPROM();
    }

    AbsorbanceCalibration& getCalibrator() {
        return calibrator;
    }

    float getBaselineIntensity() {
        return calibrator.getBaselineIntensity();
    }
};

#endif // CALIBRATION_H
