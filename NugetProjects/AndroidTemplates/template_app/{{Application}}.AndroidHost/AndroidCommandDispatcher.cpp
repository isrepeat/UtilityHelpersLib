#include "AndroidCommandDispatcher.h"

#if defined(__ANDROID__)
namespace {{application}}::android_host {
    AndroidCommandDispatcher::AndroidCommandDispatcher(JNIEnv* environment, jobject target)
        : target(environment, target)
        , dispatch(this->target.GetMethod<void(jint, jni::Utf8Bytes, jni::Utf8Bytes)>("dispatch")) {
    }

    //
    // API
    //
    void AndroidCommandDispatcher::Dispatch(application::core::HostCommand command, const application::core::HostCommandData& data) const {
        this->dispatch.Call(this->target, static_cast<jint>(command), data.value, data.additionalValue);
    }
}
#endif