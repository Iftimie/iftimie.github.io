#include "safe_serial.h"
#include "config.h"
#ifdef Serial
#undef Serial
#endif
#include <stdarg.h>

SafeSerial safeSerial;

SafeSerial::SafeSerial()
: hw(nullptr), enabled_(true)
{
    // Respect compile-time flag to disable serial output entirely
#if defined(DISABLE_SERIAL_OUTPUT) && DISABLE_SERIAL_OUTPUT
    enabled_ = false;
#else
    enabled_ = true;
#endif
    // Use the underlying Serial0 object (Arduino core defines Serial -> Serial0)
    hw = &Serial0;
}

void SafeSerial::begin(unsigned long baud) {
    if (hw) hw->begin(baud);
}

void SafeSerial::setEnabled(bool e) {
    enabled_ = e;
}

bool SafeSerial::isEnabled() const {
    return enabled_;
}

size_t SafeSerial::write(uint8_t c) {
    if (!enabled_) return 0;
    if (!hw) return 0;
    return hw->write(c);
}

size_t SafeSerial::write(const uint8_t *buffer, size_t size) {
    if (!enabled_) return 0;
    if (!hw) return 0;
    return hw->write(buffer, size);
}

void SafeSerial::print(const String &s) {
    if (!enabled_) return;
    if (!hw) return;
    hw->print(s);
}

void SafeSerial::print(const char *s) {
    if (!enabled_) return;
    if (!hw) return;
    hw->print(s);
}

void SafeSerial::print(char c) { if (!enabled_ || !hw) return; hw->print(c); }
void SafeSerial::print(int n) { if (!enabled_ || !hw) return; hw->print(n); }
void SafeSerial::print(unsigned int n) { if (!enabled_ || !hw) return; hw->print(n); }
void SafeSerial::print(long n) { if (!enabled_ || !hw) return; hw->print(n); }
void SafeSerial::print(unsigned long n) { if (!enabled_ || !hw) return; hw->print(n); }

void SafeSerial::println() { if (!enabled_ || !hw) return; hw->println(); }
void SafeSerial::println(const String &s) { if (!enabled_ || !hw) return; hw->println(s); }
void SafeSerial::println(const char *s) { if (!enabled_ || !hw) return; hw->println(s); }
void SafeSerial::println(char c) { if (!enabled_ || !hw) return; hw->println(c); }
void SafeSerial::println(int n) { if (!enabled_ || !hw) return; hw->println(n); }
void SafeSerial::println(unsigned int n) { if (!enabled_ || !hw) return; hw->println(n); }
void SafeSerial::println(long n) { if (!enabled_ || !hw) return; hw->println(n); }
void SafeSerial::println(unsigned long n) { if (!enabled_ || !hw) return; hw->println(n); }

void SafeSerial::printf(const char *fmt, ...) {
    if (!enabled_ || !hw) return;
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    hw->print(buf);
}

int SafeSerial::available() {
    if (!enabled_ || !hw) return 0;
    return hw->available();
}

int SafeSerial::read() {
    if (!enabled_ || !hw) return -1;
    return hw->read();
}
