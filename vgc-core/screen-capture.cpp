#include "screen-capture.h"

namespace vgc {
    D3D11 D3D11::s_instance;

    const D3D_FEATURE_LEVEL D3D11::s_featureLevels[1] =
    {
        D3D_FEATURE_LEVEL_11_0
    };

    D3D11::D3D11()
    {
        m_device = nullptr;
        m_deviceContext = nullptr;
        CheckResult(D3D11CreateDevice(
            NULL,
            D3D_DRIVER_TYPE_HARDWARE,
            NULL,
            0,
            s_featureLevels,
            (UINT)std::size(s_featureLevels),
            D3D11_SDK_VERSION,
            &m_device,
            NULL,
            &m_deviceContext));
    }

    ID3D11Device* D3D11::Device()
    {
        return s_instance.m_device;
    }

    ID3D11DeviceContext* D3D11::DC()
    {
        return s_instance.m_deviceContext;
    }

    D3D11::~D3D11()
    {
        SafeRelease(m_deviceContext);
        SafeRelease(m_device);
    }

    void ScreenCapture::InitDevices()
    {
        IDXGIDevice* dxgiDevice;
        IDXGIAdapter* dxgiAdapter;
        IDXGIOutput* dxgiOutput;
        IDXGIOutput1* dxgiOutput1;

        auto device = D3D11::Device();

        CheckResult(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
        CheckResult(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&dxgiAdapter)));
        SafeRelease(dxgiDevice);

        CheckResult(dxgiAdapter->EnumOutputs(m_monitorIndex, &dxgiOutput));
        SafeRelease(dxgiAdapter);

        CheckResult(dxgiOutput->GetDesc(&m_outputDesc));
        CheckResult(dxgiOutput->QueryInterface(IID_PPV_ARGS(&dxgiOutput1)));
        SafeRelease(dxgiOutput);

        CheckResult(dxgiOutput1->DuplicateOutput(device, &m_outputDuplication));
        SafeRelease(dxgiOutput1);

