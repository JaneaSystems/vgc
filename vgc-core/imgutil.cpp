#include <Wincodec.h>
#include <functional>

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

namespace vgc::imgutil
{
    HRESULT SaveImageAsPngW(ImageData img, LPCWSTR path)
    {
        if (!img.buffer.size() || !path)
        {
            return E_INVALIDARG;
        }

        HRESULT hr = S_OK;
        IWICImagingFactory* factory = nullptr;
        IWICBitmapEncoder* encoder = nullptr;
        IWICBitmapFrameEncode* frame = nullptr;
        IWICStream* stream = nullptr;
        GUID pixelFormat = GUID_WICPixelFormat32bppPBGRA;

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
            return hr;
        }

        SafeRelease(frame);
        SafeRelease(encoder);
        SafeRelease(stream);
        SafeRelease(factory);

        return S_OK;
    }
}

