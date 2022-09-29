#pragma once
#include "IMediaRecorderFactory.h"
#include <libhelpers/Event.hpp>
#include "MediaRecorderParams.h"
#include "MediaRecorderErrorsEnum.h"

#include <vector>

class MediaRecorderFactory : public IMediaRecorderFactory {
public:
    MediaRecorderFactory(
        IMFByteStream* outputStream,
        MediaRecorderParams params,
        bool useCPUForEncoding,
        bool nv12VideoSamples,
        std::shared_ptr<IEvent<Native::MediaRecorderEventArgs>> recordEventCallback);

    std::unique_ptr<IMediaRecorder> CreateMediaRecorder() override;

    static std::vector<uint32_t> GetAvailableAudioNumChannels();
    static std::vector<uint32_t> GetAvailableAudioSampleRate();
    static std::vector<uint32_t> GetAvailableAudioBitsPerSecond();
    static std::vector<wchar_t>  GetAvailibleCodecs();

private:
    bool useCPUForEncoding;
    bool nv12VideoSamples;
    MediaRecorderParams params;
    Microsoft::WRL::ComPtr<IMFByteStream> outputStream;
    std::shared_ptr<IEvent<Native::MediaRecorderEventArgs>> recordEventCallback;
};