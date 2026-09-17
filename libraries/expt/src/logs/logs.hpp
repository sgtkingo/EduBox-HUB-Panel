/**
 * @file logs.h
 * @brief Header file for logging functions.
 * 
 * @copyright 2024 MTA
 * @author 
 * Ing. Jiri Konecny
 */

 #ifndef LOGS_H
 #define LOGS_H
 
 /*********************
 *      INCLUDES
 *********************/

 #include <string>
 #include <cstdarg>
 #include <cstdio>
 #include "../config.hpp"  ///< Configuration file inclusion


 /**
  * @brief Builds a formatted message string.
  * 
  * @param format The format string (like in printf).
  * @param ... Additional arguments for formatting.
  * @return std::string The formatted message.
  */
 std::string buildMessage(const char *format, ...);
 
 /**
  * @brief Logs a formatted message to the appropriate output (Serial for Arduino, stdout for PC).
  * 
  * This function supports variadic arguments similar to `printf()`.
  * 
    * @param format The format string (like in printf).
    * @param ... Additional arguments for formatting.
 */
 void logMessage(const char *format, ...);

 enum DebugVerboseLevel {
     DEBUG_VERBOSE_ERRORS = 1,
     DEBUG_VERBOSE_IMPORTANT = 2,
     DEBUG_VERBOSE_ALL = 3
 };

 /**
  * @brief Logs a formatted debug message when ENABLE_DEBUG is enabled.
  *
  * Output format: [DEBUG][<source>]: <message> reason=<reason>
  */
 void debugLogMessage(int level, const char *source, const char *reason, const char *format, ...);
 void debugLogMessage(const char *source, const char *reason, const char *format, ...);

 /**
  * @brief Enables or disables USB CDC/Serial log output.
  *
  * When disabled, Arduino log output is buffered in RAM and logMessage() will not
  * attempt to initialize or write to Serial. Use this during USB MSC transfer mode
  * where the same physical USB port is temporarily owned by the storage bridge.
  */
 void setLoggerUsbCdcAvailable(bool available);

 /**
  * @brief Flushes buffered log messages after USB CDC/Serial is available again.
  */
 void flushBufferedLogMessages();

/**
  * @brief Initializes the logger system.
  * 
  * This function sets up the logging environment, such as initializing Serial communication for Arduino.
  */
 void initLogger(unsigned int baudrate = UART0_BAUDRATE, unsigned int timeout = UART0_TIMEOUT);

 
 #endif // LOGS_H
