#pragma once

#include <SoftwareSerial.h>
#include "Color.h"

class Display;
Display* gLogDisplay = nullptr;

enum LogType {
    LOG_MSG, LOG_WARN, LOG_ERROR
};

// TODO: Make this logging more comprehensive and also log to the screen/over the network
template<typename... ArgsT>
inline void LogEx(LogType type, const char* fmt, ArgsT... args) {

    Serial.printf(fmt, args...);

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
        gLogDisplay->printf(fmt, args...);
        
        gLogDisplay->setTextColor(oldTextColor);        
    }
}

#define Log(msg, fmt...)   LogEx(LOG_MSG, "MSG - " msg "\n",##fmt)
#define Warn(msg, fmt...)  LogEx(LOG_WARN, "WARN - " msg "\n",##fmt)
#define Error(msg, fmt...) LogEx(LOG_ERROR, "ERROR - " msg "\n",##fmt)

