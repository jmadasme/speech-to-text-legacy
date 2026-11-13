# SpeechToTextAux - Documentacion para desarrolladores

## Arquitectura del programa

### Diagrama de modulos

```
+------------------------------------------------------+
|                      main.c                          |
|  Punto de entrada, inicializacion COM, ventana       |
|  oculta, bucle de mensajes, Ctrl+C handler           |
|                                                      |
|  Depende de: recognizer.h, console_utils.h,          |
|              resource.h, windows.h                    |
+---------------------------+--------------------------+
                            |
          +-----------------+-----------------+
          |                                   |
          v                                   v
+---------------------------+  +----------------------------+
|      recognizer.c/h       |  |     console_utils.c/h      |
|  Motor de reconocimiento  |  |  Utilidades de consola     |
|  SAPI (COM)               |  |  con formato y color       |
|                           |  |                            |
|  Depende de:              |  |  Depende de:               |
|    windows.h, sapi.h,     |  |    windows.h, stdio.h      |
|    resource.h,            |  |                            |
|    console_utils.h        |  |                            |
+---------------------------+  +----------------------------+
                           
+---------------------------+
|       resource.h          |
|  Constantes compartidas:  |
|  WM_RECO_EVENT,           |
|  codigos de error,        |
|  LCIDs de idioma          |
+---------------------------+
```

### Flujo de control general

El programa sigue un modelo de programacion orientado a eventos tipico de Windows:

1. **Inicializacion**: Configura todos los subsistemas (COM, SAPI, ventana, consola)
2. **Bucle de eventos**: Espera pasivamente mensajes de Windows
3. **Procesamiento de eventos**: Cuando SAPI reconoce voz, envia un mensaje que es procesado en el contexto del hilo principal
4. **Finalizacion**: Detiene el reconocimiento y libera todos los recursos en orden inverso al de inicializacion

### Modelo de concurrencia

El programa es monohilo (un solo hilo de ejecucion para el programa). Sin embargo, el motor SAPI utiliza sus propios hilos internos para procesar el audio. La comunicacion entre los hilos internos de SAPI y el programa se realiza de forma segura mediante mensajes de Windows (PostMessage). Esto garantiza que todo el codigo del programa se ejecute en un solo hilo, eliminando condiciones de carrera.

## Explicacion detallada de cada modulo

### main.c

**Archivo**: `src/main.c` (289 lineas)

**Proposito**: Punto de entrada del programa. Gestiona el ciclo de vida completo: inicializacion de COM, creacion de la ventana oculta para eventos SAPI, configuracion del motor de reconocimiento, bucle de mensajes y limpieza final.

**Estructura**:

```
main.c
  |-- Variables globales
  |     |-- g_ctx (RecognizerContext)  - Contexto global del reconocedor SAPI
  |     |-- g_hwnd (HWND)              - Handle de la ventana oculta
  |     |-- g_running (volatile int)   - Flag de ejecucion (1 = corriendo, 0 = deteniendo)
  |
  |-- CtrlHandler() (static)
  |     Callback de consola para Ctrl+C y Ctrl+Break
  |
  |-- WndProc() (static)
  |     Window procedure para la ventana oculta
  |
  |-- main()
        Punto de entrada del programa
```

**Funciones**:

#### `static BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)`

- **Proposito**: Manejador de senales de consola registrado via `SetConsoleCtrlHandler`.
- **Cuando se ejecuta**: Cuando el usuario presiona Ctrl+C (CTRL_C_EVENT) o Ctrl+Break (CTRL_BREAK_EVENT), o cuando el usuario cierra la ventana de consola.
- **Contexto de ejecucion**: Se ejecuta en un hilo separado del sistema, no en el hilo principal. Por esta razon, no debe llamar a funciones de Windows que no sean seguras para hilos (como printf o SetConsoleTextAttribute).
- **Comportamiento**:
  1. Verifica que el evento sea Ctrl+C o Ctrl+Break
  2. Si `g_running` ya es 0, retorna TRUE (evita procesamiento multiple)
  3. Establece `g_running = 0`
  4. Publica un mensaje WM_CLOSE a la ventana oculta via `PostMessage()`
  5. Retorna TRUE (evento manejado, el sistema no debe mostrar el mensaje "Terminar programa")
