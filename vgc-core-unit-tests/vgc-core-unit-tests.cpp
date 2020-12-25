#include "pch.h"
#include "../vgc-core/png.h"
#include "../vgc-core/gif.h"
#include "../vgc-core/quantization.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace vgc;

namespace vgccoreunittests
{
    TEST_CLASS(vgccoreunittests)
    {
    public:
        
        // TODO: Cleanup created file
        TEST_METHOD(TestSaveImageAsPng)
        {
            ImageData img(800, 600);

            for (UINT i = 0; i < img.height; i++)
            {
                for (UINT j = 0; j < img.width; j++)
                {
                    img[i][4 * j + 0] = i * 255 / img.height;
                    img[i][4 * j + 1] = j * 255 / img.width;
                    img[i][4 * j + 2] = 0;
                    img[i][4 * j + 3] = 255;
                }
            }

            HRESULT hr = SaveImageAsPngFileW(img, L"img.png");
            Assert::IsTrue(SUCCEEDED(hr));
            Assert::IsTrue(hr == S_OK);
        }

        // TODO: Cleanup created files
        TEST_METHOD(TestSaveImageAsPngMultithread)
        {
            auto threadCount = std::thread::hardware_concurrency();
            std::vector<std::thread> threads(threadCount);
            std::vector<HRESULT> results(threadCount);

            for (UINT i = 0; i < threadCount; i++)
            {
                auto task = [&](size_t taskId)
                {
                    ImageData img(800, 600);

                    for (UINT i = 0; i < img.height; i++)
                    {
                        for (UINT j = 0; j < img.width; j++)
                        {
                            img[i][4 * j + 0] = i * 255 / img.height;
                            img[i][4 * j + 1] = j * 255 / img.width;
                            img[i][4 * j + 2] = 0;
                            img[i][4 * j + 3] = 255;
                        }
                    }

                    std::wstring fileName = L"img";
                    fileName += std::to_wstring(taskId);
                    fileName += L".png";

                    results[taskId] = SaveImageAsPngFileW(img, fileName.c_str());
                };

                threads[i] = std::thread(task, i);
            }

            for (UINT i = 0; i < threadCount; i++)
            {
                threads[i].join();
            }
            
            for (UINT i = 0; i < threadCount; i++)
            {
                Assert::IsTrue(SUCCEEDED(results[i]));
            }
        }

        // TODO: Cleanup created file
        TEST_METHOD(TestLoadSaveImageLossless)
        {
            // Rolling hash base
            const UINT base = 498731971;
            ImageData img(799, 599);

            std::random_device randomDevice;
            std::uniform_int_distribution<UINT> distibution(0, 255);

            UINT hashedBefore = 0;
            UINT hashedAfter = 0;

            for (BYTE& byte : img.buffer)
            {
                byte = distibution(randomDevice);
                hashedBefore = base * hashedBefore + byte;
            }

            HRESULT hr = SaveImageAsPngFileW(img, L"img.png");
            Assert::IsTrue(SUCCEEDED(hr));

            img = ImageData(799, 599);

            hr = LoadImageFromPngFileW(img, L"img.png");
            Assert::IsTrue(SUCCEEDED(hr));

            for (BYTE& byte : img.buffer)
            {
                hashedAfter = base * hashedAfter + byte;
            }

            Assert::AreEqual(hashedBefore, hashedAfter);
        }

        // TODO: Cleanup created file
        TEST_METHOD(TestSaveImageAsGifTiny)
        {
            ImageData img(3, 2);

            for (UINT i = 0; i < img.height; i++)
            {
                for (UINT j = 0; j < img.width; j++)
                {
                    img[i][4 * j + 0] = i * 255 / img.height;
                    img[i][4 * j + 1] = j * 255 / img.width;
                    img[i][4 * j + 2] = 0;
                    img[i][4 * j + 3] = 255;
                }
            }

            SimpleGifEncoder<SimpleQuantizer> gifImg(L"img-tiny.gif", img.width, img.height);
            gifImg.AddFrame(img, 100);
        }

        // TODO: Cleanup created file
        TEST_METHOD(TestSaveImageAsGif)
        {
            ImageData img(799, 430);

            SimpleGifEncoder<SimpleQuantizer> gifImg(L"img.gif", img.width, img.height);

            for (UINT f = 0; f < 100; f++)
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
    };
}
