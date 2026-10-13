#pragma once

#include <lilka.h>

#include "settings.h"

// Ініціалізувати циферблат: створити графічний буфер і підібрати розмір шрифту.
// Викликати після lilka::begin().
void clockFaceBegin(const Settings& settings);

// Намалювати час (HH:MM) на буфері та вивести його на екран.
void clockFaceRender(const Settings& settings, const struct tm& time, bool colonVisible);

// Показати службове повідомлення (для станів "підключення", "помилка" тощо).
void clockFaceMessage(const Settings& settings, const String& line1, const String& line2 = "");
