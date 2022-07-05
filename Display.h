#pragma once

#include <SPI.h>
#include <Adafruit_SSD1351.h>
#include <Adafruit_GFX.h>

#include "Color.h"

class Display: public Adafruit_SSD1351 {

	static inline constexpr uint8 kWidth  = SSD1351WIDTH; 
	static inline constexpr uint8 kHeight = SSD1351HEIGHT; 

	static inline constexpr uint8_t kSCL  = D5;
	static inline constexpr uint8_t kSDA  = D7; 
	static inline constexpr uint8_t kRES  = D3;
	static inline constexpr uint8_t kDC   = D0;
	static inline constexpr uint8_t kCS   = D8;

	static constexpr bool ValidDigitalOutPin(uint8_t pin) {
	
		 //TODO: make sure there are all valid
		return pin == D0 ||
			   pin == D8 ||
			   pin == D1 ||
			   pin == D2 ||
			   pin == D3 ||
			   pin == D4;
	}

	public:  
	
		struct BackBuffer: GFXcanvas16 {
			
			BackBuffer(): GFXcanvas16(kWidth, kHeight) {}

			using Buffer = uint16_t[kHeight][kWidth];

			Buffer& GetBuffer() {
				return *reinterpret_cast<Buffer*>(getBuffer());
			}

		};

		BackBuffer backBuffer;
	
		Display(): Adafruit_SSD1351(kWidth, kHeight, &SPI, kCS, kDC, kRES) {
	
			static_assert(kSCL == D5, "SCL must be connected to D5 for fast SPI");
			static_assert(kSDA == D7, "SDA must be connected to D5 for fast SPI");
			
			static_assert(ValidDigitalOutPin(kRES), "kRES is not a valid digital output");
			static_assert(ValidDigitalOutPin(kDC),  "kDC is not a valid digital output");
			static_assert(ValidDigitalOutPin(kCS),  "kCS is not a valid digital output");
		}
		

		inline void Init() {

			// TODO: Look at this on the scope and see what speed we're actually getting on the spi buss.
			// NOTE: ESP8266 caps out at 80MHz HSPI
			// begin(35000000);

			// Note: 20MHz is max freq in datasheet and corresponds to ~76.3Hz
			begin(20000000);

			//Note: we need to wait 100ms after on command is sent to the display 
			// at the end of begin() before we can send the display commands. Otherwise
			// we can get undefined behavior
			delay(100);

			//clear out garbage screen content
			fillScreen(Color::kBlack.RGB16());
			enableDisplay(true);
		}

		inline void Clear() {
			fillScreen(Color::kBlack.RGB16());
			setCursor(0, 0);
		}

		enum ColorOrder: uint8 {
			COLOR_ORDER_RGB = 0b01'110100,
			COLOR_ORDER_BGR = 0b01'110000,
		};

		inline void SetColorOrder(ColorOrder colorOrder) {
			uint8 commandData = colorOrder;
			sendCommand(SSD1351_CMD_SETREMAP, &commandData, 1);
		}
		
		// Note: Like Draw(), but raw backbuffer bytes are draw to the screen in Big Endian order.
		// 		 In order for colors to appear correctly on screen Backbuffer must be in big endian 
		//		 ([RRRRR:GGG][GGG:BBBBB]) order instead of default little endian order ([GGG:BBBBB][RRRRR:GGG]).
		// 		 If you are using this function make sure to use Color::RGB16BE() so colors appear correctly on screen.
		inline void FastDrawBE() {

			startWrite();
			setAddrWindow(0, 0, kWidth, kHeight);
		
			uint16* pixels = backBuffer.getBuffer();			
			constexpr uint32 kNumPixels = kWidth*kHeight;

			hwspi._spi->writeBytes(reinterpret_cast<uint8*>(pixels), kNumPixels*sizeof(uint16));

			endWrite();
		}

		inline void Draw() {
			drawRGBBitmap(0, 0, backBuffer.getBuffer(), kWidth, kHeight);
		}
		
};
