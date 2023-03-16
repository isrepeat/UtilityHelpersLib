#include "ChunkMerger.h"
#include <libhelpers/HMathCP.h>
#include <libhelpers/HTime.h>
#include "MediaFormat/MediaFormatCodecsSupport.h"
#include "Platform/PlatformClassFactory.h"

// NOTE: Merging takes longer than just writing bytes into a stream. Maybe optimize somehow?
ChunkMerger::ChunkMerger(IMFByteStream* outputStream,
	Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeAudioIn, Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeAudioOut,
	Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeVideoIn, Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeVideoOut,
	const IVideoCodecSettings* settings, std::vector<std::wstring>&& filesToMerge, std::wstring containerExt) 
	: mediaTypeAudioIn{ mediaTypeAudioIn }
	, mediaTypeAudioOut{ mediaTypeAudioOut }
	, mediaTypeVideoIn{ mediaTypeVideoIn }
	//, mediaTypeVideoOut{ mediaTypeVideoOut } // not use for now, because merging HEVC -> HEVC was failed
	, filesToMerge{ filesToMerge }
	, settings{ settings }
{
	Microsoft::WRL::ComPtr<IMFSourceReader> reader;
	HRESULT hr = MFCreateSourceReaderFromURL(filesToMerge[0].c_str(), nullptr, reader.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	Microsoft::WRL::ComPtr<IMFByteStream> byteStream = outputStream;
	hr = MFCreateSinkWriterFromURL(containerExt.c_str(), byteStream.Get(), nullptr, writer.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	if (mediaTypeAudioOut == nullptr) {
		useAudioStream = false;
	}
	if (mediaTypeVideoOut == nullptr) {
		useVideoStream = false;
	}

	if (useAudioStream) {
		hr = writer->AddStream(this->mediaTypeAudioOut.Get(), &audioStreamIndexToWrite);
		H::System::ThrowIfFailed(hr);

		hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, this->mediaTypeAudioIn.Get());
		H::System::ThrowIfFailed(hr);

		Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeAudioForWriter;
		hr = reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, mediaTypeAudioForWriter.GetAddressOf());
		H::System::ThrowIfFailed(hr);

		hr = writer->SetInputMediaType(audioStreamIndexToWrite, mediaTypeAudioForWriter.Get(), nullptr);
		H::System::ThrowIfFailed(hr);
	}
	if (useVideoStream) {
		// HEVC TRANSCODING WORKAROUND:
		// Incoming video must be encoded in H264 for always types, otherwise we can't mrege form HEVC to HEVC,
		// but can H264 -> HEVC or H264 -> H264.
		this->mediaTypeVideoOut = CreateVideoOutMediaType();

		hr = writer->AddStream(this->mediaTypeVideoOut.Get(), &videoStreamIndexToWrite);
		H::System::ThrowIfFailed(hr);

		hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, this->mediaTypeVideoIn.Get());
		H::System::ThrowIfFailed(hr);

		Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeVideoForWriter;
		hr = reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, mediaTypeVideoForWriter.GetAddressOf());
		H::System::ThrowIfFailed(hr);

		hr = writer->SetInputMediaType(videoStreamIndexToWrite, mediaTypeVideoForWriter.Get(), NULL);
		H::System::ThrowIfFailed(hr);
	}

	hr = writer->BeginWriting();
	H::System::ThrowIfFailed(hr);
}



Microsoft::WRL::ComPtr<IMFMediaType> ChunkMerger::CreateVideoOutMediaType()
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

		uint32_t testBitrate;
		hr = mediaType->GetUINT32(MF_MT_AVG_BITRATE, &testBitrate);
		H::System::ThrowIfFailed(hr);

		hr = MFSetAttributeSize(mediaType.Get(), MF_MT_FRAME_SIZE, basicSettings->width, basicSettings->height);
		H::System::ThrowIfFailed(hr);

		hr = MFSetAttributeRatio(mediaType.Get(), MF_MT_FRAME_RATE, basicSettings->fps, 1);
		H::System::ThrowIfFailed(hr);
	}

	return mediaType;
}


