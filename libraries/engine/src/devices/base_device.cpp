/**
 * @file base_device.cpp
 * @brief Implementation of generic device management functions.
 * 
 * This file implements device helper functions, including the factory function for creating
 * device instances. The factory function dynamically allocates a device, calls its
 * initialization method, and returns a pointer. If initialization fails, it logs the error,
 * cleans up, and rethrows the exception.
 * 
 * @copyright 2024 MTA
 * @author Ing. Jiri Konecny
 * 
 */

#include "base_device.hpp"

/*Global functions*/


/*Create functions*/
//implemented in header file as generic function

/*General functions*/

bool configDevice(BaseDevice *device, const std::string &config) {
    if(device == nullptr) {
        debugLogMessage("configDevice", "device pointer invalid", "device is null");
        return false;
    }
    device->clearError();

    try {
        debugLogMessage("configDevice", "runtime config", "device=%s payload=%s", device->UID.c_str(), config.c_str());
        std::unordered_map<std::string, std::string> params = parseParamsFromString(config, CASE_SENSITIVE);
        device->config(params);
        return true;
    } catch (const Exception &ex) {
        ex.print();
        device->setError(ex.flush(0));
        return false;
    }
    catch (const std::exception &e)
    {
        std::string msg = buildMessage("Standard exception during synchronization: %s\n", e.what());
        Exception("configDevice", msg).print();
        device->setError(msg);
        return false;
    }
    catch(...)
    {
        std::string msg = "Unknown exception during synchronization!\n";
        Exception("configDevice", msg).print();
        device->setError(msg);
        return false;
    }
}


bool updateDevice(BaseDevice *device, const std::string &update) {
    if(device == nullptr) {
        debugLogMessage("updateDevice", "device pointer invalid", "device is null");
        return false;
    }
    device->clearError();

    try {
        debugLogMessage("updateDevice", "runtime update", "device=%s payload=%s", device->UID.c_str(), update.c_str());
        std::unordered_map<std::string, std::string> params = parseParamsFromString(update, CASE_SENSITIVE);
        device->update(params);
        return true;   
    } catch (const Exception &ex) {
        ex.print();
        device->setError(ex.flush(0));
        return false;
    }
    catch (const std::exception &e)
    {
        std::string msg = buildMessage("Standard exception during synchronization: %s\n", e.what());
        Exception("updateDevice", msg).print();
        device->setError(msg);
        return false;
    }
    catch(...)
    {
        std::string msg = "Unknown exception during synchronization!\n";
        Exception("updateDevice", msg).print();
        device->setError(msg);
        return false;
    }
}

bool controlDevice(BaseDevice *device, const std::string &control) {
    if(device == nullptr) {
        debugLogMessage("controlDevice", "device pointer invalid", "device is null");
        return false;
    }
    device->clearError();

    try {
        debugLogMessage("controlDevice", "runtime control", "device=%s payload=%s", device->UID.c_str(), control.c_str());
        std::unordered_map<std::string, std::string> params = parseParamsFromString(control, CASE_SENSITIVE);
        device->control(params);
        return true;
    } catch (const Exception &ex) {
        ex.print();
        device->setError(ex.flush(0));
        return false;
    }
    catch (const std::exception &e)
    {
        std::string msg = buildMessage("Standard exception during device control: %s\n", e.what());
        Exception("controlDevice", msg).print();
        device->setError(msg);
        return false;
    }
    catch(...)
    {
        std::string msg = "Unknown exception during device control!\n";
        Exception("controlDevice", msg).print();
        device->setError(msg);
        return false;
    }
}

void printDevice(BaseDevice *device) {
    if(device == nullptr) {
        debugLogMessage("printDevice", "device pointer invalid", "device is null");
        return;
    }
    device->clearError();
    
    try {
        device->print();
    } catch (const Exception &ex) {
        ex.print();
        device->setError(ex.flush(0));
        return;
    }
    catch (const std::exception &e)
    {
        std::string msg = buildMessage("Standard exception during synchronization: %s\n", e.what());
        Exception("printDevice", msg).print();
        device->setError(msg);
        return;
    }
    catch(...)
    {
        std::string msg = "Unknown exception during synchronization!\n";
        Exception("printDevice", msg).print();
        device->setError(msg);
        return;
    }
}

