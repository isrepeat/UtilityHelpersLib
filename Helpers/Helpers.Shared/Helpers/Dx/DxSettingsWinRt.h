#pragma once
#include <Helpers/common.h>

#if COMPILE_FOR_WINRT == 1
#include "DxSettings.h"

namespace HELPERS_NS {
    namespace Dx {
        namespace WinRt {
            public value struct Adapter {
                Platform::String^ Description;
            };

            public ref class DxSettingsWinRt sealed {
            public:
                DxSettingsWinRt()
                {}

                property bool MSAA {
                    bool get() {
                        return dxSettings.IsMSAAEnabled();
                    }
                    void set(bool v) {
                        dxSettings.EnableMSAA(v);
                    }
                }

                property bool VSync {
                    bool get() {
                        return dxSettings.IsVSyncEnabled();
                    }
                    void set(bool v) {
                        dxSettings.EnableVSync(v);
                    }
                }

                property WFCollections::IObservableVector<WinRt::Adapter>^ Adapters {
                    Windows::Foundation::Collections::IObservableVector<WinRt::Adapter>^ get() {
                        dxSettings.UpdateAdapters();
                        auto result = ref new PCollections::Vector<WinRt::Adapter>;

                        for (auto& adapter : dxSettings.GetAdapters()) {
                            result->Append(WinRt::Adapter{ 
                                ref new Platform::String(adapter.description.c_str()) 
                                });
                        }
                        return result;
                    }
                };

            private:
                DxSettings dxSettings;
            };
        }
    }
}
#endif