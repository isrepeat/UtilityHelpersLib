#pragma once
#include <Helpers/Dx/DxSettings.h>
#include "Delegates.h"

namespace Helpers {
    namespace WinRt {
        namespace Dx {
            public value struct LUID {
                uint32_t LowPart;
                int32_t HighPart;
            };

            public ref class Adapter sealed {
            internal:
                Adapter(H::Dx::Adapter adapter);

            public:
                property Platform::String^ Description {
                    Platform::String^ get();
                }
                property uint32_t Idx {
                    uint32_t get();
                }
                property LUID AdapterLUID {
                    LUID get();
                }

            internal:
                const H::Dx::Adapter adapter;
            };


            public ref class DxSettings sealed {
            public:
                DxSettings();

                event EventHandler^ MsaaChanged;
                event EventHandler^ VSyncChanged;
                event EventHandler^ CurrentAdapterChanged;
                event EventHandler^ AdaptersUpdated;

                property bool MSAA {
                    bool get();
                    void set(bool enabled);
                }
                property bool VSync {
                    bool get();
                    void set(bool enabled);
                }

                property Adapter^ CurrentAdapter {
                    Adapter^ get();
                    void set(Adapter^ adapter);
                }
                property WFCollections::IObservableVector<Adapter^>^ Adapters {
                    WFCollections::IObservableVector<Adapter^>^ get();
                }

            internal:
                H::Dx::DxSettingsHandlers& GetDxSettingsHandlers();

            private:
                H::Dx::DxSettings dxSettings;
            };
        }
    }
}