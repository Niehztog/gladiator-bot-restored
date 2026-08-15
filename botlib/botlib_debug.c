/*
 * botlib_debug.c — Win32 crash tracing for gladiator.dll.
 *
 * DllMain opens gladiator_debug.log (falling back to C:\) and installs an
 * SEH filter that dumps the faulting address, registers and stack.  Inert on
 * POSIX.
 *
 * Not part of the reconstruction: none of this is in either 1999 binary, so no
 * function here carries the `// gladiator.dll:` / `// gladi386.so:` address
 * annotation the recovered TUs do.
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

/* The log is opened in DllMain because fopen() can fail at crash time on a
 * corrupted heap, but nothing is written until a crash actually fires — so a
 * clean run leaves a 0-byte file, which DLL_PROCESS_DETACH then unlinks.
 * Non-zero size is therefore itself the crash indicator. */
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

/* DllMain — DLL load/unload. */
void botlib_install_exception_handler(void);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    (void)hinstDLL; (void)lpReserved;

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        blog_open();
        botlib_log("DllMain: DLL_PROCESS_ATTACH — gladiator.dll loaded");
        botlib_log("  Base address: 0x%08X", (unsigned)hinstDLL);
        /* Faithful install site: the original did it here, not in
         * GetBotAPI. */
        botlib_install_exception_handler();
        break;

    case DLL_PROCESS_DETACH:
        botlib_log("DllMain: DLL_PROCESS_DETACH — gladiator.dll unloading");
        if (g_log) {
            int clean = !g_log_dirty;
            fclose(g_log);
            g_log = NULL;
            /* Clean shutdown — drop the empty file. */
            if (clean && g_log_path[0])
                remove(g_log_path);
        }
        break;
    }
    return TRUE;
}

/* If `v` points into any loaded image, write " (modname+0xRVA)" into `out`;
 * otherwise leave it empty.  MEM_IMAGE filters out heap/stack/free pages. */
static void blog_annotate_module(uintptr_t v, char *out, size_t outsz)
{
    out[0] = '\0';
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery((LPCVOID)v, &mbi, sizeof(mbi)) != sizeof(mbi))
        return;
    if (mbi.State != MEM_COMMIT || mbi.Type != MEM_IMAGE || !mbi.AllocationBase)
        return;
    char path[MAX_PATH];
    if (!GetModuleFileNameA((HMODULE)mbi.AllocationBase, path, sizeof(path)))
        return;
    const char *name = strrchr(path, '\\');
    name = name ? name + 1 : path;
    unsigned rva = (unsigned)(v - (uintptr_t)mbi.AllocationBase);
    snprintf(out, outsz, "  (%s+0x%X)", name, rva);
}

/* SEH filter — catches hard crashes anywhere in the process. */
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

        /* VirtualQuery's allocation base for the crash address IS the
         * HMODULE of the containing image, so this names whichever module
         * actually holds EIP — not just gladiator.dll. */
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

        /* gladiator.dll's own base + vma-relative offset (meaningful only
         * when EIP is inside it). */
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

        /* Registers.  DWORD is cast to unsigned for %X; same bits. */
#if defined(_M_X64) || defined(__x86_64__)
        fprintf(g_log, "  RAX=%016llX RBX=%016llX RCX=%016llX RDX=%016llX\n",
            (unsigned long long)ctx->Rax,
            (unsigned long long)ctx->Rbx,
            (unsigned long long)ctx->Rcx,
            (unsigned long long)ctx->Rdx);
        fprintf(g_log, "  RSI=%016llX RDI=%016llX RSP=%016llX RBP=%016llX\n",
            (unsigned long long)ctx->Rsi,
            (unsigned long long)ctx->Rdi,
            (unsigned long long)ctx->Rsp,
            (unsigned long long)ctx->Rbp);
        uintptr_t *sp = (uintptr_t *)(uintptr_t)ctx->Rsp;
        uintptr_t *bp = (uintptr_t *)(uintptr_t)ctx->Rbp;
#else
        fprintf(g_log, "  EAX=0x%08X  EBX=0x%08X  ECX=0x%08X  EDX=0x%08X\n",
                (unsigned)ctx->Eax, (unsigned)ctx->Ebx, (unsigned)ctx->Ecx, (unsigned)ctx->Edx);
        fprintf(g_log, "  ESI=0x%08X  EDI=0x%08X  ESP=0x%08X  EBP=0x%08X\n",
                (unsigned)ctx->Esi, (unsigned)ctx->Edi, (unsigned)ctx->Esp, (unsigned)ctx->Ebp);
        uintptr_t *sp = (uintptr_t *)(uintptr_t)ctx->Esp;
        uintptr_t *bp = (uintptr_t *)(uintptr_t)ctx->Ebp;
#endif

        /* 64 entries from ESP — enough to reach the return address past the
         * local frame.  Slots pointing into an image get a module!RVA
         * annotation so the call chain needs no manual map lookups. */
        fprintf(g_log, "  Stack (ESP-relative):\n");
        for (int i = 0; i < 64; i++) {
            if (IsBadReadPtr(sp + i, sizeof(*sp)))
                break;
            uintptr_t v = sp[i];
            char ann[MAX_PATH + 32] = {0};
            blog_annotate_module(v, ann, sizeof(ann));
            fprintf(g_log, "    [ESP+%03d] 0x%0*llX%s\n",
                    i * (int)sizeof(*sp), (int)(sizeof(*sp) * 2),
                    (unsigned long long)v, ann);
        }
        /* Also dump EBP frame */
        fprintf(g_log, "  EBP frame:\n");
        for (int i = -4; i <= 8; i++) {
            uintptr_t *p = bp + i;
            if (IsBadReadPtr(p, sizeof(*p)))
                continue;
            uintptr_t v = *p;
            char ann[MAX_PATH + 32] = {0};
            blog_annotate_module(v, ann, sizeof(ann));
            fprintf(g_log, "    [EBP%+d] 0x%0*llX%s\n",
                    i * (int)sizeof(*bp), (int)(sizeof(*bp) * 2),
                    (unsigned long long)v, ann);
        }
        fflush(g_log);
    }
    OutputDebugStringA("gladiator.dll: fatal crash — see gladiator_debug.log\n");
    return EXCEPTION_CONTINUE_SEARCH;
}

/* Installed from DllMain. */
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