- **Consideraciones de diseno**:
  - No llama a Console_printInfo() ni ninguna funcion de E/S (ver I-002 en auditoria)
  - Usa PostMessage en lugar de SendMessage para evitar deadlocks
  - La variable `g_running` es `volatile` porque es modificada desde un hilo diferente

#### `static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)`

- **Proposito**: Window procedure para la ventana oculta. Procesa los mensajes de Windows, incluyendo los eventos de reconocimiento de SAPI.
- **Mensajes manejados**:
  - **WM_RECO_EVENT**: Evento de reconocimiento SAPI. Llama a `Recognizer_handleEvent(&g_ctx)` para procesar los resultados.
  - **WM_CLOSE**: Inicia la secuencia de cierre. Muestra "Deteniendo reconocimiento..." y llama a `DestroyWindow()`.
  - **WM_DESTROY**: Establece `g_hwnd = NULL` (para prevenir doble destruccion) y publica WM_QUIT via `PostQuitMessage(0)`.
  - **default**: Llama a `DefWindowProc()` para el procesamiento por defecto.
- **Nota**: La ventana nunca se muestra. Su unico proposito es recibir mensajes de Windows. Se crea con el estilo `WS_EX_TOOLWINDOW` para que no aparezca en la barra de tareas.

#### `int main(int argc, char *argv[])`

- **Proposito**: Punto de entrada del programa.
- **Parametros**: No usa argc ni argv (se silencian con `(void)argc; (void)argv;`).
- **Flujo**:
  1. `SetConsoleTitleW()` - Configura el titulo de la consola
  2. `SetConsoleCtrlHandler()` - Registra CtrlHandler
  3. `CoInitializeEx(NULL, COINIT_MULTITHREADED)` - Inicializa COM en modo multi-hilo
  4. `RegisterClassEx()` - Registra la clase de ventana "SpeechToTextAuxWindowClass"
  5. `CreateWindowEx()` - Crea la ventana oculta
  6. `Recognizer_init(&g_ctx, g_hwnd)` - Inicializa el motor SAPI
  7. `Recognizer_setAudioInput(&g_ctx)` - Configura entrada de audio
  8. `Recognizer_setLanguage(&g_ctx)` - Configura idioma espanol
  9. `Recognizer_start(&g_ctx)` - Inicia el reconocimiento
  10. Bucle de mensajes: `while(g_running) { GetMessage(); TranslateMessage(); DispatchMessage(); }`
  11. `Recognizer_stop(&g_ctx)` - Detiene el reconocimiento
  12. `Recognizer_cleanup(&g_ctx)` - Libera recursos SAPI
  13. `DestroyWindow(g_hwnd)` si g_hwnd != NULL
  14. `CoUninitialize()` - Finaliza COM
- **Manejo de errores**: Cada paso verifica HRESULT o codigos de error y, si falla, muestra el error correspondiente y sale con un codigo de error apropiado.

### recognizer.c

**Archivo**: `src/recognizer.c` (342 lineas)

**Proposito**: Implementa el ciclo de vida completo del motor de reconocimiento de voz de Windows (SAPI) utilizando COM en C puro. Gestiona la creacion, configuracion, inicio, detencion y limpieza del motor, asi como el procesamiento de eventos de reconocimiento.

**API expuesta**: 6 funciones (declaradas en recognizer.h).

**Estructuras de datos**:

#### `RecognizerContext` (definida en recognizer.h)

```c
typedef struct {
    ISpRecognizer   *recognizer;    // Motor de reconocimiento SAPI
    ISpRecoContext  *recoContext;   // Contexto de reconocimiento
    ISpRecoGrammar  *recoGrammar;   // Gramatica de dictado
    volatile int     is_active;     // 1 si el reconocimiento esta activo
    HWND             hwnd;          // Ventana oculta para notificaciones
} RecognizerContext;
```

