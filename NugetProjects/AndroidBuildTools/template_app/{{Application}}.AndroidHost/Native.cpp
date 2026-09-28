#if defined(__ANDROID__)
#include <jni.h>
#endif

#include <XamlRuntime/Binding.h>

#include "../!Generated/{{Application}}.Application/Xaml/Page/MainPage.xaml.h"

#include <memory>

namespace _details {
    struct MainPageViewModel final {};

    std::unique_ptr<xaml::Element> CreateMainPage() {
        MainPageViewModel viewModel;
        xaml::BindingScope bindings;
        return xaml::generated::MainPage::Create(viewModel, bindings);
    }
} // namespace _details

#if defined(__ANDROID__)
extern "C" JNIEXPORT jstring JNICALL Java_{{JniPackage}}_MainActivity_nativeMessage(JNIEnv *environment, jobject) {
    const auto page = _details::CreateMainPage();
    if (!page) {
        return environment->NewStringUTF(
            "{{Application}} native MainPage could not be created");
    }
    return environment->NewStringUTF("{{Application}} native library is ready");
}
#else
extern "C" int ApplicationVersion() {
    const auto page = _details::CreateMainPage();
    if (!page) {
        return 0;
    }
    return 1;
}
#endif