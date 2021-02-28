#include <iostream>

#include "duktape.h"

int main() {
    std::cout << "Hello!" << std::endl;

    duk_context* ctx = duk_create_heap_default();

    return 0;
}
