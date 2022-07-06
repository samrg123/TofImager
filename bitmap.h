#pragma once

#include "Color.h"
#include "mathUtil.h"
#include "FixedPoint.h"

// TODO: Make a proper BitmapClass and use it in TofSensor::Data::RenderBitmap()

template<
    typename DstT, int16 kDstHeight, int16 kDstWidth,
    typename SrcT, int16 kSrcHeight, int16 kSrcWidth
>
constexpr void InterpolateBitmap(DstT (&dst)[kDstHeight][kDstWidth], const SrcT (&src)[kSrcHeight][kSrcWidth]) {

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