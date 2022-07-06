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

