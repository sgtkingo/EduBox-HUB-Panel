# EduBox HUB Panel Developer Map

Tento dokument je orientacni mapa projektu pro vyvojare. Cilem neni nahradit
detailni API dokumentaci v hlavickach, ale rychle vysvetlit workflow, vlastnictvi
komponent, datove toky a mista, kam sahnout pri typickych upravach.

## Co projekt dela

EduBox HUB Panel je LVGL HMI aplikace pro ESP32-S3 7" displej. Firmware zobrazuje
zarizeni definovana v JSON databazi, umi pripojit vybrana zarizeni na virtualni piny,
komunikuje s upstream endpointem pres textovy protokol VSCP a zobrazuje runtime hodnoty.

Zjednoduseny tok:

```text
ESP32 UI
  -> nacte DB.json ze storage
  -> postavi katalog zarizeni
  -> uzivatel vybere komunikaci, zarizeni a piny
  -> firmware posle VSCP INIT / CONNECT
  -> v runtime cyklu posila VSCP UPDATE
  -> GUI kresli aktualni hodnoty, grafy a DataBundle zaznamy
```

Upstream muze byt Python emulator, EduBox HUB nebo vlastni hardware implementujici VSCP.

## Root adresare

| Cesta | Ucel |
| --- | --- |
| `ui/` | Arduino sketch, hardware init, LVGL display/touch setup, hlavni `setup()` a `loop()`. |
| `libraries/engine/` | Vlastni aplikacni engine: GUI, runtime manager, storage, device katalog, DataBundle. |
| `libraries/vscp/` | Sdílený VSCP codec, transportní adaptéry, klient a server. |
| `libraries/ui/` | SquareLine/LVGL boot UI generovana vrstva. Aplikacni GUI je hlavne v `engine/src/gui`. |
| `libraries/expt/` | Logovani, splash hlasky a exception framework. |
| `libraries/lvgl/`, `libraries/LovyanGFX/`, `libraries/ArduinoJson/` | Vendored third-party knihovny. |
| `storage/data/DB.json` | Kanonicka host/dev kopie device katalogu. |
| `ui/data/DB.json` | Kopie katalogu pro Arduino/firmware workflow. |
| `emulator/` | Python VSCP emulator, virtualni a real sensor runners. |
| `docs/` | Instalace, workflow poznamky, obrazky a wiki drafty. |
| `bin/` | Exportovane binarky. |

## Firmware entrypoint

Hlavni sketch je `ui/ui.ino`.

Zodpovednosti:

- nastavi Elecrow/LovyanGFX RGB display,
- inicializuje LVGL draw buffer,
- inicializuje touch driver z `ui/touch.h`,
- vytvori globalni runtime objekty:
  - `DeviceCatalog deviceCatalog`
  - `DeviceBrowserState deviceBrowserState`
  - `DeviceManager deviceManager`
  - `DeviceVisualizationSession deviceVisualizationSession`
  - `DataBundleManager dataBundleManager`
  - `GuiManager guiManager`
- zavola `guiManager.init(DEVICE_DB_STORAGE_PATH)`,
- po bootu otevre Main Menu,
- v `loop()` vola `guiManager.redraw()`.

`DEVICE_DB_STORAGE_PATH` vychazi z `STORAGE_DEFAULT_DEVICE_DB_PATH` v
`libraries/engine/src/config.hpp`.

## Konfigurace buildu a storage

Hlavni config je `libraries/engine/src/config.hpp`.

Dulezite volby:

- `ENABLE_DEBUG`: jednotny debug switch pro projekt, defaultne `1`.
- `DEBUG_VERBOSE_LEVEL`: uroven detailu debug logu, defaultne `2`.
  - `1`: chyby a kriticke debug informace.
  - `2`: chyby, warningy a dulezite operace / vysledky procesu.
  - `3`: vse, vcetne detailnich konverzi a nizkourovnovych debug stop.
