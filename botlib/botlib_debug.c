/*
 * botlib_debug.c — Diagnostic crash tracing for gladiator.dll
 *
 * Adds a DllMain that opens a log file, and provides BOTLIB_LOG() which
 * key functions can call.  The log file path is:
 *   C:\gladiator_debug.log   (or the game directory if that fails)
 *
 * To enable: add botlib_debug.o to OBJS_ in the Makefile.
 * The log shows exactly which function was last entered before the crash.
 */

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

void botlib_log(const char *fmt, ...)
{
    /* No-op: stdio removed to avoid CRT heap interference with Yamagi's Z_TagMalloc */
    (void)fmt;
}

#ifdef _WIN32
#include <windows.h>

/* -----------------------------------------------------------------------
 * Log file handle — opened in DllMain, written by BOTLIB_LOG()
 * --------------------------------------------------------------------- */
static FILE *g_log = NULL;

static void blog_open(void)
{
    const char *paths[] = {
        "gladiator_debug.log",
        "C:/gladiator_debug.log",
        NULL
    };
    for (int i = 0; paths[i]; i++) {
        g_log = fopen(paths[i], "w");
        if (g_log) {
            fputs("=== gladiator.dll debug log ===\n", g_log);
            fflush(g_log);
            return;
        }
    }
}

/* -----------------------------------------------------------------------
 * DllMain — called by Windows when the DLL is loaded/unloaded
 * --------------------------------------------------------------------- */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    (void)hinstDLL; (void)lpReserved;

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        blog_open();
        botlib_log("DllMain: DLL_PROCESS_ATTACH — gladiator.dll loaded");
        botlib_log("  Base address: 0x%08X", (unsigned)hinstDLL);
        break;

    case DLL_PROCESS_DETACH:
        botlib_log("DllMain: DLL_PROCESS_DETACH — gladiator.dll unloading");
        if (g_log) { fclose(g_log); g_log = NULL; }
        break;
    }
    return TRUE;
}

/* -----------------------------------------------------------------------
 * Structured Exception handler — wraps GetBotAPI to catch hard crashes
 * --------------------------------------------------------------------- */
static LONG WINAPI gladiator_exception_filter(EXCEPTION_POINTERS *ep)
{
    EXCEPTION_RECORD *er = ep->ExceptionRecord;
    CONTEXT          *ctx = ep->ContextRecord;
    DWORD code = er->ExceptionCode;
    void *addr = er->ExceptionAddress;

    if (g_log) {
        HMODULE hmod = GetModuleHandleA("gladiator.dll");
        fprintf(g_log, "CRASH: exception 0x%08X at 0x%08X\n",
                (unsigned)code, (unsigned)(intptr_t)addr);
        fprintf(g_log, "  gladiator.dll runtime base: 0x%08X\n",
                (unsigned)(intptr_t)hmod);
        if (hmod) {
            unsigned base = (unsigned)(intptr_t)hmod;
            unsigned eip  = (unsigned)(intptr_t)addr;
            if (eip >= base)
                fprintf(g_log, "  crash file offset (vma 0x69700000-relative): 0x%08X\n",
                        eip - base + 0x69700000u);
        }

        /* For access violations, log the faulting memory address */
        if (code == 0xC0000005 && er->NumberParameters >= 2) {
            fprintf(g_log, "  AV type  : %s\n",
                    er->ExceptionInformation[0] == 0 ? "READ" :
                    er->ExceptionInformation[0] == 1 ? "WRITE" : "EXECUTE");
            fprintf(g_log, "  AV addr  : 0x%08X  (address that was accessed)\n",
                    (unsigned)er->ExceptionInformation[1]);
        }

        /* CPU registers at crash.  Cast DWORD (= long unsigned int) → unsigned
         * for %X to silence -Wformat= without changing the printed bits. */
        fprintf(g_log, "  EAX=0x%08X  EBX=0x%08X  ECX=0x%08X  EDX=0x%08X\n",
                (unsigned)ctx->Eax, (unsigned)ctx->Ebx, (unsigned)ctx->Ecx, (unsigned)ctx->Edx);
        fprintf(g_log, "  ESI=0x%08X  EDI=0x%08X  ESP=0x%08X  EBP=0x%08X\n",
                (unsigned)ctx->Esi, (unsigned)ctx->Edi, (unsigned)ctx->Esp, (unsigned)ctx->Ebp);

        /* Dump stack from ESP — show 64 entries to capture return address past local frame */
        fprintf(g_log, "  Stack (ESP-relative):\n");
        unsigned *sp = (unsigned *)(intptr_t)ctx->Esp;
        for (int i = 0; i < 64; i++) {
            if (IsBadReadPtr(sp + i, sizeof(unsigned)))
                break;
            fprintf(g_log, "    [ESP+%03d] 0x%08X\n", i*4, sp[i]);
        }
        /* Also dump EBP frame */
        fprintf(g_log, "  EBP frame:\n");
        unsigned *bp = (unsigned *)(intptr_t)ctx->Ebp;
        for (int i = -4; i <= 8; i++) {
            unsigned *p = bp + i;
            if (IsBadReadPtr(p, sizeof(unsigned)))
                continue;
            fprintf(g_log, "    [EBP%+d] 0x%08X\n", i*4, *p);
        }
        fflush(g_log);
    }
    OutputDebugStringA("gladiator.dll: fatal crash — see gladiator_debug.log\n");
    return EXCEPTION_CONTINUE_SEARCH;
}

/* Call this once at the start of GetBotAPI to install the handler */
void botlib_install_exception_handler(void)
{
    SetUnhandledExceptionFilter(gladiator_exception_filter);
    botlib_log("Exception handler installed");
}

#else  /* !_WIN32 — Linux/POSIX: no DllMain, no SEH */

void botlib_install_exception_handler(void)
{
    /* No-op on POSIX: use GDB / ASAN / UBSAN for crash diagnosis instead. */
}

#endif /* _WIN32 */

