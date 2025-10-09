#pragma once
#include "common.h"
#include <system_error>
#include <string_view>
#include <algorithm>
#include <cwctype>
#include <vector>
#include <string>
#include <memory>

#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max

namespace HELPERS_NS {
	namespace Text {
		// Универсальные ошибки конвертации как std::system_error
		inline std::system_error MakeWin32Error(
			char const* where
		) {
			const DWORD err = ::GetLastError();
			return std::system_error(
				static_cast<int>(err),
				std::system_category(),
				where
			);
		}

		//
		// ░ Узкие -> широкие (любая кодовая страница)
		// ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
		//
		inline std::wstring MultiByteToWide(
			std::string_view bytes,
			UINT codePage,
			DWORD flags = MB_ERR_INVALID_CHARS
		) {
			if (bytes.empty()) {
				return std::wstring{};
			}

			// Может быть > INT_MAX на теории, проверим
			if (bytes.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
				throw std::length_error("MultiByteToWide: input too large");
			}

			int const srcLen = static_cast<int>(bytes.size());

			// 1) Узнаём нужную длину UTF-16 без завершающего нуля
			int const wlen = ::MultiByteToWideChar(
				codePage,
				flags,
				bytes.data(),
				srcLen,
				nullptr,
				0
			);
			if (wlen <= 0) {
				throw MakeWin32Error("MultiByteToWideChar(size)");
			}

			std::wstring wstr;
			wstr.resize(static_cast<size_t>(wlen));

			// 2) Конвертируем
			int const written = ::MultiByteToWideChar(
				codePage,
				flags,
				bytes.data(),
				srcLen,
				wstr.data(),
				wlen
			);
			if (written != wlen) {
				throw MakeWin32Error("MultiByteToWideChar(convert)");
			}

			return wstr;
		}

		//
		// ░ Широкие -> узкие (любая кодовая страница)
		// ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
		//
		inline std::string WideToMultiByte(
			std::wstring_view wtext,
			UINT codePage,
			DWORD flags = WC_ERR_INVALID_CHARS
		) {
			if (wtext.empty()) {
				return std::string{};
			}

			if (wtext.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
				throw std::length_error("WideToMultiByte: input too large");
			}

			int const srcLen = static_cast<int>(wtext.size());

			int const blen = ::WideCharToMultiByte(
				codePage,
				flags,
				wtext.data(),
				srcLen,
				nullptr,
				0,
				nullptr,
				nullptr
			);
			if (blen <= 0) {
				throw MakeWin32Error("WideCharToMultiByte(size)");
			}

			std::string bytes;
			bytes.resize(static_cast<size_t>(blen));

			int const written = ::WideCharToMultiByte(
				codePage,
				flags,
				wtext.data(),
				srcLen,
				bytes.data(),
				blen,
				nullptr,
				nullptr
			);
			if (written != blen) {
				throw MakeWin32Error("WideCharToMultiByte(convert)");
			}

			return bytes;
		}

		//
		// ░ Удобные алиасы для UTF-8 <-> UTF-16
		// ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
		//
		inline std::wstring Utf8ToUtf16(std::string_view bytes) {
			return MultiByteToWide(
				bytes,
				CP_UTF8,
				MB_ERR_INVALID_CHARS
			);
		}

		inline std::string Utf16ToUtf8(std::wstring_view wtext) {
			return WideToMultiByte(
				wtext,
				CP_UTF8,
				WC_ERR_INVALID_CHARS
			);
		}

		//
		// ░ Вспомогательные векторные варианты
		// ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
		//
		inline std::wstring VecToWString(std::vector<wchar_t> const& vec) {
			return std::wstring{ vec.begin(), vec.end() };
		}

		inline std::string VecToString(std::vector<char> const& vec) {
			return std::string{ vec.begin(), vec.end() };
		}
	}
} // namespace HELPERS_NS


#if COMPILE_FOR_CLR
#include <msclr/marshal.h>

namespace HELPERS_NS {
	namespace CLR {
		namespace Text {
			inline std::string ToUtf8(System::String^ s) {
				if (s == nullptr) {
					return {};
				}

				cli::array<System::Byte>^ bytes = System::Text::Encoding::UTF8->GetBytes(s);

				std::string out;
				out.resize(bytes->Length);

				// Копируем за один проход
				if (bytes->Length > 0) {
					cli::pin_ptr<System::Byte> p = &bytes[0];
					std::memcpy(out.data(), p, static_cast<size_t>(bytes->Length));
				}

				return out;
			}


			inline std::wstring ToWStr(System::String^ s) {
				if (s == nullptr) {
					return std::wstring{};
				}

				static_assert(sizeof(wchar_t) == 2, "wchar_t must be UTF-16 on Windows");

				// .NET String — UTF-16. Получаем прямой доступ к внутреннему буферу
				cli::pin_ptr<const wchar_t> pinned = ::PtrToStringChars(s);

				// Копируем содержимое в std::wstring
				return std::wstring{ 
					static_cast<const wchar_t*>(pinned),
					static_cast<std::size_t>(s->Length)
				};
			}