- `STORAGE_OPTION_SPIFFS`: vychozi dev rezim, katalog je `/DB.json`.
- `STORAGE_OPTION_SD`: katalog je `/data/DB.json`.
- `STORAGE_SEED_DEFAULT_DB_ON_MISSING`: ve SPIFFS rezimu umi zalozit default DB z embedded stringu.
- `FILE_TRANSFER_USB_MSC_ENABLED`: zapina USB MSC bridge pro Transfer Mode.
- `FILE_TRANSFER_USB_MSC_VENDOR_ID`, `FILE_TRANSFER_USB_MSC_PRODUCT_ID`, `FILE_TRANSFER_USB_MSC_REVISION`: USB identifikace MSC disku.

Debug logy pouzivaji `debugLogMessage(source, reason, format, ...)` a maji tvar:

```text
DEBUG: <message> reason=<reason> source=<source>
```

Vyjimky nepouzivaji debug logging helper. Chytaji se v `catch` blocich a vypisuji
se pres `Exception::print()`, ktery rekurzivne vypise i inner exceptions. Tento
vypis neni vazany na `ENABLE_DEBUG` a ma tvar:

```text
(<source>) EXCEPTION: <message>
```

Instalacni a build workflow je v `docs/INSTALL.md`.

Prakticky build:

1. Otevrit `ui/ui.ino` v Arduino IDE.
2. Sketchbook location nastavit na root projektu.
3. Board: `ESP32S3 Dev Module`, esp32 core testovan s `2.0.17`.
4. Zkompilovat nebo exportovat binary.

## Runtime inicializace

`GuiManager::init()` v `libraries/engine/src/gui/gui_manager.cpp` je hlavni startovni sekvence aplikace:

1. Inicializuje Crash GUI.
2. Mountne storage pres `StorageManager`.
3. Inicializuje `DataBundleManager`.
4. Nacte `DeviceCatalog` z JSON DB.
5. Vycisti browse/session state.
6. Inicializuje `DeviceManager`.
7. Inicializuje core obrazovky pres `GuiScreenRegistry`.

Pokud storage nebo katalog selze, zobrazi se Crash obrazovka.

## Device katalog a DB.json

Katalog vlastni `DeviceCatalog`:

- `libraries/engine/src/managers/device_catalog.hpp`
- `libraries/engine/src/managers/device_catalog.cpp`

JSON parser a serializer jsou zde:

- `libraries/engine/src/devices/json_device_builder.hpp`
- `libraries/engine/src/devices/json_device_builder.cpp`

Schema v `DB.json`:

```json
{
  "version": "1.0",
  "application": "board",
  "devices": {
    "cpu_temp": {
      "uid": "cpu_temp",
      "role": "sensor",
      "type": "CPU Temp",
      "description": "...",
      "values": {
        "temp": { "value": 0.0, "unit": "C", "dtype": "float" }
      },
      "configs": {},
      "default": {
        "values": { "temp": 0.0 },
        "pins": ""
      }
    }
  }
}
```

Podporovane `role`:

- `sensor`: telemetry hodnoty, lze nahravat a grafovat,
- `actuator`: runtime control hodnoty, nezobrazuje se jako klasicky recording stream,
- `hybrid`: kombinace podle definice hodnot/configu.

Podporovane `dtype`:

- `int`
- `float`
- `double`
- `string`

`values` jsou runtime hodnoty. Kazda hodnota muze mit marker `access`:

- `access: "read"` nebo chybejici `access`: hodnota se cte pres `UPDATE`.
- `access: "write"`: hodnota se zapisuje pres `CONTROL`.

Aliasove hodnoty pro write smer jsou `out`, `output` a `control`; pro read smer
`in` a `input`. Parser je normalizuje na `read`/`write`.

`configs` jsou persistentni/konfiguracni hodnoty posilane pres `CONFIG`.
Hybridni zarizeni muze mit soucasne read values, write values i configs.

Priklad hybridniho testovaciho zarizeni:

```json
"H00": {
  "uid": "H00",
  "role": "hybrid",
  "type": "Temperature Regulator",
  "values": {
    "set_point": { "value": 25, "unit": "C", "dtype": "int", "access": "write" },
    "temp": { "value": 20, "unit": "C", "dtype": "int", "access": "read" }
  },
  "configs": {
    "speed": { "value": 2, "dtype": "int", "restrictions": { "min": 1, "max": 5, "step": 1 } }
  }
}
```

