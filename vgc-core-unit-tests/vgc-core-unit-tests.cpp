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
	};
}
