#include "logger.h"

#include <esp_log.h>
#include <lilka.h>

// Діагностичне логування вимкнене за замовчуванням для максимальної
// автономності. Увімкнути можна через build_flags: -D LILKA_CLOCK_LOGS=1
#ifndef LILKA_CLOCK_LOGS
#    define LILKA_CLOCK_LOGS 0
#endif

namespace {

#if LILKA_CLOCK_LOGS

// UART0 за замовчуванням виведено на GPIO43 (TX) / GPIO44 (RX) роз'єму розширення.
#    ifndef UART_LOG_TX
#        define UART_LOG_TX 43
#    endif
#    ifndef UART_LOG_RX
#        define UART_LOG_RX 44
#    endif

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

// Продублювати повідомлення на USB-C та UART0.
void logMessage(bool isError, const char* format, va_list args) {
    char buffer[LOG_BUFFER_SIZE];
    va_list copy;
    va_copy(copy, args);
    const int length = vsnprintf(buffer, sizeof(buffer), format, copy);
    va_end(copy);
    if (length <= 0) {
        return;
    }

    const size_t toWrite =
        static_cast<size_t>(length) < sizeof(buffer) ? static_cast<size_t>(length) : sizeof(buffer) - 1;
    Serial0.write(reinterpret_cast<const uint8_t*>(buffer), toWrite);
    Serial0.write('\n');

    if (isError) {
        lilka::serial.err("%s", buffer);
    } else {
        lilka::serial.log("%s", buffer);
    }
}

#endif // LILKA_CLOCK_LOGS

} // namespace

void loggerBegin() {
#if LILKA_CLOCK_LOGS
    Serial0.begin(UART_LOG_BAUD, SERIAL_8N1, UART_LOG_RX, UART_LOG_TX);
    previousVprintf = esp_log_set_vprintf(uartLogVprintf);
#endif
}

void loggerInfo(const char* format, ...) {
#if LILKA_CLOCK_LOGS
    va_list args;
    va_start(args, format);
    logMessage(false, format, args);
    va_end(args);
#else
    (void)format;
#endif
}

void loggerError(const char* format, ...) {
#if LILKA_CLOCK_LOGS
    va_list args;
    va_start(args, format);
    logMessage(true, format, args);
    va_end(args);
#else
    (void)format;
#endif
}