- **recognizer (ISpRecognizer)**: El motor de reconocimiento principal. Es el objeto COM que gestiona la entrada de audio y el procesamiento del habla.
- **recoContext (ISpRecoContext)**: El contexto de reconocimiento. Asocia el motor con la ventana que recibira las notificaciones. Permite configurar que eventos nos interesan y crear gramaticas.
- **recoGrammar (ISpRecoGrammar)**: La gramatica de dictado. Define el conjunto de palabras y frases que el reconocedor puede identificar. En modo dictado, es abierta (cualquier palabra del idioma).
- **is_active**: Flag que indica si el reconocimiento esta actualmente activo. Se usa para saber si debemos detenerlo durante la limpieza.
- **hwnd**: Handle de la ventana oculta. Se pasa a `SetNotifyWindowMessage()` para que SAPI sepa a que ventana enviar los eventos.

**Funciones**:

#### `HRESULT Recognizer_init(RecognizerContext *ctx, HWND hwnd)`

- **Proposito**: Inicializa todos los objetos COM de SAPI.
- **Parametros**:
  - `ctx`: Puntero a estructura RecognizerContext (debe ser no inicializada o estar en cero)
  - `hwnd`: Handle de la ventana que recibira los mensajes de evento
- **Proceso**:
  1. Limpia la estructura a cero (todos los punteros a NULL, is_active = 0)
  2. Guarda el HWND en ctx->hwnd
  3. Crea el motor de reconocimiento: `CoCreateInstance(CLSID_SpRecognizer, ..., IID_ISpRecognizer, ...)`
  4. Crea el contexto de reconocimiento: `recognizer->CreateRecoContext(&recoContext)`
  5. Configura notificacion por mensaje de ventana: `recoContext->SetNotifyWindowMessage(hwnd, WM_RECO_EVENT, 0, 0)`
  6. Suscribe al evento SPEI_RECOGNITION: `recoContext->SetInterest(SPFEI(SPEI_RECOGNITION), ...)`
  7. Crea la gramatica: `recoContext->CreateGrammar(0, &recoGrammar)`
- **Orden de creacion**: recognizer -> recoContext -> recoGrammar
- **Manejo de errores**: Cada paso verifica el HRESULT. Si falla, llama a `Recognizer_cleanup()` para liberar lo que ya se creo y retorna el error.

#### `HRESULT Recognizer_setAudioInput(RecognizerContext *ctx)`

- **Proposito**: Configura la entrada de audio usando el dispositivo predeterminado del sistema.
- **Proceso**:
  1. Llama a `recognizer->SetInput(NULL, TRUE)`
     - NULL = usar el dispositivo de entrada predeterminado del sistema
     - TRUE = permitir conversion de formato de audio (fAllowFormatConversion)
- **Error tipico**: `SPERR_NO_AUDIO_INPUT (0x80045002)` si no hay ningun dispositivo de entrada disponible.

#### `HRESULT Recognizer_setLanguage(RecognizerContext *ctx)`

- **Proposito**: Configura el motor de reconocimiento para usar espanol (LCID 0x0C0A).
- **Proceso**:
  1. Crea un objeto `ISpObjectTokenCategory` para la categoria de reconocedores (SPCAT_RECOGNIZERS)
  2. Enumera los tokens que tengan el atributo `Language=0C0A`
  3. Si encuentra un token, lo configura como reconocedor activo via `SetRecognizer()`
  4. Si no encuentra, usa el reconocedor predeterminado del sistema y muestra una advertencia
- **Valor de retorno**:
  - `S_OK`: Se configuro correctamente el idioma espanol
  - `S_FALSE`: No se encontro el idioma, se uso el predeterminado (con advertencia al usuario)
  - HRESULT de error: Fallo en la configuracion
- **Nota tecnica**: Usa `ISpObjectTokenCategory` + `EnumTokens` en lugar de `SpEnumTokens()` para maxima portabilidad con MinGW-w64.

#### `HRESULT Recognizer_start(RecognizerContext *ctx)`

- **Proposito**: Activa el reconocimiento de voz.
- **Proceso**:
  1. Carga la gramatica de dictado: `recoGrammar->LoadDictation(NULL, SPLO_STATIC)`
     - SPLO_STATIC = toda la gramatica se carga inmediatamente en memoria
  2. Activa el dictado: `recoGrammar->SetDictationState(SPRS_ACTIVE)`
  3. Establece `ctx->is_active = 1`

#### `HRESULT Recognizer_stop(RecognizerContext *ctx)`

