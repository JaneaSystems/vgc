#pragma once

#include "pch.h"
#include "bit-stream.h"
#include "image-data.h"
#include "quantization.h"
#include "lzw.h"

namespace vgc
{
    /*
     * A straightforward, single-threaded implementation of the GIF standard.
     */
    template<class Quantizer>
    class SimpleGifEncoder
    {
        struct FileStreamFunc
        {
            std::ofstream& ofs;

            FileStreamFunc(std::ofstream& ofs) : ofs(ofs) {}

            void operator() (BYTE b)
            {
                ofs << b;
            }
        };

        std::ofstream m_fileStream;
        BitStream<FileStreamFunc> m_bitStream;
        USHORT m_width;
        USHORT m_height;
        Quantizer m_quantizer{};
        bool m_finished;

        void WriteGifHeaders()
        {
            // GIF Magic number
            m_fileStream << "GIF89a";

            // Image dimensions
            m_bitStream << m_width << m_height;

            // Dummy global palette
            m_fileStream << '\xf0';
            for (int i = 0; i < 8; i++)
            {
                m_fileStream << '\0';
            }

            // Set up the animation
            m_fileStream << '\x21' << '\xff' << '\x0b' << "NETSCAPE2.0" << '\x03';
            m_fileStream << '\x01' << '\0' << '\0' << '\0';
        }

    public:

        /*
         * Construct a new GIF encoder. It will record the Gif in a file with the given
         * path. You must also specify the size of the Gif beforehand.
         */
        SimpleGifEncoder(const std::wstring filePath, UINT width, UINT height) :
            m_fileStream(filePath, std::ios::binary),
            m_bitStream(FileStreamFunc(m_fileStream)),
            m_width(width),
            m_height(height),
            m_finished(false)
        {
            WriteGifHeaders();
        }

        /*
         * Add a frame to the GIF frame sequence, using the given delay.
         * The delay is given in hundredths of a second, and it must be at least two,
         * as most GIF viewers don't support the value 1.
         * The frame is processed using the quantizer given as a template parameter.
         */
        void AddFrame(const ImageData& img, USHORT delay)
        {
            if (img.width != m_width || img.height != m_height || m_finished)
            {
                // TODO log unexpected error?
                return;
            }

            QuantizationOutput quantization = m_quantizer(img);

            // Graphics control extension header
            m_bitStream << '\x21' << '\xf9' << '\x04' << '\x05' << delay << '\0' << '\0';

            // Image descriptor block
            m_bitStream << '\x2c';

            // GIF allows us to only redraw a portion of the image, but we'll
            // draw the entire canvas each time.

            // This sets up the (left, top) coordinate of the new portion
            m_bitStream << (USHORT)0 << (USHORT)0;

            // This sets up the size
            m_bitStream << m_width << m_height;

            // Color table size
            m_bitStream << (BYTE)(0x7f + quantization.bitsPerPixel);

            for (auto color : quantization.palette)
            {
                m_bitStream << color.r << color.g << color.b;
            }

            // Bits per pixel, initialize the encoder/decoder
            m_bitStream << (BYTE)(quantization.bitsPerPixel);

            std::vector<BYTE> lzwOutput = CompressLZW(quantization.pixels, quantization.bitsPerPixel);
            InsertByteLengthHeaders(lzwOutput);
            m_fileStream.write(reinterpret_cast<char*>(lzwOutput.data()), lzwOutput.size());
            m_bitStream << '\0';
        }

        /*
         * Proclaim that there are no more frames to be written. This adds the GIF footer and
         * closes the file stream. It's also called by the destructor. Calling it again
         * does nothing.
         */
        void Finish()
        {
            if (!m_finished)
            {
                m_finished = true;
                m_bitStream << '\x3b';
                m_fileStream.close();
            }
        }

        ~SimpleGifEncoder()
        {
            Finish();
        }
    };
}
