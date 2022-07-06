#pragma once

#include "util.h"
#include "HeatMap.h"

#include <Wire.h>
#include <SparkFun_VL53L5CX_Library.h>

class TofSensor {

    public:
        static inline constexpr uint32 kI2CFrequency = 1000000;
        
        static inline constexpr uint8 kLidarImageSize = 8;
        static inline constexpr uint8 kLidarResolution = kLidarImageSize*kLidarImageSize;
        static inline constexpr uint8 kLidarImageMaxSigma = 3;

        static inline constexpr uint8 kLidarFrequency = 60; //Note: Recommended Max is 15Hz
        static inline constexpr uint8 kLidarPacketSize = 32; // Note: Default i2c buffer is 128
        static inline constexpr uint8 kLidarSharpnerPercentage = 0;

        // Warn: pinmode for D6 has to be set after display.Init()
        // Note: The display is connected to HSPI and only need to receives data.
        //       Therefore it only connects to HSPI pins D5, D7, D8 and leaves D6 (MISO) unused
        //       Rather than wasting the pin we override the pinmode set by display to be a GPIO input.
        static inline constexpr uint8 kLidarInterruptPin = D6;

        enum UpdateResult: uint8 { UPDATE_NONE, UPDATE_SUCCESS, UPDATE_ERROR };
       
        struct Data: VL53L5CX_ResultsData {
            uint64 timestamp = 0;
            

            template<typename BufferT, int16 kHeight, int16 kWidth>
            void RenderBitmap(BufferT (&bitmap)[kHeight][kWidth]) const {

                constexpr int8 kBlockWidth  = kWidth  / kLidarImageSize;
                constexpr int8 kBlockHeight = kHeight / kLidarImageSize;

                // Get min-max range
                int16 min = 4097;
                int16 max = 0;
                for(int16 i = 0; i < kLidarResolution; ++i) {
                    
                    if(nb_target_detected[i] 
                        && range_sigma_mm[i] <= kLidarImageMaxSigma
                    ) {

                        int16 distance = distance_mm[i]; 
                        if(distance < min) {
                            min = distance;
                        }

                        if(distance > max) {
                            max = distance;
                        }
                    }
                }
                int16 minMaxDelta = Max(1, max - min);

                //TODO: Get min/max values for data and normalize to that
                for(int16 blockY = 0; blockY < kLidarImageSize; ++blockY) {

                    int16 y = blockY * kBlockHeight;
                    int16 dataYOffset = blockY * kLidarImageSize;

                    for(int16 blockX = 0; blockX < kLidarImageSize; ++blockX) {

                        int16 x = blockX * kBlockWidth;

                        //Note: Tof sensor returns transpose of x so we use x's complement to display data in correct direction
                        int16 dataXOffset = (kLidarImageSize-1) - blockX;
                        int16 dataOffset = dataYOffset + dataXOffset;

                        Color color;
                        if(nb_target_detected[dataOffset] == 0) {

                            color = Color::kBlack;

                        } else {

                            int16 distance = distance_mm[dataOffset];

                            // Map distance to color
                            HeatMap::PercentT normalizedDistance = Clamp(HeatMap::PercentT(distance - min, minMaxDelta), 0, 1);
                            color = HeatMap::InterpolateColor(1 - normalizedDistance, HeatMap::kMagmaColorMap);
                        }

                        // copy color to buffer
                        BufferT bufferValue = color;
                        for(int16 yOffset = 0; yOffset < kBlockHeight; ++yOffset) {
        
                            // TODO: Make FastFill align bitmap to 32 bits and fill 2 pixels at a time via uint32
                            BufferT* column = bitmap[y + yOffset] + x;
                            FastFill(column, column + kBlockWidth, bufferValue);
                        }
                    }
                }

            }            
        };

    private:

        SparkFun_VL53L5CX lidar;
        uint64 lidarTimestamp = 0;

        static IRAM_FUNC void LidarInterrupt(void* tofSensor_) {

            uint64 currentTime = micros64();
            TofSensor* tofSensor = static_cast<TofSensor*>(tofSensor_);

            tofSensor->lidarTimestamp = currentTime;
        }

    public:

        void Init() {

            Wire.begin(); //This resets to 100kHz I2C
            Wire.setClock(kI2CFrequency);

            while(true) {

                Log("Initializing Lidar");
                
                if(lidar.begin() == true) {

                    Log("Lidar connected");
                    break;
                }

                // TODO: Send reset signal to lidar!                
                Warn("Lidar not found. Retrying...");
                delay(500);
            }

            lidar.setWireMaxPacketSize(kLidarPacketSize);

            lidar.setResolution(kLidarResolution);            
            int currentTofResolution;
            while((currentTofResolution = lidar.getResolution()) != kLidarResolution) {

                Warn("Failed to set lidar resolution to %d. Current Resolution: %d. Retrying...", 
                     kLidarResolution, currentTofResolution);
                lidar.setResolution(kLidarResolution);
            }

            Log("Set lidar resolution: %d\n", kLidarResolution);

            lidar.setSharpenerPercent(kLidarSharpnerPercentage);

            lidar.setTargetOrder(SF_VL53L5CX_TARGET_ORDER::STRONGEST);

            lidar.setRangingMode(SF_VL53L5CX_RANGING_MODE::CONTINUOUS);
            lidar.setRangingFrequency(kLidarFrequency);
            lidar.startRanging();
            Log("Started lidar ranging\n");

            pinMode(kLidarInterruptPin, INPUT_PULLUP);
            attachInterruptArg(digitalPinToInterrupt(kLidarInterruptPin), LidarInterrupt, this, FALLING);
            Log("Installed lidar interrupt");
        }

        UpdateResult Update(Data& data) {

            return CriticalSection([&](){

                if(lidarTimestamp == data.timestamp) {
                    return UPDATE_NONE;
                }

                if(lidar.getRangingData(&data)) {
                    data.timestamp = lidarTimestamp;
                    return UPDATE_SUCCESS;
                }

                return UPDATE_ERROR;
            });
        }
};