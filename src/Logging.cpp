#include "Logging.h"

namespace DNice {
    duk_ret_t native_println(duk_context* ctx) {
        std::cout << duk_safe_to_string(ctx, -1) << std::endl;
        return 0;
    }

    void registerLoggingApi(duk_context* ctx) {
        duk_push_c_function(ctx, native_println, DUK_VARARGS);
        duk_put_global_string(ctx, "println");
    }
}