## Device model

Zakladni runtime model je `BaseDevice`:

- `libraries/engine/src/devices/base_device.hpp`
- `libraries/engine/src/devices/base_device.cpp`

Dulezite veci:

- `UID`, `Type`, `Description`, `Role`
- `Values` a `Configs`
- `DeviceParamAccess` pro rozliseni `READ`/`WRITE` runtime hodnot
- pin assignment (`assignPin`, `unassignPin`)
- sync flags:
  - `isValuesSync`
  - `isConfigsSync`
  - `isControlsSync`
  - `redrawPending`
- `requestRuntimeUpdate()` oznaci runtime hodnoty jako stale, aby dalsi sync poslal `UPDATE`.
- `syncValues()`, `syncConfigs()`, `syncControls()` volaji VSCP protocol layer.
- `usesUpdateChannel()` vraci true, pokud device ma alespon jednu readable value.
- `usesControlChannel()` vraci true, pokud device ma writable value a neni cisty sensor.

Globalni helper `syncDevice(BaseDevice*)` obaluje `device->synchronize()` do exception-safe API.

## DeviceManager a piny

Runtime manager je:

- `libraries/engine/src/managers/device_manager.hpp`
- `libraries/engine/src/managers/device_manager.cpp`

Zodpovednosti:

- drzi `PinMap` s `NUM_PINS = 18`,
- prideluje/odebira zarizeni na piny,
- explicitne inicializuje VSCP spojeni,
- pri `connect()`:
  1. zajisti `INIT`,
  2. odpoji drive prirazena zarizeni,
  3. propise vybrane piny do `BaseDevice`,
  4. posle `CONNECT` pro kazde prirazene zarizeni,
- pri runtime `resync(device)` v running rezimu:
  1. zavola `device->requestRuntimeUpdate()`,
  2. zavolá `deviceManager.sync(device)`.

Pin UI pracuje s virtualnimi piny. Stav pinu definuje:

- `libraries/engine/src/managers/pin_structure.hpp`

## VSCP protocol

Firmware implementace VSCP je v `libraries/vscp`.

Hlavni tridy/soubory:

- `libraries/vscp/src/vscp_types.*`
- `libraries/vscp/src/vscp_codec.*`
- `libraries/vscp/src/vscp_client.*`
- `libraries/vscp/src/vscp_server.*`
- `libraries/vscp/src/io/vscp_transport.*`
- `libraries/vscp/src/io/vscp_stream_transport.*`

Aktuální API verze je v `vscp::API_VERSION`, nyní `1.4`.

Wire format je URL-like query string:

```text
?type=INIT&app=board&db=1.0&api=1.4
?status=1

?type=CONNECT&id=cpu_temp&pins=1,4
?id=cpu_temp&status=1

?type=UPDATE&id=cpu_temp
?id=cpu_temp&status=1&temp=23.4
```

Podporovane typy requestu:

- `INIT`: handshake app/db/api.
- `UPDATE`: cteni runtime hodnot.
- `CONFIG`: zapis konfigurace.
- `CONTROL`: zapis runtime control hodnot pro actuator/hybrid writable values.
- `RESET`: reset zarizeni.
- `CONNECT`: pripojeni zarizeni na piny.
- `DISCONNECT`: odpojeni zarizeni.

Sketch vlastní `HardwareSerial vscpSerial`, obalí jej do `vscp::StreamTransport` a předá jeden `vscp::Client` do `DeviceManager`.
Výchozí UART parametry desky jsou v `libraries/engine/src/config.hpp`.

## GUI architektura

GUI vrstva je v `libraries/engine/src/gui`.

Hlavni soucasti:

