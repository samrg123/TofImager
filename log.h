#pragma once

#include "debug.h"
#include "Color.h"

constexpr bool kLogToGdb = kEnableDebugging;
constexpr bool kLogToSerial = !kLogToGdb;
constexpr uint kLogBufferBytes = 1024;

class Display;
Display* gLogDisplay = nullptr;

enum LogType {
    LOG_MSG, LOG_WARN, LOG_ERROR
};

// TODO: Make this logging more comprehensive and also log to the screen/over the network
template<typename... ArgsT>
inline void LogEx(LogType type, const char* fmt, ArgsT... args) {

    static char logBuffer[kLogBufferBytes] = {};

    int logLen = snprintf(nullptr, 0, fmt, args...);
    char* logStr = (logLen < kLogBufferBytes) ? logBuffer : new char[logLen+1];
    sprintf(logStr, fmt, args...);

    // Log to GDB
    if constexpr(kEnableDebugging) {
        gdbstub_write(logStr, logLen);
    }

    // Log to Serial
    if constexpr(kLogToSerial) {
        Serial.write(logStr, logLen);
    }

    // TODO: optimize this!
    // TODO: Add scrolling buffer
    if(gLogDisplay) {

        Color color;
        switch(type) {

            case LOG_MSG: {
                color = Color::kGreen;
            } break;

            case LOG_WARN: {
                color = Color::kYellow;
            } break;

            case LOG_ERROR: {
                color = Color::kRed;
            } break;

            default: color = Color::kWhite;
        }

        uint16 oldTextColor = gLogDisplay->GetTextColor();
        gLogDisplay->setTextColor(RGB16(color));
        
        gLogDisplay->Clear();
        gLogDisplay->write(logStr, logLen);
        
        gLogDisplay->setTextColor(oldTextColor);        
    }

    // cleanup allocated memory
    if(logStr != logBuffer) delete[] logStr;
}

#define Log(msg, fmt...)   LogEx(LOG_MSG, "MSG - " msg "\n",##fmt)
#define Warn(msg, fmt...)  LogEx(LOG_WARN, "WARN - " msg "\n",##fmt)
#define Error(msg, fmt...) LogEx(LOG_ERROR, "ERROR - " msg "\n",##fmt)
