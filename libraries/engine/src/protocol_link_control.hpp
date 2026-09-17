#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

// GUI-facing, hardware-independent link operations. No NimBLE or LVGL here.
enum class ProtocolLinkState : uint8_t { Off, Idle, Scanning, Connecting, Securing, Ready, Retry, Error };
struct WirelessPeerInfo { char address[18]{}, name[40]{}; int rssi = 0; };
struct ProtocolLinkInfo {
    ProtocolLinkState state = ProtocolLinkState::Off;
    std::array<WirelessPeerInfo, 8> peers{};
    size_t count = 0;
    char rememberedAddress[18]{}, error[96]{};
    uint16_t mtu = 23;
};
class ProtocolLinkControl {
public:
    virtual ~ProtocolLinkControl() = default;
    virtual void selectCable() = 0;
    virtual void scanWireless() = 0;
    virtual bool connectWireless(size_t index, uint32_t pin) = 0;
    virtual bool connectRemembered() = 0;
    virtual void stopWireless() = 0;
    virtual void forgetWireless() = 0;
    virtual ProtocolLinkInfo info() const = 0;
    virtual bool wirelessSelected() const = 0;
};
