/**
 * @file device_manager.cpp
 * @brief Definition of the manager 
 * 
 * This source defines the manager functions and implementations.
 * 
 * @copyright 2025 MTA
 * @author Ing. Jiri Konecny, Ondřej Wrubel
 */

/*********************
 *      INCLUDES
 *********************/

#include <sstream>
#include <utility>
#include <map>
#include "device_manager.hpp"
#include "helpers.hpp"
#include "expt.hpp"
#include "../config.hpp"

DeviceManager::DeviceManager(DeviceCatalog &catalog, vscp::Client &protocolClient)
    : catalog(catalog), protocolClient(protocolClient) {
}

DeviceManager::~DeviceManager() {
}

std::vector<BaseDevice *> DeviceManager::collectAssignedDevicesFromPinMap() const
{
    std::vector<BaseDevice *> uniqueDevices;

    for (const auto &virtualPin : PinMap) {
        if (!virtualPin.isAssigned()) {
            continue;
        }

        if (!std::count(uniqueDevices.begin(), uniqueDevices.end(), virtualPin.assignedDevice)) {
            uniqueDevices.push_back(virtualPin.assignedDevice);
        }
    }

    return uniqueDevices;
}

std::vector<BaseDevice *> DeviceManager::filterCompleteDevices(const std::vector<BaseDevice *> &devices) const
{
    std::vector<BaseDevice *> completeDevices;
    for (BaseDevice *device : devices) {
        if (device && device->isPinAssignmentComplete()) {
            completeDevices.push_back(device);
        }
    }
    return completeDevices;
}

std::vector<BaseDevice *> DeviceManager::filterConnectedDevices(const std::vector<BaseDevice *> &devices) const
{
    std::vector<BaseDevice *> connectedDevices;
    for (BaseDevice *device : devices) {
        if (device && device->isPinAssignmentComplete() && device->isPinConnectionActive()) {
            connectedDevices.push_back(device);
        }
    }
    return connectedDevices;
}

void DeviceManager::resetPinState(size_t pinIndex)
{
    if (!isValidPinIndex(pinIndex)) {
        debugLogMessage("DeviceManager::resetPinState", "pin index invalid", "pinIndex=%u", static_cast<unsigned int>(pinIndex));
        return;
    }

    detachDeviceFromPin(pinIndex);
    PinMap[pinIndex].pinNumber = static_cast<int>(pinIndex);
    PinMap[pinIndex].locked = false;
    PinMap[pinIndex].lockReason = PinLockReason::NONE;
    PinMap[pinIndex].lockDescription = "";
    PinMap[pinIndex].state = PinState::AVAILABLE;
}

void DeviceManager::applyAssignedPinsToDevices() const
{
    for (const auto &virtualPin : PinMap) {
        if (virtualPin.isAssigned()) {
            virtualPin.assignedDevice->assignPin(std::to_string(virtualPin.pinNumber));
        }
    }
}

void DeviceManager::disconnectAssignedDevices(const std::vector<BaseDevice *> &devices)
{
    for (BaseDevice *device : devices) {
        disconnectDevice(device, protocolClient);
    }
}

bool DeviceManager::connectAssignedDevices(const std::vector<BaseDevice *> &devices)
{
    bool result = true;
    for (BaseDevice *device : devices) {
        result &= connectAssignedDevice(device);
    }
    return result;
}

bool DeviceManager::isValidPinIndex(size_t pinIndex) const
{
    return pinIndex < NUM_PINS;
}

VirtualPin *DeviceManager::getPinState(size_t pinIndex)
{
    if (!isValidPinIndex(pinIndex)) {
        return nullptr;
    }

    return &PinMap[pinIndex];
}

const VirtualPin *DeviceManager::getPinState(size_t pinIndex) const
{
    if (!isValidPinIndex(pinIndex)) {
        return nullptr;
    }

    return &PinMap[pinIndex];
}

bool DeviceManager::detachDeviceFromPin(size_t pinIndex)
{
    VirtualPin *pin = getPinState(pinIndex);
    if (!pin) {
        debugLogMessage("DeviceManager::detachDeviceFromPin", "pin index invalid", "pinIndex=%u", static_cast<unsigned int>(pinIndex));
        return false;
    }

    BaseDevice *device = pin->assignedDevice;
    if (device) {
        debugLogMessage("DeviceManager::detachDeviceFromPin", "pin assignment", "device=%s pin=%d", device->UID.c_str(), pin->pinNumber);
        device->unassignPin(std::to_string(pin->pinNumber));
    }

    pin->unassignDevice();
    return true;
}

