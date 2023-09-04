import argparse
from PIL import Image
import numpy as np
import os

kImageWidth = 128
kImageHeight = 128
kImageShape = (kImageWidth, kImageHeight)

def writeCppHeader(headerPath: str, utilPath:str, rgb16: np.array):

    assert len(rgb16.shape) == 2
    width,height = rgb16.shape

    filename = os.path.splitext(os.path.basename(headerPath))[0]

    bufferName = f"k{filename.title()}"
    print(f"Creating buffer '{bufferName}' in {headerPath}...")

    outputStr = ""
    outputStr+= f"#pragma once\n\n"
    outputStr+= f"#include \"{utilPath}\"\n\n"

    outputStr+= f"FLASH_LITERAL constexpr uint16 {bufferName}[{height}][{width}] = {{\n"
    
    pixelsPerRow = 16
    pixelStrs = [ ", ".join(f"0x{rbg:04X}" for rbg in pixelRow) for pixelRow in rgb16.reshape(-1, pixelsPerRow) ] 

    outputStr+= "\t" + ",\n\t".join(pixelStrs) + "\n" 

    outputStr+= f"}};\n"

    with open(headerPath, 'w') as header:
        header.write(outputStr)

def main():
    argParser = argparse.ArgumentParser()
    
    argParser.add_argument("image", action="store", help=f"specify input image file to convert")
    argParser.add_argument("--output", required=False, action="store", help=f"specify output header file to save")
    argParser.add_argument("--util", required=False, action="store", help=f"specify the path the util.h file to include in output file", default="../util.h")

    args = argParser.parse_args()

    imgPath = args.image
    utilPath = args.util
    outputFilePath = args.output or f"{os.path.splitext(imgPath)[0]}.h"

    print(f"Converting: '{imgPath}' to: '{outputFilePath}' with utilPath: '{utilPath}'...")

    # read in file
    with Image.open(imgPath) as img:

        # resize image to correct size
        img = img.resize(kImageShape, resample=Image.LANCZOS)

        # convert into 888 rgb 32 bit array
        img = img.convert("RGB")
        rgb32 = np.asarray(img)

    # convert to 16bit 565 color
    rgb16 = np.array([
        (np.uint16((r >> 3) & 0x1F) << 11) |  
        (np.uint16((g >> 2) & 0x3F) <<  5) | 
        (np.uint16((b >> 3) & 0x1F) <<  0)

        for r,g,b in rgb32.reshape(-1, 3)
    ], dtype=np.uint16).reshape(*kImageShape)
    
    # output to file
    writeCppHeader(outputFilePath, utilPath, rgb16)


if __name__ == "__main__":
    main()