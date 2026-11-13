/**
 * recognizer.c - Implementación del módulo de reconocimiento SAPI
 *
 * SpeechToTextAux - Reconocimiento de voz en español vía SAPI
 * Proyecto: AG.CreadorContenido
 * Plataforma: Windows 10, C + COM/SAPI
 *
 * Este módulo maneja el ciclo de vida completo del motor de
 * reconocimiento de voz de Windows (SAPI) usando COM en C puro.
 * El flujo de eventos es asíncrono: SAPI notifica a través de
 * mensajes de Windows (WM_RECO_EVENT) que son procesados en la
 * ventana procedimiento principal.
 */

#include "recognizer.h"
#include "console_utils.h"

/* ------------------------------------------------------------------ */
/*  Helper: libera un puntero COM y lo pone a NULL                     */
/* ------------------------------------------------------------------ */

#define SAFE_RELEASE(p)  do {                   \
    if ((p) != NULL) {                          \
        (p)->lpVtbl->Release(p);                \
        (p) = NULL;                             \
    }                                           \
} while (0)

/* ------------------------------------------------------------------ */
/*  Recognizer_init                                                    */
/* ------------------------------------------------------------------ */

HRESULT Recognizer_init(RecognizerContext *ctx, HWND hwnd)
{
    HRESULT hr;

    if (ctx == NULL) {
        return E_POINTER;
    }

    /* Limpiar estructura */
    ctx->recognizer  = NULL;
    ctx->recoContext = NULL;
    ctx->recoGrammar = NULL;
    ctx->is_active   = 0;
    ctx->hwnd        = hwnd;

    /* -------------------------------------------------------------- */
    /*  1. Crear el motor de reconocimiento ISpRecognizer             */
    /* -------------------------------------------------------------- */
    hr = CoCreateInstance(&CLSID_SpRecognizer, NULL, CLSCTX_ALL,
                          &IID_ISpRecognizer, (void**)&ctx->recognizer);
    if (FAILED(hr)) {
        Console_printHResult(hr, "CoCreateInstance(CLSID_SpRecognizer)");
        return hr;
    }

    /* -------------------------------------------------------------- */
    /*  2. Crear el contexto de reconocimiento ISpRecoContext         */
    /* -------------------------------------------------------------- */
    hr = ctx->recognizer->lpVtbl->CreateRecoContext(ctx->recognizer,
                                                     &ctx->recoContext);
    if (FAILED(hr)) {
        Console_printHResult(hr, "ISpRecognizer::CreateRecoContext");
        Recognizer_cleanup(ctx);
        return hr;
    }

    /* -------------------------------------------------------------- */
    /*  3. Configurar notificación por mensaje de ventana             */
    /* -------------------------------------------------------------- */
    hr = ctx->recoContext->lpVtbl->SetNotifyWindowMessage(
            ctx->recoContext, hwnd, WM_RECO_EVENT, 0, 0);
    if (FAILED(hr)) {
        Console_printHResult(hr, "ISpRecoContext::SetNotifyWindowMessage");
        Recognizer_cleanup(ctx);
        return hr;
    }

    /* -------------------------------------------------------------- */
    /*  4. Suscribirse a eventos de reconocimiento                    */
    /*     SPEI_RECOGNITION = 38  =>  bitmask (1ULL << 38)           */
    /* -------------------------------------------------------------- */
    {
        const ULONGLONG interest = SPFEI(SPEI_RECOGNITION);
        hr = ctx->recoContext->lpVtbl->SetInterest(
                ctx->recoContext, interest, interest);
    }
    if (FAILED(hr)) {
        Console_printHResult(hr, "ISpRecoContext::SetInterest");
        Recognizer_cleanup(ctx);
        return hr;
    }

    /* -------------------------------------------------------------- */
    /*  5. Crear la gramática de dictado ISpRecoGrammar               */
    /* -------------------------------------------------------------- */
    hr = ctx->recoContext->lpVtbl->CreateGrammar(
            ctx->recoContext, 0, &ctx->recoGrammar);
    if (FAILED(hr)) {
        Console_printHResult(hr, "ISpRecoContext::CreateGrammar");
        Recognizer_cleanup(ctx);
        return hr;
    }

    return S_OK;
}

/* ------------------------------------------------------------------ */
/*  Recognizer_setAudioInput                                          */
/* ------------------------------------------------------------------ */

