#include "png.h"

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
}
