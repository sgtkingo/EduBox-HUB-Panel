"""Check fixed manual Y scaling independently of GUI timing."""
from pathlib import Path
import subprocess
import tempfile
import unittest

GUI = Path(__file__).resolve().parents[1] / "src/gui"


class ManualScaleTest(unittest.TestCase):
    def test_bounds_and_fixed_y(self):
        with tempfile.TemporaryDirectory(prefix="edubox-scale-") as directory:
            root = Path(directory)
            source = root / "main.cpp"
            source.write_text(r'''
#include "chart_manual_scale.hpp"
#include <cassert>
#include <limits>
#include <initializer_list>
int main() {
    ChartManualScale scale;
    assert(scale.valid());
    for (const double magnitude : {1e-12, 1.0, 1e12}) {
        scale.yMin = -magnitude; scale.yMax = magnitude;
        assert(scale.valid());
        assert(scale.normalizedY(-magnitude) == 0);
        assert(scale.normalizedY(0) == 0.5);
        assert(scale.normalizedY(magnitude) == 1);
        assert(std::isnan(scale.normalizedY(2 * magnitude)));
        assert(std::isnan(scale.normalizedY(-2 * magnitude)));
        assert(std::isnan(scale.normalizedY(std::numeric_limits<double>::quiet_NaN())));
    }
    scale.yMax = scale.yMin;
    assert(!scale.valid());
    scale.yMax = std::numeric_limits<double>::infinity();
    assert(!scale.valid());
    scale.yMax = std::numeric_limits<double>::quiet_NaN();
    assert(!scale.valid());
    scale.yMin = -std::numeric_limits<double>::max();
    scale.yMax = std::numeric_limits<double>::max();
    assert(!scale.valid());
}
''')
            executable = root / "manual_scale.exe"
            subprocess.run(["g++", "-std=c++17",
                            "-I", str(GUI), str(source), "-o", str(executable)], check=True)
            subprocess.run([str(executable)], check=True, timeout=10)


if __name__ == "__main__":
    unittest.main()