HRESULT Recognizer_setAudioInput(RecognizerContext *ctx)
{
    HRESULT hr;

    if (ctx == NULL || ctx->recognizer == NULL) {
        return E_POINTER;
    }

    /* ISpRecognizer::SetInput(NULL, TRUE) usa el dispositivo de
     * entrada predeterminado del sistema. El segundo parámetro
     * (fAllowFormatConversion = TRUE) permite a SAPI convertir
     * el formato de audio si es necesario.
     */
    hr = ctx->recognizer->lpVtbl->SetInput(ctx->recognizer, NULL, TRUE);
    if (FAILED(hr)) {
        /* SPERR_NO_AUDIO_INPUT se define como un HRESULT específico */
        Console_printHResult(hr, "ISpRecognizer::SetInput (predeterminado)");
        return hr;
    }

    return S_OK;
}

/* ------------------------------------------------------------------ */
/*  Recognizer_setLanguage                                            */
/* ------------------------------------------------------------------ */

HRESULT Recognizer_setLanguage(RecognizerContext *ctx)
{
    HRESULT hr;

    if (ctx == NULL || ctx->recognizer == NULL) {
        return E_POINTER;
    }

    /* -------------------------------------------------------------- */
    /*  Intentar encontrar un reconocedor que soporte español         */
    /*  (LCID 0x0C0A = Spanish - Spain).                              */
    /*  Usamos ISpObjectTokenCategory + EnumTokens en lugar de        */
    /*  SpEnumTokens() para máxima portabilidad con MinGW-w64.        */
    /* -------------------------------------------------------------- */

    ISpObjectTokenCategory *pCategory = NULL;

    hr = CoCreateInstance(&CLSID_SpObjectTokenCategory, NULL,
                          CLSCTX_ALL, &IID_ISpObjectTokenCategory,
                          (void**)&pCategory);
    if (SUCCEEDED(hr) && pCategory != NULL) {

        hr = pCategory->lpVtbl->SetId(pCategory, SPCAT_RECOGNIZERS, FALSE);
        if (SUCCEEDED(hr)) {

            IEnumSpObjectTokens *pEnum = NULL;
            hr = pCategory->lpVtbl->EnumTokens(
                    pCategory, SAPI_LANG_ATTR_ES, NULL, &pEnum);

            if (SUCCEEDED(hr) && pEnum != NULL) {

                ISpObjectToken *pToken = NULL;
                hr = pEnum->lpVtbl->Next(pEnum, 1, &pToken, NULL);
                if (SUCCEEDED(hr) && pToken != NULL) {

                    /* Encontramos un token de español */
                    hr = ctx->recognizer->lpVtbl->SetRecognizer(
                            ctx->recognizer, pToken);
                    pToken->lpVtbl->Release(pToken);

                    if (SUCCEEDED(hr)) {
                        /* Éxito: reconocedor configurado en español */
                        pEnum->lpVtbl->Release(pEnum);
                        pCategory->lpVtbl->Release(pCategory);
                        return S_OK;
                    }
                }
                pEnum->lpVtbl->Release(pEnum);
            }
        }
        pCategory->lpVtbl->Release(pCategory);
    }

    /* -------------------------------------------------------------- */
    /*  Fallback: usar el reconocedor predeterminado del sistema       */
    /* -------------------------------------------------------------- */
    Console_printWarning(
        "No se encontr\u00F3 reconocedor SAPI para espa\u00F1ol "
        "(0x0C0A). Usando reconocedor predeterminado del sistema. "
        "Instale el paquete de voz en espa\u00F1ol desde "
        "Configuraci\u00F3n > Hora e idioma > Voz.");
    hr = ctx->recognizer->lpVtbl->SetRecognizer(ctx->recognizer, NULL);
    if (FAILED(hr)) {
        Console_printHResult(hr, "ISpRecognizer::SetRecognizer(NULL)");
    }

    /* Devolvemos S_FALSE para indicar que se usó el predeterminado */
    return (SUCCEEDED(hr)) ? S_FALSE : hr;
}

/* ------------------------------------------------------------------ */
/*  Recognizer_start                                                  */
/* ------------------------------------------------------------------ */

