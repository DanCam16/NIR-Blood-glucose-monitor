#ifndef SIGNAL_PROCESSING_H
#define SIGNAL_PROCESSING_H

#include "config.h"

// ===== CIRCULAR BUFFER FOR REAL-TIME PROCESSING =====
class CircularBuffer {
private:
    float buffer[BUFFER_SIZE];
    int head;
    int tail;
    int count;

public:
    CircularBuffer() : head(0), tail(0), count(0) {}

    void push(float value) {
        buffer[head] = value;
        head = (head + 1) % BUFFER_SIZE;
        if (count < BUFFER_SIZE) {
            count++;
        } else {
            tail = (tail + 1) % BUFFER_SIZE;
        }
    }

    float pop() {
        if (count == 0) return 0.0;
        float value = buffer[tail];
        tail = (tail + 1) % BUFFER_SIZE;
        count--;
        return value;
    }

    float getMean() {
        if (count == 0) return 0.0;
        float sum = 0.0;
        for (int i = 0; i < count; i++) {
            sum += buffer[(tail + i) % BUFFER_SIZE];
        }
        return sum / count;
    }

    float getStdDev() {
        if (count < 2) return 0.0;
        float mean = getMean();
        float variance = 0.0;
        for (int i = 0; i < count; i++) {
            float diff = buffer[(tail + i) % BUFFER_SIZE] - mean;
            variance += diff * diff;
        }
        return sqrt(variance / (count - 1));
    }

    float getMax() {
        if (count == 0) return 0.0;
        float maxVal = buffer[tail];
        for (int i = 0; i < count; i++) {
            float val = buffer[(tail + i) % BUFFER_SIZE];
            if (val > maxVal) maxVal = val;
        }
        return maxVal;
    }

    float getMin() {
        if (count == 0) return 0.0;
        float minVal = buffer[tail];
        for (int i = 0; i < count; i++) {
            float val = buffer[(tail + i) % BUFFER_SIZE];
            if (val < minVal) minVal = val;
        }
        return minVal;
    }

    int getCount() { return count; }
    void clear() { head = 0; tail = 0; count = 0; }
};

// ===== IIR LOW-PASS FILTER =====
class IIRLowPassFilter {
private:
    float alpha;
    float prevOutput;

public:
    IIRLowPassFilter(float cutoffFreq, float samplingRate) {
        // Calculate alpha based on cutoff frequency
        // alpha = (2 * π * Fc) / (2 * π * Fc + Fs)
        alpha = (2.0 * 3.14159 * cutoffFreq) / (2.0 * 3.14159 * cutoffFreq + samplingRate);
        alpha = constrain(alpha, 0.0, 1.0);
        prevOutput = 0.0;
    }

    void setAlpha(float newAlpha) {
        alpha = constrain(newAlpha, 0.0, 1.0);
    }

    float filter(float input) {
        prevOutput = alpha * input + (1.0 - alpha) * prevOutput;
        return prevOutput;
    }

    void reset() { prevOutput = 0.0; }
};

// ===== MOVING AVERAGE FILTER =====
class MovingAverageFilter {
private:
    float buffer[MOVING_AVG_WINDOW];
    int index;
    float sum;
    int count;

public:
    MovingAverageFilter() : index(0), sum(0.0), count(0) {
        for (int i = 0; i < MOVING_AVG_WINDOW; i++) {
            buffer[i] = 0.0;
        }
    }

    float filter(float input) {
        sum -= buffer[index];
        buffer[index] = input;
        sum += input;
        index = (index + 1) % MOVING_AVG_WINDOW;
        
        if (count < MOVING_AVG_WINDOW) {
            count++;
        }
        
        return sum / count;
    }

    void reset() {
        index = 0;
        sum = 0.0;
        count = 0;
        for (int i = 0; i < MOVING_AVG_WINDOW; i++) {
            buffer[i] = 0.0;
        }
    }
};

// ===== PEAK DETECTOR =====
class PeakDetector {
private:
    float threshold;
    bool isPeak;
    float lastValue;

public:
    PeakDetector(float thresh = 50.0) : threshold(thresh), isPeak(false), lastValue(0.0) {}

    bool detectPeak(float input) {
        isPeak = false;
        if (input > threshold && lastValue <= input) {
            isPeak = true;
        }
        lastValue = input;
        return isPeak;
    }

    void setThreshold(float thresh) { threshold = thresh; }
};

// ===== SIGNAL STATISTICS =====
class SignalStatistics {
private:
    CircularBuffer buffer;
    int updateCounter;

public:
    float mean;
    float stdDev;
    float maxValue;
    float minValue;
    float range;
    float rms;

    SignalStatistics() : updateCounter(0), mean(0), stdDev(0), 
                        maxValue(0), minValue(0), range(0), rms(0) {}

    void update(float value) {
        buffer.push(value);
        updateCounter++;
        
        if (updateCounter >= MOVING_AVG_WINDOW) {
            updateCounter = 0;
            calculateStatistics();
        }
    }

    void calculateStatistics() {
        mean = buffer.getMean();
        stdDev = buffer.getStdDev();
        maxValue = buffer.getMax();
        minValue = buffer.getMin();
        range = maxValue - minValue;
        
        // Calculate RMS
        rms = 0.0;
        for (int i = 0; i < buffer.getCount(); i++) {
            rms += pow(buffer.buffer[i], 2);
        }
        rms = sqrt(rms / max(1, buffer.getCount()));
    }

    void printStatistics() {
        Serial.print("Mean: ");
        Serial.print(mean);
        Serial.print(" | StdDev: ");
        Serial.print(stdDev);
        Serial.print(" | Max: ");
        Serial.print(maxValue);
        Serial.print(" | Min: ");
        Serial.print(minValue);
        Serial.print(" | RMS: ");
        Serial.println(rms);
    }
};

// ===== SIGNAL PROCESSOR CLASS =====
class SignalProcessor {
private:
    IIRLowPassFilter lpFilter;
    MovingAverageFilter mavFilter;
    CircularBuffer processedBuffer;
    SignalStatistics stats;

public:
    SignalProcessor() 
        : lpFilter(LOW_PASS_CUTOFF, SAMPLING_RATE) {}

    float processSignal(float rawADC) {
        // Stage 1: Normalize ADC value (0-1023 to 0-REFERENCE_VOLTAGE)
        float normalized = (rawADC / ADC_MAX_VALUE) * REFERENCE_VOLTAGE;

        // Stage 2: IIR Low-Pass Filter (remove high-frequency noise)
        float filtered = lpFilter.filter(normalized);

        // Stage 3: Moving Average Filter (smooth signal)
        float smoothed = mavFilter.filter(filtered);

        // Stage 4: Store in processing buffer
        processedBuffer.push(smoothed);
        stats.update(smoothed);

        return smoothed;
    }

    float getRawSignal(int rawADC) {
        return (rawADC / ADC_MAX_VALUE) * REFERENCE_VOLTAGE;
    }

    float getFilteredSignal() {
        return processedBuffer.getMean();
    }

    CircularBuffer& getBuffer() {
        return processedBuffer;
    }

    SignalStatistics& getStatistics() {
        return stats;
    }

    void reset() {
        lpFilter.reset();
        mavFilter.reset();
        processedBuffer.clear();
    }
};

#endif // SIGNAL_PROCESSING_H