bool DeviceManager::init() {
    if(initialized)
    {
        debugLogMessage("DeviceManager::init", "init reset", "manager already initialized; erasing pin map");
        erase();
    }

    initialized = false;
    Status = ManagerStatus::ERROR;
    if (!catalog.isInitialized()) {
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "DeviceManager::init", "dependency not initialized", "device catalog must be initialized first");
        throw DeviceInitializationFailException("DeviceManager::init", "Device catalog must be initialized before runtime manager init.");
    }

    Status = ManagerStatus::READY;
    resetPinMap();
    debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "DeviceManager::init", "init", "initialization done pinCount=%d", NUM_PINS);
    return initialized = true;
}

bool DeviceManager::ensureProtocolInitialized()
{
    if (protocolClient.isInitialized()) {
        return true;
    }

    return initializeProtocolConnection();
}

bool DeviceManager::initializeProtocolConnection()
{
    debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "DeviceManager::initializeProtocolConnection", "protocol init", "initializing protocol on demand app=%s db=%s", catalog.getApplication().c_str(), catalog.getVersion().c_str());

    vscp::ResponseStatus response;
    response.error = "Protocol initialization failed";
    for (size_t i = 0; i < DeviceManager::MAX_INIT_ATTEMPTS; i++)
    {
        debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "DeviceManager::initializeProtocolConnection", "protocol init", "attempt=%u", static_cast<unsigned int>(i + 1));
        response = protocolClient.init(vscp::String(catalog.getApplication().c_str()), vscp::String(catalog.getVersion().c_str()));
        if (response.status == vscp::Status::Ok)
        {
            debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "DeviceManager::initializeProtocolConnection", "protocol init", "initialized successfully");
            return true;
        }

        debugLogMessage(DEBUG_VERBOSE_ERRORS, "DeviceManager::initializeProtocolConnection", "protocol init failed", "retrying error=%s", response.error.c_str());
        delay_ms(500);
    }

    debugLogMessage(DEBUG_VERBOSE_ERRORS, "DeviceManager::initializeProtocolConnection", "protocol init failed", "failed permanently error=%s", response.error.c_str());
    return false;
}

BaseDevice* DeviceManager::getDevice(std::string uid) {
    return catalog.getDevice(uid);
}

void DeviceManager::addDevice(BaseDevice* device) {
    (void)device;
}

bool DeviceManager::sync(std::string id) {
    BaseDevice* device = getDevice(id);
    if (device) return sync(device);

    return false;
}

bool DeviceManager::sync(BaseDevice *device) {
    return syncDevice(device, protocolClient);
}

void DeviceManager::print(std::string uid) {
    BaseDevice* device = getDevice(uid);
    printDevice(device);
}

void DeviceManager::print(BaseDevice *device) {
    printDevice(device);
}

bool DeviceManager::resync(BaseDevice *device) 
{
    if(!isRunning()) return false;
    if(device) {
        if (device->usesUpdateChannel()) {
            debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "DeviceManager::resync", "runtime sync", "requesting update device=%s", device->UID.c_str());
        }
        device->requestRuntimeUpdate();
    }
    return sync(device);
}

bool DeviceManager::connect() 
{
    debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "DeviceManager::connect", "pin connection", "connect requested");
    if (!ensureProtocolInitialized()) {
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "DeviceManager::connect", "protocol init failed", "cannot connect assigned devices");
        return false;
    }

    const std::vector<BaseDevice *> assignedDevices = collectAssignedDevicesFromPinMap();
    const std::vector<BaseDevice *> completeDevices = filterCompleteDevices(assignedDevices);
    debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "DeviceManager::connect", "pin connection", "assigned device count=%u complete=%u", static_cast<unsigned int>(assignedDevices.size()), static_cast<unsigned int>(completeDevices.size()));
    if (completeDevices.empty()) {
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "DeviceManager::connect", "pin connection failed", "no fully assigned devices");
        return false;
    }

    return connectAssignedDevices(completeDevices);
}

