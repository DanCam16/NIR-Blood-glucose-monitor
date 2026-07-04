#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===== OLED DISPLAY MANAGER =====
class DisplayManager {
private:
    Adafruit_SSD1306 display;
    unsigned long lastUpdateTime;
    bool isInitialized;

public:
    DisplayManager() : display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1) {
        isInitialized = false;
        lastUpdateTime = 0;
    }

    /**
     * Initialize OLED display
     */
    bool initialize() {
        if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
            Serial.println("ERROR: SSD1306 allocation failed");
            return false;
        }

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Initializing...");
        display.display();

        isInitialized = true;
        return true;
    }

    /**
     * Display splash screen with device info
     */
    void showSplashScreen() {
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(10, 10);
        display.println("GLUCOSE");
        display.setCursor(25, 30);
        display.println("MONITOR");
        display.setTextSize(1);
        display.setCursor(15, 50);
        display.println("IR LED 940nm");
        display.display();
        delay(2000);
    }

    /**
     * Display comprehensive glucose reading screen with absorbance
     * Shows: Intensity, Absorbance, Glucose, Battery
     */
    void showGlucoseReading(float glucoseValue, float absorbance, 
                           float intensity, int batteryPercent, bool isCalibrated) {
        if (!isInitialized) return;
        if (millis() - lastUpdateTime < OLED_UPDATE_RATE) return;

        display.clearDisplay();

        // Title
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Blood Glucose Monitor");

        // Draw horizontal line
        display.drawLine(0, 8, OLED_WIDTH, 8, SSD1306_WHITE);

        // IR Intensity
        display.setCursor(0, 12);
        display.print("IR Intensity: ");
        display.println((int)intensity);

        // Absorbance (Modified Beer-Lambert Law)
        display.setCursor(0, 20);
        display.print("Absorbance: ");
        display.println(absorbance, 3);

        // Glucose value (large font)
        display.setTextSize(2);
        display.setCursor(0, 30);
        if (glucoseValue >= 0) {
            display.print(glucoseValue, 1);
        } else {
            display.print("---");
        }
        display.setTextSize(1);
        display.setCursor(75, 35);
        display.println("mmol/L");

        // Status and Battery on same line
        display.setCursor(0, 48);
        if (!isCalibrated) {
            display.println("NOT CALIBRATED");
        } else {
            display.print("OK");
        }

        display.setCursor(60, 48);
        display.print("Bat: ");
        display.print(batteryPercent);
        display.println("%");

        // Timestamp
        display.setTextSize(1);
        display.setCursor(0, 56);
        display.print("T: ");
        display.println(millis() / 1000);

        display.display();
        lastUpdateTime = millis();
    }

    /**
     * Display baseline calibration screen
     */
    void showBaselineCalibrationScreen() {
        if (!isInitialized) return;

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("BASELINE SETUP");

        display.drawLine(0, 8, OLED_WIDTH, 8, SSD1306_WHITE);

        display.setCursor(0, 15);
        display.println("Step 1: Remove finger");
        display.setCursor(0, 25);
        display.println("from sensor.");
        display.setCursor(0, 35);
        display.println("Press button when");
        display.setCursor(0, 45);
        display.println("ready to measure");
        display.setCursor(0, 55);
        display.println("baseline intensity.");

        display.display();
    }

    /**
     * Display calibration screen
     */
    void showCalibrationScreen(int step, int totalSteps, float refGlucose) {
        if (!isInitialized) return;

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("CALIBRATION MODE");

        display.drawLine(0, 8, OLED_WIDTH, 8, SSD1306_WHITE);

        display.setCursor(0, 15);
        display.print("Step ");
        display.print(step);
        display.print(" of ");
        display.println(totalSteps);

        display.setCursor(0, 25);
        display.print("Reference: ");
        display.print(refGlucose);
        display.println(" mg/dL");

        display.setCursor(0, 35);
        display.println("Waiting for stable");
        display.println("sensor reading...");

        // Progress bar
        int barWidth = (step * OLED_WIDTH) / totalSteps;
        display.drawRect(0, 55, OLED_WIDTH, 8, SSD1306_WHITE);
        display.fillRect(1, 56, barWidth - 2, 6, SSD1306_WHITE);

        display.display();
    }

    /**
     * Display sensor data waveform (simple line graph)
     */
    void showWaveform(const float* data, int dataPoints, float minVal, float maxVal) {
        if (!isInitialized || dataPoints < 2) return;

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Signal Waveform");

        // Scale data to fit on display
        float range = maxVal - minVal;
        if (range < 0.001) range = 1.0;  // Prevent division by very small number
        
        int graphHeight = 40;
        int graphY = 15;
        int graphWidth = OLED_WIDTH;

        // Draw grid
        display.drawRect(10, graphY, graphWidth - 20, graphHeight, SSD1306_WHITE);

        // Draw waveform
        for (int i = 1; i < dataPoints && i < graphWidth - 20; i++) {
            float val1 = data[i - 1];
            float val2 = data[i];

            int y1 = graphY + graphHeight - (int)(((val1 - minVal) / range) * graphHeight);
            int y2 = graphY + graphHeight - (int)(((val2 - minVal) / range) * graphHeight);

            // Constrain y values to graph bounds
            y1 = constrain(y1, graphY, graphY + graphHeight);
            y2 = constrain(y2, graphY, graphY + graphHeight);

            display.drawLine(10 + i - 1, y1, 10 + i, y2, SSD1306_WHITE);
        }

        // Display stats
        display.setTextSize(1);
        display.setCursor(0, 60);
        display.print("Min: ");
        display.print(minVal, 1);
        display.print(" Max: ");
        display.println(maxVal, 1);

        display.display();
    }

    /**
     * Display error message
     */
    void showError(const char* errorMsg) {
        if (!isInitialized) return;

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("ERROR");
        display.drawLine(0, 8, OLED_WIDTH, 8, SSD1306_WHITE);

        display.setCursor(0, 15);
        display.println(errorMsg);

        display.display();
    }

    /**
     * Display loading animation
     */
    void showLoadingScreen(int progress) {
        if (!isInitialized) return;

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Initializing...");

        // Progress bar
        int barWidth = (progress * (OLED_WIDTH - 10)) / 100;
        display.drawRect(5, 30, OLED_WIDTH - 10, 15, SSD1306_WHITE);
        display.fillRect(6, 31, barWidth, 13, SSD1306_WHITE);

        display.setCursor(40, 50);
        display.print(progress);
        display.println("%");

        display.display();
    }

    /**
     * Display battery status
     */
    void showBatteryStatus(float voltage, int percent, bool isCharging) {
        if (!isInitialized) return;

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Battery Status");
        display.drawLine(0, 8, OLED_WIDTH, 8, SSD1306_WHITE);

        display.setTextSize(2);
        display.setCursor(20, 15);
        display.print(percent);
        display.println("%");

        display.setTextSize(1);
        display.setCursor(0, 35);
        display.print("Voltage: ");
        display.print(voltage, 2);
        display.println("V");

        display.setCursor(0, 45);
        if (isCharging) {
            display.println("Status: CHARGING");
        } else {
            display.println("Status: DISCHARGING");
        }

        // Battery icon
        display.drawRect(80, 30, 40, 20, SSD1306_WHITE);
        display.drawRect(120, 35, 5, 10, SSD1306_WHITE);
        int fillWidth = (percent * 36) / 100;
        display.fillRect(82, 32, fillWidth, 16, SSD1306_WHITE);

        display.display();
    }

    /**
     * Display settings/menu screen
     */
    void showMenuScreen(const char* menuItems[], int itemCount, int selectedIndex) {
        if (!isInitialized) return;

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("MENU");
        display.drawLine(0, 8, OLED_WIDTH, 8, SSD1306_WHITE);

        for (int i = 0; i < itemCount && i < 7; i++) {
            display.setCursor(5, 15 + i * 8);

            if (i == selectedIndex) {
                display.println(">");
                display.setCursor(15, 15 + i * 8);
                display.println(menuItems[i]);
            } else {
                display.setCursor(15, 15 + i * 8);
                display.println(menuItems[i]);
            }
        }

        display.display();
    }

    /**
     * Display device information and calibration status
     */
    void showDeviceInfo(float r_squared, float baselineIntensity, bool isCalibrated) {
        if (!isInitialized) return;

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("DEVICE INFO");
        display.drawLine(0, 8, OLED_WIDTH, 8, SSD1306_WHITE);

        display.setCursor(0, 12);
        display.println("Research Prototype");

        display.setCursor(0, 22);
        if (isCalibrated) {
            display.println("CALIBRATED");
            display.setCursor(0, 30);
            display.print("R^2: ");
            display.println(r_squared, 4);
            display.setCursor(0, 38);
            display.print("I0: ");
            display.println(baselineIntensity, 1);
        } else {
            display.println("NOT CALIBRATED");
            display.setCursor(0, 30);
            display.println("Run calibration");
        }

        display.display();
    }

    /**
     * Clear display
     */
    void clear() {
        if (!isInitialized) return;
        display.clearDisplay();
        display.display();
    }

    /**
     * Check if display is initialized
     */
    bool isInit() { return isInitialized; }

    /**
     * Get raw display object for custom drawing
     */
    Adafruit_SSD1306& getRawDisplay() {
        return display;
    }
};

#endif // DISPLAY_H
