// WORKAROUND: Channel.cpp already exist in Helpers.Shared because add "Native" postfix
#include "ChannelNative.h"
#include "Helpers/Channel.h"
#include "Helpers/Callback.hpp"

namespace CppFeatures {
	Channel::Channel()
        : channel{ std::make_unique<H::Channel<details::ChannelBaseMessages, uint8_t>>() }
	{}

    Channel::~Channel() 
    {}

	void Channel::Create(const std::wstring& pipeName, H::Callback<bool, int, std::vector<uint8_t>> listenHandler, int timeout) {
        this->channel->SetFullClassName(L"Channel->channel");
        this->channel->SetInterruptHandler([this] {
            //working = false;
            });
        try {
            this->channel->Create(pipeName,
                [this, listenHandler](H::Channel<details::ChannelBaseMessages>::Msg_t message, H::Channel<details::ChannelBaseMessages>::WriteFunc Write) {
                    return listenHandler(static_cast<int>(message->type), std::move(message->payload));
                },
                timeout
            );
        }
        catch (H::PipeError err) {
            LOG_ERROR_S(&this->channel, "Catch PipeError = {}", magic_enum::enum_name(err));
            //working = false;
        }
	}

	void Channel::Open(const std::wstring& pipeName, H::Callback<bool, int, std::vector<uint8_t>> listenHandler, int timeout) {
        this->channel->SetFullClassName(L"Channel->channel");
        this->channel->SetInterruptHandler([this] {
            //working = false;
            });

        try {
            this->channel->Open(pipeName,
                [this, listenHandler](H::Channel<details::ChannelBaseMessages>::Msg_t message, H::Channel<details::ChannelBaseMessages>::WriteFunc Write) {
                    return listenHandler(static_cast<int>(message->type), std::move(message->payload));
                },
                timeout
            );
        }
        catch (H::PipeError err) {
            LOG_ERROR_S(&this->channel, "Catch PipeError = {}", magic_enum::enum_name(err));
            //working = false;
        }
	}

    void Channel::Write(int msgType, std::vector<uint8_t> payload) {
        this->channel->Write(static_cast<details::ChannelBaseMessages>(msgType), std::move(payload));
    }
}