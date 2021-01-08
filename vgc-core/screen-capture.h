#pragma once

#include "pch.h"
#include "image-data.h"

#pragma comment(lib, "d3d11")

namespace vgc
{
    // TODO document code below.

    class D3D11
    {
        static D3D11 s_instance;
        ID3D11Device* m_device;
        ID3D11DeviceContext* m_deviceContext;
        static const D3D_FEATURE_LEVEL s_featureLevels[1];

        D3D11();
    public:
        ~D3D11();

        /*
         * Returns the singleton Direct3D 11 Device.
         */
        static ID3D11Device* Device();

        /*
         * Returns the singleton Direct3D 11 Device Context.
         */
        static ID3D11DeviceContext* DC();

        /*
         * Converts the given Direct3D texture to an ImageData object. The texture doesn't need
         * to have CPU access enabled but if it doesn't, there could be a performance penalty.
         */
        static ImageData TextureToImage(ID3D11Texture2D* texture);

        /*
         * Creates a new Direct3D texture with given size.
         * You must free the returned texture after use by calling SafeRelease.
         */
        static ID3D11Texture2D* CreateCPUTexture(UINT width, UINT height, DXGI_FORMAT format);
    };

    class ScreenCapture
    {
        UINT m_monitorIndex;

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

        void InitDevices();
        void CreateCPUBuffer();
        void CreateGDIBuffer();
    public:

        /*
         * Capture the current screen contents and copy the output to the GDI buffer.
         */
        void GrabImage();

        /*
         * Draws the currently active cursor on the image in the GDI buffer.
         */
        void DrawCursor();

        /*
         * Returns the captured image in the GDI buffer
         */
        ImageData OutputImage();

        /*
         * Returns the time when the last frame was captured,
         * in nanoseconds since the ScreenCapture was created.
         */
        unsigned long long GetLastFrameTime();

        /*
         * Copies the specified subregion of the GDI buffer to the given texture resource at the given coordinates.
         * If the requested capture area goes beyond the edges of the captured screen, it will be shrunk to fit,
         * and only the shrunk area will be copied.
         */
        void OutputSubregion(ID3D11Texture2D* dest, RECT capture, LONG destX, LONG destY);

        /*
         * Returns the pixel format acquired by DirectX output duplication
         */
        DXGI_FORMAT GetPixelFormat();

        ScreenCapture(UINT monitorIndex);

        ~ScreenCapture();
    };
}
