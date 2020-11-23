#pragma once

#include <windows.h>

namespace vgc::imgutil
{
    /*
     * Contains information about an image. Data is organized as a sequence of rows,
     * top-down. Each row is a sequence of pixels, from the left to the right. Each
     * pixel consists of 4 bytes, in order, they represent Blue, Green, Red and Alpha
     * channels. This struct is a thin wrapper, its members are public and doesn't provide
     * memory management.
     */
    struct ImageData
    {
        UINT width = 0;
        UINT height = 0;
        BYTE* data = nullptr;

        SIZE_T BufferSize() const
        {
            return (SIZE_T)4 * width * height;
        }

        BYTE* operator[] (SIZE_T row) const
        {
            return data + row * width * 4;
        }
    };

    /*
     * Save an image using the Portable Network Graphics format
     * to a given file path.
     */
    HRESULT SaveImageAsPngW(ImageData img, LPCWSTR path);
}
