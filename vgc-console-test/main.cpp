#include "../vgc-core/imgutil.h"
#include <ShellScalingApi.h>
using namespace std;
using namespace vgc::imgutil;

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

    ScreenRecorder rec([](auto arg) {});
}

void test_run2()
{
    const int w = 2560, h = 1440;
    SimpleGifEncoder<SimpleQuantizer> gifImg(L"img.gif", w, h);

    ScreenRecorder rec([&](ImageData& img) {
        gifImg.AddFrame(img, 3);
    });

    for (int i = 0; i < 100; i++)
    {
        rec.GrabImage();
        rec.DrawCursor();
        rec.OutputImage();
    }
}

int main()
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    test_run2();
}
