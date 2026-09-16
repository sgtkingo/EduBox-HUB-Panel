"""Run the production GT911 driver with a bounded mock I2C bus (requires g++)."""
from pathlib import Path
import subprocess
import tempfile
import unittest

DRIVER = Path(__file__).resolve().parents[2] / "gt911-arduino"


class Gt911Test(unittest.TestCase):
    def test_invalid_frames_and_configuration(self):
        with tempfile.TemporaryDirectory(prefix="edubox-touch-") as directory:
            root = Path(directory)
            (root / "Arduino.h").write_text(r'''
#pragma once
#include <stdint.h>
#include <stddef.h>
#define OUTPUT 1
#define INPUT 0
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline void delay(int) {}
inline uint8_t lowByte(uint16_t x) { return x & 255; }
inline uint8_t highByte(uint16_t x) { return x >> 8; }
''')
            (root / "Wire.h").write_text(r'''
#pragma once
#include "Arduino.h"
#include <vector>
#include <cassert>
struct WireMock {
    uint8_t memory[65536] = {};
    uint16_t reg = 0, shortReg = 0;
    bool nack = false;
    std::vector<uint8_t> tx, rx;
    size_t cursor = 0;
    void begin(int, int) {}
    void beginTransmission(int) { tx.clear(); }
    size_t write(uint8_t value) {
        if (tx.size() >= 34) return 0;
        tx.push_back(value); return 1;
    }
    int endTransmission() {
        if (nack) return 2;
        assert(tx.size() >= 2);
        reg = (tx[0] << 8) | tx[1];
        for (size_t i = 2; i < tx.size(); ++i) memory[reg + i - 2] = tx[i];
        return 0;
    }
    uint8_t requestFrom(int, uint8_t count) {
        assert(count <= 32);
        if (reg == shortReg) --count;
        rx.assign(memory + reg, memory + reg + count); cursor = 0;
        return count;
    }
    int available() { return rx.size() - cursor; }
    int read() { return available() ? rx[cursor++] : -1; }
};
inline WireMock Wire;
''')
            (root / "main.cpp").write_text(r'''
#include "TAMC_GT911.h"
#include <cassert>
void point(int index, uint16_t x, uint16_t y) {
    const int reg = GT911_POINT_1 + index * 8;
    Wire.memory[reg] = index;
    Wire.memory[reg + 1] = lowByte(x); Wire.memory[reg + 2] = highByte(x);
    Wire.memory[reg + 3] = lowByte(y); Wire.memory[reg + 4] = highByte(y);
}
int main() {
    for (int i = 0; i < GT911_CONFIG_SIZE; ++i)
        Wire.memory[GT911_CONFIG_START + i] = i;
    TAMC_GT911 driver(19, 20, 255, 255, 800, 480);
    driver.begin();
    assert(Wire.memory[GT911_X_OUTPUT_MAX_LOW] == lowByte(800));
    assert(Wire.memory[GT911_X_OUTPUT_MAX_HIGH] == highByte(800));
    assert(Wire.memory[GT911_Y_OUTPUT_MAX_LOW] == lowByte(480));
    assert(Wire.memory[GT911_Y_OUTPUT_MAX_HIGH] == highByte(480));
    uint8_t sum = 0;
    for (int i = 0; i < GT911_CONFIG_SIZE; ++i) sum += Wire.memory[GT911_CONFIG_START + i];
    assert(sum == 0 && Wire.memory[GT911_CONFIG_FRESH] == 1);

    Wire.memory[GT911_POINT_INFO] = 0x81; point(0, 100, 200); driver.read();
    assert(driver.isTouched && driver.touches == 1);
    assert(driver.points[0].x == 700 && driver.points[0].y == 280);
    Wire.memory[GT911_POINT_INFO] = 0x01; driver.read(); // not ready
    assert(driver.isTouched && driver.points[0].x == 700);
    assert(Wire.memory[GT911_POINT_INFO] == 0x01);
    Wire.memory[GT911_POINT_INFO] = 0x80; driver.read(); // release
    assert(!driver.isTouched && driver.touches == 0);

    for (int count = 6; count < 16; ++count) {
        driver.points[4].x = 123;
        Wire.memory[GT911_POINT_INFO] = 0x80 | count; driver.read();
        assert(!driver.isTouched && driver.touches == 0 && driver.points[4].x == 123);
    }
    Wire.memory[GT911_POINT_INFO] = 0x81; point(0, 882, 617); driver.read();
    assert(!driver.isTouched && driver.touches == 0);
    Wire.memory[GT911_POINT_INFO] = 0x82; point(0, 100, 200); point(1, 300, 400);
    Wire.shortReg = GT911_POINT_1 + 8; driver.read();
    assert(!driver.isTouched && driver.touches == 0 && driver.points[0].x == 700);
    Wire.shortReg = 0;
    Wire.memory[GT911_POINT_INFO] = 0x85;
    for (int i = 0; i < 5; ++i) point(i, 100 * i, 50 * i);
    driver.read();
    assert(driver.isTouched && driver.touches == 5);
    assert(driver.points[0].x == 800 && driver.points[0].y == 480);
    Wire.nack = true; driver.read();
    assert(!driver.isTouched && driver.touches == 0);
}
''')
            executable = root / "gt911_test.exe"
            subprocess.run([
                "g++", "-std=c++17", "-I", str(root), "-I", str(DRIVER),
                str(DRIVER / "TAMC_GT911.cpp"), str(root / "main.cpp"),
                "-o", str(executable),
            ], check=True)
            subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    unittest.main()
