#pragma once
#include <io/vscp_transport.hpp>
// Selection is owned by the same main-loop context as vscp::Client.
// The caller must close the old session BEFORE selecting a different endpoint.
class SelectedProtocolTransport : public vscp::Transport {
    vscp::Transport* selected_;
public:
    explicit SelectedProtocolTransport(vscp::Transport& initial) : selected_(&initial) {}
    void select(vscp::Transport& transport) { selected_ = &transport; }
    bool isAvailable() const override { return selected_->isAvailable(); }
protected:
    vscp::ReadStatus readLineImpl(vscp::String& message) override { return selected_->readLine(message); }
    bool writeLineImpl(const vscp::String& message) override { return selected_->writeLine(message); }
};
