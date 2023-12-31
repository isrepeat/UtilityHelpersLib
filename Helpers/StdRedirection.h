#pragma once
#include <functional>
#include <future>
#include <mutex>

namespace H {
    // Use this class before first use ::GetStdHandle(...) to avoid side effects, for example as singleton. 
    // All std HANDLEs that was saved with ::GetStdHandle(...) before redirection will not be redirected.
    // All std HANDLEs that was saved with ::GetStdHandle(...) during redirection will not be restored.
    class StdRedirection {
    public:
        StdRedirection();
        ~StdRedirection();

        void BeginRedirect(std::function<void(std::vector<char>)> readCallback);
        void EndRedirect();

    private:
        enum PipeStream {
            READ,
            WRITE
        };

        const int PIPE_BUFFER_SIZE = 65536;
        const int OUTPUT_BUFFER_SIZE = 260; // MAX_PATH

        std::mutex mx;

        int ioPipes[2];
        int oldStdOut;
        std::vector<char> outputBuffer;

        std::atomic<bool> redirectInProcess;
        std::future<void> listeningRoutine;
    };
}