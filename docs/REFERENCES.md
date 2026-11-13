# Referencias · Speech to Text Legacy

Documentación externa que se usó para entender el hardware, los protocolos y las
libertades del proyecto. No es una bibliografía decorativa: cada enlace apunta a
la fuente que explica una decisión concreta.

## Fuentes técnicas

| Fuente | Para qué se consultó |
|---|---|
| [Windows SAPI 5](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ms723627(v=vs.85)) | Speech API y síntesis/reconocimiento. |
| [Win32 API](https://learn.microsoft.com/en-us/windows/win32/api/) | Ventanas, mensajes y consola. |

## Versiones de este repositorio

`SpeechToTextAux`

Cada carpeta puede tener dependencias y versiones distintas. Cuando el código
depende de una versión específica, esa versión está declarada en el README o en
el archivo de configuración de la carpeta, no en este documento.

## Criterio de preservación

- El código y los documentos se copiaron desde la carpeta original de trabajo.
- Se excluyeron `.git`, dependencias instaladas, compilados, registros y bases
  de datos locales.
- Se redactaron tokens, claves privadas y contraseñas detectadas en texto.
- Las plantillas `.example`, `.sample` y los valores de demostración se
  conservan porque documentan la configuración esperada.

## Limitaciones conocidas

Transcribir una conversación en un equipo Windows antiguo no siempre permite instalar un modelo grande ni enviar audio a un servicio externo.

La documentación de cada versión describe el alcance real. Si una función no
está implementada, se dice; si un hardware no fue probado, también.
