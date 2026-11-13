/**
 * main.c - Punto de entrada del programa SpeechToTextAux
 *
 * SpeechToTextAux - Reconocimiento de voz en español vía SAPI
 * Proyecto: AG.CreadorContenido
 * Plataforma: Windows 10, C + COM/SAPI
 *
 * Flujo principal:
 *   1. Registrar manejador de Ctrl+C
 *   2. Inicializar COM (CoInitializeEx con COINIT_MULTITHREADED)
 *   3. Crear una ventana oculta para recibir eventos SAPI
 *   4. Inicializar y configurar el reconocedor SAPI
 *   5. Bucle de mensajes (GetMessage / DispatchMessage)
 *   6. Al salir, liberar recursos y finalizar COM
 */

#include <windows.h>
#include "resource.h"
#include "recognizer.h"
#include "console_utils.h"

/* ------------------------------------------------------------------ */
/*  Variables globales (mínimas y controladas)                         */
/* ------------------------------------------------------------------ */

static RecognizerContext g_ctx;    /**< Contexto global del reconocedor */
static HWND              g_hwnd;   /**< Handle de la ventana oculta    */
static volatile int      g_running = 1; /**< Flag de ejecución         */

/* ------------------------------------------------------------------ */
/*  Manejador de Ctrl+C                                               */
/* ------------------------------------------------------------------ */

/**
 * Callback invocado por Windows cuando el usuario presiona Ctrl+C
 * o Ctrl+Break. Se ejecuta en un hilo separado, por lo que solo
 * modificamos una variable atómica y enviamos una señal a la
 * ventana oculta para desbloquear GetMessage().
 */
static BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    if (fdwCtrlType == CTRL_C_EVENT || fdwCtrlType == CTRL_BREAK_EVENT) {
        /* Evitar que se muestre el mensaje varias veces */
        if (!g_running) {
            return TRUE;
        }

        g_running = 0;

        /* Enviar WM_CLOSE a la ventana oculta para que el bucle de
         * mensajes se despierte y procese la salida ordenadamente. */
        if (g_hwnd != NULL) {
            PostMessage(g_hwnd, WM_CLOSE, 0, 0);
        }
        return TRUE;  /* Evento manejado */
    }
    return FALSE;  /* Pasar al siguiente manejador */
}

/* ------------------------------------------------------------------ */
/*  Ventana procedimiento (oculta)                                     */
/* ------------------------------------------------------------------ */

