#pragma once

#include "types.h"
#include "macros.h"

#include "log.h"

#include "metaProgramming.h"

#include "memUtil.h"

#include "mathUtil.h"
#include "FixedPoint.h"

// TODO: Move this out to stringUtil.h
#include <WString.h>

// TODO: Move to intellisense.h
// Hack to prevent intelisense failing to infer templates in WString.h
#ifdef __INTELLISENSE__
    template<typename T>
    inline String operator+(const String& str, T arg) {
        String result = String(str).concat(arg);
        return result;
    }
#endif

template<typename LambdaT>
auto CriticalSection(LambdaT lambda) {
    noInterrupts();

    using ReturnT = decltype(lambda());
    if constexpr(std::is_same<ReturnT, void>::value) {

        lambda();    
        interrupts();
        return;
    
    } else {
        
        auto result = lambda();
        interrupts();
        return result;
    }
} 

// TODO: Pull this out to a better location
// TODO: Define EXCCAUSE constants ... right now they're only accessible in xtensa/corebits after you include GDBStub.h,
//       But we may want to access them when debugging is disabled.
using ExceptionHandler = fn_c_exception_handler_t;

inline ExceptionHandler GetExceptionHandler(int exceptionCause) {

    return CriticalSection([=]() {
        
        //temporarily set exception handler to default, capture old handler and restore it  
        ExceptionHandler handler = _xtos_set_exception_handler(exceptionCause, nullptr);
        _xtos_set_exception_handler(exceptionCause, handler);

        return handler;
    });
}
