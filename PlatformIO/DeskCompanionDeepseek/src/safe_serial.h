#ifndef SAFE_SERIAL_H
#define SAFE_SERIAL_H

#include <Arduino.h>

class SafeSerial {
public:
    SafeSerial();
    void begin(unsigned long baud);
    void setEnabled(bool e);
    bool isEnabled() const;

    // Common Print methods used across the project
    size_t write(uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);

    void print(const String &s);
    void print(const char *s);
    void print(char c);
    void print(int n);
    void print(unsigned int n);
    void print(long n);
    void print(unsigned long n);
    void println();
    void println(const String &s);
    void println(const char *s);
    void println(char c);
    void println(int n);
    void println(unsigned int n);
    void println(long n);
    void println(unsigned long n);

    void printf(const char *fmt, ...);
    int available();
    int read();

private:
    HardwareSerial *hw;
    bool enabled_;
};

extern SafeSerial safeSerial;

#endif // SAFE_SERIAL_H
