#include "MediaRecorder.h"
#include "CodecsTable.h"
#include "MediaFormat/MediaFormatCodecsSupport.h"
#include "Platform/PlatformClassFactory.h"
#include <limits>
#include <filesystem>
#include <libhelpers/HardDrive.h>
#include <libhelpers/HSystem.h>
#include <libhelpers/HMathCP.h>
#include <libhelpers/HTime.h>
#include <memcpy.h>
#include <mfapi.h>


// encoder restrictions can be found here : https://msdn.microsoft.com/en-us/library/windows/desktop/dd742785(v=vs.85).aspx
const uint32_t MediaRecorder::AllowedNumChannels[] = { 1 , 2 };
const uint32_t MediaRecorder::AllowedSampleRate[] = { 44100, 48000 };
const uint32_t MediaRecorder::AllowedBytesPerSecond[] = { 12000, 16000, 20000, 24000 };

MediaRecorder::MediaRecorder() {}

MediaRecorder::MediaRecorder(
    IMFByteStream* outputStream,
    MediaRecorderParams params,
    bool useCPUForEncoding,
    bool nv12VideoSamples,
    std::shared_ptr<IEvent<MediaRecorderErrorsEnum>> recordErrorCallback)
    : stream(outputStream), params(std::move(params))
    , audioStreamIdx((std::numeric_limits<DWORD>::max)())
    , videoStreamIdx((std::numeric_limits<DWORD>::max)()), audioPtsHns(0)
    , videoPtsHns(0), cpuEncoding{ useCPUForEncoding }, nv12Textures{ nv12VideoSamples }, recordErrorCallback{ recordErrorCallback }
{
    //this->InitializeSinkWriter(outputStream, useCPUForEncoding, nv12VideoSamples);
    targetRecordDisk = H::HardDrive::GetDiskLetterFromPath(this->params.targetRecordPath);

    DefineContainerType();
    auto newStream = StartNewChunk();

    this->InitializeSinkWriter(newStream, useCPUForEncoding, nv12VideoSamples);
    //this->InitializeSinkWriter(outputStream, useCPUForEncoding, nv12VideoSamples);  
}

MediaRecorder::~MediaRecorder() {

}

bool MediaRecorder::HasAudio() const {
    bool has = this->audioStreamIdx != (std::numeric_limits<DWORD>::max)();
    return has;
}

bool MediaRecorder::HasVideo() const {
    bool has = this->videoStreamIdx != (std::numeric_limits<DWORD>::max)();
    return has;
}

int64_t MediaRecorder::LastVideoPtsHns() const {
    return this->videoPtsHns;
}

int64_t MediaRecorder::LastAudioPtsHns() const {
    return this->audioPtsHns;
}

int64_t MediaRecorder::LastPtsHns() const {
    auto ptsHns = (std::max)(this->audioPtsHns, this->videoPtsHns);
    return ptsHns;
}

void MediaRecorder::StartRecord() {
    HRESULT hr = S_OK;

    hr = this->sinkWriter->BeginWriting();
    H::System::ThrowIfFailed(hr);
}

