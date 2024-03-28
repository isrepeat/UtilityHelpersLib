#include "pch.h"
#include "DxSettings.h"

namespace Helpers {
    namespace WinRt {
        namespace Dx {
            Adapter::Adapter(H::Dx::Adapter adapter)
                : adapter{ adapter }
            {}

            Platform::String^ Adapter::Description::get() {
                return ref new Platform::String(adapter.description.c_str());
            }
            uint32_t Adapter::Idx::get() {
                return adapter.idx;
            }
            LUID Adapter::AdapterLUID::get() {
                LUID luid;

                static_assert(sizeof(luid.LowPart) == sizeof(adapter.adapterLUID.LowPart));
                static_assert(sizeof(luid.HighPart) == sizeof(adapter.adapterLUID.HighPart));

                luid.LowPart = adapter.adapterLUID.LowPart;
                luid.HighPart = adapter.adapterLUID.HighPart;

                return luid;
            }


            DxSettings::DxSettings() {
                Platform::WeakReference weakRef = Platform::WeakReference(this);

                dxSettings.GetDxSettingsHandlers()->msaaChanged = [weakRef] {
                    auto _this = weakRef.Resolve<DxSettings>();
                    if (!_this) return;
                    concurrency::create_async([=]() {
                        _this->MsaaChanged();
                        });
                };

                dxSettings.GetDxSettingsHandlers()->vsyncChanged = [weakRef] {
                    auto _this = weakRef.Resolve<DxSettings>();
                    if (!_this) return;
                    concurrency::create_async([=]() {
                        _this->VSyncChanged();
                        });
                };

                dxSettings.GetDxSettingsHandlers()->currentAdapterChanged = [weakRef] {
                    auto _this = weakRef.Resolve<DxSettings>();
                    if (!_this) return;
                    concurrency::create_async([=]() {
                        _this->CurrentAdapterChanged();
                        });
                };

                dxSettings.GetDxSettingsHandlers()->adapersUpdated = [weakRef] {
                    auto _this = weakRef.Resolve<DxSettings>();
                    if (!_this) return;
                    concurrency::create_async([=]() {
                        _this->AdaptersUpdated();
                        });
                };
            }

            bool DxSettings::MSAA::get() {
                return dxSettings.IsMSAAEnabled();
            }
            void DxSettings::MSAA::set(bool v) {
                dxSettings.EnableMSAA(v);
            }

            bool DxSettings::VSync::get() {
                return dxSettings.IsVSyncEnabled();
            }
            void DxSettings::VSync::set(bool v) {
                dxSettings.EnableVSync(v);
            }

            Adapter^ DxSettings::CurrentAdapter::get() {
                return ref new Adapter{ dxSettings.GetCurrentAdapter() };
            }
            void DxSettings::CurrentAdapter::set(Adapter^ adapter) {
                dxSettings.SetCurrentAdapterByIdx(adapter->Idx);
            }

            WFCollections::IObservableVector<Adapter^>^ DxSettings::Adapters::get() {
                auto result = ref new PCollections::Vector<Adapter^>;

                dxSettings.UpdateAdapters();
                for (auto& adapter : dxSettings.GetAdapters()) {
                    result->Append(ref new Adapter{ adapter });
                }
                return result;
            }
        }
    }
}