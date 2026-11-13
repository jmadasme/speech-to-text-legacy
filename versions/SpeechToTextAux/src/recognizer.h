/**
 * recognizer.h - Módulo de reconocimiento de voz SAPI
 *
 * SpeechToTextAux - Reconocimiento de voz en español vía SAPI
 * Proyecto: AG.CreadorContenido
 * Plataforma: Windows 10, C + COM/SAPI
 */

#ifndef RECOGNIZER_H
#define RECOGNIZER_H

#include <windows.h>
#include <sapi.h>
#include "resource.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Contexto del reconocedor                                           */
/* ------------------------------------------------------------------ */

/**
 * Estructura que agrupa todos los objetos COM necesarios para el
 * reconocimiento de voz. Se inicializa con Recognizer_init() y se
 * libera con Recognizer_cleanup().
 */
typedef struct {
    ISpRecognizer   *recognizer;    /**< Motor de reconocimiento SAPI   */
    ISpRecoContext  *recoContext;   /**< Contexto de reconocimiento     */
    ISpRecoGrammar  *recoGrammar;   /**< Gramática de dictado           */
    volatile int     is_active;     /**< 1 si el reconocimiento está    */
                                    /**<   activo, 0 si no              */
    HWND             hwnd;          /**< Ventana oculta para notific.   */
} RecognizerContext;

/* ------------------------------------------------------------------ */
/*  Funciones del ciclo de vida del reconocedor                        */
/* ------------------------------------------------------------------ */

/**
 * Inicializa los objetos COM de SAPI y crea el contexto de
 * reconocimiento y la gramática.
 *
 * @param ctx   Puntero a la estructura RecognizerContext (no inicializada).
 * @param hwnd  Handle de la ventana que recibirá los mensajes de evento
 *              de reconocimiento (WM_RECO_EVENT).
 * @return S_OK en éxito, o un HRESULT de error.
 */
HRESULT Recognizer_init(RecognizerContext *ctx, HWND hwnd);

/**
 * Configura la entrada de audio predeterminada del sistema
 * (ISpRecognizer::SetInput(NULL, TRUE)).
 *
 * @param ctx  Contexto del reconocedor inicializado.
 * @return S_OK en éxito, o SPERR_NO_AUDIO_INPUT si no hay dispositivo.
 */
HRESULT Recognizer_setAudioInput(RecognizerContext *ctx);

/**
 * Intenta configurar el motor de reconocimiento para usar el idioma
 * español (LCID 0x0C0A). Busca en el catálogo SAPI un reconocedor que
 * soporte dicho idioma. Si no lo encuentra, cae al reconocedor
 * predeterminado del sistema y retorna un código de advertencia.
 *
 * @param ctx  Contexto del reconocedor inicializado.
 * @return S_OK en éxito; S_FALSE si se usó el predeterminado;
 *         HRESULT de error en otro caso.
 */
HRESULT Recognizer_setLanguage(RecognizerContext *ctx);

/**
 * Activa el reconocimiento de voz: carga la gramática de dictado y la
 * pone en estado activo (SPRS_ACTIVE).
 *
 * @param ctx  Contexto del reconocedor.
 * @return S_OK en éxito, HRESULT de error en otro caso.
 */
HRESULT Recognizer_start(RecognizerContext *ctx);

/**
 * Desactiva el reconocimiento de voz: pone la gramática en estado
 * inactivo (SPRS_INACTIVE).
 *
 * @param ctx  Contexto del reconocedor.
 * @return S_OK en éxito, HRESULT de error en otro caso.
 */
HRESULT Recognizer_stop(RecognizerContext *ctx);

/**
 * Libera todos los objetos COM del contexto y pone los punteros a NULL.
 * Es seguro llamarla múltiples veces.
 *
 * @param ctx  Contexto del reconocedor.
 */
void Recognizer_cleanup(RecognizerContext *ctx);

/**
 * Procesa un evento de reconocimiento. Debe ser llamada desde la
 * ventana procedimiento (WndProc) al recibir WM_RECO_EVENT.
 * Extrae el texto reconocido de ISpRecoResult y lo entrega a
 * Console_printRecognized().
 *
 * @param ctx  Contexto del reconocedor.
 */
void Recognizer_handleEvent(RecognizerContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* RECOGNIZER_H */
