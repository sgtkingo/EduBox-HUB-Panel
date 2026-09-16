"""Check polling deadlines independently of the GUI frame rate (requires g++)."""
from pathlib import Path
import subprocess
import tempfile
import unittest

GUI = Path(__file__).resolve().parents[1] / "src/gui"


class PollScheduleTest(unittest.TestCase):
    def test_live_changes_stalls_and_rollover(self):
        with tempfile.TemporaryDirectory(prefix="edubox-poll-") as directory:
            root = Path(directory)
            (root / "main.cpp").write_text(r'''
#include "visualization_poll_schedule.hpp"
#include <cassert>
#include <cstdint>
int main() {
    VisualizationPollSchedule schedule;
    assert(schedule.periodMs() == 100);
    assert(!schedule.due(99));
    assert(schedule.due(100));
    assert(!schedule.due(199));
    assert(schedule.due(200));

    // A live change starts the next interval from the time of the change.
    schedule.setPeriod(10, 205);
    assert(!schedule.due(214));
    assert(schedule.due(215));
    unsigned polls = 0;
    for (uint32_t now = 216; now <= 1215; ++now) {
        if (schedule.due(now)) ++polls;
    }
    assert(polls == 100); // 10 ms polling is independent of 16 ms drawing.

    schedule.setPeriod(1000, 1215);
    assert(!schedule.due(2214));
    assert(schedule.due(2215));
    assert(schedule.due(10000)); // one poll after a long transaction/stall
    assert(!schedule.due(10000));
    assert(!schedule.due(10999));
    assert(schedule.due(11000));

    schedule.setPeriod(0, 11000);
    assert(schedule.periodMs() == 10);
    schedule.setPeriod(UINT32_MAX, 11000);
    assert(schedule.periodMs() == 1000);

    schedule.setPeriod(10, UINT32_MAX - 5);
    assert(!schedule.due(3));
    assert(schedule.due(4)); // unsigned elapsed time handles millis rollover

    // Paused/hidden visualization resets the deadline, avoiding an overdue poll.
    schedule.reset(20000);
    assert(!schedule.due(20009));
    assert(schedule.due(20010));
}
''')
            executable = root / "poll_test.exe"
            subprocess.run([
                "g++", "-std=c++17", "-I", str(GUI), str(root / "main.cpp"),
                "-o", str(executable),
            ], check=True)
            subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    unittest.main()
