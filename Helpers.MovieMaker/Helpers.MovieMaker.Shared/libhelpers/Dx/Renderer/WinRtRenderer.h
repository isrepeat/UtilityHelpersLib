#pragma once
#include "SwapChainPanelOutput.h"
#include "../Dx.h"
#include "../../Thread/critical_section.h"
#include "../../HSystem.h"
#include "../../HMath.h"
#include "../../WinRT/RunOnUIThread.h"

#include <memory>
#include <thread>
#include <type_traits>

enum class RenderThreadState {
    Work,
    Pause, // can be changed to Work to let thread continue and not exit
    Stop, // thread will exit very soon
};

#ifdef WINRT_Any
template<class T>
class WinRtRenderer {
public:
    template<class... Args>
    WinRtRenderer(
        Windows::UI::Xaml::Controls::SwapChainPanel ^panel, DxDevice *dxDev, Args&&... args)
        : dxDev(dxDev)
        , output(dxDev, panel)
        , pointerMoves(false)
        , renderer(dxDev, &this->output, std::forward<Args>(args)...)
        , renderThreadState(RenderThreadState::Stop)
        , panel(panel)
    {
        H::RunOnUIThread(this->panel->Dispatcher,
            [&]
            {
                auto window = Windows::UI::Xaml::Window::Current->CoreWindow;
                auto displayInformation = Windows::Graphics::Display::DisplayInformation::GetForCurrentView();

                this->wnd = Windows::UI::Xaml::Window::Current;

                this->visChangedToken = window->VisibilityChanged += H::System::MakeTypedEventHandler(
                    [=](Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args)
                    {
                        if (!args->Visible) {
                            this->renderThreadState = RenderThreadState::Pause;
                        }
                        else {
                            if (this->renderThreadState == RenderThreadState::Stop) {
                                if (this->renderThread.joinable()) {
                                    this->renderThread.join();
                                }

                                this->StartRenderThread();
                            }
                            else {
                                this->renderThreadState = RenderThreadState::Work;
                            }
                        }
                    });

                this->orientChangedToken = displayInformation->OrientationChanged += H::System::MakeTypedEventHandler(
                    [=](Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args)
                    {
                        concurrency::critical_section::scoped_lock lk(this->cs);
                        auto panel = this->output.GetSwapChainPanel();
                        auto orientation = sender->CurrentOrientation;
                        auto dpi = sender->LogicalDpi;
                        auto scale = DirectX::XMFLOAT2(panel->CompositionScaleX, panel->CompositionScaleY);
                        auto size = DirectX::XMFLOAT2((float)panel->ActualWidth, (float)panel->ActualHeight);

                        this->TryResize(size, scale, dpi, orientation);
                    });

                this->dispContentInvalidatedToken = displayInformation->DisplayContentsInvalidated += H::System::MakeTypedEventHandler(
                    [=](Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args)
                    {
                        int stop = 324;
                    });

                this->dpiChangedToken = displayInformation->DpiChanged += H::System::MakeTypedEventHandler(
                    [=](Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args)
                    {
                        concurrency::critical_section::scoped_lock lk(this->cs);
                        auto panel = this->output.GetSwapChainPanel();
                        auto orientation = sender->CurrentOrientation;
                        auto dpi = sender->LogicalDpi;
                        auto scale = DirectX::XMFLOAT2(panel->CompositionScaleX, panel->CompositionScaleY);
                        auto size = DirectX::XMFLOAT2((float)panel->ActualWidth, (float)panel->ActualHeight);

                        this->TryResize(size, scale, dpi, orientation);
                    });

                this->compScaleChangedToken = panel->CompositionScaleChanged += H::System::MakeTypedEventHandler(
                    [=](Windows::UI::Xaml::Controls::SwapChainPanel^ sender, Platform::Object^ args)
                    {
                        auto displayInformation = Windows::Graphics::Display::DisplayInformation::GetForCurrentView();

                        concurrency::critical_section::scoped_lock lk(this->cs);
                        auto orientation = displayInformation->CurrentOrientation;
                        auto dpi = displayInformation->LogicalDpi;
                        auto scale = DirectX::XMFLOAT2(sender->CompositionScaleX, sender->CompositionScaleY);
                        auto size = DirectX::XMFLOAT2((float)sender->ActualWidth, (float)sender->ActualHeight);

                        this->TryResize(size, scale, dpi, orientation);
                    });

                this->sizeChangedToken = panel->SizeChanged += ref new Windows::UI::Xaml::SizeChangedEventHandler(
                    [=](Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ args)
                    {
                        auto displayInformation = Windows::Graphics::Display::DisplayInformation::GetForCurrentView();
                        auto size = DirectX::XMFLOAT2(args->NewSize.Width, args->NewSize.Height);

                        concurrency::critical_section::scoped_lock lk(this->cs);
                        auto panel = this->output.GetSwapChainPanel();
                        auto orientation = displayInformation->CurrentOrientation;
                        auto dpi = displayInformation->LogicalDpi;
                        auto scale = DirectX::XMFLOAT2(panel->CompositionScaleX, panel->CompositionScaleY);

                        this->TryResize(size, scale, dpi, orientation);
                    });
            });

        this->renderer.OutputParametersChanged();

        this->StartRenderThread();
        this->inputThread = std::thread([=]() { this->Input(panel); });
    }

