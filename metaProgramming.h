#pragma once

#include <type_traits>

#include "SpanningInteger.h"

template<typename DeferT, typename ValT>
constexpr ValT Defer(ValT val) { return val; }
