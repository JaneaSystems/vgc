#include "bit-stream.h"

namespace vgc
{
    void InsertByteLengthHeaders(std::vector<BYTE>& bytes)
    {
        size_t dataR = bytes.size();
        size_t bytesToAppend = (bytes.size() + 254) / 255;
        bytes.resize(bytes.size() + bytesToAppend);

        size_t destR = bytes.size();

        while (dataR > 0)
        {
            size_t blockSize = std::min(size_t(255), dataR);
            memmove(bytes.data() + destR - blockSize, bytes.data() + dataR - blockSize, blockSize);
            destR -= blockSize;
            dataR -= blockSize;
            bytes[--destR] = (BYTE)blockSize;
        }
    }
}
