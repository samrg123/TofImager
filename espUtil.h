#pragma once

#include "Arduino.h"

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
