# Library picture crash investigation

The reported firmware ELF SHA256 starts with `99093c8a3`. The matching ELF was
found in the local Arduino build cache and used to decode the backtrace:

```text
LibraryGui::showLibrary -> updateDetail
DeviceCatalogBrowserRenderer::renderDeviceDetail -> renderPicture
StorageManager::exists -> fs::FS::exists -> VFSImpl::exists
VFSFileImpl::VFSFileImpl -> fopen -> _fopen_r -> __sfp
__retarget_lock_init_recursive -> lock_init_generic -> abort
```

The abort occurs while creating a newlib FILE mutex, before PNG decoding. This
is a failed RTOS semaphore allocation, which cannot be caught as a C++ exception.
Arduino's `SD.exists()` opens an existing file via `fopen`; a missing picture does
not take that regular-file path.

The build uses `CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=4096` and
`CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=0`. LVGL previously used ordinary
`malloc`/`realloc`, so its many small widget allocations competed with FILE/RTOS
locks for internal RAM even with PSRAM installed. LVGL allocations (including
LodePNG buffers) now explicitly use PSRAM. Exhausted PSRAM does not fall back to
internal RAM. Builds without PSRAM retain an internal allocator.

Debug messages were disabled because `engine/src/config.hpp` defined
`ENABLE_DEBUG=0` before `expt/src/config.hpp` supplied its conditional default.
The engine setting now enables debug at level 3. Storage probes print the path,
internal free bytes and PSRAM free bytes before
`exists()`. LVGL warnings are registered with the project logger; failed preview
decoding displays a built-in placeholder instead of retrying the file at draw time.
With the reported `CDCOnBoot=default` configuration, `Serial` uses UART0 at
115200 baud. Builds with CDC on boot send the project's Serial logs to USB CDC.

Host allocator regression test:

```powershell
python libraries/engine/tests/test_lvgl_heap.py
```

On-device validation still requires flashing the corrected firmware and opening
Library with an SD picture present. Check the new `storage probe` and
`picture decode start` messages, then switch entities and reopen Library to verify
that memory is reclaimed. The supplied backtrace proves the mutex failure; actual
free-memory values and successful rendering must be measured on the device.

## Follow-up: interrupt watchdog during the diagnostic

The subsequent boot log reached the end of `/records/` listing, then reported
`Interrupt wdt timeout on CPU1`, with PC `0x420c5840` and return address
`0x420c5924`. The uploaded ELF resolves these to `multi_heap_get_info_tlsf` and
`tlsf_walk_pool`. At this point `AppConfigManager::load` invokes `exists`, whose
new diagnostic called `heap_caps_get_largest_free_block`. Disassembly confirms
that this query calls `heap_caps_get_info`, walking pools under the heap lock.
It has been removed from the synchronous probe. Free-byte queries use the heap's
stored counters instead of walking individual blocks.

This identifies where the follow-up watchdog fired; it does not establish why
the pool walk took so long. If later allocation failures persist, investigate
heap integrity as well as available memory. The PSRAM fix still requires device
validation after this diagnostic correction.

## Follow-up: invalid GT911 touch samples

After successful boot, LVGL reported coordinates outside 800 x 480. The bundled
GT911 driver accepted status/point bytes without checking I2C errors, interpreted
the touch count before checking the ready bit, and looped over a 4-bit count while
`points` had only five entries. A failed read returning `0xff` could therefore
write past `points`, providing another possible source of heap/memory damage in
the earlier failures. Actual corruption from this path has not been captured on
the device, so the initial allocation failure cannot be attributed exclusively to
legitimate GUI memory usage.

The driver now requires complete reads, bounds counts to five, validates raw
coordinates before unsigned rotation, and publishes only a complete frame.
The adapter rejects mapped coordinates outside the display. Configuration reads
and writes are chunked to fit Wire buffers; resolution values and a deterministic
checksum are sent before the configuration-fresh flag. Host regression checks:

```powershell
python libraries/engine/tests/test_gt911.py
```