void ChunkMerger::Merge() {
	try {
		for each (auto file in filesToMerge) {
			Microsoft::WRL::ComPtr<IMFSourceReader> reader;
			HRESULT hr = MFCreateSourceReaderFromURL(file.c_str(), nullptr, reader.GetAddressOf());
			H::System::ThrowIfFailed(hr);

			if (useAudioStream) {
				hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, mediaTypeAudioIn.Get());
				H::System::ThrowIfFailed(hr);
			}

			if (useVideoStream) {
				hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, mediaTypeVideoIn.Get());
				H::System::ThrowIfFailed(hr);
			}

			bool videoDone = false;
			bool audioDone = false;

			while (true) {
				if (useVideoStream && useAudioStream) {
					videoDone = WriteInner(writer, reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, videoStreamIndexToWrite, false);

					while ((audioHns < videoHns && !audioDone) || (videoDone && !audioDone)) {
						audioDone = WriteInner(writer, reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, audioStreamIndexToWrite, true);
					}
					if (videoDone && audioDone) {
						firstSamplesCount = 2048;
						break;
					}
				}
				else if (useAudioStream) {
					while (!audioDone) {
						audioDone = WriteInner(writer, reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, audioStreamIndexToWrite, true);
					}

					firstSamplesCount = 2048;
					break;
				}
				else if (useVideoStream) {
					videoDone = WriteInner(writer, reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, videoStreamIndexToWrite, false);
					if (videoDone)
						break;
				}
			}
		}

		auto hr = writer->Finalize();
		if (hr != MF_E_SINK_NO_SAMPLES_PROCESSED) { // occured when was called BeginWritting but not calls WriteSample yet
			H::System::ThrowIfFailed(hr);
		}
	}
	catch (HResultException& ex) {
		// If was error during merging try end writing with already handled chunks
		if (ex.GetHRESULT() == MF_E_INVALIDSTREAMNUMBER) {
			auto hr = writer->Finalize();
			H::System::ThrowIfFailed(hr);
		}
		else {
			throw;
		}
	}
}

bool ChunkMerger::WriteInner(Microsoft::WRL::ComPtr<IMFSinkWriter> writer, Microsoft::WRL::ComPtr<IMFSourceReader> reader, DWORD readFrom, DWORD writeTo, bool audio) {
	// NOTE: If not enaugh space on disk where merging chunks may be 0xC00D36B3: MF_E_INVALIDSTREAMNUMBER
	HRESULT hr = reader->SetStreamSelection(readFrom, true);
	H::System::ThrowIfFailed(hr);

	Microsoft::WRL::ComPtr<IMFSample> sample;
	DWORD stream;
	LONGLONG pts;

	hr = reader->ReadSample(readFrom, 0, nullptr, &stream, &pts, sample.GetAddressOf());
	if (FAILED(hr) || (sample == nullptr && stream == MF_SOURCE_READERF_ENDOFSTREAM))
		return true;

	
	LONGLONG duration;
	
	hr = sample->GetSampleDuration(&duration);
	H::System::ThrowIfFailed(hr);

	if (audio) {
		if (firstSamplesCount > 0) {
			Microsoft::WRL::ComPtr<IMFMediaBuffer> mediaBuffer;
			hr = sample->ConvertToContiguousBuffer(&mediaBuffer);
			DWORD len;
			hr = mediaBuffer->GetCurrentLength(&len);

			firstSamplesCount -= len / 2;
			return false;
		}
		hr = sample->SetSampleTime(audioHns);
		audioHns += duration;
	}
	else {
		hr = sample->SetSampleTime(videoHns);
		videoHns += duration;
	}
	H::System::ThrowIfFailed(hr);
	hr = writer->WriteSample(writeTo, sample.Get());
	if (FAILED(hr))
		return true;

	return false;
}