| Soubor | Ucel |
| --- | --- |
| `gui_manager.*` | Centralni router/state manager. Volan z `ui.ino`. |
| `gui_state.hpp` | Enum vsech obrazovek. |
| `gui_router.hpp` | Rozhrani, ktere obrazovky pouzivaji pro navigaci. |
| `gui_navigation_policy.*` | Rozhoduje, kam vedou back/flow akce. |
| `gui_runtime_policy.*` | Zapina/vypina runtime polling podle obrazovky. |
| `gui_screen_registry.*` | Vlastni instance obrazovek, inicializuje/hide/render. |
| `main_menu_gui.*` | Main menu. |
| `communication_selection_gui.*` | Volba komunikace; Cable dela VSCP INIT, wireless zatim disabled. |
| `device_selection_gui.*` | Vyber zarizeni a pin assignment. |
| `signals_visualization_gui.*` | Runtime vizualizace, graf, recording, settings overlay. |
| `signals_*_panel.*` | Mensi panely pro runtime obrazovku: graf, list hodnot, toolbar, settings, feedback. |
| `data_bundle_selection_gui.*` | Databank/DataBundle prehled. |
| `file_transfer_gui.*` | Transfer Mode screen pro vystaveni SD karty pocitaci pres USB MSC. |
| `library_gui.*`, `library_editor_gui.*` | Editace device katalogu. |
| `crash_gui.*` | Fatal error obrazovka. |
| `credits_gui.hpp` | Credits. |

### Navigacni flow

Main runtime flow:

```text
MAIN_MENU
  -> COMMUNICATION_SELECTION
  -> SELECTION
  -> CONNECTION
  -> VISUALIZATION
```

Poznamky:

- `GuiNavigationPolicy::beginVisualizationFlow()` vraci `COMMUNICATION_SELECTION`.
- `COMMUNICATION_SELECTION` zatim podporuje jen Cable.
- Po cable volbe se dela VSCP `INIT`.
- `SELECTION` slouzi k vyberu device.
- `CONNECTION` slouzi k pin assignmentu.
- `VISUALIZATION` spousti runtime polling.

Databank:

```text
MAIN_MENU -> DATA_BUNDLE_SELECTION -> MAIN_MENU
VISUALIZATION -> DATA_BUNDLE_SELECTION -> VISUALIZATION
```

O navrat rozhoduje `databankReturnToVisualization` v `GuiNavigationPolicy`.

File Transfer:

```text
MAIN_MENU -> FILE_TRANSFER -> MAIN_MENU
DATA_BUNDLE_SELECTION export -> FILE_TRANSFER -> MAIN_MENU
```

`FILE_TRANSFER` ma vlastni user-action vrstvu. Otevreni obrazovky pouze zobrazi
bily preparation screen. Transfer pipeline se spusti az po potvrzeni `OK`.
Behem aktivni session je `Back` zakazane a ukonceni jde pres cervene
`End session`, ktere stopne USB MSC, remountne SD a vrati uzivatele do Main Menu.

### Runtime loop a redraw

`GuiManager::redraw()`:

1. zavola `lv_timer_handler()`,
2. pri intervalu cca 100 ms vola `deviceManager.resync(currentDevice)`,
3. pokud je stav `VISUALIZATION`, vola `SignalsVisualizationGui::drawCurrentDevice()`.

`SignalsVisualizationGui` kresli jen pri `currentDevice->getRedrawPending()`.
Runtime graf drzi delsi interni historii a zobrazuje okno o velikosti `HISTORY_CAP`.
Drag po grafu posouva zobrazeny vyrez historie. Autoscale se pocita pro aktualni vyrez.

## Runtime vizualizace

Klicove soubory:

- `signals_visualization_gui.*`: orchestrace cele obrazovky.
- `signals_chart_panel.*`: LVGL chart helper.
- `signals_list_panel.*`: seznam live hodnot a editoru.
- `signals_toolbar_panel.*`: pause/update/record/clear/settings.
- `signals_settings_panel.*`: overlay pro Databank, clear bundles, credits.
- `signals_feedback_panel.*`: alerty a confirmation dialogy.

Chovani:

- Normal running: GUI manager periodicky vola `DeviceManager::resync()`, ktery posila `UPDATE`.
- Pause: zastavi running flag. Manual update button posle jeden `UPDATE`.
- Values/Configs panel:
  - runtime obrazovka ma prepinac mezi `Values` a `Configs`,
  - read `Values` jsou live karty,
  - write `Values` jsou editory posilane pres `CONTROL` a maji oranzovy podklad,
  - `Configs` jsou editory posilane pres `CONFIG` a maji modry podklad.
