#pragma once

#include "vscp_client.hpp"
#include <cstdint>

// Panel-specific session policy. The injected, shared VSCP client only implements
// protocol transactions; it knows nothing about polling, failure limits or GUI.
class RuntimeProtocolSession {
    vscp::Client& client;
    uint8_t failures = 0;
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
    explicit RuntimeProtocolSession(vscp::Client& protocolClient) : client(protocolClient) {}
    bool connectionLost() const { return lost; }
    uint8_t consecutiveFailures() const { return failures; }
    bool isInitialized() const { return !initRequired && client.isInitialized(); }
    void invalidateInitialization() { initRequired = true; }
    const char* apiVersion() const { return client.apiVersion(); }

    vscp::ResponseStatus init(const vscp::String& application = "", const vscp::String& database = "") {
        auto response = client.init(application, database);
        initRequired = response.status != vscp::Status::Ok;
        return response;
    }
    vscp::ResponseStatus connect(const vscp::String& uid, const vscp::String& pins) {
        if (!isInitialized()) return stopped("Protocol not initialized");
        auto response = client.connect(uid, pins);
        if (response.status == vscp::Status::Ok) {
            failures = 0;
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
