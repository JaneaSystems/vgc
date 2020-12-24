#pragma once

#include "pch.h"

namespace vgc
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

        ImageData(UINT width, UINT height);
        BYTE* operator[] (SIZE_T row);
        const BYTE* operator[] (SIZE_T row) const;
    };
}
