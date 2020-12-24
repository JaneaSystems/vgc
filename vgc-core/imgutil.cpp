#include "pch.h"
#include "imgutil.h"

namespace
{
    void CheckResult(HRESULT hr)
    {
        if (!SUCCEEDED(hr))
        {
            throw hr;
        }
    }

    template<class T>
    void SafeRelease(T& obj)
    {
        if (obj)
        {
            obj->Release();
            obj = nullptr;
        }
    }
}

namespace vgc
{
    HRESULT SaveImageAsPngFileW(ImageData& img, LPCWSTR path)
    {
        if (!img.buffer.size() || !path)
        {
            return E_INVALIDARG;
        }

        IWICImagingFactory* factory = nullptr;
        IWICBitmapEncoder* encoder = nullptr;
        IWICBitmapFrameEncode* frame = nullptr;
        IWICStream* stream = nullptr;
        GUID pixelFormat = GUID_WICPixelFormat32bppBGRA;
        HRESULT result = S_OK;

        try
        {
            CheckResult(CoInitialize(NULL));
            CheckResult(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)));
            CheckResult(factory->CreateStream(&stream));
            CheckResult(stream->InitializeFromFilename(path, GENERIC_WRITE));
            CheckResult(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder));
            CheckResult(encoder->Initialize(stream, WICBitmapEncoderNoCache));
            CheckResult(encoder->CreateNewFrame(&frame, nullptr));
            CheckResult(frame->Initialize(nullptr));
            CheckResult(frame->SetSize(img.width, img.height));
            CheckResult(frame->SetPixelFormat(&pixelFormat));
            CheckResult(frame->WritePixels(img.height, img.width * 4, (UINT)img.buffer.size(), img.buffer.data()));
            CheckResult(frame->Commit());
            CheckResult(encoder->Commit());
        }
        catch (HRESULT hr)
        {
            result = hr;
        }

        SafeRelease(frame);
        SafeRelease(encoder);
        SafeRelease(stream);
        SafeRelease(factory);

        return result;
    }

    HRESULT LoadImageFromPngFileW(ImageData& img, LPCWSTR path)
    {
        if (!path)
        {
            return E_INVALIDARG;
        }

        IWICImagingFactory* factory = nullptr;
        IWICBitmapDecoder* decoder = nullptr;
        IWICBitmapFrameDecode* frame = nullptr;
        IWICStream* stream = nullptr;
        GUID pixelFormat;
        UINT width, height;
        HRESULT result = S_OK;

        try
        {
            CheckResult(CoInitialize(NULL));
            CheckResult(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)));
            CheckResult(factory->CreateStream(&stream));
            CheckResult(stream->InitializeFromFilename(path, GENERIC_READ));
            CheckResult(factory->CreateDecoder(GUID_ContainerFormatPng, nullptr, &decoder));
            CheckResult(decoder->Initialize(stream, WICDecodeMetadataCacheOnDemand));
            CheckResult(decoder->GetFrame(0, &frame));
            CheckResult(frame->GetSize(&width, &height));
            try
            {
                img = ImageData(width, height);
            }
            catch (std::bad_alloc)
            {
                throw E_OUTOFMEMORY;
            }

            CheckResult(frame->GetPixelFormat(&pixelFormat));
            // The image has to be saved using this pixel format
            if (pixelFormat != GUID_WICPixelFormat32bppBGRA)
            {
                return E_FAIL;
            }

            CheckResult(frame->CopyPixels(NULL, width * 4, width * height * 4, img.buffer.data()));
        }
        catch (HRESULT hr)
        {
            result = hr;
        }

        SafeRelease(frame);
        SafeRelease(decoder);
        SafeRelease(stream);
        SafeRelease(factory);

        return result;
    }

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

