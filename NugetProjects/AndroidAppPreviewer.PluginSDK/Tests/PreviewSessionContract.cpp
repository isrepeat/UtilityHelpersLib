#include <Windows.h>

#include <AndroidAppPreviewer.PluginSDK/AndroidAppPreviewerPlugin.h>

#include <stdexcept>
#include <iostream>
#include <string>

// Первый плагин использует базовую реализацию ApplyScenario.
// Второй переопределяет её и принимает MainPage с JSON {}.
int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            throw std::invalid_argument("Expected default-plugin.dll and extended-plugin.dll");
        }
        for (int index = 1; index < argc; ++index) {
            const auto module = LoadLibraryExA(argv[index], nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
            if (module == nullptr) {
                throw std::runtime_error("LoadLibrary failed: " + std::to_string(GetLastError()));
            }
            using namespace AndroidAppPreviewerPluginSDK;
            const auto getAbi = reinterpret_cast<const xp_plugin_abi* (*)(uint32_t)>(GetProcAddress(module, "xp_get_abi"));
            if (getAbi == nullptr) {
                throw std::runtime_error("Missing xp_get_abi");
            }
            const auto* abi = getAbi(1);
            if (abi == nullptr) {
                throw std::runtime_error("Unsupported ABI");
            }
            auto* session = abi->session.create(720, 1600);
            if (session == nullptr) {
                throw std::runtime_error(abi->metadata.last_error());
            }
            const bool accepted = abi->session.apply_preview_scenario(session, "MainPage", "{}") != 0;
            if (accepted != (index == 2)) {
                throw std::runtime_error("Scenario override was not dispatched correctly");
            }
            if (abi->session.apply_preview_scenario(session, nullptr, "{}") != 0) {
                throw std::runtime_error("Null page must be rejected");
            }
            abi->session.destroy(session);
            FreeLibrary(module);
        }
        std::cout << "PASS: default scenario rejection, derived scenario dispatch, null validation, destruction\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}