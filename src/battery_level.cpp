#include "battery_level.h"

#include <lilka.h>

namespace {

// Типова крива розряду LiPo-комірки: напруга холостого ходу -> відсоток заряду.
// Значення впорядковані за спаданням напруги.
struct BatteryPoint {
    float voltage;
    int level;
};

constexpr BatteryPoint LIPO_CURVE[] = {
    {4.20f, 100}, {4.15f, 95}, {4.11f, 90}, {4.08f, 85}, {4.02f, 80}, {3.98f, 75}, {3.95f, 70}, {3.91f, 65},
    {3.87f, 60},  {3.85f, 55}, {3.84f, 50}, {3.82f, 45}, {3.80f, 40}, {3.79f, 35}, {3.77f, 30}, {3.75f, 25},
    {3.73f, 20},  {3.71f, 15}, {3.69f, 10}, {3.61f, 5},  {3.27f, 0},
};

constexpr int LIPO_CURVE_SIZE = sizeof(LIPO_CURVE) / sizeof(LIPO_CURVE[0]);

// Лінійна інтерполяція відсотка за напругою.
int levelForVoltage(float voltage) {
    if (voltage >= LIPO_CURVE[0].voltage) {
        return 100;
    }
    if (voltage <= LIPO_CURVE[LIPO_CURVE_SIZE - 1].voltage) {
        return 0;
    }

    for (int i = 0; i < LIPO_CURVE_SIZE - 1; i++) {
        const BatteryPoint& high = LIPO_CURVE[i];
        const BatteryPoint& low = LIPO_CURVE[i + 1];
        if (voltage <= high.voltage && voltage >= low.voltage) {
            const float span = high.voltage - low.voltage;
            const float t = (voltage - low.voltage) / span;
            return low.level + static_cast<int>(t * (high.level - low.level) + 0.5f);
        }
    }
    return 0;
}

} // namespace

float batteryReadVoltage() {
    const uint16_t raw = lilka::battery.readRawValue();
    return static_cast<float>(raw) / 4095.0f * LILKA_BATTERY_MAX_MEASURABLE_VOLTAGE;
}

int batteryReadLevel() {
    const float voltage = batteryReadVoltage();
    if (voltage < 0.5f) {
        return -1;
    }
    return levelForVoltage(voltage);
}
