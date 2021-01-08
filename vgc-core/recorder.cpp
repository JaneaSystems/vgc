#include "recorder.h"

namespace vgc
{
	void PrimaryScreenRecorder::PersistImage(ImageData& image)
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

		m_persistFileNames.push_back(std::async(task));
	}

	void PrimaryScreenRecorder::SaveFrame()
	{
		m_screenCapture.DrawCursor();
		m_screenCapture.OutputSubregion(m_texture, m_area, 0, 0);
		auto image = D3D11::TextureToImage(m_texture);
		std::cerr << "Saving frame " << m_frameTimestamps.size() << '\n';
		PersistImage(image);
		m_frameTimestamps.emplace_back(m_screenCapture.GetLastFrameTime());
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

			if (m_frameTimestamps.empty())
			{
				// First frame
				m_recordingStartTime = m_screenCapture.GetLastFrameTime();
				SaveFrame();
			}
			else
			{
				double minimumFrameTimestamp = m_frameTimestamps.size() * 1e9 / m_fpsLimit + m_recordingStartTime;
				auto lastFrameTime = m_screenCapture.GetLastFrameTime();
				if (lastFrameTime >= minimumFrameTimestamp)
				{
					SaveFrame();
				}
			}
		}
	}

	PrimaryScreenRecorder::PrimaryScreenRecorder(RECT area, double fpsLimit) :
		m_area(area),
		m_screenCapture(0),
		m_state(Idle),
		m_recordingStartTime(-1),
		m_fpsLimit(fpsLimit),
		m_stopTime(0)
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

			std::unique_lock lock(m_mutex);
			m_stopTime = m_screenCapture.GetTime();
		}
	}

	void PrimaryScreenRecorder::ExportToGif(LPCWSTR filePath)
	{
		std::unique_lock lock(m_mutex);
		m_cv.wait(lock, [&]() { return m_state == Stopped; });

		SimpleGifEncoder<SimpleQuantizer> gif(filePath, m_area.right - m_area.left, m_area.bottom - m_area.top);

		auto delays = TimestampsToGifDelays(m_frameTimestamps, m_stopTime);

		for (size_t i = 0; i < m_persistFileNames.size(); i++)
		{
			auto fileName = m_persistFileNames[i].get();
			if (delays[i] > 0)
			{
				ImageData img(0, 0);
				auto image = LoadImageFromPngFileW(img, fileName.c_str());
				std::wcerr << i << L" -> " << fileName << L"\n";
				gif.AddFrame(img, delays[i]);
			}
			else
			{
				std::cerr << "Skipped frame " << i << '\n';
			}
			
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