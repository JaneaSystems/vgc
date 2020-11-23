#pragma once

#include <windows.h>
#include <vector>

namespace vgc::imgutil
{
    /*
     * Contains information about an image. Data is organized as a sequence of rows,
     * top-down. Each row is a sequence of pixels, from the left to the right. Each
     * pixel consists of 4 bytes, in order, they represent Blue, Green, Red and Alpha
     * channels.
     */
    struct ImageData
    {
        UINT width = 0;
        UINT height = 0;
        std::vector<BYTE> buffer;

        ImageData(UINT width, UINT height) : width(width), height(height), buffer((size_t)4 * width * height)
        {
        }

        BYTE* operator[] (SIZE_T row)
        {
            return buffer.data() + row * width * 4;
        }

        const BYTE* operator[] (SIZE_T row) const
        {
            return buffer.data() + row * width * 4;
        }
    };

    /*
     * Save an image using the Portable Network Graphics format
     * to a given file path.
     */
    HRESULT SaveImageAsPngW(ImageData& img, LPCWSTR path);
}