        m_outputDuplication->GetDesc(&m_outputDuplDesc);
        {
            auto c = m_outputDesc.DesktopCoordinates;
            std::cerr << c.left << ' ' << c.top << ' ' << c.right << ' ' << c.bottom << '\n';
        }
    }

    void ScreenCapture::CreateCPUBuffer()
    {
        m_destImage = D3D11::CreateCPUTexture(m_outputDuplDesc.ModeDesc.Width, m_outputDuplDesc.ModeDesc.Height, m_outputDuplDesc.ModeDesc.Format);
    }

    void ScreenCapture::CreateGDIBuffer()
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

        CheckResult(D3D11::Device()->CreateTexture2D(&desc, NULL, &m_gdiImage));
    }

    Timestamp ScreenCapture::GetTime()
    {
        return (std::chrono::steady_clock::now() - m_creationTime).count();
    }

    void ScreenCapture::GrabImage()
    {
        SafeRelease(m_desktopImage);
        CheckResult(m_outputDuplication->AcquireNextFrame(INFINITE, &m_frameInfo, &m_desktopResource));
        CheckResult(m_desktopResource->QueryInterface(IID_PPV_ARGS(&m_desktopImage)));
        SafeRelease(m_desktopResource);
        D3D11::DC()->CopyResource(m_gdiImage, m_desktopImage);
        CheckResult(m_outputDuplication->ReleaseFrame());
        m_lastFrameTime = GetTime();
    }

    void ScreenCapture::DrawCursor()
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

    ImageData ScreenCapture::OutputImage()
    {
        // Using the CPU enabled m_destImage here to avoid creating/deleting CPU texture buffers in TextureToImage
        D3D11::DC()->CopyResource(m_destImage, m_gdiImage);
        return D3D11::TextureToImage(m_gdiImage);
    }

    Timestamp ScreenCapture::GetLastFrameTime()
    {
        return m_lastFrameTime;
    }

    void ScreenCapture::OutputSubregion(ID3D11Texture2D* dest, RECT capture, LONG destX, LONG destY)
    {
        D3D11_BOX region;

        if (m_outputDesc.DesktopCoordinates.left > capture.left)
        {
            destX += m_outputDesc.DesktopCoordinates.left - capture.left;
            capture.left = m_outputDesc.DesktopCoordinates.left;
        }

        if (m_outputDesc.DesktopCoordinates.top > capture.top)
        {
            destY += m_outputDesc.DesktopCoordinates.top - capture.top;
            capture.top = m_outputDesc.DesktopCoordinates.top;
        }

        capture.right = std::min(capture.right, m_outputDesc.DesktopCoordinates.right);
        capture.bottom = std::min(capture.bottom, m_outputDesc.DesktopCoordinates.bottom);

        region.left = capture.left - m_outputDesc.DesktopCoordinates.left;
        region.right = capture.right - m_outputDesc.DesktopCoordinates.left;
        region.top = capture.top - m_outputDesc.DesktopCoordinates.top;
        region.bottom = capture.bottom - m_outputDesc.DesktopCoordinates.top;
        region.front = 0;
        region.back = 1;
        D3D11::DC()->CopySubresourceRegion(dest, 0, destX, destY, 0, m_gdiImage, 0, &region);
    }

    DXGI_FORMAT ScreenCapture::GetPixelFormat()
    {
        return m_outputDuplDesc.ModeDesc.Format;
    }

    ScreenCapture::ScreenCapture(UINT monitorIndex) : m_monitorIndex(monitorIndex)
    {
        m_creationTime = std::chrono::steady_clock::now();
        InitDevices();
        CreateCPUBuffer();
        CreateGDIBuffer();
    }

    ScreenCapture::~ScreenCapture()
    {
        SafeRelease(m_desktopImage);
        SafeRelease(m_gdiImage);
        SafeRelease(m_destImage);
        SafeRelease(m_desktopResource);
        SafeRelease(m_outputDuplication);
    }

    ImageData D3D11::TextureToImage(ID3D11Texture2D* texture)
    {
        ID3D11Texture2D* textureCPU = nullptr;
        D3D11_TEXTURE2D_DESC desc;

        texture->GetDesc(&desc);

        D3D11_MAPPED_SUBRESOURCE resource;
        HRESULT hr = DC()->Map(texture, 0, D3D11_MAP_READ_WRITE, 0, &resource);
        bool failed = FAILED(hr);

        if (failed)
        {
            // Try again, first copying the texture to a CPU enabled one.
            D3D11_TEXTURE2D_DESC textureDesc;
            texture->GetDesc(&textureDesc);
            textureCPU = CreateCPUTexture(textureDesc.Width, textureDesc.Height, textureDesc.Format);
            DC()->CopyResource(textureCPU, texture);
            hr = DC()->Map(textureCPU, 0, D3D11_MAP_READ_WRITE, 0, &resource);

            if (FAILED(hr))
            {
                // Something's failing, give up
                SafeRelease(textureCPU);
                throw hr;
            }
        }

        UINT bytes = resource.DepthPitch;
        BYTE* raw = reinterpret_cast<BYTE*>(resource.pData);

        ImageData img(desc.Width, desc.Height);

        for (size_t i = 0; i < desc.Height; i++)
        {
            auto base = raw + i * resource.RowPitch;
            std::copy(base, base + 4ull * desc.Width, img[i]);
        }

        if (failed)
        {
            DC()->Unmap(textureCPU, 0);
            SafeRelease(textureCPU);
        }
        else
        {
            DC()->Unmap(texture, 0);
        }
        return img;
    }

    ID3D11Texture2D* D3D11::CreateCPUTexture(UINT width, UINT height, DXGI_FORMAT format)
    {
        ID3D11Texture2D* texture;
        D3D11_TEXTURE2D_DESC desc;

        desc.Width = width;
        desc.Height = height;
        desc.Format = format;
        desc.ArraySize = 1;
        desc.BindFlags = 0;
        desc.MiscFlags = 0;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.MipLevels = 1;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
        desc.Usage = D3D11_USAGE_STAGING;

        CheckResult(D3D11::Device()->CreateTexture2D(&desc, NULL, &texture));
        return texture;
    }
}
