#pragma once

#include "Esp.h"
#include "Wifi.h"
#include "Display.h"

#include "util.h"
#include "Timer.h"

#include "HeatMap.h"

#include <Wire.h>
#include <SparkFun_VL53L5CX_Library.h>

class TofImager {
    private:

        static inline constexpr int kBaudRate = 115200;

        static inline constexpr uint8 kTofImageSize = 8;
        static inline constexpr uint8 kTofResolution = kTofImageSize*kTofImageSize; 

        static inline constexpr uint8 kTofFrequency = 60;

        Esp esp;
        Display display;

        SparkFun_VL53L5CX tofSensor;
        uint64 tofSensorDataTime = 0;
        uint64 deltaTofSensorDataTime = 0;

    public: 

        template<typename... ArgsT>
        void Printf(const char* fmt, ArgsT... args) {
            Serial.printf(fmt, args...);
            display.printf(fmt, args...);
        }

        template<int16 kHeight, int16 kWidth>
        void RenderBitmap(const VL53L5CX_ResultsData& data, uint16 (&bitmap)[kHeight][kWidth]) {

            //TODO: Get min/max values for data and normalize to that

            // Note: in pixel/block
            constexpr int8 kBlockWidth  = kWidth  / kTofImageSize;
            constexpr int8 kBlockHeight = kHeight / kTofImageSize;

            for(int16 blockY = 0; blockY < kTofImageSize; ++blockY) {

                int16 y = blockY * kBlockHeight;                
                int16 distanceYOffset = blockY*kTofImageSize;

                for(int16 blockX = 0; blockX < kTofImageSize; ++blockX) {

                    int16 x = blockX * kBlockWidth;

                    //Note: Tof sensor returns transpose of x so we use x's complement to display data in correct direction
                    int16 distanceXOffset = (kTofImageSize-1) - blockX;
                    int16 distance = data.distance_mm[distanceYOffset + distanceXOffset];

                    // TODO: Make sure that this is power of 2 when we compute upper and lower bounds
                    // constexpr uint16 kMaxDistance = 4000;
                    constexpr uint16 kMaxDistance = 4097;
                    HeatMap::PercentT normalizedDistance = HeatMap::PercentT(distance, kMaxDistance-1);

                    Color color = HeatMap::InterpolateColor(1 - normalizedDistance, HeatMap::kMagmaColorMap);

                    uint16 color16BE = color.RGB16BE();
                    
                    // TODO: replace with memcpy
                    for(int16 yOffset = 0; yOffset < kBlockHeight; ++yOffset) {
                        for(int16 xOffset = 0; xOffset < kBlockWidth; ++xOffset) {
                            bitmap[y + yOffset][x + xOffset] = color16BE;                        
                        }
                    }
                }
            }
        }

        static IRAM_FUNC void TofSensorInterrupt(void* tofImager_) {

            TofImager* tofImager = static_cast<TofImager*>(tofImager_);

            uint64 currentTime = micros64();
            tofImager->deltaTofSensorDataTime = currentTime - tofImager->tofSensorDataTime;
            tofImager->tofSensorDataTime = currentTime;
        }