bool syncDevice(BaseDevice *device, RuntimeProtocolSession &protocolClient) {
    if(device == nullptr) {
        debugLogMessage("syncDevice", "device pointer invalid", "device is null");
        return false;
    }
    device->clearError();

    try {
        debugLogMessage("syncDevice", "runtime sync", "device=%s", device->UID.c_str());
        return device->synchronize(protocolClient);
    } catch (const Exception &ex) {
        ex.print();
        device->setError(ex.flush(0));
        return false;
    }
    catch (const std::exception &e)
    {
        std::string msg = buildMessage("Standard exception during synchronization: %s\n", e.what());
        Exception("syncDevice", msg).print();
        device->setError(msg);
        return false;
    }
    catch(...)
    {
        std::string msg = "Unknown exception during synchronization!\n";
        Exception("syncDevice", msg).print();
        device->setError(msg);
        return false;
    }
}

bool initDevice(BaseDevice *device) {
    if(device == nullptr) {
        debugLogMessage("initDevice", "device pointer invalid", "device is null");
        return false;
    }
    device->clearError();

    try {
        debugLogMessage("initDevice", "device init", "device=%s", device->UID.c_str());
        device->init();
        return true;
    } catch (const Exception &ex) {
        ex.print();
        device->setError(ex.flush(0));
        return false;
    }
    catch (const std::exception &e)
    {
        std::string msg = buildMessage("Standard exception during synchronization: %s\n", e.what());
        Exception("initDevice", msg).print();
        device->setError(msg);
        return false;
    }
    catch(...)
    {
        std::string msg = "Unknown exception during synchronization!\n";
        Exception("initDevice", msg).print();
        device->setError(msg);
        return false;
    }
}

bool connectDevice(BaseDevice *device, RuntimeProtocolSession &protocolClient) {
    if(device == nullptr) {
        debugLogMessage("connectDevice", "device pointer invalid", "device is null");
        return false;
    }
    device->clearError();

    if(device->getPins().empty()) {
        debugLogMessage("connectDevice", "pin assignment", "device=%s has no pins assigned", device->UID.c_str());
        return false; // No pins assigned, nothing to connect
    }

    try {
        debugLogMessage("connectDevice", "device connect", "device=%s pins=%s", device->UID.c_str(), device->getPins().c_str());
        return device->connect(protocolClient);
    } catch (const Exception &ex) {
        ex.print();
        device->setError(ex.flush(0));
        return false;
    }
    catch (const std::exception &e)
    {
        std::string msg = buildMessage("Standard exception during synchronization: %s\n", e.what());
        Exception("connectDevice", msg).print();
        device->setError(msg);
        return false;
    }
    catch(...)
    {
        std::string msg = "Unknown exception during synchronization!\n";
        Exception("connectDevice", msg).print();
        device->setError(msg);
        return false;
    }
}

bool disconnectDevice(BaseDevice *device, RuntimeProtocolSession &protocolClient) {
    if(device == nullptr) {
        debugLogMessage("disconnectDevice", "device pointer invalid", "device is null");
        return false;
    }
    device->clearError();

    if(device->getPins().empty()) {
        debugLogMessage("disconnectDevice", "pin assignment", "device=%s has no pins assigned", device->UID.c_str());
        return true; // No pins assigned, nothing to disconnect
    }

    try {
        debugLogMessage("disconnectDevice", "device disconnect", "device=%s pins=%s", device->UID.c_str(), device->getPins().c_str());
        return device->disconnect(protocolClient);
    } catch (const Exception &ex) {
        ex.print();
        device->setError(ex.flush(0));
        return false;
    }
    catch (const std::exception &e)
    {
        std::string msg = buildMessage("Standard exception during synchronization: %s\n", e.what());
        Exception("disconnectDevice", msg).print();
        device->setError(msg);
        return false;
    }
    catch(...)
    {
        std::string msg = "Unknown exception during synchronization!\n";
        Exception("disconnectDevice", msg).print();
        device->setError(msg);
        return false;
    }
}


