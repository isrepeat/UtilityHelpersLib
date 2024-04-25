#pragma once
#include "common.h"
#include <string_view>
#include <mutex>

namespace HELPERS_NS {
    template <typename MutexT, typename T>
    class LockedObj {
    public:
        static constexpr std::string_view templateNotes = "Primary template for value types";

        LockedObj(MutexT& mtx, T& obj)
            : lk(mtx)
            , obj(obj)
        {}

        LockedObj(LockedObj&& other)
            : lk{ std::move(other.lk) }
            , obj{ std::move(other.obj) }
        {}

        LockedObj& operator=(LockedObj&& other) {
            if (this != &other) {
                this->lk = std::move(other.lk);
                this->obj = std::move(other.obj);
            }
            return *this;
        }

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

    //
    // Partial template specializatoin where T is container = C<Args...>.
    // Can be used for pointer classes with overloaded operator->
    //
    template<typename MutexT, template<typename> class C, typename... Args>
    class LockedObj<MutexT, C<Args...>> {
    public:
        static constexpr std::string_view templateNotes = "Pointer specialization. Obj must overload operator->.";

        LockedObj(MutexT& mtx, C<Args...>& obj)
            : lk(mtx)
            , obj(obj)
        {}

        LockedObj(LockedObj&& other)
            : lk{ std::move(other.lk) }
            , obj{ std::move(other.obj) }
        {}

        LockedObj& operator=(LockedObj&& other) {
            if (this != &other) {
                this->lk = std::move(other.lk);
                this->obj = std::move(other.obj);
            }
            return *this;
        }

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



    template<typename MutexT, typename T>
    class ThreadSafeObjectBase {
    public:
        using Locked = LockedObj<MutexT, T>;

        ThreadSafeObjectBase() = default;

        template <typename... Args>
        ThreadSafeObjectBase(Args&&... args)
            : obj{ std::forward<Args>(args)... }
        {}

        ThreadSafeObjectBase(ThreadSafeObjectBase& other) = delete;
        ThreadSafeObjectBase& operator=(ThreadSafeObjectBase& other) = delete;

        template<typename OtherT, std::enable_if_t<std::is_convertible_v<OtherT, T>, int> = 0>
        ThreadSafeObjectBase(OtherT&& otherObj)
            : obj{ std::move(otherObj) }
        {}

        template<typename OtherT, std::enable_if_t<std::is_convertible_v<OtherT, T>, int> = 0>
        ThreadSafeObjectBase& operator=(OtherT&& otherObj) {
            if (&this->obj != &otherObj) {
                this->obj = std::move(otherObj);
            }
            return *this;
        }

        Locked Lock() {
            return Locked(this->mtx, this->obj);
        }

        std::unique_ptr<Locked> LockUniq() {
            return std::make_unique<Locked>(this->mtx, this->obj);
        }

    private:
        template<typename, typename>
        friend class ThreadSafeObjectBase;

        MutexT mtx;
        T obj;
    };



    template<typename MutexT, typename T>
    class ThreadSafeObjectBaseUniq {
    public:
        using Locked = LockedObj<MutexT, T>;

        ThreadSafeObjectBaseUniq()
            : obj{ std::make_unique<T>() }
        {}

        template <typename... Args>
        ThreadSafeObjectBaseUniq(Args&&... args)
            : obj{ std::make_unique<T>(std::forward<Args>(args)...) }
        {}

        ThreadSafeObjectBaseUniq(ThreadSafeObjectBaseUniq& other) = delete;
        ThreadSafeObjectBaseUniq& operator=(ThreadSafeObjectBaseUniq& other) = delete;


        template <typename OtherT, std::enable_if_t<std::is_convertible_v<std::unique_ptr<OtherT>, std::unique_ptr<T>>, int> = 0>
        ThreadSafeObjectBaseUniq(std::unique_ptr<OtherT>&& otherObj)
            : obj{ std::move(otherObj) }
        {}

        template <typename OtherT, std::enable_if_t<std::is_convertible_v<std::unique_ptr<OtherT>, std::unique_ptr<T>>, int> = 0>
        ThreadSafeObjectBaseUniq& operator=(std::unique_ptr<OtherT>&& otherObj) {
            if (this->obj.get() != otherObj.get()) {
                this->obj = std::move(otherObj);
            }
            return *this;
        }

        Locked Lock() {
            return Locked(this->mtx, *this->obj);
        }

        std::unique_ptr<Locked> LockUniq() {
            return std::make_unique<Locked>(this->mtx, *this->obj);
        }

    private:
        MutexT mtx;
        std::unique_ptr<T> obj;
    };

    template<class T>
    using ThreadSafeObject = ThreadSafeObjectBase<std::mutex, T>;

    template<class T>
    using ThreadSafeObjectRecusrive = ThreadSafeObjectBase<std::recursive_mutex, T>;


    template<class T>
    using ThreadSafeObjectUniq = ThreadSafeObjectBaseUniq<std::mutex, T>;

    template<class T>
    using ThreadSafeObjectUniqRecusrive = ThreadSafeObjectBaseUniq<std::recursive_mutex, T>;


    // TODO: overload for ENSURE_METHOD_IS_GUARDED(lockClass) and ENSURE_METHOD_IS_GUARDED(lockClass, mutexClass)
    //#define ENSURE_METHOD_IS_GUARDED std::lock_guard<std::mutex>&

    template<template<typename> class LockT = std::lock_guard, typename MutexT = std::mutex>
    struct GuardMethodHelper {
        using type = LockT<MutexT>&;
        using defaultGuard = std::lock_guard<std::mutex>&;
    };

    using MethodIsGuarded = typename GuardMethodHelper<>::defaultGuard;
}