- Chart:
  - float/double hodnoty se pro LVGL integer chart skaluji `x100`,
  - u grafu je label aktualniho scalingu (`Scaling 1x`, `Scaling x100`),
  - zapisy do grafu pouzivaji `lv_chart_set_value_by_id`,
  - range se prepocitava podle aktualniho okna.
- Record:
  - start zavola `DataBundleManager::startRecording(device->Type, device->UID)`,
  - nove body se ukladaji pri prichodu runtime vzorku,
  - stop zavola `DataBundleManager::saveRecording()`.

## DataBundle a Databank

Manager:

- `libraries/engine/src/managers/data_bundle_manager.hpp`
- `libraries/engine/src/managers/data_bundle_manager.cpp`
- `libraries/engine/src/managers/data_bundle_types.hpp`

GUI:

- `libraries/engine/src/gui/data_bundle_selection_gui.*`

Storage:

- Bundles jsou ukladane pod `/records/`.
- CSV format:

```csv
DeviceName;DeviceUid;SampleIndex;RuntimeMs;SignalName;Value
CPU Temp;cpu_temp;0;0;temp;23.4
CPU Temp;cpu_temp;1;105;temp;23.5
```

Zivotni cyklus:

1. `startRecording(deviceName, deviceUid)` pripravi unikatni file path.
2. `saveNewDataPoint(signalName, value)` buffruje body v RAM.
3. `saveRecording()` zapise CSV, refreshne seznam souboru a vycisti aktivni recording.
4. `getBundlePage(page)` refreshne seznam a vrati 6 preview zaznamu.
5. `DataBundleSelectionGui` zobrazi karty a preview grafy.

Pozor:

- Recording nema smysl bez prichoziho `UPDATE`; prazdny recording se neuklada.
- `RuntimeMs` je relativni cas od startu recordingu, proto nevyzaduje RTC ani NTP.
- Preview graf skaluje decimalni hodnoty `x100`, stejne jako runtime graf.

## Persistent storage

Storage abstraction:

- `libraries/engine/src/managers/storage_manager.hpp`
- `libraries/engine/src/managers/storage_manager.cpp`

Podporuje:

- SPIFFS
- SD

API:

- `storageManager().init()`
- `exists(path)`
- `ensureDirectory(path)`
- `open(path, mode)`
- `remove(path)`
- `listFiles(directory)`
- `totalBytes()`, `usedBytes()`

SPIFFS nuance:

- nema realne adresare jako SD, `ensureDirectory()` vraci true.
- `listFiles("/records/")` prochazi root a filtruje prefix.

SD layout:

- `/data/DB.json`: device katalog.
- `/data/pics/`: obrazky devices.
- `/data/config.json`: persistentni konfigurace aplikace.
- `/records/`: DataBundle CSV zaznamy.

## File Transfer / USB MSC

Transfer Mode vystavi SD kartu pocitaci jako USB Mass Storage Class disk. SPIFFS
se nikdy nevystavuje, zustava jen pro demo/debug rezim.

Klicove soubory:

- `libraries/engine/src/gui/file_transfer_gui.*`: preparation screen, stavove barvy,
  `OK` start session a `End session` ukonceni.
- `libraries/engine/src/managers/file_transfer_service.*`: stavovy automat transferu,
  SD lock/unlock a napojeni bridge.
- `libraries/engine/src/managers/file_transfer_usb_msc_bridge.*`: Arduino `USBMSC`
  backend, raw `onRead`/`onWrite` callbacky nad SD sektory.
- `libraries/engine/src/managers/storage_manager.*`: `enterTransferLock()` a
  `exitTransferLock()`.
- `libraries/expt/src/logs/logs.*`: USB CDC aware logging; behem MSC session
  `logMessage()` neotevira `Serial` a bufferuje zpravy v RAM.

Lifecycle:

1. Uživatel otevre Transfer Screen.
2. Screen ukaze preparation info; zatim se nesaha na SD ani USB MSC.
3. Po `OK` se vykresli zluty `Connection...` stav a vynuti se `lv_timer_handler()`
   + `lv_refr_now(nullptr)`.
