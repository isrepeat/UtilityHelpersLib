#pragma once
#include "StdRedirection.h"
#include "Logger.h"
#include <fcntl.h>
#include <io.h>

namespace H {
    StdRedirection::StdRedirection()
        : ioPipes{ -1, -1 } // initialize with default values to suppress compiler warning
        , oldStdOut{ -1 }
        , redirectInProcess{ false }
    {
        LOG_FUNCTION_ENTER("StdRedirection()");
        setvbuf(stdout, NULL, _IONBF, 0); // set "no buffer" for stdout (no need fflush manually)
    }

    StdRedirection::~StdRedirection() {
        LOG_FUNCTION_SCOPE("StdRedirection()");
        EndRedirect();
        // If durring redirection std HANDLEs was saved with ::GetStdHandle(...) it not restored
    }

    void StdRedirection::BeginRedirect(std::function<void(std::vector<char>)> readCallback) {
        LOG_FUNCTION_ENTER("BeginRedirect()");
        LOG_ASSERT(readCallback, "readCallback is empty");
        std::unique_lock lk{ mx };

        if (redirectInProcess.exchange(true)) {
            LOG_WARNING_D("Redirect already started, ignore");
            return;
        }

        oldStdOut = _dup(_fileno(stdout)); // save original stdout descriptor

        if (_pipe(ioPipes, PIPE_BUFFER_SIZE, O_BINARY) == -1) { // make pipe's descriptors for READ / WRITE streams
            Dbreak;
            throw std::exception("_pipe(...) Failed to init ioPipes");
        }
        if (_dup2(ioPipes[WRITE], _fileno(stdout)) == -1) { // redirect stdout to the pipe
            Dbreak;
            throw std::exception("_dup2(...) Failed redirect stdout to the pipe");
        }
        _close(ioPipes[WRITE]); // we no need work with WRITE stream so close it now


        listeningRoutine = std::async(std::launch::async, [this, readCallback] {
            while (redirectInProcess) {
                outputBuffer.resize(OUTPUT_BUFFER_SIZE);

                // NOTE: _read(...) block current thread until any data arrives in the ioPipes[READ]
                int readBytes = _read(ioPipes[READ], outputBuffer.data(), outputBuffer.size());
                if (readBytes == -1) {
                    Dbreak;
                    throw std::exception("_read(...) Failed when read from ipPipe[READ]");
                }
                else if (readBytes == 0) { // read end of file
                    return;
                }

                outputBuffer.resize(readBytes); // truncate
                if (readCallback) {
                    readCallback(std::move(outputBuffer));
                }
            }
            });
    }

    void StdRedirection::EndRedirect() {
        LOG_FUNCTION_ENTER("EndRedirect()");
        std::unique_lock lk{ mx };

        if (!redirectInProcess.exchange(false)) {
            LOG_WARNING_D("Redirect already finished, ignore");
            return;
        }

        if (_dup2(oldStdOut, _fileno(stdout)) == -1) { // restore stdout to original (saved) descriptor
            Dbreak;
            throw std::exception("_dup2(...) Failed restore stdout");
        }
        // after stdout restored the _read(...) return 0  in  listeningRoutine
        _close(oldStdOut);

        if (listeningRoutine.valid())
            listeningRoutine.get(); // May throw exception

        LOG_DEBUG("Redirection finished");
    }
}