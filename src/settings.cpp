#include "settings.h"

#include <ArduinoJson.h>
#include <SD.h>
#include <lilka.h>

#include "logger.h"

namespace {

// Типовий часовий пояс: Europe/Kyiv (EET/EEST).
constexpr const char* DEFAULT_TZ = "EET-2EEST,M3.5.0/3,M10.5.0/4";
constexpr const char* DEFAULT_NTP = "pool.ntp.org";
constexpr const char* SETTINGS_PATH = "/settings.json";

// Перетворити "#RRGGBB" у 16-бітний колір (5-6-5).
// Повертає false, якщо рядок порожній або має хибний формат.
bool parseHexColor(const char* text, uint16_t& out) {
    if (text == nullptr) {
        return false;
    }
    if (*text == '#') {
        text++;
    }
    if (strlen(text) != 6) {
        return false;
    }

    char* end = nullptr;
    long value = strtol(text, &end, 16);
    if (end == text || *end != '\0') {
        return false;
    }

    uint8_t r = (value >> 16) & 0xFF;
    uint8_t g = (value >> 8) & 0xFF;
    uint8_t b = value & 0xFF;
    out = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    return true;
}

void readString(JsonVariantConst value, String& target) {
    if (value.is<const char*>()) {
        target = value.as<const char*>();
    }
}

} // namespace

void settingsSetDefaults(Settings& settings) {
    settings.wifiSsid = "";
    settings.wifiPassword = "";
    settings.timezone = DEFAULT_TZ;
    settings.ntpServer = DEFAULT_NTP;
    parseHexColor("#FF0000", settings.foreground);
    parseHexColor("#000000", settings.background);
    settings.blinkColon = false;
    settings.showBattery = true;
}

SettingsStatus settingsLoad(Settings& settings) {
    settingsSetDefaults(settings);

    // Спроба монтування під час lilka::begin() могла не вдатися через
    // поганий контакт картки, тож пробуємо ще кілька разів.
    constexpr int SD_MOUNT_ATTEMPTS = 3;
    for (int attempt = 1; !lilka::fileutils.isSDAvailable() && attempt <= SD_MOUNT_ATTEMPTS; attempt++) {
        loggerError("SD card mount attempt %d/%d...", attempt, SD_MOUNT_ATTEMPTS);
        delay(200);
        lilka::fileutils.initSD();
    }

    if (!lilka::fileutils.isSDAvailable()) {
        loggerError("SD card is not available (check FAT32 formatting and seating)");
        return SettingsStatus::SdUnavailable;
    }

    if (!SD.exists(SETTINGS_PATH)) {
        loggerError("File %s not found on SD card", SETTINGS_PATH);
        return SettingsStatus::FileNotFound;
    }

    File file = SD.open(SETTINGS_PATH, FILE_READ);
    if (!file) {
        loggerError("Failed to open %s for reading", SETTINGS_PATH);
        return SettingsStatus::FileNotFound;
    }

    String content = file.readString();
    file.close();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, content);
    if (error) {
        loggerError("Failed to parse %s: %s", SETTINGS_PATH, error.c_str());
        return SettingsStatus::ParseError;
    }

    readString(doc["wifi"]["ssid"], settings.wifiSsid);
    readString(doc["wifi"]["password"], settings.wifiPassword);
    readString(doc["time"]["tz"], settings.timezone);
    readString(doc["time"]["ntpServer"], settings.ntpServer);

    if (doc["display"]["blinkColon"].is<bool>()) {
        settings.blinkColon = doc["display"]["blinkColon"].as<bool>();
    }

    if (doc["display"]["showBattery"].is<bool>()) {
        settings.showBattery = doc["display"]["showBattery"].as<bool>();
    }

    uint16_t color = settings.foreground;
    if (doc["display"]["fg"].is<const char*>()) {
        parseHexColor(doc["display"]["fg"].as<const char*>(), color);
    }
    settings.foreground = color;

    color = settings.background;
    if (doc["display"]["bg"].is<const char*>()) {
        parseHexColor(doc["display"]["bg"].as<const char*>(), color);
    }
    settings.background = color;

    loggerInfo("Settings loaded from SD card (ssid: \"%s\")", settings.wifiSsid.c_str());
    return SettingsStatus::Ok;
}
