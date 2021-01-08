#include "../vgc-core/gif.h"
#include "../vgc-core/recorder.h"
#include "../vgc-core/quantization.h"
using namespace std;
using namespace vgc;

void test_run1()
{
    ImageData img(2560, 1440);

    SimpleGifEncoder<SimpleQuantizer> gifImg(L"img.gif", img.width, img.height);

    for (UINT f = 0; f < 256; f++)
    {
        for (UINT i = 0; i < img.height; i++)
        {
            for (UINT j = 0; j < img.width; j++)
            {
                BYTE b = i * 255 / img.height;
                BYTE g = j * 255 / img.width;
                BYTE r = 0;
                BYTE a = 255;

                g ^= b ^ f;

                img[i][4 * j + 0] = b;
                img[i][4 * j + 1] = g;
                img[i][4 * j + 2] = r;
                img[i][4 * j + 3] = a;
            }
        }

        gifImg.AddFrame(img, 2);
    }

    ScreenCapture rec(0);
}

void test_run2()
{
    const int w = 2560, h = 1440;
    SimpleGifEncoder<SimpleQuantizer> gifImg(L"img.gif", w, h);

    unsigned long long lastFrameTime = 0;

    ScreenCapture rec(0);

    using namespace std::chrono;

    for (auto t = steady_clock::now(); (steady_clock::now() - t).count() < 5'000'000'000ull;)
    {
        rec.GrabImage();
        rec.DrawCursor();
        auto img = rec.OutputImage();
        auto elapsedTime = rec.GetLastFrameTime();

        auto frameTime = (elapsedTime - lastFrameTime) / 10'000'000;
        if (frameTime < 2)
        {
            continue;
        }

        lastFrameTime += frameTime * 10'000'000;
        gifImg.AddFrame(img, (USHORT)frameTime);
    }
}

void test_run3()
{
    const int w = 400, h = 800;
    SimpleGifEncoder<SimpleQuantizer> gifImg(L"img.gif", w, h);

    unsigned long long lastFrameTime = 0;

    ScreenCapture rec(0);

    auto texture = D3D11::CreateCPUTexture(w, h, DXGI_FORMAT_B8G8R8A8_UNORM);

    using namespace std::chrono;

    for (auto t = steady_clock::now(); (steady_clock::now() - t).count() < 5'000'000'000ull;)
    {
        rec.GrabImage();
        rec.DrawCursor();
        rec.OutputSubregion(texture, RECT{ .left = 100, .top = 100, .right = 100 + w, .bottom = 100 + h }, 0, 0);
        auto elapsedTime = rec.GetLastFrameTime();

        auto frameTime = (elapsedTime - lastFrameTime) / 10'000'000;
        if (frameTime < 2)
        {
            continue;
        }

        lastFrameTime += frameTime * 10'000'000;
        gifImg.AddFrame(D3D11::TextureToImage(texture), (USHORT)frameTime);
    }
}

void test_run4()
{
    // An example of how to record multiple screens.

    const int w = 4920, h = 1240, left = 100, top = 100;
    SimpleGifEncoder<SimpleQuantizer> gifImg(L"img.gif", w, h);

    unsigned long long lastFrameTime = 0;

    ScreenCapture rec0(0);
    ScreenCapture rec1(1);

    auto texture = D3D11::CreateCPUTexture(w, h, DXGI_FORMAT_B8G8R8A8_UNORM);

    using namespace std::chrono;

    for (auto t = steady_clock::now(); (steady_clock::now() - t).count() < 5'000'000'000ull;)
    {
        rec0.GrabImage();
        rec1.GrabImage();
        rec0.DrawCursor();
        rec1.DrawCursor();

        rec0.OutputSubregion(texture, RECT{ .left = left, .top = top, .right = left + w, .bottom = top + h }, 0, 0);
        rec1.OutputSubregion(texture, RECT{ .left = left, .top = top, .right = left + w, .bottom = top + h }, 0, 0);

        auto elapsedTime = rec0.GetLastFrameTime();

        auto frameTime = (elapsedTime - lastFrameTime) / 10'000'000;
        if (frameTime < 2)
        {
            continue;
        }

        lastFrameTime += frameTime * 10'000'000;
        gifImg.AddFrame(D3D11::TextureToImage(texture), (USHORT)frameTime);
    }

    SafeRelease(texture);
}

void test_run5()
{
    // Test run of the recorder
    const int w = 1280, h = 720, left = 1280, top = 720;

    PrimaryScreenRecorder recorder(RECT{ .left = left, .top = top, .right = left + w, .bottom = top + h });

    recorder.Start();
    Sleep(25000);
    recorder.Stop();
    recorder.ExportToGif(L"img.gif");
    Sleep(1000);
}

int main()
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    test_run5();
}
