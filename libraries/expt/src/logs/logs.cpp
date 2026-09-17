/**
 * @file logs.c
 * @brief Implementation of logging functions.
 * 
 * @copyright 2024 MTA
 * @author 
 * Ing. Jiri Konecny
 */

#include "logs.hpp"

#include <array>

#ifndef DEBUG_VERBOSE_LEVEL
#define DEBUG_VERBOSE_LEVEL DEBUG_VERBOSE_IMPORTANT
#endif

namespace {
constexpr size_t LOGGER_BUFFERED_LINE_COUNT = 16;
constexpr size_t LOGGER_BUFFERED_LINE_LENGTH = 256;

bool loggerUsbCdcAvailable = true;
std::array<std::array<char, LOGGER_BUFFERED_LINE_LENGTH>, LOGGER_BUFFERED_LINE_COUNT> bufferedLogLines{};
size_t bufferedLogStart = 0;
size_t bufferedLogCount = 0;

void bufferLogLine(const char *message) {
    if (!message) {
        return;
    }

    size_t target = (bufferedLogStart + bufferedLogCount) % LOGGER_BUFFERED_LINE_COUNT;
    if (bufferedLogCount == LOGGER_BUFFERED_LINE_COUNT) {
        target = bufferedLogStart;
        bufferedLogStart = (bufferedLogStart + 1) % LOGGER_BUFFERED_LINE_COUNT;
    } else {
        ++bufferedLogCount;
    }

    snprintf(bufferedLogLines[target].data(), LOGGER_BUFFERED_LINE_LENGTH, "%s", message);
}
}

std::string buildMessage(const char *format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return std::string(buffer);
}

#if defined(ARDUINO_H_ENV) && defined(ARDUINO)
    #include <Arduino.h>  ///< Include Arduino Serial functions
#elif defined(STDIO_H_ENV)
    #include <stdio.h>    ///< Include standard I/O functions
#endif

void logMessage(const char *format, ...) {
    va_list args;
    va_start(args, format);

    #if defined(ARDUINO_H_ENV) && defined(ARDUINO)
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        if (!loggerUsbCdcAvailable)
        {
            bufferLogLine(buffer);
            va_end(args);
            return;
        }

        if(!Serial)
        {
            initLogger();
        }
        Serial.println(buffer);  // Print via Arduino Serial
        Serial.flush();
    #elif defined(STDIO_H_ENV)
        vprintf(format, args);  // Print via standard console
    #endif
    va_end(args);
}

void setLoggerUsbCdcAvailable(bool available) {
    loggerUsbCdcAvailable = available;
}

void flushBufferedLogMessages() {
#if defined(ARDUINO_H_ENV) && defined(ARDUINO)
    if (!loggerUsbCdcAvailable) {
        return;
    }

    if (!Serial) {
        initLogger();
    }

    while (bufferedLogCount > 0) {
        const char *line = bufferedLogLines[bufferedLogStart].data();
        Serial.println(line);
        bufferedLogStart = (bufferedLogStart + 1) % LOGGER_BUFFERED_LINE_COUNT;
        --bufferedLogCount;
    }
    Serial.flush();
#elif defined(STDIO_H_ENV)
    while (bufferedLogCount > 0) {
        printf("%s\n", bufferedLogLines[bufferedLogStart].data());
        bufferedLogStart = (bufferedLogStart + 1) % LOGGER_BUFFERED_LINE_COUNT;
        --bufferedLogCount;
    }
#else
    bufferedLogStart = 0;
    bufferedLogCount = 0;
#endif
}

static int clampDebugVerboseLevel(int level) {
    if (level < DEBUG_VERBOSE_ERRORS) {
        return DEBUG_VERBOSE_ERRORS;
    }
    if (level > DEBUG_VERBOSE_ALL) {
        return DEBUG_VERBOSE_ALL;
    }
    return level;
}

static bool isDebugLevelEnabled(int level) {
#if ENABLE_DEBUG && DEBUG_VERBOSE_LEVEL > 0
    return clampDebugVerboseLevel(level) <= clampDebugVerboseLevel(DEBUG_VERBOSE_LEVEL);
#else
    (void)level;
    return false;
#endif
}

static void sanitizeDebugPayload(char *buffer) {
    if (!buffer) {
        return;
    }

    for (size_t i = 0; buffer[i] != '\0'; ++i) {
        if (buffer[i] == '\r' || buffer[i] == '\n') {
            buffer[i] = ' ';
        }
    }
}

static void debugLogMessageVa(int level, const char *source, const char *reason, const char *format, va_list args) {
#if ENABLE_DEBUG
    if (!isDebugLevelEnabled(level)) {
        return;
    }

    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format ? format : "", args);
    sanitizeDebugPayload(buffer);

    logMessage(
        "[DEBUG][%s]: %s reason=%s",
        source ? source : "unknown",
        buffer,
        reason ? reason : "-");
#else
    (void)level;
    (void)source;
    (void)reason;
    (void)format;
    (void)args;
#endif
}

void debugLogMessage(int level, const char *source, const char *reason, const char *format, ...) {
    va_list args;
    va_start(args, format);
    debugLogMessageVa(level, source, reason, format, args);
    va_end(args);
}

void debugLogMessage(const char *source, const char *reason, const char *format, ...) {
    va_list args;
    va_start(args, format);
    debugLogMessageVa(DEBUG_VERBOSE_ALL, source, reason, format, args);
    va_end(args);
}

void initLogger(unsigned int baudrate, unsigned int timeout) {
    #if defined(ARDUINO_H_ENV) && defined(ARDUINO)
        if (!loggerUsbCdcAvailable) {
            return;
        }
        Serial.begin(baudrate); // Initialize Serial for Arduino
        Serial.setTimeout(timeout); // Set timeout for Serial receive
    #elif defined(STDIO_H_ENV)
        // No initialization needed for standard console
        printf("Logger initialized for standard console...\n");
    #endif
}

