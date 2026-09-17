/**
 * @file exceptions.cpp
 * @brief Definitions of the Exception class for error handling.
 * 
 * This source file defines the Exception functions and implementations.
 * 
 * @copyright 2025 MTA
 * @author 
 * Ing. Jiri Konecny
 */

 #include "exceptions.hpp"

Exception::Exception(std::exception ex)
{
    Code = ErrorCode::NOT_DEFINED_ERROR;
    Name = "Unknown Exception";
    Message = ex.what();
    Source = "std::exception";
    innerException = nullptr;
}

Exception::Exception(Exception* ex)
{
    Code = ErrorCode::NOT_DEFINED_ERROR;
    Name = "Unknown Exception";
    Message = "Unknown error";
    Source = "Unknown source";
    innerException = ex;
}

Exception::Exception(const std::string &source, Exception* inner)
{
    Code = ErrorCode::NOT_DEFINED_ERROR;
    Name = "Unknown Exception";
    Message = "Unknown error";
    Source = source;
    innerException = inner;
}

Exception::Exception(const std::string &message, ErrorCode code, Exception* inner)
{
    Code = code;
    Name = "Unknown Exception";
    Message = message;
    Source = "Unknown source";
    innerException = inner;
}

Exception::Exception(const std::string &source, const std::string &message, ErrorCode code, Exception* inner)
    : Code(code), Name("Unknown Exception"), Message(message), Source(source), innerException(inner) {}

Exception::Exception(const std::string &name, const std::string &source, const std::string &message, ErrorCode code, Exception* inner)
    : Code(code), Name(name), Message(message), Source(source), innerException(inner) {}


Exception::~Exception() { delete innerException; }

std::string Exception::flush(int level) const {
    std::string message = "";
    for (int i = 0; i < level; i++) {
        message += buildMessage(" \t");
    }
    message += buildMessage("(%s) EXCEPTION: %s\n", Source.c_str(), Message.c_str());
    if (innerException) {
        message += innerException->flush(level + 1);
    }
    return message;
}

void Exception::print() const {
    if (Code == ErrorCode::CRITICAL_ERROR_CODE) {
        // For critical errors, splash the message as well
        std::string message = flush(0);
        splashMessage("%s", message.c_str());
    }
    for (const Exception* current = this; current; current = current->innerException) {
        debugLogMessage(DEBUG_VERBOSE_ERRORS, current->Source.c_str(), "exception",
                        "EXCEPTION: %s", current->Message.c_str());
    }
}
