#pragma once

#include <future>
#include <memory>

// Ripped this from https://stackoverflow.com/a/56834117/8549849
template <class F>
void call_async(F&& fun) {
    auto futptr = std::make_shared<std::future<void>>();
    *futptr = std::async(std::launch::async, [futptr, fun]() {
        // We capture futptr in this lambda, which increases its ref count. By calling the function passed to this from
        // THIS lambda, we get around the fact that std::future's destructor waits for the lambda to complete.
        fun();
    });
}
