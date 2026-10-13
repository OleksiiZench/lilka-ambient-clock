#pragma once

// Логування з дублюванням на USB-C та на UART0.
//
// Плата зібрана з ARDUINO_USB_CDC_ON_BOOT=1, тож `lilka::serial` пише у
// вбудований USB-C. UART0 виведено на контакти TX/RX роз'єму розширення
// (GPIO43 TX / GPIO44 RX), саме туди й дублюються логи.
//
// USB-UART-перехідник під'єднується так (швидкість 115200):
//   Adapter RX  <- Lilka TX  (контакт TX роз'єму)
//   Adapter TX  -> Lilka RX  (контакт RX, лише якщо потрібне введення)
//   Adapter GND -> Lilka GND (контакт GND роз'єму)

// Ініціалізувати UART-логер. Викликати після lilka::begin(), щоб зберегти
// штатне логування на USB-C.
void loggerBegin();

// Повідомлення рівня info: USB-C + UART.
void loggerInfo(const char* format, ...);

// Повідомлення рівня error: USB-C + UART.
void loggerError(const char* format, ...);
