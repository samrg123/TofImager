#pragma once

#if defined(DEBUG) && DEBUG

    #include <GDBStub.h>
    constexpr bool kEnableDebugging = true;

    #if defined(NON32XFER_HANDLER) || defined(MMU_IRAM_HEAP)

        #include <core_esp8266_non32xfer.h>
        #include <xtensa/corebits.h>

        #include "macros.h"
        #include "espUtil.h"

        ExceptionHandler gNon32xferExceptionHandler = nullptr;

        // Custom boot steps injected early on in the boot process right
        // after SDK initializes and ESP8266 pins are configured, but before anything else
        void initVariant() {

            // Note: gdb_init replaces the exception handler for EXCCAUSE_LOAD_STORE_ERROR that non32xfer uses to enable pgmem byte reads.
            //       so we need to put it restore it after every call to `gbd_init()`
            // Note: `install_non32xfer_exception_handler()` only installs its exception handler on the first invocation 
            //        so we must call it before `user_init()` [IE must be called in initVariant or early]

            // Install gdb exception handlers so `install_non32xfer_exception_handler` knows what to fall back on if 
            // it fails to complete the load/store it fails   
            gdb_init();

            // Install handler for all illegal load-store (non 32bit read from pgmem)
            // Note: because this function only works once we cache its exception handler so we can reinstall it later
            install_non32xfer_exception_handler();
            gNon32xferExceptionHandler = GetExceptionHandler(EXCCAUSE_LOAD_STORE_ERROR);
        }

        // Note: `init_done()` calls gdb_init() again right before it invokes constructors so we
        //       reinstall the non32xferExceptionHandler before any other constructors get called
        PRE_CRT_CONSTRUCTOR void ReinstallNon32xferExceptionHandler() {
            _xtos_set_exception_handler(EXCCAUSE_LOAD_STORE_ERROR, gNon32xferExceptionHandler);    
        }

    #endif

#else 

    #include <gdb_hooks.h>
    constexpr bool kEnableDebugging = false;

#endif