- **Proposito**: Desactiva el reconocimiento de voz.
- **Proceso**:
  1. Establece `ctx->is_active = 0`
  2. Desactiva el dictado: `recoGrammar->SetDictationState(SPRS_INACTIVE)`

#### `void Recognizer_cleanup(RecognizerContext *ctx)`

- **Proposito**: Libera todos los objetos COM asociados al contexto.
- **Proceso**:
  1. Si esta activo, llama a `Recognizer_stop()`
  2. Libera recoGrammar mediante `SAFE_RELEASE`
  3. Libera recoContext mediante `SAFE_RELEASE`
  4. Libera recognizer mediante `SAFE_RELEASE`
  5. Pone ctx->hwnd = NULL y ctx->is_active = 0
- **Orden de liberacion**: Inverso al de creacion: recoGrammar -> recoContext -> recognizer
- **Seguridad**: Es seguro llamarla multiples veces (los punteros se ponen a NULL tras liberarlos)

#### `void Recognizer_handleEvent(RecognizerContext *ctx)`

- **Proposito**: Procesa los eventos de reconocimiento pendientes. Debe ser llamada desde WndProc al recibir WM_RECO_EVENT.
- **Proceso**:
  1. Bucle: mientras `GetEvents()` devuelva eventos
  2. Para cada evento con `eEventId == SPEI_RECOGNITION`:
     a. Obtiene `ISpRecoResult*` del campo `lParam` del evento SPEVENT
     b. Llama a `pResult->GetText(...)` para obtener el texto reconocido como WCHAR*
     c. Si hay texto, llama a `Console_printRecognized(pszText)`
     d. Libera la memoria del texto con `CoTaskMemFree(pszText)`
     e. Libera el objeto resultado con `pResult->Release()`
- **Nota importante sobre la propiedad**: Cuando SAPI entrega un evento SPEI_RECOGNITION, la referencia al objeto ISpRecoResult en lParam se transfiere al receptor. El programa debe llamar a Release() cuando termine de usarlo.

### console_utils.c

**Archivo**: `src/console_utils.c` (170 lineas)

**Proposito**: Proporciona funciones de salida a consola con formato (timestamp, colores) y manejo de cadenas Unicode.

**API expuesta**: 6 funciones (declaradas en console_utils.h).

**Funciones**:

#### `void Console_printTimestamp(void)`

- Obtiene la hora local del sistema via `GetLocalTime()`
- Imprime en formato `[HH:MM:SS] ` usando `printf`
- No cambia el color de la consola (usa el color por defecto)

#### `void Console_printRecognized(const WCHAR *text)`

- **Proposito**: Imprime texto reconocido con timestamp y color.
- **Parametros**: `text` - Cadena Unicode UTF-16 terminada en nulo.
- **Proceso**:
  1. Obtiene el handle de la consola (`GetStdHandle(STD_OUTPUT_HANDLE)`)
  2. Guarda los atributos actuales de la consola
  3. Establece color cian para el timestamp
  4. Llama a `Console_printTimestamp()`
  5. Establece color blanco brillante para el texto
  6. Detecta si la salida es una consola real o esta redirigida:
     - Si es consola: usa `WriteConsoleW()` para escritura Unicode directa
     - Si es pipe/archivo: usa `fwprintf(stdout, ...)` como fallback
  7. Restaura los atributos originales
- **Manejo de Unicode**: Usa WriteConsoleW para soporte completo de caracteres Unicode (acentos, enes, signos de puntuacion espanoles). Cuando la salida esta redirigida, usa fwprintf que tambien maneja Unicode.
- **Consideracion**: La deteccion de consola se hace con `GetConsoleMode()`, que retorna 0 si el handle no es una consola.

#### `void Console_printInfo(const char *message)`

- Imprime mensaje informativo en color verde con timestamp.
- Formato: `[HH:MM:SS] <mensaje>`

#### `void Console_printError(const char *message)`

- Imprime mensaje de error en color rojo con timestamp.
- Formato: `[HH:MM:SS] ERROR: <mensaje>`
- Nota: Toda la salida va a stdout (ver correccion I-003).

#### `void Console_printWarning(const char *message)`

