/* EduBox LVGL allocator: keep GUI/PNG buffers out of the RTOS internal heap. */
#ifndef EDUBOX_LVGL_HEAP_H
#define EDUBOX_LVGL_HEAP_H

#include <stdlib.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>

static inline uint32_t edubox_lv_heap_caps(void)
{
    const uint32_t external = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
    /* Check installed capacity, not free capacity: exhausted PSRAM must return
     * NULL instead of consuming the internal heap needed by fopen/RTOS locks. */
    return heap_caps_get_total_size(external) ? external : MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
}

static inline void *edubox_lv_malloc(size_t size)
{
    return heap_caps_malloc(size, edubox_lv_heap_caps());
}

static inline void *edubox_lv_realloc(void *ptr, size_t size)
{
    return heap_caps_realloc(ptr, size, edubox_lv_heap_caps());
}

static inline void edubox_lv_free(void *ptr)
{
    heap_caps_free(ptr);
}
#else
static inline void *edubox_lv_malloc(size_t size) { return malloc(size); }
static inline void *edubox_lv_realloc(void *ptr, size_t size) { return realloc(ptr, size); }
static inline void edubox_lv_free(void *ptr) { free(ptr); }
#endif

#endif
