#include "../vgc-core/imgutil.h"
using namespace std;
using namespace vgc::imgutil;

int main()
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
}