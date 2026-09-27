#if defined(__ANDROID__)
#include <jni.h>

extern "C" JNIEXPORT jstring JNICALL Java_{{JniPackage}}_MainActivity_nativeMessage(JNIEnv* environment, jobject)
{
    return environment->NewStringUTF("{{Application}} native library is ready");
}
#else
extern "C" int ApplicationVersion()
{
    return 1;
}
#endif