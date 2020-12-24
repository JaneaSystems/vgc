#pragma once

#include "pch.h"
#include "image-data.h"

namespace vgc
{
    /*
     * Represents one palette entry.
     */
    struct PaletteColor
    {
        BYTE r, g, b;
    };

    /*
     * Represents the output of a quantizer.
     * The pixels array size must be equal to the number of pixels in the original image.
     * The pixels must be ordered in the same way as in the original image.
     * The color code 0 must correspond to the transparent color.
     * The size of the palette must be equal to pow(2, bitsPerPixel).
     */
    struct QuantizationOutput
    {
        UINT bitsPerPixel = 8;
        std::vector<PaletteColor> palette;
        std::vector<BYTE> pixels;
        BYTE transparentColor;
    };

    /*
     * A simple and very fast quantizer, useful for testing.
     */
    struct SimpleQuantizer
    {
        QuantizationOutput operator() (const ImageData& img) const;
    };
}