bool DeviceManager::connectAssignedDevice(BaseDevice *device)
{
    if (!device) {
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "DeviceManager::connectAssignedDevice", "device invalid", "device is null");
        return false;
    }

    if (!ensureProtocolInitialized()) {
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "DeviceManager::connectAssignedDevice", "protocol init failed", "device=%s", device->UID.c_str());
        return false;
    }

    bool hasPinMapAssignment = false;
    for (const auto &pin : PinMap) {
        if (pin.assignedDevice == device) {
            hasPinMapAssignment = true;
            break;
        }
    }

    if (!hasPinMapAssignment) {
        debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "DeviceManager::connectAssignedDevice", "pin connection failed", "device=%s has no assigned pins", device->UID.c_str());
        return false;
    }

    if (!device->isPinAssignmentComplete()) {
        debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "DeviceManager::connectAssignedDevice", "pin connection failed", "device=%s missing=%u required=%u", device->UID.c_str(), static_cast<unsigned int>(device->getMissingPinCount()), static_cast<unsigned int>(device->getRequiredPinCount()));
        return false;
    }

    debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "DeviceManager::connectAssignedDevice", "protocol connect", "device=%s pins=%s", device->UID.c_str(), device->getPins().c_str());
    const bool connected = connectDevice(device, protocolClient);
    device->setPinConnectionActive(connected);
    debugLogMessage(connected ? DEBUG_VERBOSE_IMPORTANT : DEBUG_VERBOSE_ERRORS, "DeviceManager::connectAssignedDevice", connected ? "protocol connect" : "protocol connect failed", "device=%s connected=%d", device->UID.c_str(), connected);
    return connected;
}

void DeviceManager::erase() {
    resetPinMap();
}

/////////////////////////
// Pin management
/////////////////////////

void DeviceManager::resetPinMap() {
    for (size_t i = 0; i < NUM_PINS; ++i) {
        resetPinState(i);
    }
}

bool DeviceManager::assignDeviceToPin(BaseDevice* device, int activePin) {
    VirtualPin *pin = getPinState(static_cast<size_t>(activePin));
    if (!pin) {
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "DeviceManager::assignDeviceToPin", "pin index invalid", "pin=%d", activePin);
        return false;
    }
    if (!device) {
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "DeviceManager::assignDeviceToPin", "device invalid", "pin=%d", activePin);
        return false;
    }
    if (!device->isPinAllowed(pin->pinNumber)) {
        debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "DeviceManager::assignDeviceToPin", "pin not allowed", "device=%s pin=%d", device->UID.c_str(), pin->pinNumber);
        return false;
    }
    if (!device->canAssignMorePins()) {
        debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "DeviceManager::assignDeviceToPin", "pin assignment limit", "device=%s pin=%d assigned=%u required=%u", device->UID.c_str(), activePin, static_cast<unsigned int>(device->getAssignedPinCount()), static_cast<unsigned int>(device->getRequiredPinCount()));
        return false;
    }

    const bool assigned = pin->assignDevice(device);
    debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "DeviceManager::assignDeviceToPin", "pin assignment", "device=%s pin=%d result=%d", device ? device->UID.c_str() : "-", activePin, assigned);
    return assigned;
}

bool DeviceManager::unassignDeviceFromPin(int activePin) {
    const bool result = detachDeviceFromPin(static_cast<size_t>(activePin));
    debugLogMessage("DeviceManager::unassignDeviceFromPin", "pin assignment", "pin=%d result=%d", activePin, result);
    return result;
}

bool DeviceManager::unassignAllPinsForDevice(BaseDevice *device)
{
    if (!device) {
        return false;
    }

    bool changed = false;
    for (size_t i = 0; i < NUM_PINS; ++i) {
        if (PinMap[i].assignedDevice == device) {
            unassignDeviceFromPin(static_cast<int>(i));
            changed = true;
        }
    }

    return changed;
}

