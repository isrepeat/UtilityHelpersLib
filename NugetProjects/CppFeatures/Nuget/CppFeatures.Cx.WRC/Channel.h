#pragma once
#include "../CppFeatures.Shared/ChannelNative.h"
#include "NamespacesAliases.h"

namespace CppFeatures {
    namespace Cx {
        public delegate bool ListenHandler(int, Platform::String^);

        public ref class Channel sealed {
        public:
            Channel();
            virtual ~Channel();

            void Create(Platform::String^ pipeName, ListenHandler^ listenHandler);
            void Open(Platform::String^ pipeName, ListenHandler^ listenHandler);
            
            void Write(int msgType, Platform::String^ payload);

        private:
            static bool ListenCallback(Channel^ _this, int msgType, std::vector<uint8_t> payload);

        private:
            CppFeatures::Channel channelNative;
            ListenHandler^ listenHandler;
        };
    }
}