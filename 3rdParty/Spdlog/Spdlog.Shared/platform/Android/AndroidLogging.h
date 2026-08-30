#pragma once

#include <spdlog/logger.h>

#include <string_view>

namespace utility_helpers::android {

// Создаёт process-wide логгер с выводом в Android Logcat. Повторный вызов безопасен.
void initializeLogging(std::string_view tag = "UtilityHelpers");

// Возвращает логгер, создавая его с тегом по умолчанию при первом обращении.
spdlog::logger& log();

} // namespace utility_helpers::android