4. `FileTransferService::start()` overi SD, nastavi `transferModeActive`, vypne
   CDC logging output a zavola `storageManager().enterTransferLock()`.
5. Storage manager odmountuje SD z HMI filesystemu.
6. `FileTransferUsbMscBridge::start()` znovu mountne SD pro raw blokovy pristup,
   nastavi `USBMSC` callbacky, zavola `usbMsc.begin(blockCount, 512)` a `USB.begin()`.
7. PC vidi HMI jako Mass Storage disk.
8. `End session` zavola `FileTransferService::stop()`: `usbMsc.mediaPresent(false)`,
   `usbMsc.end()`, SD unmount z bridge, `storageManager().exitTransferLock()`,
   obnoveni CDC log vystupu a flush buffered logu.

Backend je aktivni jen pokud build splni podminky:

- `FILE_TRANSFER_USB_MSC_ENABLED == 1`.
- Arduino ESP32 core poskytuje `USB.h` a `USBMSC.h`.
- Board je v ESP32-S3 native USB OTG/TinyUSB rezimu (`ARDUINO_USB_MODE == 0`).
- Pro serial/log channel pres stejny konektor ma zustat zapnute USB CDC on boot.

Pokud podminky nejsou splnene, bridge vrati srozumitelnou chybu a Transfer Screen
prejde do cerveneho error stavu bez pokusu predstirat USB disk.

## Emulator

Python emulator je v `emulator/`.

Hlavni soubory:

| Soubor | Ucel |
| --- | --- |
| `emulator/engine/emulator.py` | VSCP endpoint, parser, INIT/UPDATE/CONFIG/CONTROL/CONNECT/DISCONNECT. |
| `emulator/virt_runner.py` | Virtualni serial/emulator runner. |
| `emulator/real_runner.py` | Runner pro real sensor nadstavby. |
| `emulator/virt_patterns_runner.py` | Pattern/simulation runner. |
| `emulator/engine/emulator_patterns.py` | Simulacni patterny. |
| `emulator/engine/sensors/` | Sensor moduly pro real/virtual zdroje. |
| `emulator/engine/tests/` | Python testy emulatoru. |

Emulator nacita device defaults z:

1. `data/DB.json`, pokud existuje legacy root kopie
2. fallback `ui/data/DB.json`
3. fallback hardcoded katalog

Emulator respektuje `access` u values:

- `UPDATE` vraci pouze readable values.
- `CONTROL` prijima pouze writable values.
- `CONFIG` uklada config parametry pro dalsi simulaci.

Specialni testovaci device `H00` / `Temperature Regulator` simuluje realny hybridni tok:

```text
?type=UPDATE&id=H00
?id=H00&status=1&temp=22

?type=CONTROL&id=H00&set_point=35
?id=H00&status=1

?type=CONFIG&id=H00&speed=5
?id=H00&status=1
```

Pri dalsich `UPDATE` se `temp` postupne priblizuje k `set_point`. `speed` v rozsahu
`1..5` urcuje krok ve stupnich na jeden update. Stejne chovani je v
`emulator/engine/emulator.py` i `emulator/engine/emulator_patterns.py`.

Instalace:

```bash
pip install -r emulator/requirements.txt
```

Typicke spusteni:

```bash
python emulator/virt_runner.py
python emulator/real_runner.py
python emulator/virt_patterns_runner.py
```

Priklad VSCP z konzole/logu:

```text
Received: ?type=INIT&app=board&db=1.0&api=1.4
Sent: ?status=1
Received: ?type=CONNECT&id=cpu_temp&pins=1,4
Sent: ?id=cpu_temp&status=1
Received: ?type=UPDATE&id=cpu_temp
Sent: ?id=cpu_temp&status=1&temp=0.21
```

## Typicke vyvojarske scenare

### Pridat nove zarizeni

1. Upravit `storage/data/DB.json`.
2. Spustit `python storage/sync_db.py`, aby se aktualizovalo `ui/data/DB.json`, `ui/data/pics` a embedded fallback header.
3. Pridat device entry s `uid`, `role`, `type`, `values`, `configs`, `default`.
4. U write hodnot nastavit `access: "write"`, jinak zustanou read/UPDATE.
5. U hybridnich zarizeni overit, ze readable values, writable values a configs davaji smysl pro runtime UI.
6. Pokud emulator nema specialni senzor, default hodnoty se nactou automaticky z DB.
7. Otestovat selection, connection, visualization.

