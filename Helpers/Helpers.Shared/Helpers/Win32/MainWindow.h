#pragma once
#include "Helpers/common.h"
#include "Helpers/HWindows.h"
#include "Helpers/Signal.h"
#include "Helpers/Math.h"

#include <condition_variable>
#include <functional>
#include <thread>

namespace HELPERS_NS {
	namespace Win32 {
		class MainWindow {
		public:
			H::Signal<void()> eventQuit;
			H::Signal<void(H::Size)> eventWindowSizeChanged;

		public:
			MainWindow(
				HINSTANCE hInstance,
				int width,
				int height,
				int x = 100,
				int y = 100,
				DWORD windowStyle = WS_OVERLAPPEDWINDOW);

			virtual ~MainWindow();

			HWND GetHwnd();
			H::Size GetSize();

			void RunMessageLoop();
			void Show();
			void Hide();

		protected:
			static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
			virtual LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

		private:
			std::mutex mx;

			HINSTANCE hInstance;
			H::Size windowSize;

			HWND hWnd;
			std::atomic<bool> isThreadsStopped = true;
		};
	}
}