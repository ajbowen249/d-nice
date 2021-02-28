#include "ProcessQueue.h"

#include "Logging.h"
#include "OS.h"
#include "generated/callbackCachejs.h"
#include "generated/promisejs.h"

#include <optional>

#define LOCK(mut) \
    std::lock_guard<std::mutex> lock(mut); \


namespace DNice {
    ProcessQueue ProcessQueue::GlobalQueue;

    ProcessQueue::ProcessQueue() :
        _duktape(duk_create_heap_default()) {
            // IMPROVE: Move standard library stuff elsewhere
            duk_eval_string(_duktape, CALLBACKCACHE_JS);
            duk_eval_string(_duktape, PROMISE_JS);
            registerLoggingApi(_duktape);
            registerOSApi(_duktape);
        }

    void ProcessQueue::enqueue(QueueCallback&& func) {
        LOCK(_mutex);

        _deque.push_back(func);
    }

    bool ProcessQueue::churn() {
        std::optional<QueueCallback> maybeCB;

        {
            LOCK(_mutex);
            if (_deque.size() != 0) {
                maybeCB = _deque.back();
                _deque.pop_back();
            }
        }

        if (!maybeCB) {
            return false;
        }

        (*maybeCB)(_duktape);
        return true;
    }

    duk_int_t ProcessQueue::registerCallback(duk_context* ctx) {
        duk_get_global_string(ctx, "registerCallback");
        duk_dup(ctx, -3); // Push the resolve function
        duk_dup(ctx, -3); // Push the reject function (still -3 because we just pushed another item)
        duk_call(ctx, 2); // Call registerCallback with the supplied resolve and reject
        auto callbackId = duk_get_int(ctx, -1);
        duk_pop(ctx); // Pop the ID return

        return callbackId;
    }

    void ProcessQueue::completePromise(duk_context* ctx, duk_int_t callbackId, std::function<void(const PromiseContext&)>&& func) {
        auto rejectOrResolve = [ctx, callbackId](bool isResolve) {
            return [ctx, callbackId, isResolve]() {
                duk_get_global_string(ctx, isResolve ? "resolveCallback" : "rejectCallback");
                duk_push_int(ctx, callbackId); // Push the callback ID
                duk_dup(ctx, -3); // Copy our object to send back to the top
                duk_call(ctx, 2); // Call rejectCallback with our value
                duk_pop_2(ctx); // Pop the return plus the original object value
            };
        };

        func({
            rejectOrResolve(true),
            rejectOrResolve(false),
        });
    }
}
