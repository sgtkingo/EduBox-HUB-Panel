/**
 * @file base_device.hpp
 * @brief Declaration and implementation of the runtime device model and helpers.
 *
 * This header defines the BaseDevice runtime type together with shared helper
 * functions for configuration, synchronization, connection and diagnostics.
 *
 * @copyright 2025 MTA
 * @author Ing. Jiri Konecny
 *
 */

#ifndef BASE_DEVICE_HPP
#define BASE_DEVICE_HPP

/*********************
 *      INCLUDES
 *********************/
#include "../config.hpp"
#include "../runtime_protocol_session.hpp"
#include "../exceptions/devices_exceptions.hpp" ///< Device related exceptions.
#include "../helpers.hpp"    ///< Helper functions.

#include <string>
#include <unordered_map>
#include <map>
#include <array>
#include <cstddef>
#include <vector>
#include <utility>
#include <algorithm>

#define HISTORY_CAP 10 ///< History capacity.

/**
 * @enum DeviceStatus
 * @brief Enumeration representing possible device statuses.
 *
 * - OK: Device is operating normally.
 * - ERROR: Device has encountered an error.
 * - OFFLINE: Device is offline.
 */
enum class DeviceStatus
{
    OK = 1,     ///< Device operating normally.
    ERROR = -1, ///< Device has an error.
    OFFLINE = 0 ///< Device is offline.
};

/**
 * @enum DeviceCommandsEnum
 * @brief Enumeration representing possible device protocol commands.
 *
 * - CONFIG: Configure the device.
 * - UPDATE: Read runtime values from the device.
 * - CONTROL: Send runtime control payload to the device.
 * - INIT: Initialize the device.
 * - RESET: Reset the device.
 */
enum class DeviceCommandsEnum
{
    CONFIG,
    UPDATE,
    CONTROL,
    INIT,
    RESET,
    CONNECT,
    DISCONNECT
};

/**
 * @enum enum class DeviceDataType
 * @brief Enumeration representing possible parametrs data types.
 *
 * - INT: int.
 * - DOUBLE: double.
 *  - FLOAT: float.
 * - STRING: string.
 */
enum class DeviceDataType
{
    INT,
    DOUBLE,
    FLOAT,
    STRING
};

/**
 * @enum DeviceParamAccess
 * @brief Declares whether a runtime value is read from hardware or written through CONTROL.
 */
enum class DeviceParamAccess
{
    READ,
    WRITE
};

/**
 * @enum DeviceRole
 * @brief Declares whether the runtime device behaves as a sensor, actuator, or both.
 */
enum class DeviceRole
{
    SENSOR,
    ACTUATOR,
    HYBRID
};


/**
 * @struct DeviceRestrictions
 * @brief Structure for device parameter restrictions.
 *
 * This structure can be used to define restrictions for device parameters such as minimum,
 * maximum, step size, and options (for enum types).
 */
struct DeviceRestrictions
{
    std::string Min;
    std::string Max;
    std::string Step;
    std::string Options; ///< Comma separated list of options (for enum types).
};

/**
 * @struct DeviceParam
 * @brief Structure for device parameters.
 *
 * This structure can be used to store runtime values and configuration parameters.
 */
struct DeviceParam
{
    std::string Value;                ///< Parameter value.
    std::string Unit;                 ///< Parameter unit.
    DeviceDataType DType;                   ///< Parameter data type.
    DeviceParamAccess Access = DeviceParamAccess::READ; ///< Runtime value access direction.
    int lastHistoryIndex;             ///< Last history index.
    std::string History[HISTORY_CAP]; ///< Parameter history.
    DeviceRestrictions Restrictions;  ///< Parameter restrictions.
};

/**
 * @class BaseDevice
 * @brief Abstract base class for devices.
 *
 * Defines common properties and virtual methods for device initialization,
 * configuration, synchronization, control, and diagnostics.
 */
class BaseDevice
{
protected:
    bool redrawPending = true;  ///< Flag to indicate if the device needs to be redrawn.
    bool isConfigsSync = false; ///< Flag to indicate if config state is synchronized with the real device.
    bool isValuesSync = false;  ///< Flag to indicate if runtime values are synchronized with the real device.
    bool isControlsSync = true; ///< Flag to indicate if control payload is synchronized with real device.
    bool pinConnectionActive = false; ///< True after the assigned pins are acknowledged by VSCP CONNECT.

    std::unordered_map<std::string, DeviceParam> Values;            ///< Runtime values.
    std::unordered_map<std::string, DeviceParam> Configs;           ///< Persistent configuration values.
    std::vector<std::string> ValueKeyOrder;                        ///< Stable UI order for value keys.
    std::vector<std::string> ConfigKeyOrder;                       ///< Stable UI order for config keys.
    std::vector<std::string> Pins;                                 ///< Assigned device pins.
    std::vector<std::string> PinDefinitions;                       ///< Logical device pin names from the datasheet/schema.
    std::map<std::string, std::string> PinAssignments;             ///< Logical pin name to assigned hardware pin number.
    std::string AllowedPins;                                       ///< Allowed device pins, stored as comma separated values.
    std::vector<int> AllowedPinsList;                              ///< Parsed allowed pin numbers for fast runtime lookup.
    std::string Picture = "placeholder:device";                    ///< Optional picture asset reference.
    DeviceRole Role = DeviceRole::SENSOR;                          ///< Runtime device role.