void MediaRecorder::Record(const Microsoft::WRL::ComPtr<IMFSample> &sample, bool audio) {
    HRESULT hr = S_OK;
    int64_t samplePts = 0;
    int64_t sampleDuration = 0;

    if (audio) {
        //https://stackoverflow.com/questions/33401149/ffmpeg-adding-0-05-seconds-of-silence-to-transcoded-aac-file
        if (samplesNumber > params.mediaFormat.GetAudioCodecSettings()->GetBasicSettings()->sampleRate * 10 && samplesNumber % 1024 == 0) {
            ResetSinkWriterOnNewChunk();
        }
    }
    else {
        if (framesNumber > params.mediaFormat.GetVideoCodecSettings()->GetBasicSettings()->fps * 5) {
            ResetSinkWriterOnNewChunk();
        }
    }

    hr = sample->GetSampleTime(&samplePts);
    H::System::ThrowIfFailed(hr);

    hr = sample->GetSampleDuration(&sampleDuration);
    H::System::ThrowIfFailed(hr);

    DWORD streamIdx = -1;
    int64_t *ptsPtr = nullptr;

    if (audio) {

        Microsoft::WRL::ComPtr<IMFMediaBuffer> mediaBuffer;
        hr = sample->ConvertToContiguousBuffer(&mediaBuffer);
      
        DWORD len;
        hr = mediaBuffer->GetCurrentLength(&len);
        auto samples = len / (audioSampleBits/8 * params.mediaFormat.GetAudioCodecSettings()->GetBasicSettings()->numChannels);

        samplesNumber += samples;
        streamIdx = this->audioStreamIdx;
        ptsPtr = &this->audioPtsHns;
        sample->SetSampleTime(chunkAudioPtsHns);
        chunkAudioPtsHns += sampleDuration;
    }
    else {
        streamIdx = this->videoStreamIdx;
        ptsPtr = &this->videoPtsHns;
        sample->SetSampleTime(chunkVideoPtsHns);
        chunkVideoPtsHns += sampleDuration;
        ++framesNumber;
    }

    this->WriteSample(sample, streamIdx);

    *ptsPtr = samplePts + sampleDuration;
}

void MediaRecorder::EndRecord() {
    FinalizeRecord();

    std::vector<std::wstring> chunks;
    chunks.reserve(chunkNumber);
    for (size_t i = 0; i < chunkNumber; ++i) {
        auto chunkFile = params.chunksPath + params.chunksGuid + L"-" + std::to_wstring(i) + containerExt;
        if (std::filesystem::file_size(chunkFile) > 0)
            chunks.push_back(std::move(chunkFile));
    }
    ChunkMerger merger{ this->stream.Get(), MediaRecorder::CreateAudioOutMediaType(this->params.mediaFormat.GetAudioCodecSettings(), audioSampleBits), std::move(chunks), containerExt };
    //ChunkMerger merger{ this->stream.Get(), CreateAudioAACOutMediaType(), std::move(chunks), containerExt };
    //ChunkMerger merger{ this->stream.Get(), std::move(chunks) };
    merger.Merge();

}

void MediaRecorder::Restore(IMFByteStream* outputStream, std::vector<std::wstring>&& chunks) {
    ChunkMerger merger{ this->stream.Get(), MediaRecorder::CreateAudioOutMediaType(this->params.mediaFormat.GetAudioCodecSettings(), audioSampleBits), std::move(chunks), containerExt };
    merger.Merge();
}

const MediaRecorderParams &MediaRecorder::GetParams() const {
    return this->params;
}

Microsoft::WRL::ComPtr<IMFMediaType> MediaRecorder::GetVideoTypeIn() const {
    return this->videoTypeIn;
}

Microsoft::WRL::ComPtr<IMFMediaType> MediaRecorder::GetVideoTypeOut() const {
    return this->videoTypeOut;
}

