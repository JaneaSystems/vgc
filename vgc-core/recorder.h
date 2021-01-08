#pragma once

#include "pch.h"
#include "image-data.h"
#include "screen-capture.h"
#include "png.h"
#include "gif.h"

namespace vgc
{
	class PrimaryScreenRecorder
	{
		enum RecordingState
		{
			Idle,
			Recording,
			Stopped
		};

		RECT m_area;
		ScreenCapture m_screenCapture;
		RecordingState m_state;
		Timestamp m_recordingStartTime;
		double m_fpsLimit;
		ID3D11Texture2D* m_texture;
		Timestamp m_stopTime;

		std::mutex m_mutex;
		std::thread m_worker;
		std::condition_variable m_cv;

		std::vector<std::future<std::wstring>> m_persistFileNames;
		std::vector<Timestamp> m_frameTimestamps;

		void PersistImage(ImageData& image);
		void SaveFrame();
		void Worker();

	public:
		PrimaryScreenRecorder(RECT area, double fpsLimit = 50);
		void Start();
		void Stop();
		void ExportToGif(LPCWSTR filePath);
		~PrimaryScreenRecorder();
	};
}