# SpeechToTextAux - Datasheet

## Especificacion tecnica completa

### Identificacion del producto

| Atributo | Valor |
|----------|-------|
| **Nombre** | SpeechToTextAux |
| **Version** | 1.0.0 |
| **Proposito** | Reconocimiento de voz a texto en espanol en tiempo real desde entrada auxiliar |
| **Plataforma objetivo** | Windows 10 (x86 / x64) |
| **Lenguaje de programacion** | C (ISO C99) |
| **API de reconocimiento** | SAPI 5.4+ (Microsoft Speech API) |
| **Modelo COM** | Component Object Model (COM) |
| **Toolchains soportados** | MinGW-w64 (GCC), MSVC (Visual Studio) |
| **Licencia** | Parte de AG.CreadorContenido |

### Parametros del sistema

| Parametro | Valor |
|-----------|-------|
| **Idioma de reconocimiento** | Espanol (Espana), LCID 0x0C0A |
| **Idiomas alternativos** | Espanol (Mexico) 0x080A, Espanol (EE.UU.) 0x580A |
| **Entrada de audio** | Dispositivo predeterminado del sistema (SetInput(NULL, TRUE)) |
| **Formato de audio del dispositivo** | 16 bits, 44100 Hz, stereo (calidad CD, estandar de SAPI) |
| **Conversion de formato** | Automatica (SAPI la maneja internamente) |
| **Tipo de gramatica** | Dictation (reconocimiento de voz continua libre) |
| **Carga de gramatica** | Estatica (SPLO_STATIC) - todo el lexico disponible inmediatamente |
| **Paradigma de eventos** | Asincrono, mediante mensajes de Windows (WM_USER + 100) |
| **Resolucion de timestamp** | Milisegundos (mostrado como HH:MM:SS) |

### Recursos del sistema

| Recurso | Consumo estimado |
|---------|-----------------|
| **RAM** | < 10 MB en estado activo |
| **Disco (ejecutable)** | ~100 KB (depende del toolchain) |
| **Disco (codigo fuente)** | ~15 KB |
| **CPU** | ~0% en espera (bucle bloqueado en GetMessage); ~2-5% durante reconocimiento |
| **Hilos** | 1 (hilo principal + hilos internos de SAPI que gestiona el propio motor) |

### Codigos de error

| Simbolo | Codigo | Descripcion | Origen |
|---------|--------|-------------|--------|
| `ERR_RECO_NO_DEVICE` | 0xE001 | No se encontro dispositivo de entrada de audio | main.c:206 |
| `ERR_RECO_NO_LANGUAGE` | 0xE002 | Idioma espanol no disponible | resource.h (definido, no usado) |
| `ERR_RECO_INIT_FAIL` | 0xE003 | Fallo al inicializar el motor de reconocimiento SAPI | main.c:191 |
| `ERR_COM_INIT_FAIL` | 0xE004 | Fallo al inicializar COM | main.c:135 |

### Paquete de instalacion

| Archivo | Descripcion |
|---------|-------------|
| `SpeechToTextAux.exe` | Ejecutable compilado |
| `src/main.c` | Punto de entrada, ventana oculta, bucle de mensajes |
| `src/recognizer.c` | Implementacion del motor de reconocimiento SAPI |
| `src/recognizer.h` | Cabecera del modulo de reconocimiento |
| `src/console_utils.c` | Utilidades de consola con color y formato |
| `src/console_utils.h` | Cabecera de utilidades de consola |
| `src/resource.h` | Constantes compartidas (mensajes, errores, LCIDs) |
| `Makefile` | Script de compilacion para MSVC y MinGW |
| `docs/datasheet.md` | Esta especificacion |
| `docs/desarrollador.md` | Documentacion para desarrolladores |
| `README.md` | Documentacion principal |
| `PLAN.md` | Plan de implementacion original |
| `AUDIT_REPORT.md` | Reporte de auditoria de codigo |

## Comportamiento esperado

### Flujo de inicio

