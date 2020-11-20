#include <Wincodec.h>
#include <functional>

#include "imgutil.h"

namespace
{
	template<class T>
	void CheckResult(HRESULT& hr, T task)
	{
		if (hr == S_OK)
		{
			hr = task();
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
		if (!img.data || !path)
		{
			return E_INVALIDARG;
		}

		HRESULT hr = S_OK;
		IWICImagingFactory* factory = nullptr;
		IWICBitmapEncoder* encoder = nullptr;
		IWICBitmapFrameEncode* frame = nullptr;
		IWICStream* stream = nullptr;
		GUID pixelFormat = GUID_WICPixelFormat32bppPBGRA;

		hr = CoInitialize(NULL);
		// If S_FALSE was returned, the COM library is already initialized. This can happen in unit tests.
		BOOL coInit = hr == S_OK || hr == S_FALSE;
		hr = coInit ? S_OK : E_FAIL;
		
		CheckResult(hr, [&]() { return CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)); });
		CheckResult(hr, [&]() { return factory->CreateStream(&stream); });
		CheckResult(hr, [&]() { return stream->InitializeFromFilename(path, GENERIC_WRITE); });
		CheckResult(hr, [&]() { return factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder); });
		CheckResult(hr, [&]() { return encoder->Initialize(stream, WICBitmapEncoderNoCache); });
		CheckResult(hr, [&]() { return encoder->CreateNewFrame(&frame, nullptr); }); 
		CheckResult(hr, [&]() { return frame->Initialize(nullptr); });
		CheckResult(hr, [&]() { return frame->SetSize(img.width, img.height); });
		CheckResult(hr, [&]() { return frame->SetPixelFormat(&pixelFormat); });
		CheckResult(hr, [&]() { return frame->WritePixels(img.height, img.width * 4, (UINT)img.BufferSize(), img.data); });
		CheckResult(hr, [&]() { return frame->Commit(); });
		CheckResult(hr, [&]() { return encoder->Commit(); });

		SafeRelease(frame);
		SafeRelease(encoder);
		SafeRelease(stream);
		SafeRelease(factory);
		if (coInit)
		{
			CoUninitialize();
		}

		return hr;
	}
}