void MediaRecorder::Write(const float *audioSamples, size_t valuesCount, int64_t hns) {
    HRESULT hr = S_OK;
    BYTE *bufferData = nullptr;
    Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
    const DWORD bufferByteSize = (uint32_t)(valuesCount * sizeof(int16_t));

    // <valuesCount> must be multiple of <NumAudioChannels> in order to write full audio frame
    assert(this->params.mediaFormat.GetAudioCodecSettings());
    assert(this->params.mediaFormat.GetAudioCodecSettings()->GetBasicSettings());
    assert(valuesCount % this->params.mediaFormat.GetAudioCodecSettings()->GetBasicSettings()->numChannels == 0);
    assert(this->HasAudio());

    hr = MFCreateMemoryBuffer(bufferByteSize, buffer.GetAddressOf());
    H::System::ThrowIfFailed(hr);

    hr = buffer->Lock(&bufferData, NULL, NULL);

    if (SUCCEEDED(hr)) {
        auto src = audioSamples;
        auto dst = reinterpret_cast<int16_t *>(bufferData);

        for (size_t i = 0; i < valuesCount; i++) {
            dst[i] = (int16_t)(src[i] * INT16_MAX);
        }
    }

    buffer->Unlock();
    H::System::ThrowIfFailed(hr);

    hr = buffer->SetCurrentLength(bufferByteSize);
    H::System::ThrowIfFailed(hr);

    auto basicSettings = this->params.mediaFormat.GetAudioCodecSettings()->GetBasicSettings();

    int64_t sampleCount = valuesCount / basicSettings->numChannels;

    int64_t durationHns = H::MathCP::Convert2(
        sampleCount,
        (int64_t)basicSettings->sampleRate,
        (int64_t)H::Time::HNSResolution);

    if (hns >= 0) {
        this->audioPtsHns = hns;
    }

    this->WriteSample(buffer, this->audioPtsHns, durationHns, this->audioStreamIdx);

    this->audioPtsHns += durationHns;
}


void MediaRecorder::Write(ID3D11DeviceContext *d3dCtx, ID3D11Texture2D *tex, int64_t hns, int64_t durationHns) {
    assert(this->params.DxBufferFactory);

    Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer = this->params.DxBufferFactory->CreateBuffer(d3dCtx, tex);

    this->WriteVideoSample(buffer, hns, durationHns);
}

void MediaRecorder::Write(const void *videoData, size_t rowPitch, int64_t hns, int64_t durationHns) {
    assert(this->params.mediaFormat.GetVideoCodecSettings());
    assert(this->params.mediaFormat.GetVideoCodecSettings()->GetBasicSettings());

    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
    BYTE *bufferData = nullptr;
    auto basicSettings = this->params.mediaFormat.GetVideoCodecSettings()->GetBasicSettings();
    DWORD bufferByteSize = basicSettings->width * basicSettings->height * 4;
    hr = MFCreateMemoryBuffer(bufferByteSize, buffer.GetAddressOf());
    H::System::ThrowIfFailed(hr);

    hr = buffer->Lock(&bufferData, NULL, NULL);

    if (SUCCEEDED(hr)) {
        if (basicSettings->width * 4 != rowPitch) {
            auto src = static_cast<const uint8_t *>(videoData);
            auto dst = bufferData;

            for (uint32_t y = 0; y < basicSettings->height; y++, src += rowPitch, dst += basicSettings->height * 4) {
                memcpy(dst, src, basicSettings->width * 4);
            }
        }
        else {
            memcpy(bufferData, videoData, basicSettings->width * basicSettings->height * 4);
        }
    }

    buffer->Unlock();
    H::System::ThrowIfFailed(hr);

    hr = buffer->SetCurrentLength(bufferByteSize);
    H::System::ThrowIfFailed(hr);

    this->WriteVideoSample(buffer, hns, durationHns);
}

