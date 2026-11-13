@echo off
REM ============================================================================
REM  build.bat - Script de compilación para SpeechToTextAux (Windows)
REM  Reconocimiento de voz en español vía SAPI
REM ============================================================================
REM
REM  Uso:
REM    build          -> Compila con MinGW-w64 (si está en PATH)
REM    build msvc     -> Compila con MSVC (Visual Studio)
REM    build help     -> Muestra esta ayuda
REM
REM ============================================================================

setlocal enabledelayedexpansion

if /I "%1"=="help" goto :help
if /I "%1"=="msvc" goto :msvc
goto :mingw

:mingw
echo.
echo === Compilando con MinGW-w64 ===
echo.

where gcc >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: MinGW-w64 (gcc) no encontrado en PATH.
    echo.
    echo Instalalo desde: https://www.mingw-w64.org/
    echo O usa: build msvc  (si tienes Visual Studio)
    echo.
    pause
    exit /b 1
)

if not exist build mkdir build

echo Compilando main.c...
gcc -c -Wall -Wextra -Wpedantic -std=c99 -O2 -I. -D_WIN32_WINNT=0x0600 -o build\main.o src\main.c
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo Compilando recognizer.c...
gcc -c -Wall -Wextra -Wpedantic -std=c99 -O2 -I. -D_WIN32_WINNT=0x0600 -o build\recognizer.o src\recognizer.c
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo Compilando console_utils.c...
gcc -c -Wall -Wextra -Wpedantic -std=c99 -O2 -I. -D_WIN32_WINNT=0x0600 -o build\console_utils.o src\console_utils.c
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo Enlazando SpeechToTextAux.exe...
gcc -o SpeechToTextAux.exe build\main.o build\recognizer.o build\console_utils.o -mconsole -lole32 -lsapi -lwinmm
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo.
echo *** Compilacion exitosa: SpeechToTextAux.exe ***
echo.
goto :end

:msvc
echo.
echo === Compilando con MSVC (Visual Studio) ===
echo.
echo Asegurate de haber ejecutado vcvarsall.bat primero:
echo   "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
echo.

if not exist build mkdir build

echo Compilando main.c...
cl /nologo /W4 /WX /O2 /I. /utf-8 /c /Fobuild\main.obj src\main.c
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo Compilando recognizer.c...
cl /nologo /W4 /WX /O2 /I. /utf-8 /c /Fobuild\recognizer.obj src\recognizer.c
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo Compilando console_utils.c...
cl /nologo /W4 /WX /O2 /I. /utf-8 /c /Fobuild\console_utils.obj src\console_utils.c
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo Enlazando SpeechToTextAux.exe...
cl /nologo /FeSpeechToTextAux.exe build\main.obj build\recognizer.obj build\console_utils.obj /link ole32.lib sapi.lib /SUBSYSTEM:CONSOLE
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo.
echo *** Compilacion exitosa: SpeechToTextAux.exe ***
echo.
goto :end

:help
echo.
echo SpeechToTextAux - Script de compilacion
echo.
echo USO:
echo   build            Compila con MinGW-w64
echo   build msvc       Compila con MSVC (Visual Studio)
echo   build help       Muestra esta ayuda
echo.
exit /b 0

:end
pause