### Pridat novy runtime value/control editor

- Hodnoty jsou definovane v DB jako `values`.
- Konfigurace jsou v DB jako `configs`.
- Read values se vykresluji jako live cards; write values se vykresluji jako CONTROL editory.
- Runtime panel lze prepnout mezi `Values` a `Configs`.
- UI editor logika je v `SignalsVisualizationGui`:
  - `ensureControlEditor`
  - `syncControlEditorValue`
  - `handleDropdownConfigChanged`
  - `handleSliderConfigChanged`
  - `handleTextConfigSubmitted`

### Menit VSCP protokol

1. Upravit `libraries/vscp/src/vscp_types.*` a `libraries/vscp/src/vscp_codec.*`.
2. Aktualizovat `VSCP_API_VERSION`, pokud se mění wire contract.
3. Upravit `emulator/engine/emulator.py`, aby mirroroval firmware protocol.
4. Upravit `DeviceManager` nebo `BaseDevice`, pokud se meni sync semantics.
5. Otestovat INIT, CONNECT, UPDATE, CONFIG, CONTROL, DISCONNECT.

### Menit pin assignment

- UI: `device_selection_gui.*`, `connection_gui`/connection obrazovka podle konkretniho toku.
- Runtime state: `DeviceManager::PinMap`.
- Pin pravidla: `VirtualPin` v `pin_structure.hpp`.
- Protokolove pripojeni: `DeviceManager::connect()`.

### Menit runtime graf

- `signals_visualization_gui.*`: historie, autoscale, pan/drag, vyber chartable keys.
- `signals_chart_panel.*`: LVGL chart setup a serie.
- Pozor na float/double: LVGL chart pouziva integer `lv_coord_t`, projekt skaluje desetinne hodnoty `x100`.

### Menit DataBundle

- Recording flow: `SignalsVisualizationGui::handleRecordButtonClick`.
- Buffer/zapis/cteni: `DataBundleManager`.
- Databank UI: `DataBundleSelectionGui`.
- CSV schema: `DeviceName;DeviceUid;SampleIndex;RuntimeMs;SignalName;Value`.

### Menit storage backend

- Config: `libraries/engine/src/config.hpp`.
- Abstrakce: `StorageManager`.
- DB path:
  - SPIFFS: `/DB.json`
  - SD: `/data/DB.json`
- DataBundle path: `/records/`.
- USB MSC transfer: `FileTransferService` + `FileTransferUsbMscBridge`; SD musi byt
  pred vystavenim pocitaci zamcena pres `StorageManager::enterTransferLock()`.

### Menit File Transfer / USB MSC

- UI flow: `file_transfer_gui.*`.
- Transfer lifecycle: `FileTransferService`.
- Raw block backend: `FileTransferUsbMscBridge`.
- Nikdy nezapisovat do SD pres HMI storage, zatimco je aktivni MSC session.
- Logovani behem transferu musi zustat CDC-safe: pouzivat `setLoggerUsbCdcAvailable(false)`
  a po navratu `flushBufferedLogMessages()`.
- Board/build nastaveni je stejne dulezite jako kod: native USB OTG/TinyUSB,
  `USBMSC.h`, `USB.h`, USB CDC on boot.

## Debugging checklist

### Connection neprojde

- Overit, ze Communication screen zvolil Cable.
- Overit, ze upstream posila odpoved na:

```text
?type=INIT&app=board&db=1.0&api=1.4
```

- Firmware cesta: `CommunicationSelectionGui -> DeviceManager::initializeProtocolConnection -> vscp::Client::init`.

### Pin assignment selze

- Zkontrolovat `DeviceManager::PinMap`.
- Zkontrolovat, zda pin neni locked nebo obsazeny.
- Po potvrzeni ma `DeviceManager::connect()` poslat `DISCONNECT`/`CONNECT`.

### Runtime neposila UPDATE

