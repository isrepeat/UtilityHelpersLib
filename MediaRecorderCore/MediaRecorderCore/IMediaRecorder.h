#pragma once

#include <cstdint>
#include <wrl.h>
#include <mfapi.h>
#include <vector>
#include <string>

class IMediaRecorder {
public:
    IMediaRecorder() {}
    virtual ~IMediaRecorder() {}

    virtual bool HasAudio() const = 0;
    virtual bool HasVideo() const = 0;

    virtual int64_t LastVideoPtsHns() const = 0;
    virtual int64_t LastAudioPtsHns() const = 0;
    // default implementation is to return max(LastVideoPtsHns(), LastAudioPtsHns())
    virtual int64_t LastPtsHns() const = 0;

    virtual bool ChunkAudioSamplesWritten() const = 0;
    virtual bool ChunkVideoSamplesWritten() const = 0;

    virtual bool IsChunkMergerEnabled() = 0;

    virtual void StartRecord() = 0;
    // <audio> == true for audio sample; <audio> == false for video sample
    virtual void Record(const Microsoft::WRL::ComPtr<IMFSample> &sample, bool audio) = 0;
    virtual void EndRecord() = 0;
    virtual void Restore(IMFByteStream* outputStream, std::vector<std::wstring>&& chunks) = 0;
};