    void setAllowedPins(std::string allowedPins)
    {
        AllowedPins = std::move(allowedPins);
        AllowedPinsList.clear();

        if (AllowedPins.empty()) {
            return;
        }

        for (const auto &item : splitString(AllowedPins, ',')) {
            if (item.empty()) {
                continue;
            }
            AllowedPinsList.push_back(convertStringToType<int>(item));
        }
    }

    void setPinDefinitions(std::vector<std::string> pinDefinitions)
    {
        PinDefinitions = std::move(pinDefinitions);
        pinConnectionActive = false;
    }

    void setPicture(std::string picture)
    {
        Picture = picture.empty() ? "placeholder:device" : std::move(picture);
    }

    /**
     * @brief Set device status from protocol string.
     *
     * This function sets the device status based on the given status string.
     *
     * @param status The status string.
     */
    void setStatus(std::string status)
    {
        if (status.empty())
        {
            return;
        }

        if (status == "1")
        {
            Status = DeviceStatus::OK;
        }
        else if (status == "-1")
        {
            Status = DeviceStatus::ERROR;
        }
        else if (status == "0")
        {
            Status = DeviceStatus::OFFLINE;
        }
    }

    /**
     * @brief Set device status.
     *
     * This function sets the device status based on the given status.
     *
     * @param status The status.
     */
    void setStatus(DeviceStatus status)
    {
        Status = status;
    }


    /**
     * @brief Synchronize configuration values with the real device.
     *
     * This function sends a request to the real device to synchronize configuration values.
     */
    void syncConfigs(RuntimeProtocolSession &protocolClient)
    {
        isConfigsSync = false; // Set flag to indicate config state is not synchronized with the real device.
        redrawPending = false; // Reset redraw flag.

        vscp::Parameters configMap;
        for (const auto &pair : Configs)
        {
            configMap[vscp::String(pair.first.c_str())] = vscp::String(pair.second.Value.c_str());
        }
        debugLogMessage("BaseDevice::syncConfigs", "runtime config", "device=%s paramCount=%u", UID.c_str(), static_cast<unsigned int>(configMap.size()));
        auto response = protocolClient.config(vscp::String(UID.c_str()), configMap);
        if (response.status == vscp::Status::Error)
        {
            throw DeviceSynchronizationFailException("BaseDevice::syncConfigs", std::string(response.error.c_str()));
        }

        isConfigsSync = response.status == vscp::Status::Ok; // Set flag to indicate config state is synchronized with the real device.
        redrawPending = isConfigsSync; // Redraw after config changes are acknowledged.
    }

    /**
     * @brief Synchronize runtime values with the real device.
     *
     * This function sends a request to the real device to read runtime values.
     */
    void syncValues(RuntimeProtocolSession &protocolClient)
    {
        try
        {
            isValuesSync = false; // Set flag to indicate runtime values are not synchronized with the real device.
            redrawPending = false; // Reset redraw flag.

            auto response = protocolClient.update(vscp::String(UID.c_str()));
            if (response.status == vscp::Status::Error)
            {
                throw DeviceSynchronizationFailException("BaseDevice::syncValues", std::string(response.error.c_str()));
            }

            std::unordered_map<std::string, std::string> responseParameters;
            for (const auto &parameter : response.parameters)
            {
                responseParameters[std::string(parameter.first.c_str())] = std::string(parameter.second.c_str());
            }
            debugLogMessage("BaseDevice::syncValues", "runtime update", "device=%s paramCount=%u", UID.c_str(), static_cast<unsigned int>(responseParameters.size()));
            update(responseParameters); // Update runtime values from response parameters.

            isValuesSync = response.status == vscp::Status::Ok; // Set flag to indicate runtime values are synchronized with the real device.
            redrawPending = isValuesSync; // Redraw after runtime values are updated.
        }
        catch (...)
        {
            throw;
        }
    }



    /**
     * @brief Check if the given value meets the restrictions defined in the device parameter.
     *
     * @param value The value to check.
     * @param param The device parameter containing the restrictions.
     * @return true if the value meets the restrictions, false otherwise.
     */
    bool checkRestrictions(std::string value, const DeviceParam &param)
    {
        DeviceRestrictions restrictions = param.Restrictions;
        try
        {
            if (!restrictions.Min.empty())
            {
                double min = convertStringToType<double>(restrictions.Min);
                double val = convertStringToType<double>(value);
                debugLogMessage("BaseDevice::checkRestrictions", "math validation", "value=%s min=%s", value.c_str(), restrictions.Min.c_str());
                if (val < min)
                {
                    return false;
                }
            }

            if (!restrictions.Max.empty())
            {
                double max = convertStringToType<double>(restrictions.Max);
                double val = convertStringToType<double>(value);
                debugLogMessage("BaseDevice::checkRestrictions", "math validation", "value=%s max=%s", value.c_str(), restrictions.Max.c_str());
                if (val > max)
                {
                    return false;
                }
            }

            if (!restrictions.Options.empty())
            {
                std::vector<std::string> options = splitString(restrictions.Options, ',');
                debugLogMessage("BaseDevice::checkRestrictions", "option validation", "value=%s options=%s", value.c_str(), restrictions.Options.c_str());
                if (std::find(options.begin(), options.end(), value) == options.end())
                {
                    return false;
                }
            }
        }
        catch (const std::exception &e)
        {
            throw InvalidDataTypeException("BaseDevice::checkRestrictions", e.what());
        }

        return true;
    }

public:
    std::string UID;         ///< Unique device identifier.
    DeviceStatus Status;     ///< Device status.
    std::string Type;        ///< Device type as text.
    std::string Description; ///< Human readable device description.
    std::string LastError;   ///< Error message (if any).

