/**
 * console_utils.h - Utilidades de consola con formato y color
 *
 * SpeechToTextAux - Reconocimiento de voz en español vía SAPI
 * Proyecto: AG.CreadorContenido
 * Plataforma: Windows 10, C + COM/SAPI
 */

#ifndef CONSOLE_UTILS_H
#define CONSOLE_UTILS_H

#include <windows.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Funciones de impresión con formato                                 */
/* ------------------------------------------------------------------ */

/**
 * Imprime en consola la marca de hora actual en formato [HH:MM:SS].
 * Usa el color por defecto del terminal (no lo cambia).
 */
void Console_printTimestamp(void);

/**
 * Imprime una línea completa de texto reconocido, con timestamp
 * coloreado en cian y el texto en blanco brillante.
 *
 * @param text  Texto Unicode (UTF-16) a mostrar.
 */
void Console_printRecognized(const WCHAR *text);

/**
 * Imprime un mensaje informativo con timestamp en color verde.
 *
 * @param message  Cadena ANSI (char*) con el mensaje.
 */
void Console_printInfo(const char *message);

/**
 * Imprime un mensaje de error con timestamp en color rojo.
 * La salida se dirige a stdout.
 *
 * @param message  Cadena ANSI (char*) con el mensaje.
 */
void Console_printError(const char *message);

/**
 * Imprime una advertencia con timestamp en color amarillo.
 *
 * @param message  Cadena ANSI (char*) con el mensaje.
 */
void Console_printWarning(const char *message);

/**
 * Convierte un HRESULT en un mensaje de error legible y lo imprime.
 *
 * @param hr    Código de error COM.
 * @param prefix  Texto adicional que precede al error (p.ej. nombre
 *                de la función que falló).
 */
void Console_printHResult(HRESULT hr, const char *prefix);

#ifdef __cplusplus
}
#endif

#endif /* CONSOLE_UTILS_H */
