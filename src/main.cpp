#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include <lilka.h>

#include "clockface.h"
#include "logger.h"
#include "settings.h"

namespace {

Settings settings;
SettingsStatus settingsStatus = SettingsStatus::Ok;
bool timeSynced = false;
unsigned long lastAttemptMs = 0;

constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr unsigned long RETRY_INTERVAL_MS = 30UL * 1000UL;               // коли часу ще немає
constexpr unsigned long RESYNC_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL; // періодична пересинхронізація

bool connectWiFi(unsigned long timeoutMs) {
    if (settings.wifiSsid.isEmpty()) {
        return false;
    }
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPassword.c_str());

    const unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
        delay(250);
    }
    return WiFi.status() == WL_CONNECTED;
}

bool syncTime() {
    configTzTime(settings.timezone.c_str(), settings.ntpServer.c_str(), "time.google.com");
    struct tm timeinfo;
    return getLocalTime(&timeinfo, WIFI_CONNECT_TIMEOUT_MS);
}

// Підключитися до Wi-Fi та синхронізувати час. Блокує виконання, тому
// викликається лише поки часу ще немає або раз на кілька годин.
void connectAndSync() {
    if (settings.wifiSsid.isEmpty()) {
        switch (settingsStatus) {
            case SettingsStatus::SdUnavailable:
                loggerError("SD card is not available, cannot read settings.json");
                clockFaceMessage(settings, "Немає SD-карти", "Вставте картку з settings.json");
                break;
            case SettingsStatus::FileNotFound:
                loggerError("settings.json not found in SD card root");
                clockFaceMessage(settings, "Немає settings.json", "Покладіть файл у корінь SD");
                break;
            case SettingsStatus::ParseError:
                loggerError("settings.json is not valid JSON");
                clockFaceMessage(settings, "Помилка settings.json", "Перевірте синтаксис JSON");
                break;
            default:
                loggerError("wifi.ssid is empty in settings.json");
                clockFaceMessage(settings, "Вкажіть wifi.ssid", "у settings.json");
                break;
        }
        return;
    }

    loggerInfo("Connecting to Wi-Fi \"%s\"...", settings.wifiSsid.c_str());
    clockFaceMessage(settings, "Підключення до Wi-Fi...");
    if (!connectWiFi(WIFI_CONNECT_TIMEOUT_MS)) {
        loggerError("Wi-Fi connection failed (status %d)", WiFi.status());
        clockFaceMessage(settings, "Немає Wi-Fi", "Повторюю спробу...");
        return;
    }
    loggerInfo("Wi-Fi connected, IP: %s", WiFi.localIP().toString().c_str());

    loggerInfo("Synchronizing time via NTP \"%s\"...", settings.ntpServer.c_str());
    clockFaceMessage(settings, "Синхронізація часу...");
    if (syncTime()) {
        timeSynced = true;
        loggerInfo("Time synchronized");
    } else {
        loggerError("NTP synchronization failed");
        clockFaceMessage(settings, "Помилка NTP", "Повторюю спробу...");
    }
}

} // namespace

void setup() {
    lilka::begin();
    loggerBegin();
    loggerInfo("Lilka ambient clock starting");

    settingsStatus = settingsLoad(settings);
    clockFaceBegin(settings);

    connectAndSync();
    lastAttemptMs = millis();
}

void loop() {
    static int lastMinute = -1;
    static int lastSecond = -1;

    const unsigned long now = millis();

    if (!timeSynced && now - lastAttemptMs >= RETRY_INTERVAL_MS) {
        lastAttemptMs = now;
        connectAndSync();
    } else if (timeSynced && now - lastAttemptMs >= RESYNC_INTERVAL_MS) {
        lastAttemptMs = now;
        if (connectWiFi(WIFI_CONNECT_TIMEOUT_MS)) {
            syncTime();
        }
    }

    struct tm timeinfo;
    if (timeSynced && getLocalTime(&timeinfo, 0)) {
        const bool colonVisible = !settings.blinkColon || (timeinfo.tm_sec % 2 == 0);
        const bool minuteChanged = timeinfo.tm_min != lastMinute;
        const bool tick = settings.blinkColon && timeinfo.tm_sec != lastSecond;
        if (minuteChanged || tick) {
            clockFaceRender(settings, timeinfo, colonVisible);
            lastMinute = timeinfo.tm_min;
            lastSecond = timeinfo.tm_sec;
        }
    }

    delay(200);
}