- Imprime advertencia en color amarillo con timestamp.
- Formato: `[HH:MM:SS] ADVERTENCIA: <mensaje>`

#### `void Console_printHResult(HRESULT hr, const char *prefix)`

- Convierte un codigo HRESULT en un mensaje de error legible usando `FormatMessageW()`.
- Imprime el prefijo en rojo, seguido del codigo HRESULT en hex y el mensaje de error del sistema.
- Si FormatMessageW falla (por ejemplo, para codigos de error personalizados), solo imprime el codigo hex.

### resource.h

**Archivo**: `src/resource.h` (49 lineas)

**Proposito**: Contiene constantes compartidas entre los modulos del programa.

**Definiciones**:

| Simbolo | Valor | Descripcion |
|---------|-------|-------------|
| `WM_RECO_EVENT` | `WM_USER + 100` | Mensaje personalizado de Windows para eventos SAPI |
| `ERR_RECO_NO_DEVICE` | `0xE001` | No hay dispositivo de entrada |
| `ERR_RECO_NO_LANGUAGE` | `0xE002` | Idioma no disponible |
| `ERR_RECO_INIT_FAIL` | `0xE003` | Fallo de inicializacion SAPI |
| `ERR_COM_INIT_FAIL` | `0xE004` | Fallo de inicializacion COM |
| `LANG_ES_ES` | `0x0C0A` | LCID espanol (Espana) |
| `LANG_ES_MX` | `0x080A` | LCID espanol (Mexico) |
| `LANG_ES_US` | `0x580A` | LCID espanol (EE.UU.) |
| `SAPI_LANG_ATTR_ES` | `"Language=0C0A"` | Atributo para buscar reconocedores de espanol |

## APIs utilizadas

### API de Windows (Win32)

| Funcion | Proposito | Archivo |
|---------|-----------|---------|
| `SetConsoleTitleW` | Cambia el titulo de la ventana de consola | main.c:115 |
| `SetConsoleCtrlHandler` | Registra manejador de Ctrl+C | main.c:120 |
| `GetModuleHandleW` | Obtiene el HINSTANCE del modulo actual | main.c:141 |
| `ZeroMemory` | Rellena una estructura con ceros | main.c:143 |
| `RegisterClassEx` | Registra una clase de ventana | main.c:149 |
| `CreateWindowExW` | Crea la ventana oculta | main.c:159 |
| `DestroyWindow` | Destruye la ventana | main.c:188,204,232,281 |
| `PostMessage` | Publica un mensaje en la cola de mensajes | main.c:53 |
| `PostQuitMessage` | Publica WM_QUIT en la cola | main.c:89 |
| `GetMessage` | Obtiene un mensaje de la cola (bloqueante) | main.c:250 |
| `TranslateMessage` | Traduce mensajes de teclado virtual | main.c:261 |
| `DispatchMessage` | Despacha el mensaje a WndProc | main.c:262 |
| `GetLocalTime` | Obtiene la hora local del sistema | console_utils.c:49 |
| `GetStdHandle` | Obtiene handle de la consola | console_utils.c:59 |
| `GetConsoleScreenBufferInfo` | Obtiene atributos actuales de la consola | console_utils.c:29 |
| `SetConsoleTextAttribute` | Establece color de texto/fondo | console_utils.c:39 |
| `WriteConsoleW` | Escribe texto Unicode en la consola | console_utils.c:76 |
| `GetConsoleMode` | Detecta si el handle es una consola | console_utils.c:74 |
| `FormatMessageW` | Convierte HRESULT en mensaje legible | console_utils.c:146 |
| `LocalFree` | Libera memoria asignada por FormatMessageW | console_utils.c:161 |

### COM (Component Object Model)

