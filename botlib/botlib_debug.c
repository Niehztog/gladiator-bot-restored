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
#include <string.h>
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
 * Log file handle — opened in DllMain, written only when something
 * worth logging happens (exception filter, explicit botlib_log()).
 *
 * The file is created with "w" mode at DllMain time because fopen() can
 * fail at crash time when the heap is corrupted — we want a guaranteed
 * writable handle ready in advance.  However we deliberately do NOT
 * emit any content until an actual crash fires; that way a clean run
 * leaves a 0-byte file, and the file's mere non-zero size is a crash
 * indicator on its own.
 *
 * On DLL_PROCESS_DETACH we close the file and unlink it if nothing was
 * ever written (the common "clean shutdown" case).
 * --------------------------------------------------------------------- */
static FILE *g_log = NULL;
static char  g_log_path[MAX_PATH] = {0};
static int   g_log_dirty = 0;   /* set to 1 the moment we write anything */

static void blog_write_header_once(void)
{
    if (!g_log || g_log_dirty) return;
    fputs("=== gladiator.dll debug log ===\n", g_log);
    g_log_dirty = 1;
}

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
            strncpy(g_log_path, paths[i], sizeof(g_log_path) - 1);
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
        if (g_log) {
            int clean = !g_log_dirty;
            fclose(g_log);
            g_log = NULL;
            /* Clean shutdown (no crash content written) — delete the
             * empty placeholder file so it doesn't look like a crash
             * happened. */
            if (clean && g_log_path[0])
                remove(g_log_path);
        }
        break;
    }
    return TRUE;
}

/* -----------------------------------------------------------------------
 * blog_annotate_module — if `v` points into an executable image, write
 *   " (modname+0xRVA)" into `out`.  Otherwise leave `out` empty.
 *
 * Uses VirtualQuery + GetModuleFileNameA so it works for *any* loaded
 * module (gladiator.dll, game.dll, yquake2.exe, system DLLs), not just
 * gladiator.dll.  Filters out non-image pages (heap, stack, free) by
 * requiring MEM_IMAGE.
 * --------------------------------------------------------------------- */
static void blog_annotate_module(unsigned v, char *out, size_t outsz)
{
    out[0] = '\0';
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery((LPCVOID)(intptr_t)v, &mbi, sizeof(mbi)) != sizeof(mbi))
        return;
    if (mbi.State != MEM_COMMIT || mbi.Type != MEM_IMAGE || !mbi.AllocationBase)
        return;
    char path[MAX_PATH];
    if (!GetModuleFileNameA((HMODULE)mbi.AllocationBase, path, sizeof(path)))
        return;
    const char *name = strrchr(path, '\\');
    name = name ? name + 1 : path;
    unsigned rva = v - (unsigned)(intptr_t)mbi.AllocationBase;
    snprintf(out, outsz, "  (%s+0x%X)", name, rva);
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
        blog_write_header_once();
        fprintf(g_log, "CRASH: exception 0x%08X at 0x%08X\n",
                (unsigned)code, (unsigned)(intptr_t)addr);

        /* Identify which module actually contains EIP, not just gladiator.dll.
         * VirtualQuery on the crash address yields the allocation base of the
         * mapped image; that base IS the HMODULE of the containing DLL/EXE.
         * From there GetModuleFileNameA gives the path so we can name it. */
        MEMORY_BASIC_INFORMATION mbi;
        HMODULE eip_mod = NULL;
        char eip_modpath[MAX_PATH] = {0};
        if (VirtualQuery(addr, &mbi, sizeof(mbi)) == sizeof(mbi) &&
            mbi.AllocationBase) {
            eip_mod = (HMODULE)mbi.AllocationBase;
            GetModuleFileNameA(eip_mod, eip_modpath, sizeof(eip_modpath));
        }
        if (eip_mod) {
            const char *name = strrchr(eip_modpath, '\\');
            name = name ? name + 1 : (eip_modpath[0] ? eip_modpath : "<unknown>");
            unsigned base = (unsigned)(intptr_t)eip_mod;
            unsigned eip  = (unsigned)(intptr_t)addr;
            fprintf(g_log, "  EIP module: %s  base=0x%08X  RVA=0x%08X\n",
                    name, base, eip - base);
        } else {
            fprintf(g_log, "  EIP module: <unresolved>\n");
        }

        /* gladiator.dll's own base, for reference + the preserved
         * vma-relative offset (only meaningful if EIP is actually in
         * gladiator.dll). */
        HMODULE hmod = GetModuleHandleA("gladiator.dll");
        fprintf(g_log, "  gladiator.dll runtime base: 0x%08X\n",
                (unsigned)(intptr_t)hmod);
        if (hmod && eip_mod == hmod) {
            unsigned base = (unsigned)(intptr_t)hmod;
            unsigned eip  = (unsigned)(intptr_t)addr;
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

        /* Dump stack from ESP — show 64 entries to capture return address past local frame.
         * Annotate each slot with module!RVA if it points into a loaded image,
         * so call-chain reconstruction doesn't require manually probing every
         * address against the linker maps. */
        fprintf(g_log, "  Stack (ESP-relative):\n");
        unsigned *sp = (unsigned *)(intptr_t)ctx->Esp;
        for (int i = 0; i < 64; i++) {
            if (IsBadReadPtr(sp + i, sizeof(unsigned)))
                break;
            unsigned v = sp[i];
            char ann[MAX_PATH + 32] = {0};
            blog_annotate_module(v, ann, sizeof(ann));
            fprintf(g_log, "    [ESP+%03d] 0x%08X%s\n", i*4, v, ann);
        }
        /* Also dump EBP frame */
        fprintf(g_log, "  EBP frame:\n");
        unsigned *bp = (unsigned *)(intptr_t)ctx->Ebp;
        for (int i = -4; i <= 8; i++) {
            unsigned *p = bp + i;
            if (IsBadReadPtr(p, sizeof(unsigned)))
                continue;
            unsigned v = *p;
            char ann[MAX_PATH + 32] = {0};
            blog_annotate_module(v, ann, sizeof(ann));
            fprintf(g_log, "    [EBP%+d] 0x%08X%s\n", i*4, v, ann);
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

