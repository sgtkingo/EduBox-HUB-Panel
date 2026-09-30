#include "runtime_protocol_session.hpp"
#include "selected_protocol_transport.hpp"
#include <cassert>
#include <deque>
#include <iostream>
class Wire : public vscp::Transport {
public:
    bool available = true, writable = true;
    int writes = 0, controls = 0, byes = 0;
    std::deque<vscp::String> incoming;
    bool isAvailable() const override { return available; }
protected:
    bool writeLineImpl(const vscp::String& line) override {
        if (!writable) return false;
        ++writes;
        vscp::Request request; vscp::String error;
        assert(vscp::Codec::parseRequest(line, request, error));
        if (request.command == vscp::Command::Bye) { ++byes; return true; }
        if (request.command == vscp::Command::Control) ++controls;
        auto response = vscp::Response::ok();
        if (request.has("seq")) response.parameters["seq"] = request.value("seq");
        if (request.has("id")) response.parameters["id"] = request.value("id");
        incoming.push_back(vscp::Codec::buildResponse(response));
        return true;
    }
    vscp::ReadStatus readLineImpl(vscp::String& line) override {
        if (incoming.empty()) return vscp::ReadStatus::NoData;
        line = incoming.front(); incoming.pop_front(); return vscp::ReadStatus::Message;
    }
};
int main() {
    Wire uart, ble;
    SelectedProtocolTransport selected(uart);
    vscp::Client client(selected, 5);
    RuntimeProtocolSession session(client);
    assert(session.init().status == vscp::Status::Ok);
    assert(session.connect("A02", "15").status == vscp::Status::Ok);
    assert(session.bye() && uart.byes == 1);
    const int oldWrites = uart.writes;
    client.closeSession(); selected.select(ble); client.setSequenceEnabled(true);
    assert(session.init().status == vscp::Status::Ok);
    assert(session.connect("A02", "15").status == vscp::Status::Ok);
    assert(session.control("A02", {{"state", "1"}}).status == vscp::Status::Ok);
    assert(ble.controls == 1 && uart.writes == oldWrites);
    ble.available = false;
    client.poll(); session.notifyTransportDisconnected();
    assert(session.connectionLost() && !session.isInitialized());
    const int writes = ble.writes;
    session.control("A02", {}); session.config("A02", {}); session.update("A02");
    assert(ble.writes == writes);
    ble.available = true; // Physical reconnect is not logical restoration.
    assert(session.connectionLost());
    assert(session.init().status == vscp::Status::Ok && session.connectionLost());
    session.control("A02", {}); assert(ble.controls == 1);
    assert(session.connect("A02", "15").status == vscp::Status::Ok);
    assert(!session.connectionLost() && ble.controls == 1); // No control replay.
    ble.writable = false;
    assert(!session.bye()); // Stop locally even when BYE cannot be written.
    assert(!session.connectionLost() && !session.isInitialized());
    const int stoppedWrites = ble.writes; session.control("A02", {});
    assert(ble.writes == stoppedWrites);
    selected.select(uart); client.closeSession(); client.setSequenceEnabled(false);
    assert(session.init().status == vscp::Status::Ok);
    assert(uart.writes > oldWrites && ble.controls == 1);
    session.stopCommunication(); // An uncertain control must request a main-loop shutdown.
    assert(session.connectionLost() && session.serviceStopRequest());
    assert(!session.serviceStopRequest() && uart.byes == 2 && !client.isInitialized());
    const int afterStop = uart.writes;
    session.control("A02", {}); assert(uart.writes == afterStop);
    std::cout << "PASS Panel physical loss, explicit restore and UART/BLE isolation\n";
}
