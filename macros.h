#pragma once

#define LIKELY(x)   __builtin_expect(x, 1)
#define UNLIKELY(x) __builtin_expect(x, 0) 

#define OPTIMIZE(x) __attribute__((optimize(x)))

#ifdef __INTELLISENSE__
    #define INTELISENSE_CHOOSE(a, b...) a 
#else
    #define INTELISENSE_CHOOSE(a, b...) b 
#endif

#define STRINGIZE(x) #x 
#define DEFER_STRINGIZE(x) STRINGIZE(x)

// Looks like IRAM_ATTR breaks the assembler so we just define our own IRAM_FUNC macro that does what its supposed to do
#define IRAM_FUNC __attribute__((section( ".iram.text." __FILE__ "." DEFER_STRINGIZE(__LINE__) "." DEFER_STRINGIZE(__COUNTER__) )))

#define FLASH_LITERAL __attribute__((section( ".literal." __FILE__ "." DEFER_STRINGIZE(__LINE__) "." DEFER_STRINGIZE(__COUNTER__) )))

#define USED __attribute__((used))
#define SECTION(x) __attribute__((section(x)))

#define INLINE   __attribute__((always_inline))
#define NOINLINE __attribute__((no_inline))

#define DEFINE_FUNC(name, decl) decl asm(#name); decl

// Warn: Crt uses constructors 0-100 to initialize itself, so `priority` should most likely be > 100
#define CONSTRUCTOR(priority) __attribute__((constructor(priority)))
#define PRE_CRT_CONSTRUCTOR   CONSTRUCTOR(0)
#define POST_CRT_CONSTRUCTOR  CONSTRUCTOR(101)