```
1. El usuario ejecuta SpeechToTextAux.exe
2. La consola cambia su titulo a "SpeechToTextAux - Reconocimiento de Voz en Espanol"
3. Se muestra "[HH:MM:SS] Iniciando reconocimiento de voz en espanol..."
4. Se inicializan COM, la ventana oculta y el motor SAPI
5. Se configura el idioma espanol (o se muestra advertencia si no esta disponible)
6. Se configura la entrada de audio predeterminada
7. Se inicia el reconocimiento
8. Se muestra "[HH:MM:SS] Reconocimiento activo. Habla por el dispositivo de entrada..."
9. El programa queda a la espera de frases
```

### Flujo de reconocimiento

```
1. El usuario habla por la fuente de audio conectada
2. SAPI procesa el audio internamente (pueden pasar 0.5-2 segundos)
3. Cuando SAPI identifica una frase completa, genera un evento SPEI_RECOGNITION
4. SAPI postea un mensaje WM_RECO_EVENT a la ventana oculta del programa
5. El bucle de mensajes recoje el mensaje y lo despacha a WndProc
6. WndProc llama a Recognizer_handleEvent()
7. Se extrae el texto de ISpRecoResult mediante GetText()
8. El texto se muestra en consola: "[HH:MM:SS] <texto reconocido>"
9. Se libera la memoria del resultado y se vuelve al paso 2
```

### Flujo de detencion

```
1. El usuario presiona Ctrl+C
2. El manejador CtrlHandler establece g_running = 0 y postea WM_CLOSE
3. WndProc recibe WM_CLOSE, muestra "[HH:MM:SS] Deteniendo reconocimiento..."
4. WndProc llama a DestroyWindow(), que dispara WM_DESTROY
5. WM_DESTROY establece g_hwnd = NULL y postea WM_QUIT
6. GetMessage() retorna 0, el bucle de mensajes termina
7. Se llama a Recognizer_stop() -> SetDictationState(SPRS_INACTIVE)
8. Se llama a Recognizer_cleanup() -> Release() de todos los objetos COM
9. Se destruye la ventana si aun existe (g_hwnd != NULL)
10. Se llama a CoUninitialize()
11. Se muestra "[HH:MM:SS] Programa terminado correctamente."
12. main() retorna 0
```

### Formatos de salida en consola

**Mensajes informativos**:
```
[14:35:12] Iniciando reconocimiento de voz en espanol...
```
Color: Verde (FOREGROUND_GREEN | FOREGROUND_INTENSITY)

**Mensajes de error**:
```
[14:35:12] ERROR: No se encontro dispositivo de entrada de audio.
  HRESULT: 0x80045002 - The audio input device was not found.
```
Color: Rojo (FOREGROUND_RED | FOREGROUND_INTENSITY)

**Advertencias**:
```
[14:35:12] ADVERTENCIA: No se encontro reconocedor SAPI para espanol (0x0C0A).
```
Color: Amarillo (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)

