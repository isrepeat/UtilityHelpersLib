#include "ChunkMerger.h"
#include <libhelpers/HMathCP.h>
#include <libhelpers/HTime.h>

ChunkMerger::ChunkMerger(IMFByteStream* outputStream, Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeAudioOut, std::vector<std::wstring>&& filesToMerge, std::wstring containerExt) : filesToMerge{ filesToMerge } {	
	Microsoft::WRL::ComPtr<IMFSourceReader> reader;
	HRESULT hr = MFCreateSourceReaderFromURL(filesToMerge[0].c_str(), nullptr, reader.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	Microsoft::WRL::ComPtr<IMFByteStream> byteStream = outputStream;
	hr = MFCreateSinkWriterFromURL(containerExt.c_str(), byteStream.Get(), nullptr, writer.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	if (mediaTypeAudioOut == nullptr) {
		useAudioStream = false;
	}
	if (useAudioStream) {
		Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeAudioBetween;
		hr = MFCreateMediaType(mediaTypeAudioBetween.ReleaseAndGetAddressOf());
		H::System::ThrowIfFailed(hr);

		hr = mediaTypeAudioBetween->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
		H::System::ThrowIfFailed(hr);

		hr = mediaTypeAudioBetween->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
		H::System::ThrowIfFailed(hr);

		hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, mediaTypeAudioBetween.Get());
		H::System::ThrowIfFailed(hr);

		Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeAudioForWriter;
		hr = reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, mediaTypeAudioForWriter.GetAddressOf());
		//if (hr == MF_E_INVALIDSTREAMNUMBER) {
		//	useAudioStream = false;
		//}

		hr = writer->AddStream(mediaTypeAudioOut.Get(), &audioStreamIndexToWrite);
		H::System::ThrowIfFailed(hr);

		hr = writer->SetInputMediaType(audioStreamIndexToWrite, mediaTypeAudioForWriter.Get(), nullptr);
		H::System::ThrowIfFailed(hr);
	}

	Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeVideo;
	hr = reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, mediaTypeVideo.GetAddressOf());
	if (hr == MF_E_INVALIDSTREAMNUMBER) { // H::System::ThrowIfFailed(hr);
		useVideoStream = false;
	}
	if (useVideoStream) {
		hr = writer->AddStream(mediaTypeVideo.Get(), &videoStreamIndexToWrite);
		H::System::ThrowIfFailed(hr);

		hr = writer->SetInputMediaType(videoStreamIndexToWrite, mediaTypeVideo.Get(), NULL);
		H::System::ThrowIfFailed(hr);
	}

	hr = writer->BeginWriting();
	H::System::ThrowIfFailed(hr);
}




//ChunkMerger::ChunkMerger(IMFByteStream* outputStream, std::vector<std::wstring>&& filesToMerge) : filesToMerge{ filesToMerge } {
//	Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeVideo;
//	Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeAudio;
//
//	Microsoft::WRL::ComPtr<IMFSourceReader> reader;
//	HRESULT hr = MFCreateSourceReaderFromURL(filesToMerge[0].c_str(), nullptr, reader.GetAddressOf());
//	H::System::ThrowIfFailed(hr);
//
//	hr = reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, mediaTypeVideo.GetAddressOf());
//	H::System::ThrowIfFailed(hr);
//
//
//	hr = reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, mediaTypeAudio.GetAddressOf());
//	H::System::ThrowIfFailed(hr);
//
//
//	Microsoft::WRL::ComPtr<IMFByteStream> byteStream = outputStream;
//
//
//	hr = MFCreateSinkWriterFromURL(L".mp4", byteStream.Get(), nullptr, writer.GetAddressOf());
//	H::System::ThrowIfFailed(hr);
//
//	GUID codec;
//	UINT32 channels;
//	UINT32 bitsPerSample;
//	UINT32 avgBytesPerSecond;
//
//	hr = mediaTypeAudio->GetGUID(MF_MT_SUBTYPE, &codec);
//	H::System::ThrowIfFailed(hr);
//
//	hr = mediaTypeAudio->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);
//	H::System::ThrowIfFailed(hr);
//
//	hr = mediaTypeAudio->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate);
//	H::System::ThrowIfFailed(hr);
//
//	hr = mediaTypeAudio->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bitsPerSample);
//	H::System::ThrowIfFailed(hr);
//
//	hr = mediaTypeAudio->GetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &avgBytesPerSecond);
//	H::System::ThrowIfFailed(hr);
//
//
//	Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeAudioOut;
//	hr = MFCreateMediaType(mediaTypeAudioOut.ReleaseAndGetAddressOf());
//	H::System::ThrowIfFailed(hr);
//
//	hr = mediaTypeAudioOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
//	H::System::ThrowIfFailed(hr);
//
//
//	hr = mediaTypeAudioOut->SetGUID(MF_MT_SUBTYPE, codec);
//	H::System::ThrowIfFailed(hr);
//
//
//	hr = mediaTypeAudioOut->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, channels);
//	H::System::ThrowIfFailed(hr);
//
//	hr = mediaTypeAudioOut->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, sampleRate);
//	H::System::ThrowIfFailed(hr);
//
//
//	hr = mediaTypeAudioOut->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, avgBytesPerSecond);
//	H::System::ThrowIfFailed(hr);
//
//	hr = mediaTypeAudioOut->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, bitsPerSample);
//	H::System::ThrowIfFailed(hr);
//
//	hr = writer->AddStream(mediaTypeAudioOut.Get(), &audioStreamIndexToWrite);
//	H::System::ThrowIfFailed(hr);	
//	
//
//	Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeAudioBetween;
//	hr = MFCreateMediaType(mediaTypeAudioBetween.ReleaseAndGetAddressOf());
//	H::System::ThrowIfFailed(hr);
//
//	hr = mediaTypeAudioBetween->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
//	H::System::ThrowIfFailed(hr);
//
//
//	hr = mediaTypeAudioBetween->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
//	H::System::ThrowIfFailed(hr);
//
//	hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, mediaTypeAudioBetween.Get());
//	H::System::ThrowIfFailed(hr);
//
//	Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeForWriter;
//	hr = reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, mediaTypeForWriter.GetAddressOf());
//	H::System::ThrowIfFailed(hr);
//
//	hr = writer->SetInputMediaType(audioStreamIndexToWrite, mediaTypeForWriter.Get(), nullptr);
//	H::System::ThrowIfFailed(hr);
//
//	hr = writer->AddStream(mediaTypeVideo.Get(), &videoStreamIndexToWrite);
//	H::System::ThrowIfFailed(hr);
//
//	hr = writer->SetInputMediaType(videoStreamIndexToWrite, mediaTypeVideo.Get(), NULL);
//	H::System::ThrowIfFailed(hr);
//
//
//	hr = writer->BeginWriting();
//	H::System::ThrowIfFailed(hr);
//}

void ChunkMerger::Merge() {
	for each (auto file in filesToMerge) {
		Microsoft::WRL::ComPtr<IMFSourceReader> reader;
		HRESULT hr = MFCreateSourceReaderFromURL(file.c_str(), nullptr, reader.GetAddressOf());
		H::System::ThrowIfFailed(hr);

		if (useAudioStream) {
			Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeAudioBetween;
			hr = MFCreateMediaType(mediaTypeAudioBetween.ReleaseAndGetAddressOf());
			H::System::ThrowIfFailed(hr);

			hr = mediaTypeAudioBetween->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
			H::System::ThrowIfFailed(hr);


			hr = mediaTypeAudioBetween->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
			H::System::ThrowIfFailed(hr);

			hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, mediaTypeAudioBetween.Get());
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