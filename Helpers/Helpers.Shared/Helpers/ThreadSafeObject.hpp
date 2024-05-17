#pragma once
#include "common.h"
#include <string_view>
#include <mutex>

namespace HELPERS_NS {
    //
    // LockedObjBase
    // NOTE: CreatorT::MutexT must be declared as mutable. 
    //       Keep other "LockedObj..." constructors params as const-members 
    //       to allow make "LockedObj" object from CreatorT with non-const members.
    //
    template <typename CreatorT>
    struct LockedObjBase {
        static constexpr std::string_view templateNotes = "Primary template";
    };


    template <template<typename, typename, typename> class CreatorT, typename MutexT, typename ObjT, typename CustomLockableT>
    struct LockedObjBase<CreatorT<MutexT, ObjT, CustomLockableT>>
    {
        static constexpr std::string_view templateNotes = "Specialized for <CreatorT<MutexT, ObjT, CustomLockableT>";
        
        _Acquires_lock_(this->lk) // suppress warning - "C26115: Falling release lock 'mx'"
        LockedObjBase(MutexT& mx, const ObjT& obj, const CreatorT<MutexT, ObjT, CustomLockableT>* creator, int& ownerThreadId)
            : lk{ mx }
            , creator{ creator }
            , customLockableObj{ obj }
        {
            ownerThreadId = ::GetCurrentThreadId();
        }

        _Releases_lock_(this->lk)
        ~LockedObjBase() {
        }

        const CreatorT<MutexT, ObjT, CustomLockableT>* GetCreator() const {
            return this->creator;
        }

    private:
        std::unique_lock<MutexT> lk;
        const CreatorT<MutexT, ObjT, CustomLockableT>* creator;
        CustomLockableT customLockableObj;
    };


    template <template<typename, typename, typename> class CreatorT, typename MutexT, typename ObjT>
    struct LockedObjBase<CreatorT<MutexT, ObjT, void>>
    {
        static constexpr std::string_view templateNotes = "Specialized for <CreatorT<MutexT, ObjT, void>";
        LockedObjBase(MutexT& mx, const ObjT& /*obj*/, const CreatorT<MutexT, ObjT, void>* creator, int& ownerThreadId)
            : lk{ mx }
            , creator{ creator }
        {
            ownerThreadId = ::GetCurrentThreadId();
        }

        ~LockedObjBase() {
        }

        const CreatorT<MutexT, ObjT, void>* GetCreator() const {
            return this->creator;
        }

    private:
        std::unique_lock<MutexT> lk;
        const CreatorT<MutexT, ObjT, void>* creator;
    };


    //
    // LockedObj
    //
    template <typename CreatorT>
    struct LockedObj {
        static constexpr std::string_view templateNotes = "Primary template";
    };


    template <template<typename, typename, typename> class CreatorT, typename MutexT, typename ObjT, typename CustomLockableT>
    struct LockedObj<CreatorT<MutexT, ObjT, CustomLockableT>>
        : LockedObjBase<CreatorT<MutexT, ObjT, CustomLockableT>>
    {
        static constexpr std::string_view templateNotes = "Specialized for <CreatorT<MutexT, ObjT, CustomLockableT>>";
        using _MyBase = LockedObjBase<CreatorT<MutexT, ObjT, CustomLockableT>>;

        LockedObj(
            MutexT& mx,
            const ObjT& obj,
            const CreatorT<MutexT, ObjT, CustomLockableT>* creator,
            int& ownerThreadId)
            : _MyBase(mx, obj, creator, ownerThreadId)
            , obj{ const_cast<ObjT&>(obj) } // [by design]
        {}

        ~LockedObj() {
        }

        ObjT* operator->() const {
            return &this->obj;
        }
        ObjT& Get() const {
            return this->obj;
        }

    private:
        ObjT& obj;
    };


    template<template<typename, typename, typename> class CreatorT, typename MutexT, typename ObjT, typename... Args, typename CustomLockableT>
    struct LockedObj<CreatorT<MutexT, std::unique_ptr<ObjT, Args...>, CustomLockableT>>
        : LockedObjBase<CreatorT<MutexT, std::unique_ptr<ObjT, Args...>, CustomLockableT>>
    {
        static constexpr std::string_view templateNotes = "Specialized for <MutexT, std::unique_ptr<ObjT, Args...>, CustomLockableT>";
        using _MyBase = LockedObjBase<CreatorT<MutexT, std::unique_ptr<ObjT, Args...>, CustomLockableT>>;

        LockedObj(
            MutexT& mx,
            const std::unique_ptr<ObjT, Args...>& obj,
            const CreatorT<MutexT, std::unique_ptr<ObjT, Args...>, CustomLockableT>* creator,
            int& ownerThreadId)
            : _MyBase(mx, obj, creator, ownerThreadId)
            , obj{ const_cast<std::unique_ptr<ObjT, Args...>&>(obj) } // [by design]
        {}

        ~LockedObj() {
        }

        std::unique_ptr<ObjT, Args...>& operator->() const {
            return this->obj;
        }
        std::unique_ptr<ObjT, Args...>& Get() const {
            return this->obj;
        }

    private:
        std::unique_ptr<ObjT, Args...>& obj;
    };


