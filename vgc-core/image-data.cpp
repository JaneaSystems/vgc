#include "image-data.h"

namespace vgc
{
    ImageData::ImageData(UINT width, UINT height) : width(width), height(height), buffer((size_t)4 * width * height)
    {
    }

    BYTE* ImageData::operator[] (SIZE_T row)
    {
        return buffer.data() + row * width * 4;
    }

    const BYTE* ImageData::operator[] (SIZE_T row) const
    {
        return buffer.data() + row * width * 4;
    }
}
