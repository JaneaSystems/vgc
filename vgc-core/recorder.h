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
		int m_framesCaptured;
		ScreenCapture m_screenCapture;
		RecordingState m_state;
		unsigned long long m_recordingStartTime;
		double m_fpsLimit;
		ID3D11Texture2D* m_texture;

		std::mutex m_mutex;
		std::thread m_worker;
		std::condition_variable m_cv;

		// Must be a map because files could be created out of order
		std::map<int, std::future<std::wstring>> m_persistFileNames;

		void PersistImage(int frameIdx, ImageData& image);
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