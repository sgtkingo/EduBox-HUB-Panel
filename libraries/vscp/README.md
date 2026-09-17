# VSCP client/server library

The library implements **Virtual Sensors Communication Protocol** API `1.4`.
It is not the event-based Very Simple Control Protocol.

## Components

- `vscp_codec.*`: shared URL-like message parser and serializer;
- `src/config.hpp`: compile-time protocol and platform defaults;
- `src/io/vscp_transport.hpp`: transport interface;
- `src/io/vscp_stream_transport.*`: non-blocking Arduino `Stream` adapter;
- `src/io/vscp_iostream_transport.*`: blocking `std::istream`/`std::ostream` adapter;
- `src/io/vscp_stdio_transport.*`: blocking C `FILE*` adapter;
- `vscp_client.*`: synchronous request client for an HMI/controller;
- `vscp_server.*`: non-blocking responder routed through command handlers.

Include all public components with `#include <vscp.hpp>`.

Configuration defaults live in `src/config.hpp` and can be overridden with
compiler definitions such as `-DMAX_PROTOCOL_REQUEST_SIZE=2048`.

## Environments

Arduino builds are detected through the `ARDUINO` macro and expose
`vscp::StreamTransport`. Desktop builds default to `STDIO_H_ENV` and expose
both `vscp::IostreamTransport` and `vscp::StdioTransport`. The desktop adapters
perform blocking line reads, which is appropriate for console applications;
an event loop should run them on a dedicated input thread.

```cpp
vscp::IostreamTransport cppTransport(std::cin, std::cout);
vscp::StdioTransport cTransport(stdin, stdout);
```

Define `VSCP_ENABLE_IOSTREAM=0` or `VSCP_ENABLE_STDIO=0` to omit either desktop
adapter. Defining both `ARDUINO_H_ENV` and `STDIO_H_ENV` is rejected.

## Transport diagnostics

`PROTOCOL_VERBOSE` controls diagnostics compiled into the common transport:

- `0`: logging disabled;
- `1`: framing and write errors;
- `2`: errors plus complete `[VSCP][RX]` and `[VSCP][TX]` frames.

A logger is explicitly injected so diagnostic output cannot silently corrupt the
protocol channel. Keep the log output and protocol output on different streams.

```cpp
// ESP32: VSCP runs on protocolUart, diagnostics use the USB console.
vscp::StreamLogSink debugLog(Serial);
vscp::StreamTransport transport(
    protocolUart, vscp::MAX_MESSAGE_SIZE, &debugLog);

// Desktop C++ streams.
vscp::IostreamLogSink desktopLog(std::clog);
vscp::IostreamTransport desktopTransport(
    std::cin, std::cout, vscp::MAX_MESSAGE_SIZE, &desktopLog);

// Desktop C streams.
vscp::StdioLogSink stdioLog(stderr);
vscp::StdioTransport stdioTransport(
    stdin, stdout, vscp::MAX_MESSAGE_SIZE, &stdioLog);
```

The sink can also be changed at runtime with `Transport::setLogSink()`.

The Panel injects `PanelVscpLogSink`, which forwards diagnostics to its logger
only with `ENABLE_DEBUG=1`, `DEBUG_VERBOSE_LEVEL=3` and `PROTOCOL_VERBOSE=2`.
All Panel diagnostics start with `[DEBUG][<SOURCE>]:`; exceptions use level 1.
Carriage returns and newlines in payloads become spaces so each diagnostic stays
on one prefixed line; protocol question marks are retained. Frames retain their `?`
and parameter contents, including invalid incoming lines that fail parsing.
These are printable frames after transport normalization, not a byte dump;
the application logger limits each output line to 255 characters.
With a USB-to-UART0 bridge, keep **USB CDC On Boot disabled** (CLI:
`CDCOnBoot=default`) and open the bridge port in Serial Monitor. RX/TX traces
are enabled even when logger and protocol share UART0. The Board also receives
these diagnostic lines: if it treats them as protocol requests, its extra error
responses can interfere with INIT or cause a missing response UID. For hardware
with a separate native USB connection, `CDCOnBoot=cdc` moves Serial logging to
that USB console and keeps diagnostics off the protocol UART0.
Response parameter order is irrelevant: `?api=1.4&status=1` and
`?status=1&api=1.4` both parse successfully.

Before dispatch, the common transport removes bytes outside printable ASCII
(`32..126`) and trims surrounding whitespace on both RX and TX. The Arduino
stream adapter also emits a separator newline and flushes each complete frame,
matching the framing behavior of the original UART messenger.
