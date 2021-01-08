#include "recorder.h"

namespace vgc
{
	void PrimaryScreenRecorder::PersistImage(int frameIdx, ImageData& image)
	{
		// This was done to avoid copies of ImageData
		// Very explicit lifetime control is needed.
		ImageData* imageLocal = new ImageData(0, 0);
		std::swap(image, *imageLocal);

		auto task = [=]()
		{
			WCHAR pathBuffer[MAX_PATH];
			WCHAR fileNameBuffer[MAX_PATH];
			GetTempPathW(MAX_PATH, pathBuffer);
			UINT status = GetTempFileNameW(pathBuffer, L"vgc", 0, fileNameBuffer);

			if (status == 0)
			{
				delete imageLocal;
				return std::wstring();
			}

			SaveImageAsPngFileW(*imageLocal, fileNameBuffer);
			delete imageLocal;

			return std::wstring(fileNameBuffer);
		};

		m_persistFileNames[frameIdx] = std::async(task);
	}

	void PrimaryScreenRecorder::SaveFrame()
	{
		m_screenCapture.DrawCursor();
		m_screenCapture.OutputSubregion(m_texture, m_area, 0, 0);
		auto image = D3D11::TextureToImage(m_texture);
		std::cerr << "Saving frame " << m_framesCaptured << '\n';
		PersistImage(m_framesCaptured, image);
		m_framesCaptured += 1;
	}

	void PrimaryScreenRecorder::Worker()
	{
		while (1)
		{
			std::unique_lock lock(m_mutex);
			m_cv.wait(lock, [&]() { return m_state != Idle; });

			if (m_state == Stopped)
			{
				return;
			}

			m_screenCapture.GrabImage();

			if (m_framesCaptured == 0)
			{
				// First frame
				m_recordingStartTime = m_screenCapture.GetLastFrameTime();
				SaveFrame();
			}
			else
			{
				double minimumFrameTimestamp = m_framesCaptured * 1e9 / m_fpsLimit + m_recordingStartTime;
				if (m_screenCapture.GetLastFrameTime() >= minimumFrameTimestamp)
				{
					SaveFrame();
				}
			}
		}
	}

	PrimaryScreenRecorder::PrimaryScreenRecorder(RECT area, double fpsLimit) :
		m_area(area),
		m_framesCaptured(0),
		m_screenCapture(0),
		m_state(Idle),
		m_recordingStartTime(-1),
		m_fpsLimit(fpsLimit)
	{
		m_texture = D3D11::CreateCPUTexture(area.right - area.left, area.bottom - area.top, m_screenCapture.GetPixelFormat());
		m_worker = std::thread([&]() { Worker(); });
	}

	void PrimaryScreenRecorder::Start()
	{
		{
			std::unique_lock lock(m_mutex);
			m_state = Recording;
		}
		m_cv.notify_all();
	}

	void PrimaryScreenRecorder::Stop()
	{
		if (m_state == Recording)
		{
			m_state = Stopped;
		}
	}

	void PrimaryScreenRecorder::ExportToGif(LPCWSTR filePath)
	{
		std::unique_lock lock(m_mutex);
		m_cv.wait(lock, [&]() { return m_state == Stopped; });

		SimpleGifEncoder<SimpleQuantizer> gif(filePath, m_area.right - m_area.left, m_area.bottom - m_area.top);

		for (auto& [idx, future] : m_persistFileNames)
		{
			auto fileName = future.get();
			ImageData img(0, 0);
			auto image = LoadImageFromPngFileW(img, fileName.c_str());
			std::wcerr << idx << L" -> " << fileName << L"\n";
			gif.AddFrame(img, 2);
			DeleteFileW(fileName.c_str());
		}
	}

	PrimaryScreenRecorder::~PrimaryScreenRecorder()
	{
		std::unique_lock lock(m_mutex);
		m_cv.wait(lock, [&]() { return m_state == Stopped; });
		m_worker.join();
		SafeRelease(m_texture);
	}
}