        void Init() {

            // Turn on power LED
            // Note: LED is active low
            pinMode(LED_BUILTIN, OUTPUT);
            digitalWrite(LED_BUILTIN, LOW);
            
            display.Init();

            // Init Serial
            display.println("Connecting UART\n");
            Serial.begin(kBaudRate);
            while(!Serial) {}
            Serial.println(); // print a new line for first to sperate serial garbage from first output line

            Wire.begin(); //This resets to 100kHz I2C
            // Wire.setClock(400000); //Sensor has max I2C freq of 400kHz 
            Wire.setClock(1000000); //1MHz 

            Printf("Initializing tof sensor. This can take up to 10s. Please wait.\n");

            esp.wdtDisable();
            *((volatile uint32_t*) 0x60000900)&= ~1; // Hardware WDT OFF
            if(tofSensor.begin() == false) {

                Printf("Tof sensor not found - check your wiring. Freezing\n");
                while(1) {};
            }
            *((volatile uint32_t*) 0x60000900)|= 1; // Hardware WDT ON
            esp.wdtEnable(WDTO_1S);

            Printf("Connected Tof sensor!\n");

            // Install tofSensor interrupt
            // Warn: pinmode for D6 has to be set after display.Init()
            // Note: The display is connected to HSPI and only need to receives data.
            //       Therefore it only connects to HSPI pins D5, D7, D8 and leaves D6 (MISO) unused
            //       Rather than wasting the pin we override the pinmode set by display to be a GPIO input.
            pinMode(D6, INPUT_PULLUP);
            attachInterruptArg(digitalPinToInterrupt(D6), TofSensorInterrupt, this, FALLING);

            // Note: Default i2c buffer is 128
            tofSensor.setWireMaxPacketSize(32);
        
            tofSensor.setResolution(kTofResolution);            
            int currentTofResolution;
            while((currentTofResolution = tofSensor.getResolution()) != kTofResolution) {
                Printf("Failed to set tof Resolution to %d. Current Resolution: %d",
                       kTofResolution, currentTofResolution);
            
                tofSensor.setResolution(kTofResolution);
            }
            display.Clear();
            Printf("Set Tof sensor resolution: %d\n", kTofResolution);

            tofSensor.setRangingMode(SF_VL53L5CX_RANGING_MODE::CONTINUOUS);
            tofSensor.setRangingFrequency(kTofFrequency);
            tofSensor.startRanging();
            Printf("Started Tof sensor ranging\n");
        }

        void Update() {

            static Timer timer;
            static VL53L5CX_ResultsData tofSensorData;
            static uint64 lastTofSensorDataTime = 0;

            uint64 currentDeltaTofSensorDataTime;
            static Timer drawTimer;
            static uint64 drawDeltaTime = 0;

            enum { UPDATE_NONE, UPDATE_SUCCESS, UPDATE_ERROR };
            bool updateResult = CriticalSection([&](){

                if(tofSensorDataTime == lastTofSensorDataTime) {
                    return UPDATE_NONE;
                }

                currentDeltaTofSensorDataTime = deltaTofSensorDataTime;

                if(tofSensor.getRangingData(&tofSensorData)) {
                    lastTofSensorDataTime = tofSensorDataTime;
                    return UPDATE_SUCCESS;
                }

                return UPDATE_ERROR;
            });

            if(updateResult == UPDATE_NONE) return;


            Timer renderTimer(micros64());
            if(updateResult == UPDATE_SUCCESS) {
            
                RenderBitmap(tofSensorData, display.backBuffer.GetBuffer());
            
            } else {

                display.backBuffer.fillScreen(Color::kBlack.RGB16BE());
                display.backBuffer.setTextColor(Color::kRed.RGB16BE());
                display.backBuffer.print("Failed to update tof data");
            }

            using FpsT = float;

            float sensorFps = float(FpsT(1000000) / currentDeltaTofSensorDataTime);
            float renderFps = float(FpsT(1)/renderTimer.LapS<FpsT>());
            float drawFps   = float(FpsT(1000000)/drawDeltaTime);
            float updateFps = float(FpsT(1)/timer.LapS<FpsT>());

            display.backBuffer.setCursor(0,0);
            display.backBuffer.setTextColor(Color::kCyan.RGB16BE());
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

            // timer.Lap();
            // RenderBitmap(tofSensorData, display.backBuffer.GetBuffer());

            // float elapsedSeconds = timer.LapS();
            // float fps = 1./elapsedSeconds;

            // display.backBuffer.setCursor(0,0);
            // display.backBuffer.setTextColor(Color::kWhite.RGB16BE());
            // display.backBuffer.printf("FPS: %0.3f", fps);

            // display.Draw();
        }
        
};
