#pragma once
#include "common.h"
#include <memory>

namespace HELPERS_NS {
    // https://codereview.stackexchange.com/questions/256651/dynamic-pointer-cast-for-stdunique-ptr
    // https://stackoverflow.com/questions/26377430/how-to-perform-a-dynamic-cast-with-a-unique-ptr
    template <typename To, typename From, typename Deleter>
    std::unique_ptr<To, Deleter> dynamic_unique_cast(std::unique_ptr<From, Deleter>&& ptr) {
        if (To* cast = dynamic_cast<To*>(ptr.get())) {
            std::unique_ptr<To, Deleter> result(cast, std::move(ptr.get_deleter()));
            ptr.release();
            return result;
        }
        return {};
    }

    template <typename To, typename From>
    std::unique_ptr<To> dynamic_unique_cast(std::unique_ptr<From>&& ptr) {
        if (To* cast = dynamic_cast<To*>(ptr.get())) {
            std::unique_ptr<To> result(cast);
            ptr.release();
            return result;
        }
        return {};
    }

    template <typename SmartPointerT>
    SmartPointerT& EmptyPointerRef() {
        static std::remove_reference_t<SmartPointerT> emptyPoiner = nullptr;
        return emptyPoiner;
    }
}