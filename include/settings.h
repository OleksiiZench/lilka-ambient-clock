#pragma once

#include <Arduino.h>

// Налаштування годинника, які зчитуються з microSD-карти (файл /settings.json).
struct Settings {
    String wifiSsid;
    String wifiPassword;
    String timezone;   // POSIX TZ-рядок, напр. "EET-2EEST,M3.5.0/3,M10.5.0/4"
    String ntpServer;  // NTP-сервер, напр. "pool.ntp.org"
    uint16_t foreground;  // колір цифр (RGB565)
    uint16_t background;  // колір фону (RGB565)
    bool blinkColon;      // чи блимає двокрапка щосекунди
};

// Заповнити налаштування типовими значеннями (Europe/Kyiv, red на чорному).
void settingsSetDefaults(Settings& settings);

// Результат завантаження налаштувань.
enum class SettingsStatus {
    Ok,
    SdUnavailable,
    FileNotFound,
    ParseError,
};

// Спробувати завантажити налаштування з /settings.json на SD-карті.
//
// У будь-якому разі `settings` містить або значення з файлу, або типові.
// Повертає статус, щоб показати точну причину невдачі.
SettingsStatus settingsLoad(Settings& settings);