void MediaRecorder::InitializeSinkWriter(IMFByteStream *outputStream, bool useCPUForEncoding, bool nv12VideoSamples)
{
    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<IMFByteStream> byteStream = outputStream;
    Microsoft::WRL::ComPtr<IMFAttributes> sinkAttr;

    hr = MFCreateAttributes(sinkAttr.GetAddressOf(), 3);
    H::System::ThrowIfFailed(hr);

    if (!useCPUForEncoding) {
        hr = sinkAttr->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
        H::System::ThrowIfFailed(hr);
    }

    if (this->params.DxBufferFactory) {
        this->params.DxBufferFactory->SetAttributes(sinkAttr.Get());
    }

    //const wchar_t *containerExt = nullptr;
    //containerExt ;

    hr = MFCreateSinkWriterFromURL(containerExt.c_str(), byteStream.Get(), sinkAttr.Get(), this->sinkWriter.GetAddressOf());
    H::System::ThrowIfFailed(hr);

    if (this->params.mediaFormat.GetAudioCodecSettings()) {
        
        auto settings = this->params.mediaFormat.GetAudioCodecSettings();
        Microsoft::WRL::ComPtr<IMFMediaType> typeOut, typeIn;

        typeIn = MediaRecorder::CreateAudioInMediaType(settings, audioSampleBits);
        
        switch (settings->GetCodecType()) {
        case AudioCodecType::AAC:
            typeOut = CreateAudioAACOutMediaType();
            break;

        default:
            typeOut = CreateAudioOutMediaType(settings, audioSampleBits);
        }
        
        //typeOut = CreateAudioFlacOutMediaType();
        //typeOut = MediaRecorder::CreateAudioInMediaType(settings, audioSampleBits);

        hr = this->sinkWriter->AddStream(typeOut.Get(), &this->audioStreamIdx);
        H::System::ThrowIfFailed(hr);

        hr = this->sinkWriter->SetInputMediaType(this->audioStreamIdx, typeIn.Get(), NULL);
        H::System::ThrowIfFailed(hr);
    }

    if (this->params.mediaFormat.GetVideoCodecSettings()) {
        auto settings = this->params.mediaFormat.GetVideoCodecSettings();
        Microsoft::WRL::ComPtr<IMFMediaType> typeOut, typeIn;

        typeIn = MediaRecorder::CreateVideoInMediaType(settings, nv12VideoSamples);
        typeOut = MediaRecorder::CreateVideoOutMediaType(settings);

        hr = this->sinkWriter->AddStream(typeOut.Get(), &this->videoStreamIdx);
        H::System::ThrowIfFailed(hr);

        hr = this->sinkWriter->SetInputMediaType(this->videoStreamIdx, typeIn.Get(), NULL);
        H::System::ThrowIfFailed(hr);

        this->videoTypeIn = typeIn;
        this->videoTypeOut = typeOut;
    }
}

Microsoft::WRL::ComPtr<IMFMediaType> MediaRecorder::CreateAudioInMediaType(
    const IAudioCodecSettings *settings,
    uint32_t bitsPerSample)
{
    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<IMFMediaType> mediaType;

    hr = MFCreateMediaType(mediaType.GetAddressOf());
    H::System::ThrowIfFailed(hr);

    hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    H::System::ThrowIfFailed(hr);

    hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    H::System::ThrowIfFailed(hr);

    hr = mediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, bitsPerSample);
    H::System::ThrowIfFailed(hr);

    if (settings->HasBasicSettings()) {
        auto tmp = settings->GetBasicSettings();
        auto numChannels = tmp->numChannels;
        auto sampleRate = tmp->sampleRate;

        hr = mediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, numChannels);
        H::System::ThrowIfFailed(hr);

        hr = mediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, sampleRate);
        H::System::ThrowIfFailed(hr);

        hr = mediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, (bitsPerSample / 8) * numChannels * sampleRate);
        H::System::ThrowIfFailed(hr);

        hr = mediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, (bitsPerSample / 8) * numChannels);
        H::System::ThrowIfFailed(hr);
    }

    hr = mediaType->SetUINT32(MF_MT_COMPRESSED, FALSE);
    H::System::ThrowIfFailed(hr);

    hr = mediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    H::System::ThrowIfFailed(hr);

    return mediaType;
}

