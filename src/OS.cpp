#include "OS.h"

#include "ProcessQueue.h"
#include "generated/osjs.h"
#include "util.h"

#include <future>
#include <fstream>
#include <memory>
#include <vector>
#include <utility>

namespace DNice {
    struct ReadResult {
        bool success;
        std::string error;
        std::vector<char> bytes;
    };

    duk_ret_t os_readfile_native(duk_context* ctx) {
        // Arguments are filename, reject callback, resolve callback
        const auto filename = std::string(duk_safe_to_string(ctx, -3));

        const auto callbackId = ProcessQueue::GlobalQueue.registerCallback(ctx);

        call_async( [filename, callbackId]() {
            try {
                std::ifstream file(filename);
                if (!file) {
                    ProcessQueue::GlobalQueue.enqueue([callbackId, filename](duk_context* retCtx) {
                         ProcessQueue::GlobalQueue.completePromise(retCtx, callbackId, [retCtx, filename](const PromiseContext& promiseCtx) {
                            duk_push_string(retCtx, ("no such file " + filename).c_str());
                            promiseCtx.reject();
                        });
                    });
                    return;
                }

                const auto fileContent = std::vector<char>(
                    std::istreambuf_iterator<char>(file),
                    std::istreambuf_iterator<char>()
                );

                ProcessQueue::GlobalQueue.enqueue([callbackId, fileContent = std::move(fileContent)](duk_context* retCtx) {
                     ProcessQueue::GlobalQueue.completePromise(retCtx, callbackId, [retCtx, fileContent = std::move(fileContent)](const PromiseContext& promiseCtx) {
                         // Create and populate the basic buffer
                        auto buffer = (char*)duk_push_fixed_buffer(retCtx, fileContent.size());
                        for (size_t i = 0; i < fileContent.size(); i ++) {
                            buffer[i] = fileContent[i];
                        }

                        // Create the Nodejs-style buffer from the basic buffer
                        duk_push_buffer_object(
                            retCtx,
                            -1,
                            0,
                            fileContent.size(),
                            DUK_BUFOBJ_NODEJS_BUFFER
                        );

                        promiseCtx.resolve();

                        duk_pop(retCtx); // Delete the leftover basic buffer
                    });
                });
            } catch (...) {
                ProcessQueue::GlobalQueue.enqueue([callbackId, filename](duk_context* retCtx) {
                    ProcessQueue::GlobalQueue.completePromise(retCtx, callbackId, [retCtx, filename](const PromiseContext& promiseCtx) {
                        duk_push_string(retCtx, ("something went wrong reading " + filename).c_str());
                        promiseCtx.reject();
                    });
                });
            }
        });

        return 0;
    }

    void registerOSApi(duk_context* ctx) {
        duk_push_c_function(ctx, os_readfile_native, DUK_VARARGS);
        duk_put_global_string(ctx, "os_readFile_native");

        duk_eval_string(ctx, OS_JS);
    }
}
