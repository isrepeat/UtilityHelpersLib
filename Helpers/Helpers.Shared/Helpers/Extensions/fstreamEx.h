#pragma once
#include "Helpers/common.h"
#include "Helpers/Logger.h"
#include "Helpers/Regex.h"

#include <optional>
#include <fstream>
#include <string>
#include <vector>


namespace STD_EXT_NS {
	struct fstream_not_opened : ::std::exception {
		fstream_not_opened()
			: ::std::exception("stream not opened") {
		}
	};

	namespace details {
		int FindFileCodePageInFirstLineOfStream(::std::ifstream& fileStream) {
			if (!fileStream.is_open()) {
				throw ::std::ex::fstream_not_opened{};
			}

			auto prevFileStreamPointerPos = fileStream.tellg();
			fileStream.seekg(0, ::std::ios::beg);

			::std::string firstLine;
			if (::std::getline(fileStream, firstLine)) {
				auto matches = HELPERS_NS::Regex::GetRegexMatches(firstLine, ::std::regex("CodePage\\s*=\\s*(\\d+)"));
				if (!matches.empty()) {
					if (matches[0].capturedGroups.size() > 1) {
						// Do not recover file stream pointer pos.
						return ::std::stoi(matches[0].capturedGroups[1]);
					}
				}
			}

			fileStream.seekg(prevFileStreamPointerPos, std::ios::beg);
			return -1;
		}

		const ::std::vector<::std::vector<::std::uint8_t>> utfByteOrderMarks = {
			{ 0xEF, 0xBB, 0xBF }, // UTF8
		};

		bool CheckIfBytesArrayIsBOM(const ::std::vector<::std::uint8_t>& utfByteOrderMark, const ::std::vector<::std::uint8_t>& firstSomeBytes) {
			LOG_ASSERT(utfByteOrderMark.size() <= firstSomeBytes.size());

			for (int idx = 0; idx < utfByteOrderMark.size(); idx++) {
				if (utfByteOrderMark[idx] != firstSomeBytes[idx]) {
					return false;
				}
			}
			return true;
		}

		void SkipUTFByteOrderMarkInStream(::std::ifstream& fileStream) {
			if (!fileStream.is_open()) {
				throw std::ex::fstream_not_opened{};
			}

			auto prevFileStreamPointerPos = fileStream.tellg();
			if (prevFileStreamPointerPos != 0) {
				fileStream.seekg(0, std::ios::beg);
				LOG_WARNING_D("prevFileStreamPointerPos (= {}) was not point to begin, but after this function finish it will", static_cast<int>(prevFileStreamPointerPos));
			}

			std::vector<uint8_t> firstSomeBytes(10);
			fileStream.read((char*)firstSomeBytes.data(), firstSomeBytes.size());

			for (auto& utfByteOrderMark : utfByteOrderMarks) {
				if (CheckIfBytesArrayIsBOM(utfByteOrderMark, firstSomeBytes)) {
					fileStream.seekg(utfByteOrderMark.size(), std::ios::beg);
					return;
				}
			}
			fileStream.seekg(0, std::ios::beg);
		}
	} // namespace details


	struct ifstream : public ::std::ifstream {
		using ::std::ifstream::basic_ifstream;

		struct Data {
			::std::vector<char> byteArray;
			::std::optional<int> codePage;
		};

		Data ReadData() {
			if (!this->is_open()) {
				throw std::ex::fstream_not_opened{};
			}
			auto& ifstreamRef = static_cast<::std::ifstream&>(*this);

			details::SkipUTFByteOrderMarkInStream(ifstreamRef);

			Data data;
			auto codePage = details::FindFileCodePageInFirstLineOfStream(ifstreamRef);
			if (codePage != -1) {
				data.codePage = codePage;
			}
			data.byteArray = ::std::vector<char>{
				::std::istreambuf_iterator<char>(ifstreamRef),
				::std::istreambuf_iterator<char>()
			};
			return data;
		}
	};
}