Microsoft::WRL::ComPtr<IMFMediaType> MediaRecorder::CreateAudioOutMediaType(
    const IAudioCodecSettings *settings,
    uint32_t bitsPerSample)
{
    if (settings == nullptr)
        return nullptr;

    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<IMFMediaType> mediaType;

    // Output type
    hr = MFCreateMediaType(mediaType.ReleaseAndGetAddressOf());
    H::System::ThrowIfFailed(hr);

    hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    H::System::ThrowIfFailed(hr);

    auto codecSup = MediaFormatCodecsSupport::Instance();
    auto codecType = settings->GetCodecType();
    auto acodecId = codecSup->MapAudioCodec(codecType);

    if (codecType == AudioCodecType::AMR_NB) {
        int stop = 423;
    }

    hr = mediaType->SetGUID(MF_MT_SUBTYPE, acodecId);
    H::System::ThrowIfFailed(hr);

    auto basicSettings = settings->GetBasicSettings();
    auto bitrateSettings = settings->GetBitrateSettings();

    if (basicSettings) {
        hr = mediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, basicSettings->numChannels);
        H::System::ThrowIfFailed(hr);

        hr = mediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, basicSettings->sampleRate);
        H::System::ThrowIfFailed(hr);
    }

    switch (codecType) {
    case AudioCodecType::MP3: {
        if (!basicSettings || !bitrateSettings) {
            break;
        }

        auto factory = PlatformClassFactory::Instance()->CreateMP3CodecFactory();
        auto transform = factory->CreateIMFTransform();

        mediaType = MediaRecorder::GetBestOutputType(
            transform.Get(), mediaType.Get(), acodecId,
            basicSettings->numChannels, basicSettings->sampleRate,
            bitsPerSample, bitrateSettings->bitrate);
        break;
    }
    case AudioCodecType::WMAudioV8: {
        if (!basicSettings || !bitrateSettings) {
            break;
        }

        auto factory = PlatformClassFactory::Instance()->CreateWma8CodecFactory();
        auto transform = factory->CreateIMFTransform();

        mediaType = MediaRecorder::GetBestOutputType(
            transform.Get(), mediaType.Get(), acodecId,
            basicSettings->numChannels, basicSettings->sampleRate,
            bitsPerSample, bitrateSettings->bitrate);
        break;
    }
    case AudioCodecType::AMR_NB: {
        if (!basicSettings || !bitrateSettings) {
            break;
        }

        auto factory = PlatformClassFactory::Instance()->CreateAmrNbCodecFactory();
        auto transform = factory->CreateIMFTransform();

        if (transform) {
            mediaType = MediaRecorder::GetBestOutputType(
                transform.Get(), mediaType.Get(), acodecId,
                basicSettings->numChannels, basicSettings->sampleRate,
                bitsPerSample, bitrateSettings->bitrate);
        }

        break;
    }
    case AudioCodecType::FLAC:
    case AudioCodecType::ALAC:
    case AudioCodecType::PCM: {
        if (!basicSettings) {
            break;
        }

        hr = mediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, (bitsPerSample / 8) * basicSettings->numChannels * basicSettings->sampleRate);
        H::System::ThrowIfFailed(hr);

        hr = mediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, (bitsPerSample / 8) * basicSettings->numChannels);
        H::System::ThrowIfFailed(hr);

        hr = mediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, bitsPerSample);
        H::System::ThrowIfFailed(hr);

        /*hr = mediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
        H::System::ThrowIfFailed(hr);*/

        break;
    }
    }

    if (mediaType->GetItem(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, nullptr) != S_OK && bitrateSettings) {
        hr = mediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, bitrateSettings->bitrate / 8);
        H::System::ThrowIfFailed(hr);
    }
    
    return mediaType;
}

//TODO maybe replace codec
Microsoft::WRL::ComPtr<IMFMediaType> MediaRecorder::CreateAudioAACOutMediaType() {
    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<IMFMediaType> mediaType;

    // Output type
    hr = MFCreateMediaType(mediaType.ReleaseAndGetAddressOf());
    H::System::ThrowIfFailed(hr);

    hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    H::System::ThrowIfFailed(hr);

    auto codecSup = MediaFormatCodecsSupport::Instance();



    hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC);
    H::System::ThrowIfFailed(hr);

    auto basicSettings = this->params.mediaFormat.GetAudioCodecSettings()->GetBasicSettings();
    auto bitrateSettings = this->params.mediaFormat.GetAudioCodecSettings()->GetBitrateSettings();

    if (basicSettings) {
        hr = mediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, basicSettings->numChannels);
        H::System::ThrowIfFailed(hr);

        hr = mediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, basicSettings->sampleRate);
        H::System::ThrowIfFailed(hr);
    }

    if (mediaType->GetItem(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, nullptr) != S_OK && bitrateSettings) {
        hr = mediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, bitrateSettings->bitrate / 8);
        H::System::ThrowIfFailed(hr);
    }
    
    return mediaType;
}