    ~WinRtRenderer() {
        static_assert(std::is_base_of<IRenderer, T>::value, "Renderer must inherit from IRenderer");

        this->renderThreadState = RenderThreadState::Stop;
        this->coreInput->Dispatcher->StopProcessEvents();

        if (this->renderThread.joinable()) {
            this->renderThread.join();
        }

        if (this->inputThread.joinable()) {
            this->inputThread.join();
        }

        try {
            H::RunOnUIThread(this->panel->Dispatcher,
                [&]
                {
                    auto window = Windows::UI::Xaml::Window::Current->CoreWindow;
                    auto displayInformation = Windows::Graphics::Display::DisplayInformation::GetForCurrentView();

                    window->VisibilityChanged -= this->visChangedToken;
                    displayInformation->OrientationChanged -= this->orientChangedToken;
                    displayInformation->DisplayContentsInvalidated -= this->dispContentInvalidatedToken;
                    displayInformation->DpiChanged -= this->dpiChangedToken;
                    this->panel->CompositionScaleChanged -= this->compScaleChangedToken;
                    this->panel->SizeChanged -= this->sizeChangedToken;
                });
        }
        catch(...) {}
    }

    T *operator->() {
        return &this->renderer;
    }

    T *get() {
        return &this->renderer;
    }

    DirectX::XMFLOAT4 GetRTColor();
    void SetRTColor(DirectX::XMFLOAT4 color);
    
    Helpers::WinRt::Dx::DxSettings^ GetDxSettings() {
        return this->output.GetDxSettings();
    }

public:
    std::function<void(Windows::UI::Input::PointerPoint^)> pointerPressed;
    std::function<void(Windows::UI::Input::PointerPoint^)> pointerMoved;
    std::function<void(Windows::UI::Input::PointerPoint^)> pointerReleased;
    std::function<void(Windows::UI::Input::PointerPoint^)> pointerWheelChanged;

private:
    DxDevice *dxDev;
    SwapChainPanelOutput output;
    T renderer;
    Windows::UI::Xaml::Window ^wnd;

    concurrency::critical_section cs;

    Windows::UI::Core::CoreIndependentInputSource ^coreInput;

    std::thread renderThread;
    std::thread inputThread;

    concurrency::critical_section renderThreadStateCs;
    std::atomic<RenderThreadState> renderThreadState;

    bool pointerMoves;

