#pragma once
#include "common.h"
#include <unordered_map>
#include <type_traits>
#include <typeindex>
#include <memory>

namespace HELPERS_NS {
	//
	// IServiceProvider: типобезопасный доступ к расширениям (сервисам)
	//
	struct IServiceProvider {
	public:
		virtual ~IServiceProvider() = default;

	public:
		template <typename TInterface>
		std::shared_ptr<const TInterface> Get() const {
			auto servicePtr = this->OnGetServiceShared(typeid(TInterface));
			if (!servicePtr) {
				return {};
			}
			return std::static_pointer_cast<const TInterface>(servicePtr);
		}

	protected:
		virtual std::shared_ptr<const void> OnGetServiceShared(
			const std::type_info& typeInfo
		) const = 0;
	};


	//
	// DefaultServiceProvider: простая реализация на базе registry
	//
	struct DefaultServiceProvider final : public IServiceProvider {
	public:
		void AddService(
			const std::type_info& typeInfo,
			std::shared_ptr<const void> serviceSharedPtr
		) {
			this->mapTypeToServiceSharedPtr[std::type_index{ typeInfo }] = std::move(serviceSharedPtr);
		}

		template <typename TInterface>
		void AddService(
			std::shared_ptr<TInterface> serviceSharedPtr
		) {
			this->AddService(
				typeid(TInterface),
				std::static_pointer_cast<const void>(std::move(serviceSharedPtr))
			);
		}

	protected:
		std::shared_ptr<const void> OnGetServiceShared(
			const std::type_info& typeInfo
		) const override {
			const auto it = this->mapTypeToServiceSharedPtr.find(std::type_index{ typeInfo });
			if (it != this->mapTypeToServiceSharedPtr.end()) {
				return it->second;
			}
			return {};
		}

	private:
		std::unordered_map<std::type_index, std::shared_ptr<const void>> mapTypeToServiceSharedPtr;
	};
}