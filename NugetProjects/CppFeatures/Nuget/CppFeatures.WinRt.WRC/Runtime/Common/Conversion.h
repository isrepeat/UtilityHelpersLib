#pragma once

namespace winrt {
    template <typename ResultT>
    struct Type {
        template <typename FromT>
        static ResultT GetFrom(FromT value) {
            static_assert(false, "not found implementation");
        }
    };
}