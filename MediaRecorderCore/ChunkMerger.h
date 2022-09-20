#pragma once
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <stdio.h>
#include <mferror.h>
#include <wrl.h>
#include <shlwapi.h>
#include <string>
#include <libhelpers/HSystem.h>

class ChunkMerger {
public:
	ChunkMerger(IMFByteStream* outputStream, Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeAudioOut, std::vector<std::wstring>&& filesToMerge, std::wstring containerExts);
	//ChunkMerger(IMFByteStream* outputStream, std::vector<std::wstring>&& filesToMerge); // [not used]
	void Merge();
	

private:
	bool useVideoStream = true;
	bool useAudioStream = true;

	LONGLONG videoHns = 0;
	LONGLONG audioHns = 0;
	LONGLONG audioSamples = 0;
	UINT32 sampleRate = 0;
	//https://stackoverflow.com/questions/33401149/ffmpeg-adding-0-05-seconds-of-silence-to-transcoded-aac-file
	int firstSamplesCount = 2048;

	std::vector<std::wstring> filesToMerge;
	Microsoft::WRL::ComPtr<IMFSinkWriter> writer;

	DWORD videoStreamIndexToWrite = -1;
	DWORD audioStreamIndexToWrite = -1;

	bool WriteInner(Microsoft::WRL::ComPtr<IMFSinkWriter> writer, Microsoft::WRL::ComPtr<IMFSourceReader> reader, DWORD readFrom, DWORD writeTo, bool audio);

};