    // lv_obj_t *ui_Container; ///< Pointer to the UI widgets container.
    /**
     * @brief Equality operator for comparing devices by UID.
     *
     * @param device The device to compare with.
     * @return true if the devices have the same UID, false otherwise.
     */
    bool operator==(const BaseDevice &device) const
    {
        return UID == device.UID;
    }

    /**
     * @brief Equality operator for comparing devices by UID.
     *
     * @param uid The UID to compare with.
     * @return true if the device UID matches the given UID, false otherwise.
     */
    bool operator==(const std::string uid) const
    {
        return UID == uid;
    }

    /**
     * @brief Constructs a new BaseDevice object.
     */
    BaseDevice() : UID("DummyDevice"), Status(DeviceStatus::OK)
    {
        LastError = "";

        init();
    }

    /**
     * @brief Constructs a new BaseDevice object.
     *
     * @param uid The unique device identifier.
     */
    BaseDevice(std::string uid) : UID(uid), Status(DeviceStatus::OK)
    {
        LastError = "";

        init();
    }

    /**
     * @brief Virtual destructor.
     */
    virtual ~BaseDevice()
    {
    }

    std::unordered_map<std::string, DeviceParam> getValues() const { return Values; }
    std::vector<std::string> getValuesKeys() const
    {
        if (!ValueKeyOrder.empty()) {
            return ValueKeyOrder;
        }

        std::vector<std::string> keys;
        for (const auto &pair : Values)
        {
            keys.push_back(pair.first);
        }
        return keys;
    }
    std::unordered_map<std::string, DeviceParam> getConfigs() const { return Configs; }
    std::vector<std::string> getConfigsKeys() const
    {
        if (!ConfigKeyOrder.empty()) {
            return ConfigKeyOrder;
        }

        std::vector<std::string> keys;
        for (const auto &pair : Configs)
        {
            keys.push_back(pair.first);
        }
        return keys;
    }

    /**
     * @brief Get the redraw pending status.
     *
     * @return true if redraw is pending, false otherwise.
     */
    bool getRedrawPending() const { return redrawPending; }
    // Link loss cancels unsent state from the old generation. User must change
    // settings/controls again; Run and reconnect never replay them implicitly.
    void discardPendingSynchronization() {
        isControlsSync = isConfigsSync = true;
        isValuesSync = false; redrawPending = true;
    }

    /**
     * @brief Mark live runtime values as stale so the next synchronization polls UPDATE.
     */
    void requestRuntimeUpdate()
    {
        if (usesUpdateChannel())
        {
            isValuesSync = false;
        }
    }

    /**
     * @brief Get the runtime role of the device.
     *
     * @return DeviceRole The role of the current device.
     */
    DeviceRole getRole() const { return Role; }

    /**
     * @brief Set the runtime role of the device.
     *
     * @param role New device role.
     */
    void setRole(DeviceRole role) { Role = role; }

    /**
     * @brief Get a user-friendly role label.
     *
     * @return std::string Human readable role.
     */
    std::string getRoleLabel() const
    {
        switch (Role)
        {
        case DeviceRole::ACTUATOR:
            return "Actuator";
        case DeviceRole::HYBRID:
            return "Hybrid";
        case DeviceRole::SENSOR:
        default:
            return "Sensor";
        }
    }

    /**
     * @brief Synchronize runtime control payload with the real device.
     */
    void syncControls(RuntimeProtocolSession &protocolClient)
    {
        // Consume this user intent exactly once. Timeout may mean the Board
        // executed a relative movement but its response was lost. Never retry
        // automatically on the next visualization tick; require a new edit.
        isControlsSync = true;
        redrawPending = false;

        vscp::Parameters valueMap;
        for (const auto &pair : Values)
        {
            if (pair.second.Access == DeviceParamAccess::WRITE)
            {
                valueMap[vscp::String(pair.first.c_str())] = vscp::String(pair.second.Value.c_str());
            }
        }

        auto response = protocolClient.control(vscp::String(UID.c_str()), valueMap);
        if (response.status == vscp::Status::Error)
        {
            protocolClient.stopCommunication(); // Outcome uncertain: fail safe, no automatic retry.
            throw DeviceSynchronizationFailException("BaseDevice::syncControls", std::string(response.error.c_str()));
        }

        debugLogMessage("BaseDevice::syncControls", "runtime control", "device=%s paramCount=%u", UID.c_str(), static_cast<unsigned int>(valueMap.size()));
        isControlsSync = response.status == vscp::Status::Ok;
        redrawPending = isControlsSync;
    }

    /**
     * @brief Check whether the device exposes live values.
     *
     * @return true when at least one value exists.
     */
    bool hasValues() const { return !Values.empty(); }

    bool hasReadableValues() const
    {
        for (const auto &pair : Values) {
            if (pair.second.Access == DeviceParamAccess::READ) {
                return true;
            }
        }
        return false;
    }