- `GuiRuntimePolicy` musi byt ve stavu `VISUALIZATION`.
- `DeviceManager::isRunning()` musi byt true.
- `GuiManager::redraw()` vola `deviceManager.resync(currentDevice)` po intervalu.
- `resync()` musi volat `requestRuntimeUpdate()`.

### Graf se neprekresluje

- Zkontrolovat `redrawPending`.
- `BaseDevice::syncValues()` po uspesnem UPDATE nastavuje `redrawPending`.
- `SignalsVisualizationGui::drawCurrentDevice()` po vykresleni flag shodi.
- Float hodnoty musi byt skaleny do `lv_coord_t`.

### Record nevytvori bundle

- Musi prijit alespon jeden runtime vzorek mezi Start a Stop Record.
- `DataBundleManager::saveNewDataPoint()` se vola pri appendu runtime vzorku.
- `saveRecording()` odmita prazdny recording.
- Databank si pri `getBundlePage()` refreshuje soubory ze storage.

### Databank nic neukazuje

- Overit storage mount.
- Overit `/records/`.
- Overit CSV soubory pres `DataBundleManager::listAllBundles()`.
- Overit, ze `DataBundleSelectionGui::showDataBundles()` vola `updateBundles()`.

### Transfer Mode/USB MSC nejde spustit

- Overit, ze build pouziva `STORAGE_OPTION_SD`; SPIFFS se pres MSC nevystavuje.
- Overit, ze SD karta je fyzicky vlozena a mount probehne mimo transfer.
- Overit Arduino board option: ESP32-S3 native USB OTG/TinyUSB (`ARDUINO_USB_MODE == 0`).
- Overit, ze core poskytuje `USB.h` a `USBMSC.h`.
- Pokud PC nevidi disk, zkontrolovat USB kabel/port: musi jit o native USB konektor ESP32-S3, ne jen externi USB-UART bridge.
- Po `End session` musi log obsahovat remount SD z `StorageManager::exitTransferLock()`.

## Kodove konvence a rizika

- LVGL event callbacky by mely cist user data z `lv_event_get_current_target(e)`, pokud data patri konkretni UI kontrole.
- Nesahejte do third-party `libraries/lvgl`, `LovyanGFX`, `ArduinoJson`, pokud neni nutne.
- `DeviceCatalog` vlastni `BaseDevice*`; pri rebuild katalogu se stare instance mazou.
- `DeviceVisualizationSession` drzi aktivni runtime vyber; pri editaci katalogu je potreba session vycistit.
- Storage API vyzaduje absolutni cesty se `/`.
- SPIFFS a SD se chovaji jinak u adresaru; pouzivejte `StorageManager`, ne primo `SPIFFS`/`SD`.
- Behem Transfer Mode nevykonavat zadne HMI filesystem operace nad SD; kartu vlastni USB host.
- VSCP parser je case-sensitive podle configu; emulator by mel drzet stejnou semantiku jako firmware.

## Rychly index komponent

```text
ui/ui.ino
  hardware init, LVGL init, global managers, main loop

libraries/engine/src/config.hpp
  engine config, storage backend, debug flags

libraries/engine/src/engine.hpp
  umbrella include pro firmware sketch

libraries/engine/src/devices/
  BaseDevice, JSON DB parser/builder, embedded default DB

libraries/engine/src/managers/
  DeviceCatalog, DeviceManager, DeviceVisualizationSession,
  DeviceBrowserState, DataBundleManager, StorageManager,
  FileTransferService, FileTransferUsbMscBridge

libraries/engine/src/gui/
  all application screens and LVGL panel helpers

libraries/vscp/src/
  sdílený VSCP codec, client/server a transportní adaptéry

libraries/expt/src/
  logging, exceptions, splash messages

emulator/
  Python VSCP emulator and test runners

storage/data/DB.json, ui/data/DB.json
  kanonicky device catalog a firmware mirror
```

## Stav dokumentu

Tento soubor je urceny jako prakticka mapa. Pri vetsich zmenach architektury aktualizujte:

- navigacni flow,
- VSCP command list/API verzi,
- DB schema,
- DataBundle format,
- emulator workflow,
- storage backend notes.
