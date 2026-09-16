"""Regression checks for PSRAM allocation and internal-heap isolation (g++)."""
from pathlib import Path
import subprocess
import tempfile
import unittest


HEAP_HEADER = Path(__file__).resolve().parents[2] / "lvgl/src/misc/edubox_heap.h"


class LvglHeapTest(unittest.TestCase):
    def test_psram_exhaustion_never_falls_back_to_internal_ram(self):
        with tempfile.TemporaryDirectory(prefix="edubox-lvgl-") as directory:
            root = Path(directory)
            (root / "esp_heap_caps.h").write_text(r'''
#pragma once
#include <stdint.h>
#include <stdlib.h>
#define MALLOC_CAP_SPIRAM 1u
#define MALLOC_CAP_8BIT 2u
#define MALLOC_CAP_INTERNAL 4u
static size_t installed_psram = 8 * 1024 * 1024;
static int exhausted = 0, allocations = 0;
static uint32_t last_caps = 0;
static size_t heap_caps_get_total_size(uint32_t caps) {
    return (caps & MALLOC_CAP_SPIRAM) ? installed_psram : 327680;
}
static void *heap_caps_malloc(size_t size, uint32_t caps) {
    ++allocations; last_caps = caps;
    return exhausted ? NULL : malloc(size);
}
static void *heap_caps_realloc(void *ptr, size_t size, uint32_t caps) {
    ++allocations; last_caps = caps;
    return exhausted ? NULL : realloc(ptr, size);
}
static void heap_caps_free(void *ptr) { free(ptr); }
''')
            (root / "main.c").write_text(r'''
#include "edubox_heap.h"
#include <assert.h>
int main(void) {
    void *buffer = edubox_lv_malloc(128);
    assert(buffer && last_caps == (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    buffer = edubox_lv_realloc(buffer, 256);
    assert(buffer && last_caps == (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    ((unsigned char *)buffer)[0] = 42;
    exhausted = 1;
    int before = allocations;
    assert(edubox_lv_malloc(128) == NULL);
    assert(allocations == before + 1);
    assert(last_caps == (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    assert(edubox_lv_realloc(buffer, 512) == NULL);
    assert(((unsigned char *)buffer)[0] == 42);
    assert(last_caps == (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    edubox_lv_free(buffer);
    edubox_lv_free(NULL);
    exhausted = 0;
    installed_psram = 0;
    buffer = edubox_lv_malloc(128);
    assert(buffer && last_caps == (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    edubox_lv_free(buffer);
    return 0;
}
''')
            # LVGL is compiled as C; verify the same header also works in C++.
            for language, standard in [("c", "c11"), ("c++", "c++17")]:
                executable = root / "heap_test.exe"
                subprocess.run([
                    "g++", "-x", language, f"-std={standard}",
                    "-DARDUINO_ARCH_ESP32", "-I", str(root),
                    "-I", str(HEAP_HEADER.parent), str(root / "main.c"),
                    "-o", str(executable),
                ], check=True)
                subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    unittest.main()
