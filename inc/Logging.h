#pragma once

#include "duktape.h"

#include <iostream>

namespace DNice {
    void registerLoggingApi(duk_context* ctx);
}
