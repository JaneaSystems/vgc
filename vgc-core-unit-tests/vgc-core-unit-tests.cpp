#include "pch.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace vgc::imgutil;

namespace vgccoreunittests
{
	TEST_CLASS(vgccoreunittests)
	{
	public:
		
		TEST_METHOD(TestSaveImageAsPngW)
		{
			ImageData img;
			img.width = 800;
			img.height = 600;
			std::vector<BYTE> buffer(img.BufferSize());
			img.data = buffer.data();

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

			HRESULT hr = SaveImageAsPngW(img, L"img.png");
			Assert::IsTrue(hr == S_OK);
		}

		TEST_METHOD(TestSaveImageAsPngMultithread)
		{
			auto threadCount = std::thread::hardware_concurrency();
			std::vector<std::thread> threads(threadCount);
			std::vector<HRESULT> results(threadCount);

			for (UINT i = 0; i < threadCount; i++)
			{
				auto task = [&](size_t taskId)
				{
					ImageData img;
					img.width = 800;
					img.height = 600;
					std::vector<BYTE> buffer(img.BufferSize());
					img.data = buffer.data();

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

					results[taskId] = SaveImageAsPngW(img, fileName.c_str());
				};

				threads[i] = std::thread(task, i);
			}

			for (UINT i = 0; i < threadCount; i++)
			{
				threads[i].join();
			}
			
			for (UINT i = 0; i < threadCount; i++)
			{
				Assert::IsTrue(results[i] == S_OK);
			}
		}
	};
}
