#pragma once
#include <cassert>
#include <memory>
#include <mutex>


template <typename TClass, typename T = void*>
class _Singleton {
public:
    // Call this in constructors of singletons classes (to ensure that TClass destroy after all of them)
    static void InitSingleton() {
        GetInstance();
    }

    // Work with public Ctor without args
    static TClass& GetInstance() {
        static TClass instance;
        return instance;
    }
private:
    friend TClass;
    _Singleton() = default;
    ~_Singleton() = default;

private:
    T m_instance; // by default not used (T = void*)
};



template <typename C> 
class Singleton : public _Singleton<Singleton<C>, std::unique_ptr<C>> {
private:
    using _MyBase = _Singleton<Singleton<C>, std::unique_ptr<C>>;

public:
    using Instance_t = C&;

    Singleton() = default;
    ~Singleton() = default;

    template <typename ...Args>
    static Instance_t CreateInstance(Args... args) {
        auto& _this = _MyBase::GetInstance();
        std::unique_lock lk{ _this.mx };
        if (_this.m_instance == nullptr) {
            _this.m_instance = std::make_unique<C>(args...); // NOTE: C must have public Ctor
        }
        return *_this.m_instance;
    }

    static Instance_t GetInstance() {
        assert(_MyBase::GetInstance().m_instance);
        return *_MyBase::GetInstance().m_instance;
    }

private:
    std::mutex mx;
};



template <typename C>
class SingletonShared : public _Singleton<SingletonShared<C>, std::shared_ptr<C>> {
private:
    using _MyBase = _Singleton<SingletonShared<C>, std::shared_ptr<C>>;

public:
    using Instance_t = std::shared_ptr<C>;

    SingletonShared() = default;
    ~SingletonShared() = default;

    template <typename ...Args>
    static Instance_t CreateInstance(Args... args) {
        auto& _this = _MyBase::GetInstance();
        std::unique_lock lk{ _this.mx };
        if (_this.m_instance == nullptr) {
            _this.m_instance = std::make_shared<C>(args...);
        }
        return _this.m_instance;
    }

    static Instance_t GetInstance() {
        assert(_MyBase::GetInstance().m_instance);
        return _MyBase::GetInstance().m_instance;
    }

private:
    std::mutex mx;
};



template <typename C>
class SingletonUnscoped : public _Singleton<SingletonUnscoped<C>, C*> {
private:
    using _MyBase = _Singleton<SingletonUnscoped<C>, C*>;

public:
    using Instance_t = C*;

    SingletonUnscoped() = default;
    ~SingletonUnscoped() = default; 

    template <typename ...Args>
    static Instance_t CreateInstance(Args... args) {
        auto& _this = _MyBase::GetInstance();
        std::unique_lock lk{ _this.mx };
        if (_this.m_instance == nullptr) {
            _this.m_instance = new C(args...);
        }
        return _this.m_instance;
    }

    static Instance_t GetInstance() {
        assert(_MyBase::GetInstance().m_instance);
        return _MyBase::GetInstance().m_instance;
    }

private:
    std::mutex mx;
};