#include "quantization.h"

namespace vgc
{
    QuantizationOutput SimpleQuantizer::operator() (const ImageData& img) const
    {
        QuantizationOutput output;

        output.bitsPerPixel = 8;

        output.palette.emplace_back(PaletteColor{});

        for (UINT i = 0; i < 6; i++)
        {
            for (UINT j = 0; j < 6; j++)
            {
                for (UINT k = 0; k < 6; k++)
                {
                    output.palette.push_back(PaletteColor{ (BYTE)(i * 51), (BYTE)(j * 51), (BYTE)(k * 51) });
                }
            }
        }

        output.palette.resize(256);

        output.pixels.resize((size_t)img.width * img.height);

        for (UINT i = 0, k = 0; i < img.height; i++)
        {
            for (UINT j = 0; j < img.width; j++)
            {
                UINT b = img[i][4 * j + 0];
                UINT g = img[i][4 * j + 1];
                UINT r = img[i][4 * j + 2];

                b = (5 * b + 130) >> 8;
                g = (5 * g + 130) >> 8;
                r = (5 * r + 130) >> 8;

                output.pixels[k++] = r * 36 + g * 6 + b + 1;
            }
        }

        return output;
    }
}