    Windows::UI::Xaml::Controls::SwapChainPanel ^panel;
    Windows::Foundation::EventRegistrationToken visChangedToken;
    Windows::Foundation::EventRegistrationToken orientChangedToken;
    Windows::Foundation::EventRegistrationToken dispContentInvalidatedToken;
    Windows::Foundation::EventRegistrationToken dpiChangedToken;
    Windows::Foundation::EventRegistrationToken compScaleChangedToken;
    Windows::Foundation::EventRegistrationToken sizeChangedToken;

    void Input(Windows::UI::Xaml::Controls::SwapChainPanel ^panel) {
        this->coreInput = panel->CreateCoreIndependentInputSource(
            Windows::UI::Core::CoreInputDeviceTypes::Mouse |
            Windows::UI::Core::CoreInputDeviceTypes::Touch |
            Windows::UI::Core::CoreInputDeviceTypes::Pen);

        this->coreInput->PointerPressed += H::System::MakeTypedEventHandler(
            [=](Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args)
        {
            auto pt = args->CurrentPoint;
            this->pointerMoves = true;

#ifdef WINRT_Input
            this->pointerPressed(pt);
#endif // WINRT_Input

        });
        this->coreInput->PointerMoved += H::System::MakeTypedEventHandler(
            [=](Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args)
        {
            if (this->pointerMoves) {
                auto pt = args->CurrentPoint;

#ifdef WINRT_Input
                this->pointerMoved(pt);
#endif // DEBUG

            }
        });
        this->coreInput->PointerReleased += H::System::MakeTypedEventHandler(
            [=](Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args)
        {
            auto pt = args->CurrentPoint;
            this->pointerMoves = false;

#ifdef WINRT_Input
            this->pointerReleased(pt);
#endif // WINRT_Input

        });
        this->coreInput->PointerWheelChanged += H::System::MakeTypedEventHandler(
            [=](Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args)
        {
            auto pt = args->CurrentPoint;

#ifdef WINRT_Input
            this->pointerWheelChanged(pt);
#endif // WINRT_Input

        });

        this->coreInput->Dispatcher->ProcessEvents(Windows::UI::Core::CoreProcessEventsOption::ProcessUntilQuit);
    }

    void Render() {
        while (this->renderThreadState != RenderThreadState::Stop) {
            concurrency::critical_section::scoped_lock lk(this->cs);
            this->output.Render([this] {
                this->renderer.Render();
                });
        }
    }


    void StartRenderThread() {
        if (this->renderThreadState != RenderThreadState::Work) {
            this->renderThreadState = RenderThreadState::Work;
            this->renderThread = std::thread([=]() { this->Render(); });
        }
    }

    // Need to inspect all parameters in order to use correct size parameters under windows 8.1
    // Because under 8.1 it isn't reports events about all changes, for example it refers to DPI
    void TryResize(
        const DirectX::XMFLOAT2 &newSize,
        const DirectX::XMFLOAT2 &newScale,
        float dpi,
        Windows::Graphics::Display::DisplayOrientations orientation)
    {
        bool needResize = false;
        auto oldOrientation = this->output.GetCurrentOrientation();
        auto oldDpi = this->output.GetLogicalDpi();
        auto oldScale = this->output.GetCompositionScale();
        auto oldSize = this->output.GetLogicalSize();

        if (orientation != oldOrientation) {
            needResize = true;
            this->output.SetCurrentOrientation(orientation);
        }

        if (dpi != oldDpi) {
            needResize = true;
            this->output.SetLogicalDpi(dpi);
        }

        if (newScale != oldScale) {
            needResize = true;
            this->output.SetCompositionScale(newScale);
        }

        if (newSize != oldSize) {
            needResize = true;
            this->output.SetLogicalSize(newSize);
        }

        if (needResize) {
            this->output.Resize();
            this->renderer.OutputParametersChanged();
        }
    }
};

template<class T>
inline DirectX::XMFLOAT4 WinRtRenderer<T>::GetRTColor() {
    return this->output.GetRTColor();
}

template<class T>
inline void WinRtRenderer<T>::SetRTColor(DirectX::XMFLOAT4 color) {
    this->output.SetRTColor(color);
}

#endif // WINRT_Any