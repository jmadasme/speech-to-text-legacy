/**
 * console_utils.c - Implementación de utilidades de consola
 *
 * SpeechToTextAux - Reconocimiento de voz en español vía SAPI
 * Proyecto: AG.CreadorContenido
 * Plataforma: Windows 10, C + COM/SAPI
 */

#include "console_utils.h"

/* ------------------------------------------------------------------ */
/*  Constantes de color para la consola                                */
/* ------------------------------------------------------------------ */

/* Colores de texto (primer plano) */
#define COL_CYAN        (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define COL_GREEN       (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define COL_RED         (FOREGROUND_RED | FOREGROUND_INTENSITY)
#define COL_YELLOW      (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define COL_WHITE_BRIGHT (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define COL_DEFAULT     (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE) /* Gris claro */

/* ------------------------------------------------------------------ */
/*  Internas: guardar/restaurar atributos de consola                   */
/* ------------------------------------------------------------------ */

static WORD saveConsoleAttributes(HANDLE hConsole)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        return csbi.wAttributes;
    }
    /* Si falla, devolvemos un valor seguro (blanco sobre negro) */
    return COL_DEFAULT;
}

static void restoreConsoleAttributes(HANDLE hConsole, WORD saved)
{
    SetConsoleTextAttribute(hConsole, saved);
}

/* ------------------------------------------------------------------ */
/*  Console_printTimestamp                                             */
/* ------------------------------------------------------------------ */

void Console_printTimestamp(void)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    printf("[%02u:%02u:%02u] ", st.wHour, st.wMinute, st.wSecond);
}

/* ------------------------------------------------------------------ */
/*  Console_printRecognized                                            */
/* ------------------------------------------------------------------ */

void Console_printRecognized(const WCHAR *text)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD saved = saveConsoleAttributes(hConsole);
    DWORD mode;

    if (!text || text[0] == L'\0') {
        return;
    }

    /* Timestamp en cian */
    SetConsoleTextAttribute(hConsole, COL_CYAN);
    Console_printTimestamp();

    /* Texto reconocido en blanco brillante */
    SetConsoleTextAttribute(hConsole, COL_WHITE_BRIGHT);

    if (GetConsoleMode(hConsole, &mode)) {
        /* Consola real -> WriteConsoleW para Unicode */
        WriteConsoleW(hConsole, text, (DWORD)wcslen(text), NULL, NULL);
        WriteConsoleW(hConsole, L"\n", 1, NULL, NULL);
    } else {
        /* Redirección a archivo/pipe -> usar fwprintf */
        fwprintf(stdout, L"%s\n", text);
    }

    restoreConsoleAttributes(hConsole, saved);
}

/* ------------------------------------------------------------------ */
/*  Console_printInfo                                                  */
/* ------------------------------------------------------------------ */

void Console_printInfo(const char *message)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD saved = saveConsoleAttributes(hConsole);

    SetConsoleTextAttribute(hConsole, COL_GREEN);
    Console_printTimestamp();
    printf("%s\n", message);

    restoreConsoleAttributes(hConsole, saved);
}

/* ------------------------------------------------------------------ */
/*  Console_printError                                                 */
/* ------------------------------------------------------------------ */

void Console_printError(const char *message)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD saved = saveConsoleAttributes(hConsole);

    SetConsoleTextAttribute(hConsole, COL_RED);
    Console_printTimestamp();
    printf("ERROR: %s\n", message);

    restoreConsoleAttributes(hConsole, saved);
}

/* ------------------------------------------------------------------ */
/*  Console_printWarning                                               */
/* ------------------------------------------------------------------ */

void Console_printWarning(const char *message)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD saved = saveConsoleAttributes(hConsole);

    SetConsoleTextAttribute(hConsole, COL_YELLOW);
    Console_printTimestamp();
    printf("ADVERTENCIA: %s\n", message);

    restoreConsoleAttributes(hConsole, saved);
}

/* ------------------------------------------------------------------ */
/*  Console_printHResult                                               */
/* ------------------------------------------------------------------ */

void Console_printHResult(HRESULT hr, const char *prefix)
{
    WCHAR *msgBuf = NULL;
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS;
    DWORD langId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

    DWORD ret = FormatMessageW(flags, NULL, (DWORD)hr, langId,
                               (LPWSTR)&msgBuf, 0, NULL);

    if (ret > 0 && msgBuf) {
        /* Quitar el retorno de carro/fin de línea del mensaje */
        size_t len = wcslen(msgBuf);
        while (len > 0 && (msgBuf[len - 1] == L'\r' || msgBuf[len - 1] == L'\n')) {
            msgBuf[--len] = L'\0';
        }
        Console_printError(prefix);
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        WORD saved = saveConsoleAttributes(hConsole);
        SetConsoleTextAttribute(hConsole, COL_RED);
        printf("  HRESULT: 0x%08lX - %ws\n", (unsigned long)hr, msgBuf);
        restoreConsoleAttributes(hConsole, saved);
        LocalFree(msgBuf);
    } else {
        Console_printError(prefix);
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        WORD saved = saveConsoleAttributes(hConsole);
        SetConsoleTextAttribute(hConsole, COL_RED);
        printf("  HRESULT: 0x%08lX\n", (unsigned long)hr);
        restoreConsoleAttributes(hConsole, saved);
    }
}
