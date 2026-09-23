#pragma once

#include <edubox_ble.hpp>
#include <expt.hpp>

class PanelBleLogSink final : public edubox::ble::LogSink {
public:
    bool enabled(edubox::ble::LogLevel level) const override {
#if ENABLE_DEBUG && DEBUG_VERBOSE_LEVEL >= 2
        return static_cast<int>(level) <= DEBUG_VERBOSE_LEVEL;
#else
        (void)level;
        return false;
#endif
    }

    void write(edubox::ble::LogLevel level, const char *source, const char *reason,
               const char *message) override {
#if ENABLE_DEBUG && DEBUG_VERBOSE_LEVEL >= 2
        debugLogMessage(static_cast<int>(level), source, reason, "%s", message);
#else
        (void)level;
        (void)source;
        (void)reason;
        (void)message;
#endif
    }
};
