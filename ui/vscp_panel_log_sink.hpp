#pragma once

#include <expt.hpp>
#include <vscp.hpp>

// Use the application logger, including its transfer-mode buffering. With a
// USB/UART0 bridge, traces are visible in Serial Monitor and on the Board UART.
class PanelVscpLogSink final : public vscp::LogSink {
public:
    void writeLogLine(const vscp::String& message) override {
#if ENABLE_DEBUG && DEBUG_VERBOSE_LEVEL >= 3
        debugLogMessage(DEBUG_VERBOSE_ALL, "VSCP", "transport", "%s", message.c_str());
#else
        (void)message;
#endif
    }
};
