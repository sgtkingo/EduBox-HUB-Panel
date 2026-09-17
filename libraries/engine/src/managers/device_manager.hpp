/**
 * @file device_manager.hpp
 * @brief Declaration of the manager
 *
 * This header defines the manager class for managing devices.
 *
 * @copyright 2025 MTA
 * @author Ing. Jiri Konecny, Ondřej Wrubel
 */
#ifndef DEVICE_MANAGER_HPP
#define DEVICE_MANAGER_HPP

#define NUM_PINS 18

#include <vector>
#include <cstddef>
#include <string>
#include <array>
#include "expt.hpp"

#include "../devices/base_device.hpp"
#include "device_catalog.hpp"
#include "pin_structure.hpp"
#include "../protocol_link_control.hpp"

/**
 * @enum ManagerStatus
 * @brief Enumeration representing possible manager statuses.
 *
 * - STOPPED: Manager has stopped.
 * - RUNNING: Manager is running.
 * - ERROR: Manager has encountered an error.
 */
enum class ManagerStatus
{
    READY = 0,  ///< Manager is ready.
    STOPPED = 1, ///< Manager has stopped.
    RUNNING = 2, ///< Manager is running.
    ERROR = -1  ///< Manager has encountered an error.
};


/**
 * @class DeviceManager
 * @brief Runtime manager for pin assignments, protocol access, and connected devices.
 *
 * Provides methods for synchronizing devices and assigning them to runtime pins.
 * The persistent catalog is owned by DeviceCatalog and the active visualization
 * session is owned separately by DeviceVisualizationSession.
 */
class DeviceManager {
private:
    DeviceCatalog &catalog;                        ///< Shared device catalog loaded during boot.
    RuntimeProtocolSession protocolClient;         ///< Panel session policy over the injected shared VSCP client.
    ProtocolLinkControl* linkControl = nullptr;
    std::array<VirtualPin, NUM_PINS> PinMap;     ///< Mapping of pins to devices.

    bool initialized = false;                 ///< Initialization state flag
    ManagerStatus Status = ManagerStatus::STOPPED; ///< Current status of the manager

    std::vector<BaseDevice *> collectAssignedDevicesFromPinMap() const;
    std::vector<BaseDevice *> filterCompleteDevices(const std::vector<BaseDevice *> &devices) const;
    std::vector<BaseDevice *> filterConnectedDevices(const std::vector<BaseDevice *> &devices) const;
    void resetPinState(size_t pinIndex);
    void applyAssignedPinsToDevices() const;
    void disconnectAssignedDevices(const std::vector<BaseDevice *> &devices);
    bool connectAssignedDevices(const std::vector<BaseDevice *> &devices);
    bool isValidPinIndex(size_t pinIndex) const;
    VirtualPin *getPinState(size_t pinIndex);
    const VirtualPin *getPinState(size_t pinIndex) const;
    bool detachDeviceFromPin(size_t pinIndex);

public:
    const static uint8_t MAX_INIT_ATTEMPTS = 5; ///< Maximum initialization attempts
    /**
     * @brief Construct the runtime manager over a shared device catalog.
     */
    DeviceManager(DeviceCatalog &catalog, vscp::Client &protocolClient);

    /**
     * @brief Destructor
     */
    ~DeviceManager();

    /**
     * @brief Initialize runtime/session state after the device catalog is ready.
     */
    bool init();

    /**
     * @brief Check if the manager has been initialized
     * @return True if initialized, false otherwise
     */
    bool isInitialized() const { return initialized; }

    /**
     * @brief Check if the manager is currently running
     * @return True if running, false otherwise
     */
    bool isRunning(){ return Status == ManagerStatus::RUNNING && !protocolClient.connectionLost(); }
    bool hasLostConnection() const { return protocolClient.connectionLost(); }
    void serviceProtocolLink(bool allowPing);
    bool reconnectDevice(BaseDevice *device);
    bool reconnectProtocolLink();
    void endProtocolSession();
    void setProtocolLinkControl(ProtocolLinkControl& control) { linkControl = &control; }
    ProtocolLinkControl* getProtocolLinkControl() const { return linkControl; }
    void notifyProtocolTransportDisconnected();

    /**
     * @brief Set the running status of the manager
     * @param running True to set status to RUNNING, false to set to STOPPED
     */
    void setRunning(bool running) { Status = running ? ManagerStatus::RUNNING : ManagerStatus::STOPPED; }

