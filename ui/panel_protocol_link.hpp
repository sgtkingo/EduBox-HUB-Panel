#pragma once
#include <protocol_link_control.hpp>
#include <selected_protocol_transport.hpp>
#include <edubox_ble.hpp>
#include <cstdio>

// Physical composition only. VSCP/DeviceManager and GUI depend on the interface.
class PanelProtocolLink : public ProtocolLinkControl {
    vscp::Client& client_;
    vscp::StreamTransport& uart_;
    SelectedProtocolTransport& selected_;
    edubox::ble::Channel& channel_;
    edubox::ble::Transport& transport_;
    edubox::ble::Central& central_;
    bool wireless_ = false;
    void selectWireless() {
        client_.closeSession();
        wireless_ = true;
        selected_.select(transport_);
        client_.setSequenceEnabled(true); client_.setTimeout(10000);
    }
public:
    PanelProtocolLink(vscp::Client& client, vscp::StreamTransport& uart, SelectedProtocolTransport& selected,
                      edubox::ble::Channel& channel, edubox::ble::Transport& transport, edubox::ble::Central& central)
        : client_(client), uart_(uart), selected_(selected), channel_(channel), transport_(transport), central_(central) {}
    void selectCable() override {
        client_.closeSession(); wireless_ = false;
        central_.stop(); uart_.clearInput(); selected_.select(uart_);
        client_.setSequenceEnabled(false); client_.setTimeout(vscp::DEFAULT_TIMEOUT_MS);
    }
    void scanWireless() override { selectWireless(); central_.scan(); }
    bool connectWireless(size_t index, uint32_t pin) override { selectWireless(); return central_.select(index, pin); }
    bool connectRemembered() override { selectWireless(); return central_.connectSaved(); }
    void stopWireless() override { central_.stop(); }
    void forgetWireless() override { client_.closeSession(); central_.forget(); }
    bool wirelessSelected() const override { return wireless_; }
    bool service() {
        // Latch physical loss once, before Client::poll / any GUI exchange.
        if (!channel_.takeLoss()) return false;
        if (!wireless_) return false;
        client_.closeSession(); return true;
    }
    ProtocolLinkInfo info() const override {
        const auto source = central_.snapshot();
        ProtocolLinkInfo result;
        result.state = static_cast<ProtocolLinkState>(source.state);
        result.count = source.count; result.mtu = source.mtu;
        std::snprintf(result.rememberedAddress, sizeof(result.rememberedAddress), "%s", source.savedAddress);
        std::snprintf(result.error, sizeof(result.error), "%s", source.error);
        for (size_t i = 0; i < result.count; ++i) {
            std::snprintf(result.peers[i].address, sizeof(result.peers[i].address), "%s", source.peers[i].address);
            std::snprintf(result.peers[i].name, sizeof(result.peers[i].name), "%s", source.peers[i].name);
            result.peers[i].rssi = source.peers[i].rssi;
        }
        return result;
    }
};