    bool hasWritableValues() const
    {
        for (const auto &pair : Values) {
            if (pair.second.Access == DeviceParamAccess::WRITE) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Check whether the device exposes configurable parameters.
     *
     * @return true when at least one config exists.
     */
    bool hasConfigs() const { return !Configs.empty(); }

    /**
     * @brief Check whether the device should use CONFIG channel for synchronization.
     *
     * @return true if CONFIG sync is meaningful for this device.
     */
    bool usesConfigChannel() const
    {
        return hasConfigs();
    }

    /**
     * @brief Check whether the device should use UPDATE channel for synchronization.
     *
     * @return true if UPDATE sync is meaningful for this device.
     */
    bool usesUpdateChannel() const
    {
        return hasReadableValues();
    }

    /**
     * @brief Check whether the device should use CONTROL channel for synchronization.
     *
     * @return true if CONTROL sync is meaningful for this device.
     */
    bool usesControlChannel() const
    {
        return hasWritableValues() && Role != DeviceRole::SENSOR;
    }

    /**
     * @brief Set the redraw pending status.
     *
     * @param pending The new redraw pending status.
     */
    void setRedrawPending(bool pending) { redrawPending = pending; }

    /**
     * @brief Get the device unique identifier.
     *
     * @return The device UID.
     */
    std::string getId() const { return UID; }

    /**
     * @brief Get the device name (same as UID for compatibility).
     *
     * @return The device name.
     */
    std::string getName() const { return Type + " (" + UID + ")"; }

    /**
     * @brief Get the device type name.
     *
     * @return The device type.
     */
    std::string getTypeName() const { return Type; }

    /**
     * @brief Get the device description.
     *
     * @return The device description.
     */
    std::string getDescription() const { return Description; }

    /**
     * @brief Get the device status.
     *
     * @return The device status.
     */
    DeviceStatus getStatus() const { return Status; }

    /**
     * @brief Assign a pin to the device.
     * 
     * @param pin The pin to assign.
     */
    void assignPin(std::string pin) 
    {
        if (isInVector(Pins, pin)) {
            debugLogMessage("BaseDevice::assignPin", "pin assignment", "device=%s pin=%s already assigned", UID.c_str(), pin.c_str());
            return; // Pin already assigned
        }
        if (!canAssignMorePins()) {
            debugLogMessage("BaseDevice::assignPin", "pin assignment limit", "device=%s pin=%s assigned=%u required=%u", UID.c_str(), pin.c_str(), static_cast<unsigned int>(Pins.size()), static_cast<unsigned int>(getRequiredPinCount()));
            return;
        }
        debugLogMessage("BaseDevice::assignPin", "pin assignment", "device=%s pin=%s", UID.c_str(), pin.c_str());
        pinConnectionActive = false;
        Pins.push_back(pin);
        if (!PinDefinitions.empty()) {
            for (const std::string &tag : PinDefinitions) {
                if (PinAssignments.find(tag) == PinAssignments.end()) {
                    PinAssignments[tag] = pin;
                    break;
                }
            }
        }
    }

    void assignPin(std::string pinTag, std::string pin)
    {
        if (pinTag.empty()) {
            assignPin(std::move(pin));
            return;
        }

        if (PinAssignments.find(pinTag) != PinAssignments.end()) {
            debugLogMessage("BaseDevice::assignPin", "pin assignment", "device=%s tag=%s already assigned", UID.c_str(), pinTag.c_str());
            return;
        }
        if (isInVector(Pins, pin)) {
            debugLogMessage("BaseDevice::assignPin", "pin assignment", "device=%s pin=%s already assigned", UID.c_str(), pin.c_str());
            return;
        }
        if (!canAssignMorePins()) {
            debugLogMessage("BaseDevice::assignPin", "pin assignment limit", "device=%s tag=%s pin=%s assigned=%u required=%u", UID.c_str(), pinTag.c_str(), pin.c_str(), static_cast<unsigned int>(Pins.size()), static_cast<unsigned int>(getRequiredPinCount()));
            return;
        }

        debugLogMessage("BaseDevice::assignPin", "pin assignment", "device=%s tag=%s pin=%s", UID.c_str(), pinTag.c_str(), pin.c_str());
        pinConnectionActive = false;
        PinAssignments[pinTag] = pin;
        Pins.push_back(pin);
    }

    /**
     * @brief Unassign a pin from the device.
     * 
     * @param pin The pin to unassign.
     */
    void unassignPin(std::string pin) 
    {
        auto it = std::find(Pins.begin(), Pins.end(), pin);
        if (it != Pins.end()) {
            debugLogMessage("BaseDevice::unassignPin", "pin assignment", "device=%s pin=%s", UID.c_str(), pin.c_str());
            pinConnectionActive = false;
            Pins.erase(it);
            for (auto assignmentIt = PinAssignments.begin(); assignmentIt != PinAssignments.end(); ++assignmentIt) {
                if (assignmentIt->second == pin) {
                    PinAssignments.erase(assignmentIt);
                    break;
                }
            }
        }
    }

    /**
     * @brief Get the Pins object
     * 
     * @return std::string 
     */
    std::string getPins() const 
    {
        std::string pins;
        if (!PinDefinitions.empty()) {
            for (const std::string &tag : PinDefinitions) {
                auto it = PinAssignments.find(tag);
                if (it == PinAssignments.end() || it->second.empty()) {
                    continue;
                }
                if (!pins.empty()) {
                    pins += ",";
                }
                pins += it->second;
            }
            return pins;
        }

        for (const auto &pin : Pins) {
            if (!pins.empty())
            {
                pins += ",";
            }
            pins += pin;
        }
        return pins;
    }

    size_t getAssignedPinCount() const
    {
        return Pins.size();
    }

    size_t getRequiredPinCount() const
    {
        return PinDefinitions.size();
    }

    size_t getMissingPinCount() const
    {
        const size_t requiredPins = getRequiredPinCount();
        return Pins.size() >= requiredPins ? 0 : requiredPins - Pins.size();
    }

    bool isPinAssignmentComplete() const
    {
        return getMissingPinCount() == 0;
    }

    bool isPinConnectionActive() const
    {
        return pinConnectionActive;
    }

    void setPinConnectionActive(bool active)
    {
        pinConnectionActive = active;
    }

    bool hasPartialPinAssignment() const
    {
        return !Pins.empty() && !isPinAssignmentComplete();
    }

    bool canAssignMorePins() const
    {
        return Pins.size() < getRequiredPinCount();
    }

    /**
     * @brief Get allowed pins as a list of integers.
     *
     * Empty AllowedPins means all pins are allowed.
     *
     * @return Vector of allowed pin numbers
     */
    std::vector<int> getAllowedPinsList() const
    {
        return AllowedPinsList;
    }

    /**
     * @brief Check whether a pin number is allowed for this device.
     *
     * @param pinNumber The pin number to check
     * @return True if the pin is allowed or if the allowed list is empty
     */
    bool isPinAllowed(int pinNumber) const
    {
        if (AllowedPins.empty()) {
            return true;
        }

        return std::find(AllowedPinsList.begin(), AllowedPinsList.end(), pinNumber) != AllowedPinsList.end();
    }

    std::vector<std::string> getPinDefinitions() const
    {
        return PinDefinitions;
    }

    std::string getPinTag(int pinNumber) const
    {
        const std::string pinText = std::to_string(pinNumber);
        for (const auto &assignment : PinAssignments) {
            if (assignment.second == pinText) {
                return assignment.first;
            }
        }
        return "";
    }

    std::string getNextUnassignedPinTag() const
    {
        for (const std::string &tag : PinDefinitions) {
            if (PinAssignments.find(tag) == PinAssignments.end()) {
                return tag;
            }
        }
        return "";
    }

    std::map<std::string, std::string> getPinAssignments() const
    {
        return PinAssignments;
    }

    std::string getPicture() const
    {
        return Picture;
    }

    /**
     * @brief Connect the device to its assigned pins.
     * 
     */
    bool connect(RuntimeProtocolSession &protocolClient)
    {
        std::string pins = getPins();
        if(pins.empty()) {
            throw DevicePinAssignmentException("connectDevice", "No pins assigned to device.");
        }
        if (!isPinAssignmentComplete()) {
            throw DevicePinAssignmentException("connectDevice", "Missing " + std::to_string(getMissingPinCount()) + " of " + std::to_string(getRequiredPinCount()) + " required pins.");
        }

        auto response = protocolClient.connect(vscp::String(UID.c_str()), vscp::String(pins.c_str()));
        if (response.status == vscp::Status::Error)
        {
            throw DeviceConnectionFailException("BaseDevice::connect", std::string(response.error.c_str()));
        }

        debugLogMessage("BaseDevice::connect", "protocol connect", "device=%s pins=%s", UID.c_str(), pins.c_str());
        pinConnectionActive = response.status == vscp::Status::Ok;
        return pinConnectionActive;
    }

    /**
     * @brief Disconnect the device from its assigned pins.
     * 
     */
    bool disconnect(RuntimeProtocolSession &protocolClient)
    {
        auto response = protocolClient.disconnect(vscp::String(UID.c_str()));
        if (response.status == vscp::Status::Error)
        {
            throw DeviceConnectionFailException("BaseDevice::disconnect", std::string(response.error.c_str()));
        }

        if (response.status == vscp::Status::Ok) {
            debugLogMessage("BaseDevice::disconnect", "protocol disconnect", "device=%s pins=%s", UID.c_str(), getPins().c_str());
            Pins.clear();
            PinAssignments.clear();
            pinConnectionActive = false;
        }

        return response.status == vscp::Status::Ok;
    }

    /**
     * @brief Get value from configuration.
     *
     * This function retrieves the value of a configuration parameter by key.
     *
     * @param key The key of the configuration parameter.
     * @return The value of the configuration parameter.
     */
    template <typename T>
    T getConfig(const std::string &key)
    {
        std::string value;
        if (Configs.find(key) != Configs.end())
        {
            value = Configs[key].Value;
        }
        if (value.empty())
        {
            throw ConfigurationNotFoundException("BaseDevice::getConfig", "Configuration not found for key: " + key);
        }

        try
        {
            return convertStringToType<T>(value);
        }
        catch (const std::exception &e)
        {
            throw InvalidDataTypeException("BaseDevice::getConfig", e.what());
        }
    }

    /**
     * @brief Set configuration value.
     *
     * This function sets the value of a configuration parameter by key.
     *
     * @param key The key of the configuration parameter.
     * @param value The value to set.
     */
    void setConfig(const std::string &key, const std::string &value)
    {
        if (Configs.find(key) != Configs.end())
        {
            if (!checkRestrictions(value, Configs[key]))
            {
                throw InvalidValueException("BaseDevice::setConfig", "Value " + value + " for key " + key + " does not meet restrictions.");
            }
            debugLogMessage("BaseDevice::setConfig", "runtime memory write", "device=%s key=%s value=%s", UID.c_str(), key.c_str(), value.c_str());
            Configs[key].Value = value;
        }
        else
        {
            throw ConfigurationNotFoundException("BaseDevice::setConfig", "Configuration not found for key: " + key);
        }

        isConfigsSync = false; // Set flag to indicate config state is not synchronized with the real device.
        redrawPending = true;
    }

    /**
     * @brief Get a runtime value from the device.
     *
     * This function retrieves the value of a device runtime parameter by key.
     *
     * @param key The key of the device value parameter.
     * @return The value of the device parameter.
     */
    template <typename T>
    T getValue(const std::string &key)
    {
        std::string value;
        if (Values.find(key) != Values.end())
        {
            value = Values[key].Value;
        }
        if (value.empty())
        {
            throw ValueNotFoundException("BaseDevice::getValue", "Value not found for key: " + key);
        }

        try
        {
            return convertStringToType<T>(value);
        }
        catch (const std::exception &e)
        {
            throw InvalidDataTypeException("BaseDevice::getValue", e.what());
        }
    }

    /**
     * @brief Set a runtime value.
     *
     * This function sets the value of a device runtime parameter by key.
     *
     * @param key The key of the device value parameter.
     * @param value The value to set.
     */
    void setValue(const std::string &key, const std::string &value)
    {
        if (Values.find(key) != Values.end())
        {
            if (!checkRestrictions(value, Values[key]))
            {
                throw InvalidValueException("BaseDevice::setValue", "Value " + value + " for key " + key + " does not meet restrictions.");
            }
            debugLogMessage("BaseDevice::setValue", "runtime memory write", "device=%s key=%s value=%s historyIndex=%d", UID.c_str(), key.c_str(), value.c_str(), Values[key].lastHistoryIndex);
            Values[key].Value = value;
            Values[key].History[Values[key].lastHistoryIndex++] = value;
            if (Values[key].lastHistoryIndex >= HISTORY_CAP)
            {
                Values[key].lastHistoryIndex = 0;
            }
        }
        else
        {
            throw ValueNotFoundException("BaseDevice::setValue", "Value not found for key: " + key);
        }

        if (usesControlChannel())
        {
            isControlsSync = false;
        }

        redrawPending = true;
    }

    /**
     * @brief Get units of a device value parameter.
     *
     * This function retrieves the units of a device value parameter by key.
     *
     * @param key The key of the device value parameter.
     * @return The units of the device value parameter.
     */
    std::string getValueUnits(const std::string &key) const
    {
        auto it = Values.find(key);
        if (it != Values.end())
        {
            return it->second.Unit;
        }
        return "";
    }

    /**
     * @brief Get units of a device config parameter.
     *
     * This function retrieves the units of a device config parameter by key.
     *
     * @param key The key of the device config parameter.
     * @return The units of the device config parameter.
     */
    std::string getConfigUnits(const std::string &key) const
    {
        auto it = Configs.find(key);
        if (it != Configs.end())
        {
            return it->second.Unit;
        }
        return "";
    }

    /**
     * @brief Set exception as error and change status accordingly.
     *
     * @param error The exception as error.
     */
    void setError(std::string error)
    { 
        LastError = error;
        if (!error.empty())
        {
            setStatus(DeviceStatus::ERROR);
        }
        else
        {
            setStatus(DeviceStatus::OK);
        }
    }

    /**
     * @brief Get error message.
     *
     * @return The error message.
     */
    std::string getError() const
    {
        return LastError;
    }

    void clearError() 
    {
        setError("");
    }

    /**
     * @brief Get history of a device value parameter.
     *
     * This function retrieves the history of a device value parameter by key.
     *
     * @param key The key of the device value parameter.
     * @return The history array of the device value parameter.
     */
    std::string* getHistory(const std::string &key)
    {
        //Find key in Values and return history array
        if (Values.find(key) != Values.end())
        {
            // Return the history array
            return Values[key].History;
        }

        throw ValueNotFoundException("BaseDevice::getHistory", "Value not found for key: " + key);
    }

    /**
     * @brief Clear history of all device value parameters.
     */
    void clearHistory()
    {
        for (auto &v : Values)
        {
            debugLogMessage("BaseDevice::clearHistory", "runtime memory write", "device=%s key=%s", UID.c_str(), v.first.c_str());
            for(int i=0;i<HISTORY_CAP;i++){
                v.second.History[i] = "0";
                //logMessage("Clearing history value %s for key %s\n", v.second.History[i], v.first.c_str());
            }
            v.second.lastHistoryIndex = 0;
        }
    }

    /**
     * @brief Synchronize with the real device.
     *
     * @throws Exception if synchronization fails.
     */
    virtual bool synchronize(RuntimeProtocolSession &protocolClient)
    {
        const bool syncConfigsChannel = usesConfigChannel();
        const bool syncValuesChannel = usesUpdateChannel();
        const bool syncControlChannel = usesControlChannel();
        debugLogMessage(
            "BaseDevice::synchronize",
            "runtime sync",
            "device=%s configChannel=%d updateChannel=%d controlChannel=%d",
            UID.c_str(),
            syncConfigsChannel,
            syncValuesChannel,
            syncControlChannel);

        if (!syncConfigsChannel)
        {
            isConfigsSync = true;
        }

        if (!syncValuesChannel)
        {
            isValuesSync = true;
        }

        if (!syncControlChannel)
        {
            isControlsSync = true;
        }

        if (syncConfigsChannel && !isConfigsSync)
        {
            try
            {
                syncConfigs(protocolClient);
            }
            catch (...)
            {
                throw;
            }
        }

        if (syncControlChannel && !isControlsSync)
        {
            try
            {
                syncControls(protocolClient);
            }
            catch (...)
            {
                throw;
            }
        }

        if (syncValuesChannel && !isValuesSync)
        {
            try
            {
                syncValues(protocolClient);
            }
            catch (...)
            {
                throw;
            }
        }

        return (!syncValuesChannel || isValuesSync) &&
               (!syncConfigsChannel || isConfigsSync) &&
               (!syncControlChannel || isControlsSync);
    }

    /**
     * @brief Add a configuration parameter to the device.
     *
     * @param key The key of the configuration parameter.
     * @param param The configuration parameter to add.
     */
    void addConfigParameter(const std::string &key, const DeviceParam &param)
    {
        try
        {
            Configs[key] = param;
            if (!isInVector(ConfigKeyOrder, key)) {
                ConfigKeyOrder.push_back(key);
            }
            debugLogMessage("BaseDevice::addConfigParameter", "runtime memory write", "device=%s key=%s", UID.c_str(), key.c_str());
        }
        catch (const std::exception &e)
        {
            throw InvalidConfigurationException("BaseDevice::addConfigParameter", e.what());
        }

        isConfigsSync = false; // Set flag to indicate config state is not synchronized with the real device.
    }

    /**
     * @brief Configure the device with the given configuration map.
     *
     * @param cfg The configuration map.
     * @throws Exception if configuration fails.
     */
    virtual void config(const std::unordered_map<std::string, std::string> &cfg)
    {
        if(cfg.empty())
        {
            return;
        }
        std::string value;
        try
        {
            // Parse the config string and update the device configs.
            for (auto &c : Configs)
            {
                value = cfg.find(c.first) != cfg.end() ? cfg.at(c.first) : "";
                if (!value.empty())
                {
                    if (!checkRestrictions(value, c.second))
                    {
                        throw InvalidValueException("BaseDevice::config", "Value " + value + " for key " + c.first + " does not meet restrictions.");
                    }
                    c.second.Value = value;

                    c.second.History[c.second.lastHistoryIndex++] = value;
                    debugLogMessage("BaseDevice::config", "runtime memory write", "device=%s key=%s value=%s", UID.c_str(), c.first.c_str(), value.c_str());
                    if (c.second.lastHistoryIndex >= HISTORY_CAP)
                    {
                        c.second.lastHistoryIndex = 0;
                    }

                    redrawPending = true; // Redraw after configuration values are updated.
                }
            }
        }
        catch(...)
        {
            throw;
        }
    }

    /**
     * @brief Add a value parameter to the device.
     *
     * @param key The key of the value parameter.
     * @param param The value parameter to add.
     * @throws Exception if adding the value parameter fails.
     */
    void addValueParameter(const std::string &key, const DeviceParam &param)
    {
        try
        {
            Values[key] = param;
            if (!isInVector(ValueKeyOrder, key)) {
                ValueKeyOrder.push_back(key);
            }
            debugLogMessage("BaseDevice::addValueParameter", "runtime memory write", "device=%s key=%s", UID.c_str(), key.c_str());
        }
        catch (const std::exception &e)
        {
            throw InvalidValueException("BaseDevice::addValueParameter", new Exception(e));
        }

        isValuesSync = false; // Set flag to indicate runtime values are not synchronized with the real device.
    }

    /**
     * @brief Applies runtime control payload to device values.
     *
     * @param ctl The control map containing new device values.
     */
    virtual void control(const std::unordered_map<std::string, std::string> &ctl)
    {
        update(ctl);
    }

    /**
     * @brief Update the device with new runtime data.
     *
     * @param upd The update map containing new device data.
     * @throws Exception if update fails.
     */
    virtual void update(const std::unordered_map<std::string, std::string> &upd)
    {
        if(upd.empty())
        {
            return;
        }

        std::string value;
        try
        {
            // Parse the update string and update the device values.
            for (auto &c : Values)
            {
                value = upd.find(c.first) != upd.end() ? upd.at(c.first) : "";
                if (!value.empty())
                {
                    if (!checkRestrictions(value, c.second))
                    {
                        throw InvalidValueException("BaseDevice::update", "Value " + value + " for key " + c.first + " does not meet restrictions.");
                    }
                    c.second.Value = value;
                    c.second.History[c.second.lastHistoryIndex++] = value;
                    debugLogMessage("BaseDevice::update", "runtime memory write", "device=%s key=%s value=%s", UID.c_str(), c.first.c_str(), value.c_str());
                    if (c.second.lastHistoryIndex >= HISTORY_CAP)
                    {
                        c.second.lastHistoryIndex = 0;
                    }

                    redrawPending = true; // Redraw after runtime values are updated.
                }
            }

            std::string status = upd.find("status") != upd.end() ? upd.at("status") : "-1";
        }
        catch(...)
        {
            throw;
        }
    }

    /**
     * @brief Print device information.
     */
    void print() const
    {
        debugLogMessage(DEBUG_VERBOSE_ALL, "BaseDevice::print", "device details", "Device UID: %s\n", UID.c_str());
        debugLogMessage(DEBUG_VERBOSE_ALL, "BaseDevice::print", "device details", "\tDevice Type: %s\n", Type.c_str());
        debugLogMessage(DEBUG_VERBOSE_ALL, "BaseDevice::print", "device details", "\tDevice Description: %s\n", Description.c_str());
        debugLogMessage(DEBUG_VERBOSE_ALL, "BaseDevice::print", "device details", "\tDevice Status: %d\n", Status);
        debugLogMessage(DEBUG_VERBOSE_ALL, "BaseDevice::print", "device details", "\tDevice Error: %s\n", getError().c_str());
        debugLogMessage(DEBUG_VERBOSE_ALL, "BaseDevice::print", "device details", "\tDevice Picture: %s\n", Picture.c_str());
        debugLogMessage(DEBUG_VERBOSE_ALL, "BaseDevice::print", "device details", "\tDevice Configurations:\n");
        for (auto &c : Configs)
        {
            debugLogMessage(DEBUG_VERBOSE_ALL, "BaseDevice::print", "device details", "\t\t%s: %s %s\n", c.first.c_str(), c.second.Value.c_str(), c.second.Unit.c_str());
        }
        debugLogMessage(DEBUG_VERBOSE_ALL, "BaseDevice::print", "device details", "\tDevice Values:\n");
        for (auto &v : Values)
        {
            debugLogMessage(DEBUG_VERBOSE_ALL, "BaseDevice::print", "device details", "\t\t%s: %s %s\n", v.first.c_str(), v.second.Value.c_str(), v.second.Unit.c_str());
        }
        debugLogMessage(DEBUG_VERBOSE_ALL, "BaseDevice::print", "device details", "\tDevice Pins: %s\n", getPins().c_str());
        if (!PinDefinitions.empty()) {
            debugLogMessage(DEBUG_VERBOSE_ALL, "BaseDevice::print", "device details", "\tDevice Pin Map:\n");
            for (const std::string &tag : PinDefinitions) {
                auto it = PinAssignments.find(tag);
                debugLogMessage(DEBUG_VERBOSE_ALL, "BaseDevice::print", "device details", "\t\t%s: %s\n", tag.c_str(), it == PinAssignments.end() ? "-" : it->second.c_str());
            }
        }
        debugLogMessage(DEBUG_VERBOSE_ALL, "BaseDevice::print", "device details", "**************************************\n");
    }

    /**
     * @brief Initialize the device.
     *
     * @throws Exception if initialization fails.
     */
    virtual void init()
    {
        redrawPending = true; // Redraw after initialization.
        isConfigsSync = true; // Config state starts synchronized by default.
        isValuesSync = false; // Runtime values must be fetched from the real device.
        isControlsSync = true; // Control payload is synchronized until the user changes it locally.

        clearError();
        debugLogMessage("BaseDevice::init", "device init", "device=%s", UID.c_str());
    };
};

/**************************************************************************/
// CREATE FUNCTIONS
/**************************************************************************/

/**
 * @brief Factory function template to create a device of type T.
 *
 * This function creates a device object of type T (which must have a constructor taking an int)
 * and returns a pointer to the newly created object. If initialization fails, it logs the error,
 * deletes the partially constructed object, and rethrows the exception.
 *
 * @tparam T The device type, which must be derived from BaseDevice.
 * @param uid The unique device identifier.
 * @return T* Pointer to the newly created device.
 * @throws DeviceInitializationFailException if device initialization fails.
 */
template <typename T>
T *createDevice(std::string uid)
{
    static_assert(std::is_base_of<BaseDevice, T>::value, "T must be derived from BaseDevice");

    T *device = nullptr;
    try
    {
        device = new T(uid);
    }
    catch (const Exception &ex)
    {
        ex.print();
        const std::string detail = ex.flush(0);
        delete device;
        throw DeviceInitializationFailException("createDevice", "Error during device initialization. " + detail);
    }
    catch (const std::exception &ex)
    {
        Exception("createDevice", ex.what()).print();
        delete device;
        throw DeviceInitializationFailException("createDevice", "Error during device initialization.", new Exception(ex));
    }

    debugLogMessage("createDevice", "device init", "device=%s type=%s created successfully", device->UID.c_str(), device->Type.c_str());
    return device;
}

/**************************************************************************/
// GENERAL FUNCTIONS
/**************************************************************************/

/**
 * @brief Configure the device using the given configuration string.
 *
 * This function applies the configuration specified in the string to the provided device.
 * The configuration string should follow the expected device-specific format (e.g. "Resolution=12").
 *
 * @param device Pointer to the device to be configured.
 * @param config The configuration string.
 * @throws Exceptions should be internally resolved to prevent program from crash.
 */
bool configDevice(BaseDevice *device, const std::string &config);

/**
 * @brief Update the device with new runtime data.
 *
 * This function updates the device parameters based on the provided update string.
 * The update string should follow the expected format (e.g. "Value=3.3").
 *
 * @param device Pointer to the device to be updated.
 * @param update The update string containing new device data.
 * @throws Exceptions should be internally resolved to prevent program from crash.
 */
bool updateDevice(BaseDevice *device, const std::string &update);

/**
 * @brief Applies control payload to the device.
 *
 * @param device Pointer to the device to control.
 * @param control The control string containing runtime output values.
 * @return true if the payload was accepted locally.
 */
bool controlDevice(BaseDevice *device, const std::string &control);

/**
 * @brief Print detailed information about the device.
 *
 * This function prints device details by calling the device print method,
 * which includes both basic info and any additional device-specific data.
 *
 * @param device Pointer to the device whose information is to be printed.
 * @throws Exceptions should be internally resolved to prevent program from crash.
 */
void printDevice(BaseDevice *device);

/**
 * @brief Synchronize the device with the real device.
 *
 * This function synchronizes the device with the real hardware by calling the device synchronize() method.
 *
 * @param device Pointer to the device to be synchronized.
 * @throws Exceptions should be internally resolved to prevent program from crash.
 */
bool syncDevice(BaseDevice *device, RuntimeProtocolSession &protocolClient);

/**
 * @brief Initialize the device.
 *
 * This function initializes the device by calling the device init() method.
 *
 * @param device Pointer to the device to be initialized.
 * @throws Exceptions should be internally resolved to prevent program from crash.
 */
bool initDevice(BaseDevice *device);

/**
 * @brief Connect the device to the specified pins.
 * 
 * @param device Pointer to the device to be connected.
 * @throws Exceptions should be internally resolved to prevent program from crash.
 */
bool connectDevice(BaseDevice *device, RuntimeProtocolSession &protocolClient);

/**
 * @brief Disconnect the device from its current pins.
 * 
 * @param device Pointer to the device to be disconnected.
 * @throws Exceptions should be internally resolved to prevent program from crash.
 */
bool disconnectDevice(BaseDevice *device, RuntimeProtocolSession &protocolClient);

#endif // BASE_DEVICE_HPP
