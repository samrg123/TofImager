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

        static inline constexpr uint8 kTofMaxSigma = 3;
        static inline constexpr uint8 kTofFrequency = 60; //Note: Recommended Max is 15Hz
        static inline constexpr uint8 kTofPacketSize = 32; // Note: Default i2c buffer is 128
        static inline constexpr uint8 kTofSharpnerPercentage = 0;
        static inline constexpr uint32 kTofI2CFrequency = 1000000;

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

        template<
            typename DstT, int16 kDstHeight, int16 kDstWidth,
            typename SrcT, int16 kSrcHeight, int16 kSrcWidth
        >
        static void InterpolateBitmap(DstT (&dst)[kDstHeight][kDstWidth], const SrcT (&src)[kSrcHeight][kSrcWidth]) {

            // TODO: make this vec2
            constexpr accum16 kSrcIncrementX = accum16(kSrcWidth,  kDstWidth);
            constexpr accum16 kSrcIncrementY = accum16(kSrcHeight, kDstHeight);
        
            accum16 fixedSrcY = 0;
            for(int16 dstY = 0; dstY < kDstHeight; ++dstY) {

                //TODO: make sure this optimizes away with constexpr 
                int srcY1 = fixedSrcY.Integer();
                int srcY2 = srcY1 < (kSrcHeight-1) ? (srcY1 + 1) : srcY1;

                fixedSrcY+= kSrcIncrementY;                
                accum16 fractionY = fixedSrcY - srcY1;

                accum16 fixedSrcX = 0;
                for(int16 dstX = 0; dstX < kDstWidth; ++dstX) {

                    int srcX1 = fixedSrcX.Integer();
                    int srcX2 = srcX1 < (kSrcWidth-1) ? (srcX1 + 1) : srcX1;

                    fixedSrcX+= kSrcIncrementX;
                    accum16 fractionX = fixedSrcX - srcX1;

                    Color colorLerpX1 = Lerp(fractionX, Color(src[srcY1][srcX1]), Color(src[srcY1][srcX2])); 
                    Color colorLerpX2 = Lerp(fractionX, Color(src[srcY2][srcX1]), Color(src[srcY2][srcX2])); 
                    Color colorLerpXY = Lerp(fractionY, colorLerpX1, colorLerpX2);

                    dst[dstY][dstX] = colorLerpXY;
                }
            }
        }

        template<typename BufferT, int16 kHeight, int16 kWidth>
        static void RenderBitmap(const VL53L5CX_ResultsData& data, BufferT (&bitmap)[kHeight][kWidth]) {

            constexpr int8 kBlockWidth  = kWidth  / kTofImageSize;
            constexpr int8 kBlockHeight = kHeight / kTofImageSize;

            // Get min-max range
            int16 min = 4097;
            int16 max = 0;
            for(int16 i = 0; i < kTofImageSize*kTofImageSize; ++i) {
                
                if(data.nb_target_detected[i] 
                    && data.range_sigma_mm[i] <= kTofMaxSigma
                ) {

                    int16 distance = data.distance_mm[i]; 
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
            for(int16 blockY = 0; blockY < kTofImageSize; ++blockY) {

                int16 y = blockY * kBlockHeight;
                int16 dataYOffset = blockY * kTofImageSize;

                for(int16 blockX = 0; blockX < kTofImageSize; ++blockX) {

                    int16 x = blockX * kBlockWidth;

                    //Note: Tof sensor returns transpose of x so we use x's complement to display data in correct direction
                    int16 dataXOffset = (kTofImageSize-1) - blockX;
                    int16 dataOffset = dataYOffset + dataXOffset;

                    Color color;
                    if(data.nb_target_detected[dataOffset] == 0) {

                        color = Color::kBlack;

                    } else {

                        int16 distance = data.distance_mm[dataOffset];

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

        static IRAM_FUNC void TofSensorInterrupt(void* tofImager_) {

            uint64 currentTime = micros64();
            TofImager* tofImager = static_cast<TofImager*>(tofImager_);

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
            Wire.setClock(kTofI2CFrequency);

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

            tofSensor.setWireMaxPacketSize(kTofPacketSize);

            tofSensor.setResolution(kTofResolution);            
            int currentTofResolution;
            while((currentTofResolution = tofSensor.getResolution()) != kTofResolution) {
                Printf("Failed to set tof Resolution to %d. Current Resolution: %d",
                       kTofResolution, currentTofResolution);
            
                tofSensor.setResolution(kTofResolution);
            }
            display.Clear();
            Printf("Set Tof sensor resolution: %d\n", kTofResolution);

            tofSensor.setSharpenerPercent(kTofSharpnerPercentage);

            tofSensor.setTargetOrder(SF_VL53L5CX_TARGET_ORDER::STRONGEST);

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

                // RenderBitmap(tofSensorData, display.backBuffer.GetBufferBE());
            
                static Color tofImage[kTofImageSize][kTofImageSize];
                RenderBitmap(tofSensorData, tofImage);
                InterpolateBitmap(display.backBuffer.GetBufferBE(), tofImage);

            } else {

                display.backBuffer.fillScreen(RGB16BE(Color::kBlack));
                display.backBuffer.setTextColor(RGB16BE(Color::kRed));
                display.backBuffer.print("Failed to update tof data");
            }

            using FpsT = float;

            float sensorFps = float(FpsT(1000000) / currentDeltaTofSensorDataTime);
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

            // timer.Lap();
            // RenderBitmap(tofSensorData, display.backBuffer.GetBuffer());

            // float elapsedSeconds = timer.LapS();
            // float fps = 1./elapsedSeconds;

            // display.backBuffer.setCursor(0,0);
            // display.backBuffer.setTextColor(RGB16BE(Color::kWhite));
            // display.backBuffer.printf("FPS: %0.3f", fps);

            // display.Draw();
        }
        
};
