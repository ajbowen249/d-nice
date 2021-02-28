#pragma once

#include "duktape.h"

#include <deque>
#include <functional>
#include <mutex>

namespace DNice {
    struct PromiseContext {
        std::function<void(void)> resolve;
        std::function<void(void)> reject;
    };

    class ProcessQueue {
    public:
        typedef std::function<void(duk_context*)> QueueCallback;

        ProcessQueue();

        void enqueue(QueueCallback&& func);

        bool churn();

        duk_int_t registerCallback(duk_context* ctx);

        void completePromise(duk_context* ctx, duk_int_t callbackId, std::function<void(const PromiseContext&)>&& func);

        static ProcessQueue GlobalQueue;

    private:
        duk_context* _duktape;
        std::mutex _mutex;
        std::deque<QueueCallback> _deque;
    };
}