Microsoft::WRL::ComPtr<IMFMediaType> MediaRecorder::CreateAudioFlacOutMediaType() {
    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<IMFMediaType> mediaType;
    
    // Output type
    hr = MFCreateMediaType(mediaType.ReleaseAndGetAddressOf());
    H::System::ThrowIfFailed(hr);

    hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    H::System::ThrowIfFailed(hr);

    auto codecSup = MediaFormatCodecsSupport::Instance();

    hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_FLAC);
    H::System::ThrowIfFailed(hr);

    auto basicSettings = this->params.mediaFormat.GetAudioCodecSettings()->GetBasicSettings();
    auto bitrateSettings = this->params.mediaFormat.GetAudioCodecSettings()->GetBitrateSettings();
    
    if (basicSettings) {
        hr = mediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, basicSettings->numChannels);
        H::System::ThrowIfFailed(hr);

        hr = mediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, (audioSampleBits / 8) * basicSettings->numChannels);
        H::System::ThrowIfFailed(hr);

        hr = mediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, audioSampleBits);
        H::System::ThrowIfFailed(hr);

        hr = mediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, basicSettings->sampleRate);
        H::System::ThrowIfFailed(hr);

        hr = mediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, (audioSampleBits / 8) * basicSettings->numChannels * basicSettings->sampleRate);
        H::System::ThrowIfFailed(hr);
    }
    
    return mediaType;
}

Microsoft::WRL::ComPtr<IMFMediaType> MediaRecorder::CreateVideoInMediaType(
    const IVideoCodecSettings *settings, bool nv12VideoSamples)
{
    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<IMFMediaType> mediaType;

    hr = MFCreateMediaType(mediaType.GetAddressOf());
    H::System::ThrowIfFailed(hr);

    hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    H::System::ThrowIfFailed(hr);

    if (nv12VideoSamples)
    {
        hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
        H::System::ThrowIfFailed(hr);
    }
    else
    {
        hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_ARGB32);
        H::System::ThrowIfFailed(hr);
    }

    hr = mediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    H::System::ThrowIfFailed(hr);

    auto basicSettings = settings->GetBasicSettings();

    if (basicSettings) {
        hr = MFSetAttributeSize(mediaType.Get(), MF_MT_FRAME_SIZE, basicSettings->width, basicSettings->height);
        H::System::ThrowIfFailed(hr);

        hr = MFSetAttributeRatio(mediaType.Get(), MF_MT_FRAME_RATE, basicSettings->fps, 1);
        H::System::ThrowIfFailed(hr);
    }

    hr = MFSetAttributeRatio(mediaType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    H::System::ThrowIfFailed(hr);

    return mediaType;
}