| Funcion / Interfaz | Proposito | Archivo |
|--------------------|-----------|---------|
| `CoInitializeEx` | Inicializa COM para el hilo actual | main.c:130 |
| `CoUninitialize` | Finaliza COM | main.c:284 |
| `CoCreateInstance` | Crea una instancia de un objeto COM | recognizer.c:51,157 |
| `CoTaskMemFree` | Libera memoria asignada por SAPI (GetText) | recognizer.c:334 |
| `ISpRecognizer` | Interfaz principal del motor de reconocimiento | recognizer.h:30 |
| `ISpRecoContext` | Contexto de reconocimiento y eventos | recognizer.h:31 |
| `ISpRecoGrammar` | Gramatica de dictado | recognizer.h:32 |
| `ISpObjectTokenCategory` | Catalogo de reconocedores disponibles | recognizer.c:155 |
| `IEnumSpObjectTokens` | Enumerador de tokens del catalogo | recognizer.c:165 |
| `ISpObjectToken` | Token de un reconocedor especifico | recognizer.c:171 |
| `ISpRecoResult` | Resultado de reconocimiento (texto) | recognizer.c:316 |

### SAPI (Speech API) - Constantes

| Constante | Valor | Proposito |
|-----------|-------|-----------|
| `CLSID_SpRecognizer` | - | CLSID del motor de reconocimiento SAPI |
| `IID_ISpRecognizer` | - | IID de la interfaz ISpRecognizer |
| `SPEI_RECOGNITION` | 38 | Evento de frase reconocida |
| `SPFEI(x)` | (1ULL << x) | Macro para construir mascara de bits de eventos |
| `SPCAT_RECOGNIZERS` | - | Categoria de catalogo para reconocedores |
| `SPLO_STATIC` | - | Carga de gramatica estatica (todo en memoria) |
| `SPRS_ACTIVE` | - | Estado activo de la gramatica (escuchando) |
| `SPRS_INACTIVE` | - | Estado inactivo de la gramatica |
| `SP_GETWHOLEPHRASE` | -1 | Indice para obtener la frase completa (GetText) |

## Flujo de eventos

### Secuencia de inicializacion de COM y SAPI

```
main()
  |
  +--> CoInitializeEx(NULL, COINIT_MULTITHREADED)
  |       Inicializa COM. COINIT_MULTITHREADED = el hilo puede
  |       recibir llamadas COM desde cualquier hilo.
  |
  +--> RegisterClassEx(...)
  |       Registra "SpeechToTextAuxWindowClass" con lpfnWndProc = WndProc
  |
  +--> CreateWindowEx(WS_EX_TOOLWINDOW, ..., "SpeechToTextAuxWindowClass", ...)
  |       Crea la ventana oculta. g_hwnd = resultado.
  |
  +--> Recognizer_init(ctx, g_hwnd)
  |     |
  |     +--> CoCreateInstance(CLSID_SpRecognizer, ..., IID_ISpRecognizer, &ctx->recognizer)
  |     |       Crea el motor de reconocimiento SAPI
  |     |
  |     +--> ctx->recognizer->CreateRecoContext(&ctx->recoContext)
  |     |       Crea el contexto de reconocimiento
  |     |
  |     +--> ctx->recoContext->SetNotifyWindowMessage(g_hwnd, WM_RECO_EVENT, 0, 0)
  |     |       Configura la ventana que recibira los eventos SAPI
  |     |
  |     +--> ctx->recoContext->SetInterest(SPFEI(SPEI_RECOGNITION), ...)
  |     |       Nos interesan solo los eventos de reconocimiento
  |     |
  |     +--> ctx->recoContext->CreateGrammar(0, &ctx->recoGrammar)
  |             Crea una gramatica (aun no cargada)
  |
  +--> Recognizer_setAudioInput(ctx)
  |     +--> ctx->recognizer->SetInput(NULL, TRUE)
  |             NULL = dispositivo predeterminado del sistema
  |
  +--> Recognizer_setLanguage(ctx)
  |     +--> ISpObjectTokenCategory::SetId(SPCAT_RECOGNIZERS)
  |     +--> EnumTokens("Language=0C0A", ...) --> buscar espanol
  |     +--> Si encuentra: SetRecognizer(token)
  |     +--> Si no: SetRecognizer(NULL) + advertencia
  |
  +--> Recognizer_start(ctx)
        +--> ctx->recoGrammar->LoadDictation(NULL, SPLO_STATIC)
        |       Carga la gramatica de dictado en memoria
        +--> ctx->recoGrammar->SetDictationState(SPRS_ACTIVE)
                Activa la escucha
```

### Secuencia de un evento de reconocimiento

