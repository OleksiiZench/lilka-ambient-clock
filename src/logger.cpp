#include "logger.h"

#include <esp_log.h>
#include <lilka.h>

namespace {

// UART0 за замовчуванням виведено на GPIO43 (TX) / GPIO44 (RX) роз'єму розширення.
#ifndef UART_LOG_TX
#    define UART_LOG_TX 43
#endif
#ifndef UART_LOG_RX
#    define UART_LOG_RX 44
#endif

constexpr unsigned long UART_LOG_BAUD = 115200;
constexpr size_t LOG_BUFFER_SIZE = 256;

vprintf_like_t previousVprintf = nullptr;

// Пише в UART та передає далі в попередній обробник (логування lilka на USB-C).
int uartLogVprintf(const char* format, va_list args) {
    va_list copy;
    va_copy(copy, args);
    char buffer[LOG_BUFFER_SIZE];
    int length = vsnprintf(buffer, sizeof(buffer), format, copy);
    va_end(copy);

    if (length > 0) {
        size_t toWrite = static_cast<size_t>(length) < sizeof(buffer) ? static_cast<size_t>(length) : sizeof(buffer) - 1;
        Serial0.write(reinterpret_cast<const uint8_t*>(buffer), toWrite);
    }

    if (previousVprintf != nullptr) {
        return previousVprintf(format, args);
    }
    return length;
}

} // namespace

void loggerBegin() {
    // Дебаг-логування вимкнено для максимізації автономності
}

void loggerInfo(const char* format, ...) {
    (void)format;
}

void loggerError(const char* format, ...) {
    (void)format;
}
