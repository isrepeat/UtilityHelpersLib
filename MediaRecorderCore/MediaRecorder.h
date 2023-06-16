#pragma once
#include "IMediaRecorder.h"
#include "MediaRecorderParams.h"
#include "MediaRecorderErrorsEnum.h"

#include <cstdint>
#include <array>
#include <libhelpers\Event.hpp>
#include <libhelpers\Macros.h>
#include <libhelpers\MediaFoundation\MFUser.h>
#include "ChunkMerger.h"

// default profile of H264 can fail on sink->Finalize with video bitrate > 80 mbits.
class MediaRecorder : public MFUser, public IMediaRecorder {
public:
    NO_COPY(MediaRecorder);

    static const uint32_t AllowedNumChannels[2];
    static const uint32_t AllowedSampleRate[2];
    static const uint32_t AllowedBytesPerSecond[4];

    MediaRecorder();
    MediaRecorder(
        IMFByteStream* outputStream,
        MediaRecorderParams params,
        bool useCPUForEncoding,
        bool nv12VideoSamples,
        std::shared_ptr<IEvent<Native::MediaRecorderEventArgs>> recordEventCallback = nullptr);

    MediaRecorder(MediaRecorder&&) = default;
    ~MediaRecorder();

    MediaRecorder &operator=(MediaRecorder&&) = default;

    // IMediaRecorder
    bool HasAudio() const override;
    bool HasVideo() const override;

    int64_t LastVideoPtsHns() const override;
    int64_t LastAudioPtsHns() const override;
    int64_t LastPtsHns() const override;

    bool ChunkAudioSamplesWritten() const override;
    bool ChunkVideoSamplesWritten() const override;

    void StartRecord() override;
    void Record(const Microsoft::WRL::ComPtr<IMFSample> &sample, bool audio) override;
    void EndRecord() override;

    void Restore(IMFByteStream* outputStream, std::vector<std::wstring>&& chunks) override;

    const MediaRecorderParams &GetParams() const;

    Microsoft::WRL::ComPtr<IMFMediaType> GetVideoTypeIn() const;
    Microsoft::WRL::ComPtr<IMFMediaType> GetVideoTypeOut() const;

    Microsoft::WRL::ComPtr<IMFMediaType> CreateAudioAACOutMediaType();
    Microsoft::WRL::ComPtr<IMFMediaType> CreateAudioFlacOutMediaType();

    void Write(const float *audioSamples, size_t valuesCount, int64_t hns = -1);
    void Write(ID3D11DeviceContext *d3dCtx, ID3D11Texture2D *tex, int64_t hns = -1, int64_t durationHns = -1);
    void Write(const void *videoData, size_t rowPitch, int64_t hns = -1, int64_t durationHns = -1);

private:
    Microsoft::WRL::ComPtr<IMFByteStream> stream;
    const MediaRecorderParams params;
    const uint32_t audioSampleBits = 16;

    DWORD audioStreamIdx;
    DWORD videoStreamIdx;
    Microsoft::WRL::ComPtr<IMFSinkWriter> sinkWriter;

    int64_t audioPtsHns;
    int64_t videoPtsHns;
    
    bool cpuEncoding;
    bool nv12Textures;
    std::wstring containerExt;
    int64_t chunkAudioPtsHns;
    int64_t chunkVideoPtsHns;
    int samplesNumber = 0;
    int framesNumber = 0;

    std::wstring chunkFile;
    std::wstring chunksDisk;
    std::wstring targetRecordDisk;
    uint64_t recordedChunksSize = 0;
    uint64_t lastChunkCreatedTime = 0;

    int32_t chunkNumber = 0;
    Microsoft::WRL::ComPtr<IMFByteStream> currentOutputStream;

    Microsoft::WRL::ComPtr<IMFMediaType> videoTypeOut, videoTypeIn;

    std::shared_ptr<IEvent<Native::MediaRecorderEventArgs>> recordEventCallback;

    void InitializeSinkWriter(IMFByteStream *outputStream, bool useCPUForEncoding, bool nv12VideoSamples);
    static Microsoft::WRL::ComPtr<IMFMediaType> CreateAudioInMediaType(
        const IAudioCodecSettings *settings,
        uint32_t bitsPerSample);
    static Microsoft::WRL::ComPtr<IMFMediaType> CreateAudioOutMediaType(
        const IAudioCodecSettings *settings,
        uint32_t bitsPerSample);
    static Microsoft::WRL::ComPtr<IMFMediaType> CreateVideoInMediaType(
        const IVideoCodecSettings *settings, bool nv12VideoSamples);
    static Microsoft::WRL::ComPtr<IMFMediaType> CreateVideoOutMediaType(
        const IVideoCodecSettings *settings, MediaContainerType containerType);

    static Microsoft::WRL::ComPtr<IMFMediaType> GetBestOutputType(
        IMFTransform *transform,
        IMFMediaType *defaultType,
        const GUID &subtypeGuid,
        uint32_t numChannels,
        uint32_t sampleRate,
        uint32_t bitsPerSample,
        uint32_t bitrate);

    void WriteVideoSample(const Microsoft::WRL::ComPtr<IMFMediaBuffer> &buffer, int64_t hns = -1, int64_t durationHns = -1);
    void WriteSample(const Microsoft::WRL::ComPtr<IMFMediaBuffer> &buffer, int64_t positionHNS, int64_t durationHNS, DWORD streamIndex);
    void WriteSample(const Microsoft::WRL::ComPtr<IMFSample> &sample, DWORD streamIndex);

    int64_t GetDefaultVideoFrameDuration() const;

    void DefineContainerType();

    IMFByteStream* StartNewChunk();
    void ResetSinkWriterOnNewChunk();
    void FinalizeRecord();
};