HRESULT Recognizer_start(RecognizerContext *ctx)
{
    HRESULT hr;

    if (ctx == NULL || ctx->recoGrammar == NULL) {
        return E_POINTER;
    }

    /* -------------------------------------------------------------- */
    /*  1. Cargar la gramática de dictado (estática = todo ahora)     */
    /* -------------------------------------------------------------- */
    hr = ctx->recoGrammar->lpVtbl->LoadDictation(
            ctx->recoGrammar, NULL, SPLO_STATIC);
    if (FAILED(hr)) {
        Console_printHResult(hr, "ISpRecoGrammar::LoadDictation");
        return hr;
    }

    /* -------------------------------------------------------------- */
    /*  2. Activar el dictado (comenzar a escuchar)                   */
    /* -------------------------------------------------------------- */
    hr = ctx->recoGrammar->lpVtbl->SetDictationState(
            ctx->recoGrammar, SPRS_ACTIVE);
    if (FAILED(hr)) {
        Console_printHResult(hr, "ISpRecoGrammar::SetDictationState(ACTIVE)");
        return hr;
    }

    ctx->is_active = 1;
    return S_OK;
}

/* ------------------------------------------------------------------ */
/*  Recognizer_stop                                                   */
/* ------------------------------------------------------------------ */

HRESULT Recognizer_stop(RecognizerContext *ctx)
{
    HRESULT hr = S_OK;

    if (ctx == NULL || ctx->recoGrammar == NULL) {
        return E_POINTER;
    }

    ctx->is_active = 0;

    hr = ctx->recoGrammar->lpVtbl->SetDictationState(
            ctx->recoGrammar, SPRS_INACTIVE);
    if (FAILED(hr)) {
        Console_printHResult(hr, "ISpRecoGrammar::SetDictationState(INACTIVE)");
    }

    return hr;
}

/* ------------------------------------------------------------------ */
/*  Recognizer_cleanup                                                */
/* ------------------------------------------------------------------ */

void Recognizer_cleanup(RecognizerContext *ctx)
{
    if (ctx == NULL) {
        return;
    }

    /* Si está activo, detenerlo */
    if (ctx->is_active && ctx->recoGrammar != NULL) {
        Recognizer_stop(ctx);
    }

    /* Liberar en orden inverso al de creación */
    SAFE_RELEASE(ctx->recoGrammar);
    SAFE_RELEASE(ctx->recoContext);
    SAFE_RELEASE(ctx->recognizer);

    ctx->hwnd      = NULL;
    ctx->is_active = 0;
}

/* ------------------------------------------------------------------ */
/*  Recognizer_handleEvent                                            */
/* ------------------------------------------------------------------ */

void Recognizer_handleEvent(RecognizerContext *ctx)
{
    SPEVENT  spEvent;
    ULONG    ulFetched = 0;

    if (ctx == NULL || ctx->recoContext == NULL) {
        return;
    }

    /* Procesar todos los eventos pendientes (uno a la vez) */
    while (ctx->recoContext->lpVtbl->GetEvents(
               ctx->recoContext, 1, &spEvent, &ulFetched) == S_OK
           && ulFetched > 0) {

        if (spEvent.eEventId == SPEI_RECOGNITION) {
            /* lParam contiene un puntero ISpRecoResult.
             * La propiedad de esta referencia se transfiere a nosotros
             * (debemos llamar Release() cuando terminemos).
             */
            ISpRecoResult *pResult = (ISpRecoResult *)spEvent.lParam;

            if (pResult != NULL) {
                WCHAR *pszText = NULL;

                HRESULT hr = pResult->lpVtbl->GetText(
                        pResult,
                        SP_GETWHOLEPHRASE,  /* ulStart: desde el inicio  */
                        SP_GETWHOLEPHRASE,  /* ulEnd:   hasta el final   */
                        TRUE,               /* fUseTextReplacements      */
                        &pszText,           /* texto resultante (CoTask) */
                        NULL);              /* ppbDisplayAttributes      */

                if (SUCCEEDED(hr) && pszText != NULL && pszText[0] != L'\0') {
                    /* Mostrar el texto reconocido en la consola */
                    Console_printRecognized(pszText);

                    /* Liberar la memoria asignada por SAPI */
                    CoTaskMemFree(pszText);
                }

                /* Liberar el resultado (la referencia es nuestra) */
                pResult->lpVtbl->Release(pResult);
            }
        }
    }
}
