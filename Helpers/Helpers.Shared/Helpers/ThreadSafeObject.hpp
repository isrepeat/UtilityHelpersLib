#pragma once
#include "common.h"
#include <string_view>
#include <mutex>

namespace HELPERS_NS {
    template<class MutexT, class T>
    class ThreadSafeObjectBase {
    public:
        class Locked {
        public:
            static constexpr std::string_view templateNotes = "Primary template";

            Locked(MutexT& mtx, T& obj)
                : lk(mtx)
                , obj(obj)
            {}

            T* operator->() const {
                return &this->obj;
            }

            T& Get() const {
                return this->obj;
            }

        private:
            std::unique_lock<MutexT> lk;
            T& obj;
        };


        ThreadSafeObjectBase() = default;

        template <typename... Args>
        ThreadSafeObjectBase(Args&&... args)
            : obj(std::forward<Args>(args)...)
        {}

        ThreadSafeObjectBase(ThreadSafeObjectBase& other) = delete;
        ThreadSafeObjectBase& operator=(ThreadSafeObjectBase& other) = delete;

        ThreadSafeObjectBase(T& objOther) = delete;
        ThreadSafeObjectBase& operator=(T& objOther) = delete;

        ThreadSafeObjectBase(T&& objOther)
            : obj(std::move(objOther))
        {}
        ThreadSafeObjectBase& operator=(T&& objOther) {
            if (&this->obj != &objOther) {
                this->obj = std::move(objOther);
            }
            return *this;
        }

        Locked Lock() {
            return Locked(this->mtx, this->obj);
        }

    private:
        MutexT mtx;
        T obj;
    };

    //
    // Can be used for pointer classes with overloaded operator->
    //
    template<class MutexT, template<class> class C, class... Args>
    class ThreadSafeObjectBase<MutexT, C<Args...>> {
    public:
        class Locked {
        public:
            static constexpr std::string_view templateNotes = "Pointer specialization. Obj must overload operator->.";

            Locked(MutexT& mtx, C<Args...>& obj)
                : lk(mtx)
                , obj(obj)
            {}

            C<Args...>& operator->() const {
                return this->obj;
            }

            C<Args...>& Get() const {
                return this->obj;
            }

        private:
            std::unique_lock<MutexT> lk;
            C<Args...>& obj;
        };


        ThreadSafeObjectBase() = default;

        ThreadSafeObjectBase(Args&&... args)
            : obj(std::forward<Args>(args)...)
        {}

        ThreadSafeObjectBase(ThreadSafeObjectBase& other) = delete;
        ThreadSafeObjectBase& operator=(ThreadSafeObjectBase& other) = delete;
        
        ThreadSafeObjectBase(C<Args...>& objOther) = delete;
        ThreadSafeObjectBase& operator=(C<Args...>& objOther) = delete;

        ThreadSafeObjectBase(C<Args...>&& objOther)
            : obj(std::move(objOther))
        {}
        ThreadSafeObjectBase& operator=(C<Args...>&& objOther) {
            if (&this->obj != &objOther) {
                this->obj = std::move(objOther);
            }
            return *this;
        }

        Locked Lock() {
            return Locked(this->mtx, this->obj);
        }

    private:
        MutexT mtx;
        C<Args...> obj;
    };


    template<class T>
    using ThreadSafeObject = ThreadSafeObjectBase<std::mutex, T>;

    template<class T>
    using ThreadSafeObjectRecusrive = ThreadSafeObjectBase<std::recursive_mutex, T>;
}