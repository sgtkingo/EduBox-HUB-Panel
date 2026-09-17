#pragma once

#include "vscp_client.hpp"
#include <cstdint>

// Panel-specific session policy. The injected, shared VSCP client only implements
// protocol transactions; it knows nothing about polling, failure limits or GUI.
class RuntimeProtocolSession {
    static uint32_t defaultClock() { return static_cast<uint32_t>(vscp::detail::monotonicMilliseconds()); }
    using Clock = uint32_t (*)();
    vscp::Client& client;
    Clock clock;
    uint8_t failures = 0;
    uint8_t pingFailures = 0;
    uint32_t lastPingMs = 0;
    bool monitoring = false;
    bool lost = false;
    bool initRequired = false;

    vscp::ResponseStatus stopped(const char *message) const {
        vscp::ResponseStatus response;
        response.error = message;
        return response;
    }
    template<class Request>
    vscp::ResponseStatus runtimeRequest(Request request) {
        if (lost) return stopped("DISCONNECT: communication stopped");
        auto response = isInitialized() ? request() : stopped("Protocol not initialized");
        if (response.status == vscp::Status::Ok) failures = 0;
        else if (++failures >= 5) {
            lost = true;
            response.error = vscp::String("DISCONNECT: ") + response.error;
        }
        return response;
    }
public:
    explicit RuntimeProtocolSession(vscp::Client& protocolClient, Clock timeSource = defaultClock)
        : client(protocolClient), clock(timeSource) {}
    bool connectionLost() const { return lost; }
    uint8_t consecutiveFailures() const { return failures; }
    uint8_t consecutivePingFailures() const { return pingFailures; }
    // Run only from the UART owner (main loop), never from an interrupt/timer callback.
    // INIT arms monitoring, including while visualization is paused or not open.
    void serviceLink(bool allowPing) {
        if (!allowPing) {
            // Start a fresh 3-second interval on Pause; don't carry retries from Run.
            pingFailures = 0;
            lastPingMs = clock();
            return;
        }
        if (!monitoring || !isInitialized() || lost) return;
        const uint32_t interval = pingFailures ? 100 : 3000;
        if (static_cast<uint32_t>(clock() - lastPingMs) < interval) return;
        const auto response = client.ping();
        lastPingMs = clock(); // Retry spacing starts after the response/timeout.
        if (response.status == vscp::Status::Ok) pingFailures = 0;
        else if (++pingFailures >= 6) lost = true; // Initial probe + five retries.
    }
    bool isInitialized() const { return !initRequired && client.isInitialized(); }
    bool completeLinkReconnect() {
        if (!isInitialized()) return false;
        failures = 0;
        pingFailures = 0;
        lastPingMs = clock();
        lost = false;
        return true;
    }
    void stopCommunication() { lost = true; }
    void invalidateInitialization() { initRequired = true; monitoring = false; }
    const char* apiVersion() const { return client.apiVersion(); }

    vscp::ResponseStatus init(const vscp::String& application = "", const vscp::String& database = "") {
        auto response = client.init(application, database);
        initRequired = response.status != vscp::Status::Ok;
        monitoring = !initRequired;
        if (monitoring) {
            lastPingMs = clock();
            pingFailures = 0;
        }
        return response;
    }
    vscp::ResponseStatus connect(const vscp::String& uid, const vscp::String& pins) {
        if (!isInitialized()) return stopped("Protocol not initialized");
        auto response = client.connect(uid, pins);
        if (response.status == vscp::Status::Ok) {
            failures = 0;
            pingFailures = 0;
            lastPingMs = clock();
            lost = false;
        }
        return response;
    }
    vscp::ResponseStatus disconnect(const vscp::String& uid) { return client.disconnect(uid); }
    vscp::ResponseStatus reset(const vscp::String& uid) { return client.reset(uid); }
    vscp::ResponseStatus update(const vscp::String& uid) {
        return runtimeRequest([&] { return client.update(uid); });
    }
    vscp::ResponseStatus config(const vscp::String& uid, const vscp::Parameters& parameters) {
        return runtimeRequest([&] { return client.config(uid, parameters); });
    }
    vscp::ResponseStatus control(const vscp::String& uid, const vscp::Parameters& parameters) {
        return runtimeRequest([&] { return client.control(uid, parameters); });
    }
};
