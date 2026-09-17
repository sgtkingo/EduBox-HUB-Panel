#pragma once

#include <cmath>
#include <limits>

// Fixed Y limits; X remains controlled by the existing chart gestures.
struct ChartManualScale {
    double yMin = -1.0;
    double yMax = 1.0;

    bool valid() const {
        return std::isfinite(yMin) && std::isfinite(yMax) &&
               yMin < yMax && std::isfinite(yMax - yMin);
    }
    double normalizedY(double value) const {
        if (!std::isfinite(value) || value < yMin || value > yMax)
            return std::numeric_limits<double>::quiet_NaN();
        return (value - yMin) / (yMax - yMin);
    }
};
