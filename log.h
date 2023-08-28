#pragma once

#include "debug.h"
#include "Color.h"
#include "Display.h"

constexpr bool kLogToGdb = kEnableDebugging;
constexpr bool kLogToSerial = !kLogToGdb;
constexpr uint kLogBufferBytes = 256;

Display* gLogDisplay = nullptr;

enum LogType {
    LOG_MSG, LOG_WARN, LOG_ERROR
};

#define Log(msg, fmt...)   LogEx(LOG_MSG, "MSG - " msg "\n",##fmt)
#define Warn(msg, fmt...)  LogEx(LOG_WARN, "WARN - " msg "\n",##fmt)
#define Error(msg, fmt...) LogEx(LOG_ERROR, "ERROR - " msg "\n",##fmt)

// TODO: Make this logging more comprehensive and also log to the screen/over the network
// TODO: PLACE IN WRAPPER CLASS!
static char logBuffer[kLogBufferBytes] = {};

template<typename... ArgsT>
inline void LogEx(LogType type, const char* fmt, ArgsT... args) {

    int computedLen = snprintf(nullptr, 0, fmt, args...);    
    if(computedLen < 0) {
        Error("Encoder error while logging | LogType: %d | fmt: '%s' | num args: %d\n", 
              type, fmt, sizeof...(args)
        );
        return;
    }

    size_t logLen = computedLen;

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

