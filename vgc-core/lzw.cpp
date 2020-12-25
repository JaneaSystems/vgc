#include "pch.h"
#include "lzw.h"

namespace vgc {
    
    /*
     * LZW class wrapper. Compresses the input byte sequence using the Lempel-Ziv-Welch algorithm,
     * adapted for use with the Graphics Interchange Format (GIF).
     * Parameters:
     * * inBytes - a vector of bytes, each byte should have value between 0 and 2^bitDepth - 1
     * * bitDepth - the number of bits in each code word - a number between 1 and 8.
     * Returns the compressed byte sequence.
     * To use the output of this function in the GIF format,
     * you must also divide the data into frames by inserting data length bytes (see bit-stream.h)
     */
    std::vector<BYTE> CompressLZW(const std::vector<BYTE>& inBytes, UINT bitDepth)
    {
        std::vector<BYTE> lzwOutput;
        BitStream chunkBitStream([&](BYTE b) { lzwOutput.push_back(b); });

        // Compress the pixel sequence and write it out
        LZW lzw([&](UINT num, UINT bits) { chunkBitStream.WriteBits(num, bits); }, bitDepth);

        for (auto byte : inBytes)
        {
            lzw += byte;
        }

        lzw.Finish();
        chunkBitStream.Flush();
        return lzwOutput;
    }
}
