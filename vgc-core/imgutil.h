#pragma once

#include <windows.h>
#include <vector>
#include <iostream>
#include <mutex>
#include <thread>
#include <fstream>
#include <condition_variable>
#include <chrono>
#include <d3d11.h>
#include <dxgi1_2.h>

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
    HRESULT SaveImageAsPngFileW(ImageData& img, LPCWSTR path);

    /*
     * Load an image using the Portable Network Graphics format
     * from a given file path, and store the result into the given
     * ImageData object. Should only be used with images saved
     * using SaveImageAsPngFileW.
     */
    HRESULT LoadImageFromPngFileW(ImageData& img, LPCWSTR path);

    /*
     * Bit stream wrapper. Used for capturing the output of compression
     * and for writing the contents of an image to a file. Construct it
     * by passing it a function which takes a BYTE as an argument.
     * This function will be called each time a whole byte is added.
     */
    template<class ByteFunc>
    class BitStream
    {
        ByteFunc m_func;

        // The offset will always be in 0..7 before/after member function calls
        UINT m_offset;

        uint64_t m_part;

        void WriteByte(BYTE byte)
        {
            if (m_offset == 0)
            {
                m_func(byte);
            }
            else
            {
                m_part |= (uint64_t)byte << m_offset;
                m_func((BYTE)(m_part & 0xff));
                m_part >>= 8;
            }
        }

    public:
        BitStream(ByteFunc func) : m_func(func), m_offset(0), m_part(0) {}
        
        /*
         * Write the argument's bit representation.
         */
        template<class T>
        BitStream& operator<<(T arg)
        {
            static_assert(sizeof(T) <= 4);
            static_assert(std::is_integral<T>::value);

            for (UINT i = 0; i < sizeof(T); i++)
            {
                WriteByte((BYTE)(arg & 0xff));
                arg >>= 8;
            }

            return *this;
        }

        /*
         * Write the lowest m bits of the given 32-bit unsigned integer.
         * The number of bits written must be strictly less than 32.
         */
        BitStream& WriteBits(UINT num, UINT m)
        {
            num &= (1u << m) - 1;
            m_part |= (uint64_t)num << m_offset;
            m_offset += m;
            while (m_offset >= 8)
            {
                m_func((BYTE)(m_part & 0xff));
                m_offset -= 8;
                m_part >>= 8;
            }
            return *this;
        }

        /*
         * Return the current bit offset.
         */
        UINT Offset() const
        {
            return m_offset;
        }

        /*
         * If we have unwritten bits, fill the unassigned bits with zeroes and call the
         * receiving function.
         */
        BitStream& Flush()
        {
            if (m_offset)
            {
                m_func((BYTE)(m_part & 0xff));
                m_part = 0;
                m_offset = 0;
            }
            return *this;
        }
    };

    /*
     * Compresses the input byte sequence using the Lempel-Ziv-Welch algorithm,
     * adapted for use with the Graphics Interchange Format (GIF).
     * Construct this with two arguments:
     * * func - A function which receievs two arguments:
     * * * the produced code word (as a UINT)
     * * * the length of the code word (as a UINT)
     * * bitDepth - the number of bits in each code word - a number between 1 and 8.
     * After construction, call the += operator to insert a codeword.
     * The compression footer will be added after calling Finish() or when the
     * destructor is invoked.
     * 
     * Concurrent access to a single LZW object is not supported.
     */
    template<class CodewordFunc>
    class LZW
    {
        struct CodeTreeNode {
            USHORT next[256];

            void Clear()
            {
                memset(next, 0, sizeof next);
            }

            CodeTreeNode()
            {
                Clear();
            }
        };

        // The callback which receives the new code word.
        CodewordFunc m_func;

        // Current position in the dictionary tree, or -1 if it's undefined.
        int m_treePos;

        // Number of bits in each sample in the input stream
        const UINT m_bitDepth;

        // Current number of bits in each code word.
        UINT m_codeSize;

        // Number of used code words = size of the code tree.
        UINT m_usedCodes;

        // The code tree, stored as an array for efficiency.
        std::vector<CodeTreeNode> m_codeTree;

        // Whether we finished the encoding. Used to stop the destructor
        // from adding the compression footer if Finish() is called. 
        bool m_finished;

        USHORT Next(int source, int label)
        {
            return m_codeTree[source].next[label];
        }

        bool HasNext(int source, int label)
        {
            return Next(source, label) != 0;
        }

        UINT CodewordClear()
        {
            return 1u << m_bitDepth;
        }

        UINT CodewordEndOfInput()
        {
            return CodewordClear() + 1;
        }

        void ClearDictionary()
        {
            m_func(CodewordClear(), m_codeSize);
            m_codeSize = m_bitDepth + 1;
            m_usedCodes = CodewordEndOfInput() + 1;

            m_codeTree.resize(m_usedCodes);
            for (auto& node : m_codeTree)
            {
                node.Clear();
            }

            m_treePos = -1;
        }

        UINT CreateCode()
        {
            m_codeTree.emplace_back();
            return m_usedCodes++;
        }

        void SetDestination(int source, int label, USHORT destination)
        {
            m_codeTree[source].next[label] = destination;
        }

    public:
        
        LZW(CodewordFunc func, UINT bitDepth) :
            m_func(func),
            m_treePos(-1),
            m_bitDepth(bitDepth),
            m_finished(false)
        {
            ClearDictionary();
        }

        LZW& operator+= (BYTE value)
        {
            if (m_treePos == -1)
            {
                m_treePos = value;
            }
            else if (HasNext(m_treePos, value))
            {
                m_treePos = Next(m_treePos, value);
            }
            else
            {
                m_func((UINT)m_treePos, m_codeSize);
                UINT destination = CreateCode();
                SetDestination(m_treePos, value, destination);

                if (destination == 1u << m_codeSize)
                {
                    m_codeSize++;
                }

                if (m_usedCodes == 4096)
                {
                    ClearDictionary();
                }

                m_treePos = value;
            }

            return *this;
        }

        void Finish()
        {
            if (!m_finished && m_treePos != -1)
            {
                m_finished = true;
                m_func((UINT)m_treePos, m_codeSize);
                m_func(CodewordClear(), m_codeSize);
                m_func(CodewordEndOfInput(), m_bitDepth + 1);
            }
        }

        ~LZW()
        {
            Finish();
        }
    };

    // TODO document the code below

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
            
            // TODO: Reduce memory usage by flushing each chunk when it reaches 255 bytes.

            std::vector<BYTE> lzwOutput;
            BitStream chunkBitStream([&](BYTE b) { lzwOutput.push_back(b); });

            // Compress the pixel sequence and write it out
            LZW lzw([&](UINT num, UINT bits) { chunkBitStream.WriteBits(num, bits); }, quantization.bitsPerPixel);

            for (auto pixel : quantization.pixels)
            {
                lzw += pixel;
            }

            lzw.Finish();
            chunkBitStream.Flush();

            // Write out the chunks
            for (size_t i = 0; i < lzwOutput.size();)
            {
                BYTE len = 255;
                if (lzwOutput.size() - i < 255)
                {
                    len = (BYTE)(lzwOutput.size() - i);
                }
                m_bitStream << len;
                m_fileStream.write(reinterpret_cast<char*>(lzwOutput.data() + i), len);
                i += len;
            }

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

    // TODO document code below.

    template<class FrameFunc>
    class ScreenRecorder
    {
        static inline const D3D_FEATURE_LEVEL s_featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_0
        };

        static void CheckResult(HRESULT hr)
        {
            if (!SUCCEEDED(hr))
            {
                throw hr;
            }
        }

        template<class T>
        static void SafeRelease(T& obj)
        {
            if (obj)
            {
                obj->Release();
                obj = nullptr;
            }
        }

        FrameFunc m_frameCallback;

        ID3D11Device* m_device{ nullptr };
        ID3D11DeviceContext* m_immediateContext{ nullptr };
        IDXGIOutputDuplication* m_outputDuplication{ nullptr };
        ID3D11Texture2D* m_desktopImage{ nullptr };
        ID3D11Texture2D* m_gdiImage{ nullptr };
        ID3D11Texture2D* m_destImage{ nullptr };
        IDXGIResource* m_desktopResource{ nullptr };

        DXGI_OUTPUT_DESC m_outputDesc;
        DXGI_OUTDUPL_DESC m_outputDuplDesc;
        DXGI_OUTDUPL_FRAME_INFO m_frameInfo;
        std::chrono::steady_clock::time_point m_creationTime;

        void InitDevices()
        {
            CheckResult(D3D11CreateDevice(NULL,
                D3D_DRIVER_TYPE_HARDWARE,
                NULL,
                0,
                s_featureLevels,
                (UINT)std::size(s_featureLevels),
                D3D11_SDK_VERSION,
                &m_device,
                NULL,
                &m_immediateContext));

            IDXGIDevice* dxgiDevice;
            IDXGIAdapter* dxgiAdapter;
            IDXGIOutput* dxgiOutput;
            IDXGIOutput1* dxgiOutput1;

            // Which output device should we capture?
            UINT output = 0;

            CheckResult(m_device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
            CheckResult(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&dxgiAdapter)));
            SafeRelease(dxgiDevice);

            CheckResult(dxgiAdapter->EnumOutputs(output, &dxgiOutput));
            SafeRelease(dxgiAdapter);

            CheckResult(dxgiOutput->GetDesc(&m_outputDesc));
            CheckResult(dxgiOutput->QueryInterface(IID_PPV_ARGS(&dxgiOutput1)));
            SafeRelease(dxgiOutput);

            CheckResult(dxgiOutput1->DuplicateOutput(m_device, &m_outputDuplication));
            SafeRelease(dxgiOutput1);

            m_outputDuplication->GetDesc(&m_outputDuplDesc);
            {
                auto c = m_outputDesc.DesktopCoordinates;
                std::cerr << c.left << ' ' << c.top << ' ' << c.right << ' ' << c.bottom << '\n';
            }
        }

        void CreateCPUBuffer()
        {
            D3D11_TEXTURE2D_DESC desc;
            desc.Width = m_outputDuplDesc.ModeDesc.Width;
            desc.Height = m_outputDuplDesc.ModeDesc.Height;
            desc.Format = m_outputDuplDesc.ModeDesc.Format;
            desc.ArraySize = 1;
            desc.BindFlags = 0;
            desc.MiscFlags = 0;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.MipLevels = 1;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
            desc.Usage = D3D11_USAGE_STAGING;

            CheckResult(m_device->CreateTexture2D(&desc, NULL, &m_destImage));
        }

        void CreateGDIBuffer()
        {
            D3D11_TEXTURE2D_DESC desc;
            desc.Width = m_outputDuplDesc.ModeDesc.Width;
            desc.Height = m_outputDuplDesc.ModeDesc.Height;
            desc.Format = m_outputDuplDesc.ModeDesc.Format;
            desc.ArraySize = 1;
            desc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET;
            desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.MipLevels = 1;
            desc.CPUAccessFlags = 0;
            desc.Usage = D3D11_USAGE_DEFAULT;

            CheckResult(m_device->CreateTexture2D(&desc, NULL, &m_gdiImage));
        }

    public:

        /*
         * Capture the current screen contents and copy the output to the GDI buffer.
         */
        void GrabImage()
        {
            CheckResult(m_outputDuplication->AcquireNextFrame(INFINITE, &m_frameInfo, &m_desktopResource));
            CheckResult(m_desktopResource->QueryInterface(IID_PPV_ARGS(&m_desktopImage)));
            SafeRelease(m_desktopResource);
            m_immediateContext->CopyResource(m_gdiImage, m_desktopImage);
            CheckResult(m_outputDuplication->ReleaseFrame());
        }

        /*
         * Draws the currently active cursor on the image in the GDI buffer.
         */
        void DrawCursor()
        {
            IDXGISurface1* dxgiSurface1;
            HDC hdc;
            CheckResult(m_gdiImage->QueryInterface(IID_PPV_ARGS(&dxgiSurface1)));
            CheckResult(dxgiSurface1->GetDC(FALSE, &hdc));

            CURSORINFO info;
            info.cbSize = sizeof(info);
            if (GetCursorInfo(&info))
            {
                if (info.flags == CURSOR_SHOWING)
                {
                    LONG cursorX = info.ptScreenPos.x - m_outputDesc.DesktopCoordinates.left;
                    LONG cursorY = info.ptScreenPos.y - m_outputDesc.DesktopCoordinates.top;
                    // TODO remove cerr printout
                    std::cerr << "Cursor: " << cursorX << ' ' << cursorY << '\n';
                    DrawIconEx(hdc, cursorX, cursorY, info.hCursor, 0, 0, 0, 0, DI_NORMAL | DI_DEFAULTSIZE);
                }
            }

            CheckResult(dxgiSurface1->ReleaseDC(nullptr));
            SafeRelease(dxgiSurface1);
        }

        /*
         * Call the supplied callback function with the last captured image as the argument.
         */
        void OutputImage()
        {
            m_immediateContext->CopyResource(m_destImage, m_gdiImage);
            D3D11_MAPPED_SUBRESOURCE resource;
            UINT subresource = D3D11CalcSubresource(0, 0, 0);
            CheckResult(m_immediateContext->Map(m_destImage, subresource, D3D11_MAP_READ_WRITE, 0, &resource));

            UINT bytes = resource.DepthPitch;
            BYTE* raw = reinterpret_cast<BYTE*>(resource.pData);

            ImageData img(resource.RowPitch / 4, bytes / resource.RowPitch);

            std::copy(raw, raw + bytes, img.buffer.data());

            auto elapsedTimeNanoSeconds = (std::chrono::steady_clock::now() - m_creationTime).count();
            m_frameCallback(img, elapsedTimeNanoSeconds);

            m_immediateContext->Unmap(m_destImage, subresource);
        }

        ScreenRecorder(FrameFunc frameCallback) : m_frameCallback(frameCallback)
        {
            m_creationTime = std::chrono::steady_clock::now();
            InitDevices();
            CreateCPUBuffer();
            CreateGDIBuffer();
        }
    };
}