    /**
     * @brief Check if any device requires a redraw.
     * @return True if any device has `redrawPending` flag set, false otherwise.
     */
    bool isRedrawPending() const {
        for (const auto* device : catalog.getDevices()) {
            if (device && device->getRedrawPending()) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Get a device by its unique ID.
     * @param uid Unique identifier string
     * @return Pointer to the device, or `nullptr` if not found.
     */
    BaseDevice* getDevice(std::string uid);

    /**
     * @brief Add a device to the manager.
     * @param device Pointer to the device to add.
     */
    void addDevice(BaseDevice* device);

    /**
     * @brief Reset the pin map, unassigning all devices from pins.
     */
    void resetPinMap();

    /**
     * @brief Synchronize a device by ID.
     * @param id Unique identifier string
     */
    bool sync(std::string id);

    /**
     * @brief Synchronize a device through this manager's VSCP client.
     * @param device Device to synchronize.
     */
    bool sync(BaseDevice *device);

    /**
     * @brief Print information about a device by UID.
     * @param uid Unique identifier string
     */
    void print(std::string uid);

    /**
     * @brief Print information about the current device.
     */
    void print(BaseDevice *device);

    /**
     * @brief Resynchronize a specific device.
     */
    bool resync(BaseDevice *device);

    /**
     * @brief Initialize protocol lazily when the user reaches the connection step.
     * @return True if protocol is initialized or already ready
     */
    bool ensureProtocolInitialized();

    /**
     * @brief Send an explicit VSCP INIT handshake for the current catalog metadata.
     * @return True if the remote endpoint acknowledges the INIT request.
     */
    bool initializeProtocolConnection();

    /**
     * @brief Connect assigned devices to pins (bulk operation).
     */
    bool connect();

    /**
     * @brief Connect one fully assigned device to its selected pins through VSCP.
     * @param device Device selected on the pin connection screen.
     * @return True when VSCP CONNECT acknowledged the assignment.
     */
    bool connectAssignedDevice(BaseDevice *device);

    /**
     * @brief Erase all devices and pin assignments.
     */
    void erase();

    // --- Pin mapping ---

    /**
     * @brief Assign a device to the currently active pin.
     * @param device Pointer to the device to assign.
     */
    bool assignDeviceToPin(BaseDevice* device, int activePin);

    /**
     * @brief Unassign the device from the currently active pin.
     */
    bool unassignDeviceFromPin(int activePin);

    /**
     * @brief Unassign all pins used by a device in the current session.
     * @param device Pointer to the device to unassign.
     * @return True if at least one pin was unassigned
     */
    bool unassignAllPinsForDevice(BaseDevice *device);

    /**
     * @brief Disconnect a device through VSCP and update local pin state only after acknowledgement.
     * @param device Pointer to the device to disconnect.
     * @return True when the protocol disconnect succeeded and local pins were cleared.
     */
    bool disconnectAndUnassignDevice(BaseDevice *device);

    /**
     * @brief Get the device assigned to a specific pin.
     * @param pinIndex Index of the pin (0-based)
     * @return Pointer to the assigned device, or `nullptr` if none.
     */
    BaseDevice* getAssignedDevice(size_t pinIndex) const;

    /**
     * @brief Get the GPIO pin number for a specific pin index
     * @param pinIndex Index of the pin (0-based)
     * @return GPIO pin number, or -1 if invalid index
     */
    int getPinNumber(size_t pinIndex) const;

    /**
     * @brief Check if a pin index is available for assignment
     * @param pinIndex Index of the pin (0-based)
     * @return True if pin is available, false if occupied or invalid
     */
    bool isPinAvailable(size_t pinIndex) const;

    /**
     * @brief Check if a pin index is locked
     * @param pinIndex Index of the pin (0-based)
     * @return True if pin is locked, false if available or invalid
     */
    bool isPinLocked(size_t pinIndex) const;

    /**
     * @brief Check if any device is assigned to any pin.
     * @return True if there is at least one assigned device.
     */
    bool hasAssignedDevices() const;

    /**
     * @brief Check if at least one assigned device has all required pins.
     */
    bool hasCompleteAssignedDevices() const;

    /**
     * @brief Check if at least one assigned device has acknowledged pin connection.
     */
    bool hasConnectedAssignedDevices() const;

    /**
     * @brief Collect assigned devices that have all required pins.
     */
    std::vector<BaseDevice *> getCompleteAssignedDevices() const;

    /**
     * @brief Collect assigned devices that have all required pins and VSCP CONNECT ack.
     */
    std::vector<BaseDevice *> getConnectedAssignedDevices() const;

    /**
     * @brief Collect assigned devices that are missing required pins.
     */
    std::vector<BaseDevice *> getIncompleteAssignedDevices() const;

    /**
     * @brief Collect currently assigned devices from the pin map.
     * @return Unique assigned devices in pin-map order.
     */
    std::vector<BaseDevice *> getAssignedDevices() const { return collectAssignedDevicesFromPinMap(); }

    /**
     * @brief Get read-only access to the device catalog.
     * @return Const reference to the vector of device pointers.
     */
    const std::vector<BaseDevice*>& getDevices() const { return catalog.getDevices(); }

    /**
     * @brief Get read-only access to the pin map
     * @return Const reference to the array of pin assignments
     */
    const std::array<VirtualPin, NUM_PINS>& getPinMap() const { return PinMap; }

};

#endif // DEVICE_MANAGER_HPP 