    //
    // ThreadSafeObject
    //
    template <typename MutexT, typename ObjT, typename CustomLockableT = void>
    class ThreadSafeObject {
    public:
        static constexpr std::string_view templateNotes = "Primary temlpate";
        using _Locked = LockedObj<ThreadSafeObject<MutexT, ObjT, CustomLockableT>>;

        ThreadSafeObject()
            : obj{}
        {}

        template <typename... Args>
        ThreadSafeObject(Args&&... args)
            : obj{ std::forward<Args>(args)... }
        {}

        ThreadSafeObject(ThreadSafeObject& other) = delete;
        ThreadSafeObject& operator=(ThreadSafeObject& other) = delete;

        template<typename OtherT, std::enable_if_t<std::is_convertible_v<OtherT, ObjT>, int> = 0>
        ThreadSafeObject(OtherT&& otherObj)
            : obj{ std::move(otherObj) }
        {}

        template<typename OtherT, std::enable_if_t<std::is_convertible_v<OtherT, ObjT>, int> = 0>
        ThreadSafeObject& operator=(OtherT&& otherObj) {
            if (&this->obj != &otherObj) {
                this->obj = std::move(otherObj);
            }
            return *this;
        }

        _Locked Lock() const {
            return _Locked{ this->mx, this->obj, this, this->ownerThreadId };
        }

        std::unique_ptr<_Locked> LockUniq() const {
            return std::make_unique<_Locked>(this->mx, this->obj, this, this->ownerThreadId);
        }

    private:
        mutable MutexT mx;
        mutable int ownerThreadId = -1;
        ObjT obj;
    };


    template <typename MutexT, typename ObjT, typename CustomLockableT>
    class ThreadSafeObject<MutexT, std::unique_ptr<ObjT>, CustomLockableT> {
    public:
        static constexpr std::string_view templateNotes = "Specialized for <MutexT, std::unique_ptr<ObjT>, CustomLockableObj>";
        using _Locked = LockedObj<ThreadSafeObject<MutexT, std::unique_ptr<ObjT>, CustomLockableT>>;

        ThreadSafeObject()
            : obj{ std::make_unique<ObjT>() }
        {}

        template <typename... Args>
        ThreadSafeObject(Args&&... args)
            : obj{ std::make_unique<ObjT>(std::forward<Args>(args)...) }
        {}

        ThreadSafeObject(ThreadSafeObject& other) = delete;
        ThreadSafeObject& operator=(ThreadSafeObject& other) = delete;

        template <typename OtherT, std::enable_if_t<std::is_convertible_v<std::unique_ptr<OtherT>, std::unique_ptr<ObjT>>, int> = 0>
        ThreadSafeObject(std::unique_ptr<OtherT>&& otherObj)
            : obj{ std::move(otherObj) }
        {}

        template <typename OtherT, std::enable_if_t<std::is_convertible_v<std::unique_ptr<OtherT>, std::unique_ptr<ObjT>>, int> = 0>
        ThreadSafeObject& operator=(std::unique_ptr<OtherT>&& otherObj) {
            if (this->obj.get() != otherObj.get()) {
                this->obj = std::move(otherObj);
            }
            return *this;
        }

        _Locked Lock() const {
            return _Locked{ this->mx, this->obj, this, this->ownerThreadId };
        }

        std::unique_ptr<_Locked> LockUniq() const {
            return std::make_unique<_Locked>(this->mx, this->obj, this, this->ownerThreadId);
        }

    private:
        mutable MutexT mx;
        mutable int ownerThreadId = -1;
        std::unique_ptr<ObjT> obj;
    };


    // TODO: overload for ENSURE_METHOD_IS_GUARDED(lockClass) and ENSURE_METHOD_IS_GUARDED(lockClass, mutexClass)
    //#define ENSURE_METHOD_IS_GUARDED std::lock_guard<std::mutex>&

    template<template<typename> class LockT = std::lock_guard, typename MutexT = std::mutex>
    struct GuardMethodHelper {
        using type = LockT<MutexT>&;
        using defaultGuard = std::lock_guard<std::mutex>&;
    };

    using MethodIsGuarded = typename GuardMethodHelper<>::defaultGuard;
}