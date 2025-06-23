#pragma once
#include "common.h"
#include <cstdint>
#include <string>
#include <array>

namespace HELPERS_NS {
	class Guid {
	public:
		static Guid NewGuid();

		Guid();
		explicit Guid(const std::string& guidStr);
		static Guid Parse(const std::string& guidStr);

		// ¬озвращает неизмен€емую ссылку на массив байт (безопасный доступ)
		const std::array<uint8_t, 16>& GetBytesArray() const;

		// ¬озвращает указатель на начало массива (например, дл€ reinterpret_cast)
		const uint8_t* data() const;

		std::string ToString() const;

		bool operator==(const Guid& other) const;
		bool operator!=(const Guid& other) const;
		bool operator<(const Guid& other) const;
		bool operator>(const Guid& other) const;
		bool operator<=(const Guid& other) const;
		bool operator>=(const Guid& other) const;

	private:
		std::array<uint8_t, 16> bytesArray{};
	};
}


namespace std {
	template<>
	struct hash<HELPERS_NS::Guid> {
		std::size_t operator()(const HELPERS_NS::Guid& guid) const noexcept {
			const auto& bytesArray = guid.GetBytesArray();
			const uint64_t* pData = reinterpret_cast<const uint64_t*>(bytesArray.data());

			// ѕростой способ: XOR двух 64-битных слов
			return std::hash<uint64_t>{}(pData[0]) ^ std::hash<uint64_t>{}(pData[1]);
		}
	};
}