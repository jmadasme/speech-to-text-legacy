/**
 * resource.h - Constantes y recursos compartidos del programa
 *
 * SpeechToTextAux - Reconocimiento de voz en español vía SAPI
 * Proyecto: AG.CreadorContenido
 * Plataforma: Windows 10, C + COM/SAPI
 */

#ifndef RESOURCE_H
#define RESOURCE_H

#include <windows.h>

/* ------------------------------------------------------------------ */
/*  Mensajes personalizados de Windows                                 */
/* ------------------------------------------------------------------ */

/**
 * WM_RECO_EVENT - Mensaje enviado por SAPI al hilo principal cuando
 * ocurre un evento de reconocimiento de voz.
 * El wParam/lParam no se usan directamente; los datos del evento se
 * obtienen llamando a ISpRecoContext::GetEvents().
 */
#define WM_RECO_EVENT  (WM_USER + 100)

/* ------------------------------------------------------------------ */
/*  Códigos de error personalizados (evitan colisión con HRESULT)      */
/* ------------------------------------------------------------------ */

#define ERR_RECO_NO_DEVICE    0xE001   /* Sin dispositivo de entrada   */
#define ERR_RECO_NO_LANGUAGE  0xE002   /* Idioma español no disponible */
#define ERR_RECO_INIT_FAIL    0xE003   /* Fallo genérico de inicializ. */
#define ERR_COM_INIT_FAIL     0xE004   /* Fallo al inicializar COM     */

/* ------------------------------------------------------------------ */
/*  LCID del idioma español                                            */
/* ------------------------------------------------------------------ */

#define LANG_ES_ES  0x0C0A   /* Español (España, 3082 decimal)        */
#define LANG_ES_MX  0x080A   /* Español (México)                      */
#define LANG_ES_US  0x580A   /* Español (Estados Unidos)              */

/* ------------------------------------------------------------------ */
/*  Atributo para buscar reconocedores de español en el catálogo SAPI  */
/* ------------------------------------------------------------------ */

#define SAPI_LANG_ATTR_ES   L"Language=0C0A"

#endif /* RESOURCE_H */
