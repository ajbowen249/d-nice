#include <iostream>

#include "ProcessQueue.h"

#include "generated/defaultScriptjs.h"

#include <thread>

using namespace DNice;

int main() {
    std::cout << "Running..." << std::endl;

    ProcessQueue::GlobalQueue.enqueue([](duk_context* ctx) {
        duk_eval_string(ctx, DEFAULT_SCRIPT_JS);
    });

    while (true) {
        if (!ProcessQueue::GlobalQueue.churn()) {
            std::this_thread::yield();
        }
    }

    return 0;
}
