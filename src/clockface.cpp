#include "clockface.h"

#include "logger.h"

namespace {

lilka::Canvas* canvas = nullptr;
const uint8_t* digitFont = nullptr;
int16_t digitHeight = 0;

// Зсув усього циферблата по горизонталі (від'ємне — ліворуч).
constexpr int16_t FACE_OFFSET_X = -4;

// Великі цифрові шрифти (тільки цифри). Двокрапку малюємо вручну,
// бо повні (_tf) варіанти існують лише до 58 px.
const uint8_t* const FONT_CANDIDATES[] = {
    u8g2_font_logisoso92_tn,
    u8g2_font_logisoso78_tn,
    u8g2_font_logisoso62_tn,
    u8g2_font_logisoso58_tn,
    u8g2_font_logisoso50_tn,
    u8g2_font_logisoso46_tn,
    u8g2_font_logisoso38_tn,
    u8g2_font_logisoso32_tn,
};

// Висота гліфів шрифту U8g2 (ascent - descent).
int16_t fontHeight(const uint8_t* font) {
    return static_cast<int8_t>(font[13]) - static_cast<int8_t>(font[14]);
}

// Ширина пари цифр у заданому шрифті.
int16_t measurePair(const uint8_t* font) {
    canvas->setFont(font);
    return lilka::getTextWidth(font, "00");
}

// Намалювати цифри так, щоб їхні реальні межі були відцентровані по вертикалі
// відносно centerY (а не по метриках шрифту, які включають місце під нижні виноси).
void drawDigitsCentered(const char* text, int16_t x, int16_t centerY) {
    int16_t x1, y1;
    uint16_t w, h;
    canvas->getTextBounds(text, x, 0, &x1, &y1, &w, &h);
    const int16_t baselineY = centerY - h / 2 - y1;
    canvas->setCursor(x, baselineY);
    canvas->print(text);
}

} // namespace

void clockFaceBegin(const Settings& settings) {
    canvas = new lilka::Canvas();
    canvas->setTextWrap(false);

    const int16_t usableWidth = canvas->width() - 24;
    for (const uint8_t* font : FONT_CANDIDATES) {
        const int16_t pairWidth = measurePair(font);
        const int16_t colonWidth = pairWidth / 2;
        if (pairWidth * 2 + colonWidth <= usableWidth) {
            digitFont = font;
            break;
        }
    }
    if (digitFont == nullptr) {
        digitFont = FONT_CANDIDATES[sizeof(FONT_CANDIDATES) / sizeof(FONT_CANDIDATES[0]) - 1];
    }

    digitHeight = fontHeight(digitFont);
    canvas->setFont(digitFont);
    canvas->setTextColor(settings.foreground);

    loggerInfo("Clock face ready, digit font height: %d", digitHeight);
}

void clockFaceRender(const Settings& settings, const struct tm& time, bool colonVisible) {
    canvas->fillScreen(settings.background);
    canvas->setFont(digitFont);
    canvas->setTextColor(settings.foreground);

    const int16_t pairWidth = measurePair(digitFont);
    const int16_t colonWidth = pairWidth / 2;
    const int16_t totalWidth = pairWidth * 2 + colonWidth;
    const int16_t startX = (canvas->width() - totalWidth) / 2 + FACE_OFFSET_X;
    const int16_t centerY = canvas->height() / 2;

    char hours[3];
    char minutes[3];
    snprintf(hours, sizeof(hours), "%02d", time.tm_hour);
    snprintf(minutes, sizeof(minutes), "%02d", time.tm_min);

    drawDigitsCentered(hours, startX, centerY);
    drawDigitsCentered(minutes, startX + pairWidth + colonWidth, centerY);

    if (colonVisible) {
        const int16_t dotRadius = digitHeight / 12 > 2 ? digitHeight / 12 : 2;
        const int16_t dotOffset = digitHeight / 5;
        const int16_t centerX = startX + pairWidth + colonWidth / 2;
        canvas->fillCircle(centerX, centerY - dotOffset, dotRadius, settings.foreground);
        canvas->fillCircle(centerX, centerY + dotOffset, dotRadius, settings.foreground);
    }

    lilka::display.drawCanvas(canvas);
}

void clockFaceMessage(const Settings& settings, const String& line1, const String& line2) {
    lilka::display.fillScreen(settings.background);
    lilka::display.setFont(FONT_10x20);
    lilka::display.setTextColor(settings.foreground);

    const int16_t centerX = lilka::display.width() / 2;
    const int16_t centerY = lilka::display.height() / 2;

    lilka::display.drawTextAligned(line1.c_str(), centerX, line2.length() ? centerY - 16 : centerY,
                                   lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);
    if (line2.length()) {
        lilka::display.drawTextAligned(line2.c_str(), centerX, centerY + 16, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);
    }
}