bool DeviceManager::disconnectAndUnassignDevice(BaseDevice *device)
{
    if (!device) {
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "DeviceManager::disconnectAndUnassignDevice", "device invalid", "device is null");
        return false;
    }

    bool hasPinMapAssignment = false;
    for (const auto &pin : PinMap) {
        if (pin.assignedDevice == device) {
            hasPinMapAssignment = true;
            break;
        }
    }

    if (!hasPinMapAssignment) {
        debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "DeviceManager::disconnectAndUnassignDevice", "pin assignment", "device=%s has no assigned pins", device->UID.c_str());
        return false;
    }

    if (!ensureProtocolInitialized()) {
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "DeviceManager::disconnectAndUnassignDevice", "protocol init failed", "device=%s", device->UID.c_str());
        return false;
    }

    debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "DeviceManager::disconnectAndUnassignDevice", "protocol disconnect", "device=%s pins=%s", device->UID.c_str(), device->getPins().c_str());
    if (!disconnectDevice(device, protocolClient)) {
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "DeviceManager::disconnectAndUnassignDevice", "protocol disconnect failed", "device=%s error=%s", device->UID.c_str(), device->getError().c_str());
        return false;
    }

    return unassignAllPinsForDevice(device);
}

BaseDevice* DeviceManager::getAssignedDevice(size_t pinIndex) const {
    const VirtualPin *pin = getPinState(pinIndex);
    return pin ? pin->assignedDevice : nullptr;
}

int DeviceManager::getPinNumber(size_t pinIndex) const {
    const VirtualPin *pin = getPinState(pinIndex);
    return pin ? pin->pinNumber : -1;
}

bool DeviceManager::isPinAvailable(size_t pinIndex) const {
    const VirtualPin *pin = getPinState(pinIndex);
    return pin ? pin->isAvailable() : false;
}

bool DeviceManager::isPinLocked(size_t pinIndex) const {
    const VirtualPin *pin = getPinState(pinIndex);
    return pin ? pin->isLocked() : false;
}

bool DeviceManager::hasAssignedDevices() const
{
    for (const auto &pin : PinMap) {
        if (pin.isAssigned()) {
            return true;
        }
    }
    return false;
}

bool DeviceManager::hasCompleteAssignedDevices() const
{
    for (BaseDevice *device : collectAssignedDevicesFromPinMap()) {
        if (device && device->isPinAssignmentComplete()) {
            return true;
        }
    }
    return false;
}

bool DeviceManager::hasConnectedAssignedDevices() const
{
    for (BaseDevice *device : collectAssignedDevicesFromPinMap()) {
        if (device && device->isPinAssignmentComplete() && device->isPinConnectionActive()) {
            return true;
        }
    }
    return false;
}

std::vector<BaseDevice *> DeviceManager::getCompleteAssignedDevices() const
{
    return filterCompleteDevices(collectAssignedDevicesFromPinMap());
}

std::vector<BaseDevice *> DeviceManager::getConnectedAssignedDevices() const
{
    return filterConnectedDevices(collectAssignedDevicesFromPinMap());
}

std::vector<BaseDevice *> DeviceManager::getIncompleteAssignedDevices() const
{
    std::vector<BaseDevice *> incompleteDevices;
    for (BaseDevice *device : collectAssignedDevicesFromPinMap()) {
        if (device && !device->isPinAssignmentComplete()) {
            incompleteDevices.push_back(device);
        }
    }
    return incompleteDevices;
}

bool DeviceManager::reconnectDevice(BaseDevice *device)
{
    if (!device) return false;
    protocolClient.invalidateInitialization();
    if (!ensureProtocolInitialized()) return false;
    const bool connected = connectDevice(device, protocolClient);
    device->setPinConnectionActive(connected);
    return connected;
}

void DeviceManager::serviceProtocolLink(bool allowPing)
{
    protocolClient.serviceLink(ALLOW_PING_INTERRUPT != 0 && allowPing);
}

void DeviceManager::endProtocolSession()
{
    setRunning(false);
    const bool sent = protocolClient.bye();
    for (auto *device : catalog.getDevices()) {
        if (device) device->setPinConnectionActive(false);
    }
    if (!sent) {
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "DeviceManager::endProtocolSession",
                        "protocol bye failed", "watchdog stopped locally");
    }
}

bool DeviceManager::reconnectProtocolLink()
{
    const auto connectedDevices = getConnectedAssignedDevices();
    protocolClient.invalidateInitialization();
    if (!ensureProtocolInitialized()) return false;
    for (auto *device : connectedDevices) {
        if (!connectDevice(device, protocolClient)) {
            protocolClient.stopCommunication();
            return false;
        }
    }
    // Selection may have no connected device yet: successful INIT restores the link.
    return protocolClient.completeLinkReconnect();
}