			// Требует: MSVC/Windows, где wchar_t == 2 байта (UTF-16)
			inline std::u16string ToU16(System::String^ s) {
				if (s == nullptr) {
					return {};
				}

				static_assert(sizeof(wchar_t) == 2, "wchar_t must be UTF-16 on Windows");

				// .NET String — всегда UTF-16; получаем pointer к внутреннему буферу
				cli::pin_ptr<const wchar_t> pinned = ::PtrToStringChars(s);

				// Без ручного «пошагового» копирования: допускается побайтовое копирование
				auto const* src16 = reinterpret_cast<const char16_t*>(static_cast<const wchar_t*>(pinned));

				std::u16string out;
				out.assign(src16, src16 + s->Length);

				return out;
			}


			inline System::String^ FromUtf8(std::string_view utf8) {
				if (utf8.empty()) {
					return gcnew System::String(L"");
				}

				const int len = static_cast<int>(utf8.size());
				cli::array<System::Byte>^ bytes = gcnew cli::array<System::Byte>(len);

				// Копируем в управляемый массив
				{
					cli::pin_ptr<System::Byte> dst = &bytes[0];
					std::memcpy(dst, utf8.data(), static_cast<std::size_t>(len));
				}

				return System::Text::Encoding::UTF8->GetString(bytes);
			}


			inline System::String^ FromWStr(std::wstring_view wstr) {
				if (wstr.empty()) {
					return gcnew System::String(L"");
				}

				// Прямой конструктор из wchar_t*
				return gcnew System::String(wstr.data(), 0, static_cast<int>(wstr.size()));
			}


			inline System::String^ FromU16(std::u16string_view u16) {
				if (u16.empty()) {
					return gcnew System::String(L"");
				}

				// Без перекодировки: .NET строка — UTF-16
				return gcnew System::String(
					reinterpret_cast<const wchar_t*>(u16.data()),
					0,
					static_cast<int>(u16.size())
				);
			}
		}
	}
}
#endif


namespace HELPERS_NS {
	template <typename T>
	struct StringDeductor {
		template <typename A> StringDeductor(A) {}
		using type = T;
	};

	// user-defined deduction guides:
	StringDeductor(const char*)->StringDeductor<char>;
	StringDeductor(const wchar_t*)->StringDeductor<wchar_t>;
	StringDeductor(std::string)->StringDeductor<char>;
	StringDeductor(std::wstring)->StringDeductor<wchar_t>;
	StringDeductor(std::string_view)->StringDeductor<char>;
	StringDeductor(std::wstring_view)->StringDeductor<wchar_t>;


	inline std::wstring ToLower(std::wstring wstr) {
		std::transform(wstr.begin(), wstr.end(), wstr.begin(), std::towlower);
		return wstr;
	}
	inline std::string ToLower(std::string str) {
		std::transform(str.begin(), str.end(), str.begin(), std::towlower);
		return str;
	}
	inline std::wstring ToUpper(std::wstring wstr) {
		std::transform(wstr.begin(), wstr.end(), wstr.begin(), std::towupper);
		return wstr;
	}
	inline std::string ToUpper(std::string str) {
		std::transform(str.begin(), str.end(), str.begin(), std::towupper);
		return str;
	}

	template<typename T, typename... Args>
	int StringPrintFormat(T* buffer, size_t size, const T* format, Args... args) {
		if constexpr (std::is_same_v<T, wchar_t>) {
			return std::swprintf(buffer, size, format, args...);
		}
		else {
			return std::snprintf(buffer, size, format, args...);
		}
	}

	template<typename T, typename... Args>
	std::basic_string<T> StringFormat(const std::basic_string<T>& format, Args... args) {
		size_t size = StringPrintFormat<T>(nullptr, 0, format.c_str(), std::forward<Args>(args)...) + 1; // Extra space for '\0'
		if (size <= 0) {
			throw std::runtime_error("Error during formatting.");
		}
		std::unique_ptr<T[]> buf(new T[size]);
		StringPrintFormat<T>(buf.get(), size, format.c_str(), std::forward<Args>(args)...);
		return std::basic_string<T>(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
	}

	// Overload for raw char/wchar_t (template deduction can't work with type conversion)
	template<typename T, typename... Args>
	std::basic_string<T> StringFormat(const T* format, Args... args) {
		return StringFormat(std::basic_string<T>(format), std::forward<Args>(args)...);
	}

	template<typename T>
	std::basic_string<T> GetStringByType(std::basic_string_view<char> s1, std::basic_string_view<wchar_t> s2) {
		return std::get<std::basic_string_view<T>>(std::tuple<std::basic_string_view<char>, std::basic_string_view<wchar_t>>(s1, s2));
	}

	template<typename T>
	static constexpr std::basic_string_view<T> GetStringViewByType(std::basic_string_view<char> s1, std::basic_string_view<wchar_t> s2) {
		return std::get<std::basic_string_view<T>>(std::tuple<std::basic_string_view<char>, std::basic_string_view<wchar_t>>(s1, s2));
	}
}

#define JOIN_STRING(A, B) A B // keep space between A and B

#define MAKE_STRING_T(T, str) HELPERS_NS::GetStringByType<T>(str, L""##str)
#define MAKE_STRING_VIEW_T(T, str) HELPERS_NS::GetStringViewByType<T>(str, L""##str)

#pragma pop_macro("max")
#pragma pop_macro("min")