```
[Fuente de audio] --> habla
  |
  v
[SAPI Engine (hilo interno)]
  |   Procesa el audio, lo compara con modelos acusticos
  |   y de lenguaje del espanol
  |   Cuando identifica una frase completa (pausa >= silencio):
  |
  +--> Crea un objeto ISpRecoResult con el texto
  +--> Dispara evento SPEI_RECOGNITION
  +--> PostMessage(hwnd, WM_RECO_EVENT, 0, 0) --> cola de mensajes
  |
  v
[Cola de mensajes del programa]
  |   GetMessage() (bloqueante, retorna cuando hay mensaje)
  |
  +--> TranslateMessage()
  +--> DispatchMessage() --> WndProc()
        |
        +--> WM_RECO_EVENT --> Recognizer_handleEvent(ctx)
              |
              +--> GetEvents(1, &spEvent, &ulFetched)
              |     Obtiene el evento SPEVENT de la cola interna de SAPI
              |
              +--> spEvent.eEventId == SPEI_RECOGNITION
              |     Identifica que es un evento de frase reconocida
              |
              +--> ISpRecoResult *pResult = (ISpRecoResult *)spEvent.lParam
              |     El resultado se transfiere como lParam del evento
              |
              +--> pResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE,
              |                      TRUE, &pszText, NULL)
              |     Obtiene el texto completo de la frase reconocida
              |     pszText se asigna con CoTaskMemAlloc (debe liberarse)
              |
              +--> Console_printRecognized(pszText)
              |     Muestra el texto en consola con timestamp
              |
              +--> CoTaskMemFree(pszText)
              |     Libera el buffer de texto
              |
              +--> pResult->Release()
                    Libera la referencia al resultado (transferida a nosotros)
```

### Secuencia de finalizacion (Ctrl+C)

```
[Usuario presiona Ctrl+C]
  |
  v
[CtrlHandler() - en hilo de senales]
  |   g_running = 0
  |   PostMessage(g_hwnd, WM_CLOSE, 0, 0)
  |   return TRUE (evento manejado)
  |
  v
[Cola de mensajes: WM_CLOSE]
  |
  +--> GetMessage() lo obtiene
  +--> DispatchMessage() --> WndProc()
        |
        +--> WM_CLOSE
        |     Console_printInfo("Deteniendo reconocimiento...")
        |     DestroyWindow(hwnd)
        |       |
        |       +--> Envia WM_DESTROY
        |             |
        |             +--> WndProc(): WM_DESTROY
        |                   g_hwnd = NULL
        |                   PostQuitMessage(0) --> publica WM_QUIT
        |
        v
  [Cola de mensajes: WM_QUIT]
  |   GetMessage() retorna 0
  |   Salir del bucle
  |
  v
[main(): limpieza]
  Recognizer_stop()    --> SetDictationState(SPRS_INACTIVE)
  Recognizer_cleanup() --> SAFE_RELEASE(recoGrammar, recoContext, recognizer)
  DestroyWindow(g_hwnd) --> solo si g_hwnd != NULL (falso si vino de Ctrl+C)
  CoUninitialize()
  printf("Programa terminado correctamente.")
  return 0
```

## Como extender o modificar el programa

### Agregar soporte para varios idiomas

El sistema actualmente busca solo espanol (Espana) con LCID 0x0C0A. Para agregar soporte para otro idioma:

**Metodo 1**: Modificar la constante en `resource.h`

```c
// En resource.h, cambiar o agregar:
#define LANG_FR_FR  0x040C   // Frances (Francia)
#define SAPI_LANG_ATTR_FR  L"Language=040C"
```

**Metodo 2**: Hacer configurable el idioma (mas elaborado)

En `recognizer.c::Recognizer_setLanguage()`, aceptar un parametro con el LCID deseado, o leerlo de una variable de entorno / archivo de configuracion.

### Agregar escritura a archivo

Para agregar la funcionalidad de guardar la transcripcion a un archivo:

1. Anadir una funcion en `console_utils.c` o un nuevo modulo `file_writer.c`
2. Modificar `Console_printRecognized()` para que ademas de imprimir en consola, escriba en un archivo
3. Alternativa: anadir un flag `-o archivo.txt` en main.c

Ejemplo de extension minima:

