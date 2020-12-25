#include "../vgc-core/imgutil.h"
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

    ScreenRecorder rec(0);
}

void test_run2()
{
    const int w = 2560, h = 1440;
    SimpleGifEncoder<SimpleQuantizer> gifImg(L"img.gif", w, h);

    unsigned long long lastFrameTime = 0;

    ScreenRecorder rec(0);

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

int main()
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    test_run2();
    while (1);
}
