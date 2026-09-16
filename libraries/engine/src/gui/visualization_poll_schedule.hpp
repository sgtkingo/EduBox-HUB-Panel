#ifndef VISUALIZATION_POLL_SCHEDULE_HPP
#define VISUALIZATION_POLL_SCHEDULE_HPP

#include <cstdint>

class VisualizationPollSchedule
{
public:
    static constexpr uint32_t MIN_PERIOD_MS = 10;
    static constexpr uint32_t MAX_PERIOD_MS = 1000;

    uint32_t periodMs() const { return period; }
    void reset(uint32_t now) { lastPoll = now; }
    void setPeriod(uint32_t requested, uint32_t now)
    {
        period = requested < MIN_PERIOD_MS ? MIN_PERIOD_MS :
                 requested > MAX_PERIOD_MS ? MAX_PERIOD_MS : requested;
        reset(now);
    }
    bool due(uint32_t now)
    {
        if (static_cast<uint32_t>(now - lastPoll) < period) return false;
        // Schedule one poll after a stall, rather than a burst of missed polls.
        lastPoll = now;
        return true;
    }

private:
    uint32_t period = 100;
    uint32_t lastPoll = 0;
};

#endif