```c
// En console_utils.h
void Console_setOutputFile(const char *filename);
void Console_closeOutputFile(void);

// En console_utils.c
static FILE *g_output_file = NULL;

void Console_setOutputFile(const char *filename) {
    g_output_file = fopen(filename, "w, ccs=UTF-8");
}

void Console_closeOutputFile(void) {
    if (g_output_file) {
        fclose(g_output_file);
        g_output_file = NULL;
    }
}

// Modificar Console_printRecognized para escribir tambien al archivo
void Console_printRecognized(const WCHAR *text) {
    // ... codigo existente ...
    
    if (g_output_file) {
        fwprintf(g_output_file, L"%s\n", text);
        fflush(g_output_file);
    }
}
```

### Agregar seleccion de dispositivo de entrada

Actualmente, `Recognizer_setAudioInput()` usa el dispositivo predeterminado (`NULL`). Para permitir seleccionar un dispositivo especifico:

1. Enumerar dispositivos de audio con `IMMDeviceEnumerator` (WASAPI)
2. Mostrar lista al usuario
3. Crear un `ISpAudio` desde el dispositivo seleccionado
4. Pasarlo a `SetInput()` en lugar de NULL

### Mejorar el reconocimiento con gramaticas personalizadas

En lugar de usar dictado libre (`LoadDictation`), se pueden crear gramaticas basadas en reglas (CFG) con `LoadCmdFromFile()` o `LoadCmdFromMemory()`. Esto mejora la precision pero limita el vocabulario.

### Agregar modo de retroalimentacion visual

Se puede agregar una pantalla de estado en la consola que muestre:
- Nivel de audio de entrada (medidores VU)
- Estado del reconocimiento (escuchando / procesando)
- Frase parcial antes de ser completada (usando evento SPEI_PHRASE_START y SPEI_HYPOTHESIS)

### Modificar los colores de la consola

En `console_utils.c`, las constantes de color estan definidas al inicio del archivo:

```c
#define COL_CYAN        (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define COL_GREEN       (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define COL_RED         (FOREGROUND_RED | FOREGROUND_INTENSITY)
#define COL_YELLOW      (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define COL_WHITE_BRIGHT (...)
```

Se pueden modificar combinando las constantes:
- `FOREGROUND_RED`, `FOREGROUND_GREEN`, `FOREGROUND_BLUE`
- `FOREGROUND_INTENSITY` (version brillante)
- `BACKGROUND_RED`, `BACKGROUND_GREEN`, `BACKGROUND_BLUE`, `BACKGROUND_INTENSITY` (fondo)

### Consideraciones al compilar con diferentes toolchains

**MinGW-w64**:
- Usa `-lole32 -lsapi -lwinmm` para enlazar las librerias COM, SAPI y Windows Multimedia
- `-mconsole` especifica aplicacion de consola (evita que aparezca una ventana de consola extra)
- `-D_WIN32_WINNT=0x0600` define la version de Windows (Windows Vista+), necesario para ciertas funciones

**MSVC**:
- Usa `ole32.lib sapi.lib` en la linea de enlace
- `/SUBSYSTEM:CONSOLE` especifica aplicacion de consola
- `/utf-8` asegura que los caracteres Unicode en el codigo fuente se interpreten correctamente

### Pruebas

Para probar el programa sin una fuente de audio real:
1. Use un archivo WAV con voz en espanol como entrada predeterminada
2. Conecte la salida de audio de un telefono movil al line-in
3. Use una herramienta virtual como "Stereo Mix" para reproducir audio desde el mismo PC

### Depuracion

El programa imprime todos los errores con su codigo HRESULT hexadecimal, lo que facilita la identificacion de problemas. Los codigos de error comunes incluyen:

- `0x80045002 (SPERR_NO_AUDIO_INPUT)`: Sin dispositivo de entrada
- `0x80040154 (REGDB_E_CLASSNOTREG)`: CLSID de SAPI no registrado (SAPI no instalado)
- `0x80004002 (E_NOINTERFACE)`: Interfaz COM no soportada

Para depurar el flujo interno, se puede anadir temporalmente llamadas a `Console_printInfo()` con mensajes de seguimiento en puntos clave (inicio de init, fin de init, evento recibido, etc.).