/**
 * Window procedure para la ventana oculta que recibe los eventos
 * de reconocimiento de SAPI.
 */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg,
                                WPARAM wParam, LPARAM lParam)
{
    switch (msg) {

    case WM_RECO_EVENT:
        /* Evento de reconocimiento SAPI */
        Recognizer_handleEvent(&g_ctx);
        return 0;

    case WM_CLOSE:
        Console_printInfo("Deteniendo reconocimiento...");
        /* Iniciar la secuencia de cierre ordenado */
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        /* Evitar que la limpieza final en main() intente
         * destruir una ventana ya destruida */
        g_hwnd = NULL;
        /* Publicar WM_QUIT para que GetMessage() retorne 0 */
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

/* ------------------------------------------------------------------ */
/*  main()                                                            */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    HRESULT  hr;
    MSG      msg;
    HINSTANCE hInst;
    WNDCLASSEX wc;

    /* Silenciar advertencias de parámetros no usados */
    (void)argc;
    (void)argv;

    /* -------------------------------------------------------------- */
    /*  1. Configurar título de la ventana de consola                 */
    /* -------------------------------------------------------------- */
    SetConsoleTitleW(L"SpeechToTextAux - Reconocimiento de Voz en Espa\u00F1ol");

    /* -------------------------------------------------------------- */
    /*  2. Registrar manejador de Ctrl+C / Ctrl+Break                 */
    /* -------------------------------------------------------------- */
    if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
        Console_printError(
            "No se pudo registrar el manejador de Ctrl+C. "
            "La finalizaci\u00F3n podr\u00EDa no ser limpia.");
        /* Continuamos de todas formas */
    }

    /* -------------------------------------------------------------- */
    /*  3. Inicializar COM (apartamento multi-hilo)                   */
    /* -------------------------------------------------------------- */
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        Console_printHResult(hr,
            "Error al inicializar COM (CoInitializeEx). "
            "Aseg\u00FArese de ejecutar en Windows con SAPI instalado.");
        return ERR_COM_INIT_FAIL;
    }

    /* -------------------------------------------------------------- */
    /*  4. Registrar clase de ventana oculta                          */
    /* -------------------------------------------------------------- */
    hInst = GetModuleHandleW(NULL);

    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"SpeechToTextAuxWindowClass";

    if (!RegisterClassEx(&wc)) {
        Console_printError(
            "Error al registrar la clase de ventana oculta.");
        CoUninitialize();
        return 1;
    }

    /* -------------------------------------------------------------- */
    /*  5. Crear ventana oculta (sin borde, sin mostrar)              */
    /* -------------------------------------------------------------- */
    g_hwnd = CreateWindowExW(
                 WS_EX_TOOLWINDOW,          /* No aparece en la barra */
                 L"SpeechToTextAuxWindowClass",
                 L"SpeechToTextAux Hidden Window",
                 WS_POPUP,                  /* Sin borde ni título    */
                 0, 0, 0, 0,
                 NULL,                      /* Sin ventana padre      */
                 NULL,                      /* Sin menú               */
                 hInst,
                 NULL);

    if (g_hwnd == NULL) {
        Console_printError(
            "Error al crear la ventana oculta para eventos SAPI.");
        CoUninitialize();
        return 1;
    }

    /* -------------------------------------------------------------- */
    /*  6. Inicializar el reconocedor SAPI                            */
    /* -------------------------------------------------------------- */
    Console_printInfo(
        "Iniciando reconocimiento de voz en espa\u00F1ol...");

    hr = Recognizer_init(&g_ctx, g_hwnd);
    if (FAILED(hr)) {
        Console_printHResult(hr,
            "Error al inicializar el motor de reconocimiento SAPI. "
            "Verifique que SAPI est\u00E9 disponible en el sistema.");
        DestroyWindow(g_hwnd);
        CoUninitialize();
        return ERR_RECO_INIT_FAIL;
    }

    /* -------------------------------------------------------------- */
    /*  7. Configurar entrada de audio predeterminada                 */
    /* -------------------------------------------------------------- */
    hr = Recognizer_setAudioInput(&g_ctx);
    if (FAILED(hr)) {
        Console_printError(
            "No se encontr\u00F3 dispositivo de entrada de audio. "
            "Conecte un micr\u00F3fono o dispositivo a la entrada "
            "auxiliar (line-in) y verifique que est\u00E9 configurado "
            "como dispositivo predeterminado en Windows.");
        Recognizer_cleanup(&g_ctx);
        DestroyWindow(g_hwnd);
        CoUninitialize();
        return ERR_RECO_NO_DEVICE;
    }

    /* -------------------------------------------------------------- */
    /*  8. Configurar idioma español                                  */
    /* -------------------------------------------------------------- */
    hr = Recognizer_setLanguage(&g_ctx);
    if (hr == S_FALSE) {
        /* Ya se mostró la advertencia dentro de setLanguage */
    } else if (FAILED(hr)) {
        Console_printError(
            "Error al configurar el idioma de reconocimiento. "
            "El programa podr\u00EDa no funcionar correctamente.");
        /* Continuamos de todas formas */
    }

    /* -------------------------------------------------------------- */
    /*  9. Iniciar el reconocimiento                                  */
    /* -------------------------------------------------------------- */
    hr = Recognizer_start(&g_ctx);
    if (FAILED(hr)) {
        Console_printHResult(hr,
            "Error al iniciar el reconocimiento de voz. "
            "Verifique que el paquete de idioma espa\u00F1ol est\u00E9 "
            "instalado (Configuraci\u00F3n > Hora e idioma > Voz).");
        Recognizer_cleanup(&g_ctx);
        DestroyWindow(g_hwnd);
        CoUninitialize();
        return ERR_RECO_INIT_FAIL;
    }

    Console_printInfo(
        "Reconocimiento activo. Habla por el dispositivo de entrada "
        "(line-in/micr\u00F3fono).");
    Console_printInfo(
        "Presiona Ctrl+C para salir.");

    /* -------------------------------------------------------------- */
    /*  10. Bucle de mensajes principal                                */
    /* -------------------------------------------------------------- */

    while (g_running) {
        /* GetMessage espera hasta que llegue un mensaje. Retorna 0
         * cuando recibe WM_QUIT (posteado por PostQuitMessage). */
        int ret = GetMessage(&msg, NULL, 0, 0);
        if (ret == 0) {
            /* WM_QUIT recibido */
            break;
        }
        if (ret == -1) {
            /* Error en GetMessage */
            Console_printError("Error en el bucle de mensajes.");
            break;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    /* Si salimos por g_running == 0 pero no por WM_QUIT,
     * aseguramos la limpieza */
    if (g_running == 0) {
        /* PostQuitMessage podría ya haberse llamado vía WM_CLOSE */
    }

    /* -------------------------------------------------------------- */
    /*  11. Limpieza final                                             */
    /* -------------------------------------------------------------- */

    Console_printInfo("Liberando recursos SAPI...");
    Recognizer_stop(&g_ctx);
    Recognizer_cleanup(&g_ctx);

    if (g_hwnd != NULL) {
        DestroyWindow(g_hwnd);
        g_hwnd = NULL;
    }

    CoUninitialize();

    Console_printInfo(
        "Programa terminado correctamente.");
    return 0;
}