Microsoft::WRL::ComPtr<IMFMediaType> MediaRecorder::CreateVideoOutMediaType(
    const IVideoCodecSettings *settings)
{
    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<IMFMediaType> mediaType;

    hr = MFCreateMediaType(mediaType.GetAddressOf());
    H::System::ThrowIfFailed(hr);

    hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    H::System::ThrowIfFailed(hr);

    auto codecSup = MediaFormatCodecsSupport::Instance();
    auto vcodecId = codecSup->MapVideoCodec(settings->GetCodecType());

    hr = mediaType->SetGUID(MF_MT_SUBTYPE, vcodecId);
    H::System::ThrowIfFailed(hr);

    // default profile of H264 can fail on sink->Finalize with video bitrate > 80 mbits.
    //uint32_t avgVideoBitrate = (std::min)(videoParams->AvgBitrate, uint32_t(80 * 1000000));

    hr = mediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    H::System::ThrowIfFailed(hr);

    hr = MFSetAttributeRatio(mediaType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    H::System::ThrowIfFailed(hr);

    auto basicSettings = settings->GetBasicSettings();

    if (basicSettings) {
        hr = mediaType->SetUINT32(MF_MT_AVG_BITRATE, basicSettings->bitrate);
        H::System::ThrowIfFailed(hr);

        hr = MFSetAttributeSize(mediaType.Get(), MF_MT_FRAME_SIZE, basicSettings->width, basicSettings->height);
        H::System::ThrowIfFailed(hr);

        hr = MFSetAttributeRatio(mediaType.Get(), MF_MT_FRAME_RATE, basicSettings->fps, 1);
        H::System::ThrowIfFailed(hr);
    }

    return mediaType;
}

Microsoft::WRL::ComPtr<IMFMediaType> MediaRecorder::GetBestOutputType(
    IMFTransform *transform,
    IMFMediaType *defaultType,
    const GUID &subtypeGuid,
    uint32_t numChannels,
    uint32_t sampleRate,
    uint32_t bitsPerSample,
    uint32_t bitrate)
{
    Microsoft::WRL::ComPtr<IMFMediaType> outType;
    uint32_t outTypeIdx = 0;
    uint32_t outTypeBitrate = 0;
    HRESULT hr = S_OK;

    while (true) {
        Microsoft::WRL::ComPtr<IMFMediaType> curType;

        hr = transform->GetOutputAvailableType(0, outTypeIdx, curType.GetAddressOf());
        outTypeIdx++;
        if (hr == MF_E_NO_MORE_TYPES) {
            break;
        }
        if (hr != S_OK) {
            continue;
        }

        GUID curSubtype;

        hr = curType->GetGUID(MF_MT_SUBTYPE, &curSubtype);
        if (hr != S_OK || curSubtype != subtypeGuid) {
            continue;
        }

        uint32_t curChannels = 0, curSampleRate = 0;

        hr = curType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &curChannels);
        if (hr != S_OK) {
            continue;
        }

        hr = curType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &curSampleRate);
        if (hr != S_OK) {
            continue;
        }

        if (curChannels != numChannels || curSampleRate != sampleRate) {
            continue;
        }

        uint32_t curBitsPerSample = 0;

        hr = curType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &curBitsPerSample);
        if (hr == S_OK && !outType && curBitsPerSample == bitsPerSample) {
            outType = curType;
        }

        uint32_t curBitrate = 0;

        hr = curType->GetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &curBitrate);
        if (hr == S_OK) {
            curBitrate *= 8;

            int outBitrateDist = (int)outTypeBitrate - (int)bitrate;
            int curBitrateDist = (int)curBitrate - (int)bitrate;

            if ((curBitrate > outTypeBitrate && curBitrate <= bitrate) ||                           // set max from [min ... bitrate]
                (curBitrate > bitrate && (curBitrateDist < outBitrateDist || outBitrateDist < 0))   // set min or first from [bitrate ... max]
                )
            {
                outTypeBitrate = curBitrate;
                outType = curType;
            }
        }
    }

    if (!outType) {
        outType = defaultType;
    }

    return outType;
}

void MediaRecorder::WriteVideoSample(const Microsoft::WRL::ComPtr<IMFMediaBuffer> &buffer, int64_t hns, int64_t durationHns) {
    if (hns >= 0) {
        this->videoPtsHns = hns;
    }

    if (durationHns < 0) {
        durationHns = this->GetDefaultVideoFrameDuration();
    }

    this->WriteSample(buffer, this->videoPtsHns, durationHns, this->videoStreamIdx);

    this->videoPtsHns += durationHns;
}

