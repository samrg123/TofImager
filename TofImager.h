#pragma once

#include "Esp.h"
#include "Wifi.h"
#include "Display.h"
#include "TofSensor.h"

#include "util.h"
#include "Timer.h"
#include "bitmap.h"

class TofImager {

    public:

        static inline constexpr int kBaudRate = 115200;

    private:

        Esp esp;
        Display display;
        TofSensor tofSensor;

    public: 

        void InitSerial() {
            Log("Connecting UART\n");
            Serial.begin(kBaudRate);
            while(!Serial) {}
            Serial.println(); // print a new line for first to sperate serial garbage from first output line
        }

        void Init() {

            // Turn on power LED
            // Note: LED is active low
            pinMode(LED_BUILTIN, OUTPUT);
            digitalWrite(LED_BUILTIN, LOW);
            
            display.Init();
            gLogDisplay = &display;
            
            InitSerial();

            tofSensor.Init();

            // TODO: Init Server
        }

        void Update() {

            static Timer timer;
            static TofSensor::Data tofSensorData;
            static uint64 lastTofSensorTimestamp = 0;

            static Timer drawTimer;
            static uint64 drawDeltaTime = 0;

            TofSensor::UpdateResult updateResult = tofSensor.Update(tofSensorData);

            if(updateResult == TofSensor::UPDATE_NONE) return;

            uint64 deltaTofSensorDataTime = tofSensorData.timestamp - lastTofSensorTimestamp;
            lastTofSensorTimestamp = tofSensorData.timestamp;

            Timer renderTimer(micros64());
            if(updateResult == TofSensor::UPDATE_SUCCESS) {

                // RenderBitmap(tofSensorData, display.backBuffer.GetBufferBE());
            
                static Color tofImage[tofSensor.kLidarImageSize][tofSensor.kLidarImageSize];
                tofSensorData.RenderBitmap(tofImage);
                InterpolateBitmap(display.backBuffer.GetBufferBE(), tofImage);

            } else {

                Error("Failed to update tof data");
                return;
            }

            using FpsT = float;

            float sensorFps = float(FpsT(1000000) / deltaTofSensorDataTime);
            float renderFps = float(FpsT(1)/renderTimer.LapS<FpsT>());
            float drawFps   = float(FpsT(1000000)/drawDeltaTime);
            float updateFps = float(FpsT(1)/timer.LapS<FpsT>());

            display.backBuffer.setCursor(0,0);
            display.backBuffer.setTextColor(RGB16BE(Color::kDimGray));
            display.backBuffer.printf(
                "Sensor FPS: %0.3f\n" 
                "Render FPS: %0.3f\n" 
                "Draw   FPS: %0.3f\n" 
                "Update FPS: %0.3f\n",
                sensorFps, 
                renderFps, 
                drawFps, 
                updateFps
            );

            drawTimer.Lap();
            display.FastDrawBE();
            drawDeltaTime = drawTimer.LapUs();
        }
        
};