**Texto reconocido**:
```
[14:35:22] Hola mundo, esto es una prueba de reconocimiento de voz
```
Timestamp: Cian (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
Texto: Blanco brillante (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)

## Casos de uso

### Caso 1: Transcripcion de contenido audiovisual

Conecte la salida de audio de un televisor, radio o reproductor a la entrada line-in del PC para transcribir automaticamente noticias, documentales, podcasts o cualquier contenido hablado en espanol.

**Ventaja**: La transcripcion se genera en tiempo real, sin necesidad de servicios en la nube ni conexion a Internet.

### Caso 2: Asistencia para personas con discapacidad auditiva

Un microfono ambiental conectado al line-in captura conversaciones y las transcribe a texto en pantalla, facilitando la comprension a personas con problemas de audicion en reuniones, conferencias o clases.

### Caso 3: Monitoreo de comunicaciones por radio

Conecte la salida de audio de un equipo de radio (handy, scanner, base) a la entrada auxiliar para transcribir comunicaciones de radioaficionados, servicios de emergencia o equipos de seguridad.

### Caso 4: Captura de audio desde aplicaciones

Si la placa de sonido soporta "Stereo Mix" o "What U Hear", puede seleccionarlo como dispositivo de entrada predeterminado para transcribir el audio de cualquier aplicacion (videollamadas, reproductores multimedia, navegador web).

### Caso 5: Toma de notas automatica

Un microfono USB o de solapa conectado al PC transcribe automaticamente reuniones, entrevistas o sesiones de lluvia de ideas, generando un registro textual de lo hablado.

## Solucion de problemas comunes

### Problema: "El programa se cierra inmediatamente al iniciar"

**Causas posibles**:
- No hay dispositivo de entrada de audio conectado o habilitado
- El paquete de voz en espanol no esta instalado correctamente
- SAPI no esta disponible en el sistema

**Soluciones**:
1. Verifique que haya un dispositivo de entrada en Sonido > Grabacion
2. Asegurese de que el paquete de voz en espanol este instalado (Configuracion > Hora e idioma > Voz)
3. Ejecute `sfc /scannow` para verificar archivos del sistema

### Problema: "No se reconoce nada aunque el dispositivo de entrada funciona"

**Causas posibles**:
- El volumen de entrada es demasiado bajo
- Hay ruido de fondo que interfiere con el reconocimiento
- El microfono esta silenciado en Windows

**Soluciones**:
1. Verifique el nivel de entrada en Sonido > Grabacion > Propiedades > Niveles (debe estar al menos al 70%)
2. Reduzca el ruido ambiente o aleje la fuente de ruido del microfono
3. Verifique que el microfono no este silenciado (el icono no debe tener una X roja)

### Problema: "El reconocimiento produce texto sin sentido"

**Causas posibles**:
- El audio esta saturado (volumen demasiado alto)
- La fuente de audio no habla claramente
- El idioma configurado no coincide con el idioma hablado
- Hay eco o reverberacion

**Soluciones**:
1. Reduzca el volumen de entrada al 60-80%
2. Acerque la fuente de audio al microfono y evite que hable desde lejos
3. Verifique que este usando Espanol (si la fuente habla en otro idioma, el reconocimiento fallara)
4. Use un microfono direccional o reduzca la ganancia

### Problema: "El programa no compila"

**Causas posibles**:
- Falta sapi.h o las librerias de SAPI en el toolchain
- El toolchain no soporta Windows (compilacion cruzada desde Linux)
- Errores de enlace con ole32.lib o sapi.lib

**Soluciones**:
- **MinGW-w64**: Asegurese de tener instalado `mingw-w64-x86_64-sdk` o una version reciente que incluya los headers de SAPI
- **MSVC**: Use el Developer Command Prompt y verifique que tenga instalado el Windows SDK
- **Compilacion cruzada**: No es posible compilar desde Linux para Windows facilmente debido a la dependencia de SAPI. Use MinGW-w64 en Windows.

### Problema: "La salida aparece en blanco o sin color"

**Causas posibles**:
- La terminal no soporta las API de consola de Windows (por ejemplo, Cygwin sin la configuracion adecuada)
- La salida esta redirigida a un archivo (en ese caso, se usa fwprintf sin color)

**Soluciones**:
- Use CMD, PowerShell o Windows Terminal nativos de Windows
- Si redirige la salida a un archivo, el texto se guardara sin formato (solo texto plano)

### Problema: "El texto reconocido aparece cortado o incompleto"

**Causas posibles**:
- El hablante hace pausas muy largas (SAPI considera la pausa como fin de frase)
- El audio tiene interrupciones o perdidas de paquetes

**Soluciones**:
- Es normal que SAPI divida el audio en frases segun las pausas naturales del habla
- Para unir frases, puede redirigir la salida a un archivo y procesarlo posteriormente

### Problema: "Error 0x80045002 (SPERR_NO_AUDIO_INPUT)"

**Causa**: Windows no tiene ningun dispositivo de entrada de audio configurado como predeterminado, o el dispositivo esta deshabilitado en el Administrador de dispositivos.

**Solucion**:
1. Vaya a Configuracion > Sistema > Sonido > Entrada
2. Seleccione un dispositivo de entrada
3. Si no aparece ninguno, vaya a Administrador de dispositivos > Entradas y salidas de audio y verifique que los controladores esten habilitados
4. Si el dispositivo aparece con un icono de flecha hacia abajo, haga clic derecho y seleccione "Habilitar dispositivo"