void MediaRecorder::WriteSample(const Microsoft::WRL::ComPtr<IMFMediaBuffer> &buffer, int64_t positionHNS, int64_t durationHNS, DWORD streamIndex) {
    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<IMFSample> sample;

    hr = MFCreateSample(sample.GetAddressOf());
    H::System::ThrowIfFailed(hr);

    hr = sample->AddBuffer(buffer.Get());
    H::System::ThrowIfFailed(hr);

    hr = sample->SetSampleTime(positionHNS);
    H::System::ThrowIfFailed(hr);

    hr = sample->SetSampleDuration(durationHNS);
    H::System::ThrowIfFailed(hr);

    this->WriteSample(sample, streamIndex);
}

void MediaRecorder::WriteSample(const Microsoft::WRL::ComPtr<IMFSample> &sample, DWORD streamIndex) {
    HRESULT hr = S_OK;
    
    hr = this->sinkWriter->WriteSample(streamIndex, sample.Get());
    H::System::ThrowIfFailed(hr);
}

int64_t MediaRecorder::GetDefaultVideoFrameDuration() const {
    assert(this->params.mediaFormat.GetVideoCodecSettings());
    assert(this->params.mediaFormat.GetVideoCodecSettings()->GetBasicSettings());

    auto basicSettings = this->params.mediaFormat.GetVideoCodecSettings()->GetBasicSettings();
    int64_t hns = static_cast<int64_t>(H::Time::HNSResolution / basicSettings->fps);
    return hns;
}

void MediaRecorder::DefineContainerType() {
    auto containerType = this->params.mediaFormat.GetMediaContainerType();

    switch (containerType) {
    case MediaContainerType::MP4: containerExt = L".mp4"; break;
    case MediaContainerType::WMV: containerExt = L".wmv"; break;
    case MediaContainerType::MP3: containerExt = L".mp3"; break;
    case MediaContainerType::M4A: containerExt = L".m4a"; break;
    case MediaContainerType::FLAC: containerExt = L".flac"; break;
    case MediaContainerType::WMA: containerExt = L".wma"; break;
    case MediaContainerType::WAV: containerExt = L".wav"; break;
    case MediaContainerType::ThreeGPP: containerExt = L".3gpp"; break;
    default:
        break;
    }
}

IMFByteStream* MediaRecorder::StartNewChunk() {
    //TODO remove extension
    chunkFile = params.chunksPath + params.chunksGuid + L"-" + std::to_wstring(chunkNumber++) + containerExt;

    auto hr = MFCreateFile(MF_ACCESSMODE_READWRITE, MF_OPENMODE_DELETE_IF_EXIST, MF_FILEFLAGS_NONE, chunkFile.c_str(), currentOutputStream.GetAddressOf());
    H::System::ThrowIfFailed(hr);
    
    return currentOutputStream.Get();
}

void MediaRecorder::FinalizeRecord() {
    HRESULT hr = S_OK;
    framesNumber = 0;
    samplesNumber = 0;
    chunkAudioPtsHns = 0;
    chunkVideoPtsHns = 0;
    hr = this->sinkWriter->Finalize();
    if (hr != MF_E_SINK_NO_SAMPLES_PROCESSED) { // occured when was called BeginWritting but not calls WriteSample yet
        H::System::ThrowIfFailed(hr);
    }

    this->currentOutputStream.Reset();
    this->sinkWriter.Reset();

    recordedChunksSize += H::HardDrive::GetFilesize(chunkFile);
    auto freeSpaceTragetDisk = H::HardDrive::GetFreeMemory(targetRecordDisk);

    if (recordedChunksSize >= freeSpaceTragetDisk) {
        if (recordErrorCallback) {
            recordErrorCallback->call(MediaRecorderErrorsEnum::NotEnoughSpaceOnTargetRecordPath);
        }
    }
}

void MediaRecorder::ResetSinkWriterOnNewChunk() {
    FinalizeRecord();
    auto newStream = StartNewChunk();
    this->InitializeSinkWriter(newStream, cpuEncoding, nv12Textures);
    StartRecord();
}