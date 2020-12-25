#pragma once

#include "pch.h"

#pragma comment(lib, "d3d11")

namespace vgc
{
    // TODO document code below.

    class ScreenRecorder
    {
        static inline const D3D_FEATURE_LEVEL s_featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_0
        };

        UINT m_monitorIndex;

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
        unsigned long long m_lastFrameTime;

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

            CheckResult(m_device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
            CheckResult(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&dxgiAdapter)));
            SafeRelease(dxgiDevice);

            CheckResult(dxgiAdapter->EnumOutputs(m_monitorIndex, &dxgiOutput));
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
            SafeRelease(m_desktopImage);
            CheckResult(m_outputDuplication->AcquireNextFrame(INFINITE, &m_frameInfo, &m_desktopResource));
            CheckResult(m_desktopResource->QueryInterface(IID_PPV_ARGS(&m_desktopImage)));
            SafeRelease(m_desktopResource);
            m_immediateContext->CopyResource(m_gdiImage, m_desktopImage);
            CheckResult(m_outputDuplication->ReleaseFrame());
            m_lastFrameTime = (std::chrono::steady_clock::now() - m_creationTime).count();
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
         * Returns the captured image in the GDI buffer
         */
        ImageData OutputImage()
        {
            m_immediateContext->CopyResource(m_destImage, m_gdiImage);
            D3D11_MAPPED_SUBRESOURCE resource;
            UINT subresource = D3D11CalcSubresource(0, 0, 0);
            CheckResult(m_immediateContext->Map(m_destImage, subresource, D3D11_MAP_READ_WRITE, 0, &resource));

            UINT bytes = resource.DepthPitch;
            BYTE* raw = reinterpret_cast<BYTE*>(resource.pData);

            ImageData img(resource.RowPitch / 4, bytes / resource.RowPitch);

            std::copy(raw, raw + bytes, img.buffer.data());

            m_immediateContext->Unmap(m_destImage, subresource);
            return img;
        }

        /*
         * Returns the time when the last frame was captured,
         * in nanoseconds since the ScreenRecorder was created.
         */
        unsigned long long GetLastFrameTime()
        {
            return m_lastFrameTime;
        }

        /*
         * Copies the specified subregion of the GDI buffer to the given texture resource at the given coordinates.
         * If the requested capture area goes beyond the edges of the captured screen, it will be shrunk to fit,
         * and only the shrunk area will be copied.
         */
        void OutputSubregion(ID3D11Texture2D* dest, RECT capture, LONG destX, LONG destY)
        {
            D3D11_BOX region;
            capture.left = std::max(capture.left, m_outputDesc.DesktopCoordinates.left);
            capture.right = std::min(capture.right, m_outputDesc.DesktopCoordinates.right);
            capture.top = std::max(capture.top, m_outputDesc.DesktopCoordinates.top);
            capture.bottom = std::min(capture.bottom, m_outputDesc.DesktopCoordinates.bottom);

            region.left = capture.left - m_outputDesc.DesktopCoordinates.left;
            region.right = capture.right - m_outputDesc.DesktopCoordinates.left;
            region.top = capture.top - m_outputDesc.DesktopCoordinates.top;
            region.bottom = capture.bottom - m_outputDesc.DesktopCoordinates.top;
            region.front = 0;
            region.back = 1;
            m_immediateContext->CopySubresourceRegion(dest, 0, destX, destY, 0, m_gdiImage, 0, &region);
        }

        ScreenRecorder(UINT monitorIndex) : m_monitorIndex(monitorIndex)
        {
            m_creationTime = std::chrono::steady_clock::now();
            InitDevices();
            CreateCPUBuffer();
            CreateGDIBuffer();
        }

        ~ScreenRecorder()
        {
            SafeRelease(m_desktopImage);
            SafeRelease(m_gdiImage);
            SafeRelease(m_destImage);
            SafeRelease(m_desktopResource);
            SafeRelease(m_outputDuplication);
            SafeRelease(m_immediateContext);
            SafeRelease(m_device);
        }
    };
}
