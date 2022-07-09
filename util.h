#pragma once

#include "types.h"
#include "macros.h"

#include "log.h"

#include "metaProgramming.h"

#include "memUtil.h"

#include "mathUtil.h"
#include "FixedPoint.h"

#include "espUtil.h"

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
