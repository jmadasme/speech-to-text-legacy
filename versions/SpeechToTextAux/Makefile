# ============================================================================
#  Makefile para SpeechToTextAux
#  Reconocimiento de voz en español vía SAPI (Windows 10)
# ============================================================================
#
#  Uso:
#    nmake /f Makefile TOOLCHAIN=MSVC          (MSVC - Microsoft Visual C++)
#    mingw32-make TOOLCHAIN=MINGW              (MinGW-w64)
#    make   TOOLCHAIN=MINGW                    (MinGW-w64 en Linux cross)
#
#  Por defecto: MINGW
#
# ============================================================================

# ---------------------------------------------------------------------------
#  Configuración del toolchain
# ---------------------------------------------------------------------------

TOOLCHAIN ?= MINGW

# Directorios
SRC_DIR  = src
BUILD_DIR = build

# Archivos fuente
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/recognizer.c \
       $(SRC_DIR)/console_utils.c

# ---------------------------------------------------------------------------
#  Reglas según toolchain
# ---------------------------------------------------------------------------

ifeq ($(TOOLCHAIN), MSVC)
# ====================  Microsoft Visual C++  ===============================

CC       = cl
CFLAGS   = /nologo /W4 /WX /O2 /I. /utf-8
LDFLAGS  = /link ole32.lib sapi.lib /SUBSYSTEM:CONSOLE
OBJ_EXT  = obj
TARGET   = SpeechToTextAux.exe
RM       = del /Q
RMDIR    = rmdir /Q /S
MKDIR    = mkdir

# Compilar un fuente .c -> .obj
$(BUILD_DIR)/%.$(OBJ_EXT): $(SRC_DIR)/%.c
	@echo Compilando $<...
	@if not exist $(BUILD_DIR) $(MKDIR) $(BUILD_DIR)
	$(CC) /c $(CFLAGS) /Fo$@ $<

# Enlazar
$(TARGET): $(addprefix $(BUILD_DIR)/, main.$(OBJ_EXT) recognizer.$(OBJ_EXT) console_utils.$(OBJ_EXT))
	@echo Enlazando $(TARGET)...
	$(CC) $(CFLAGS) $** $(LDFLAGS) /Fe$@
	@echo.
	@echo *** Compilación exitosa: $(TARGET) ***

else
# ====================  MinGW-w64 (GCC)  ====================================

CC       = gcc
CFLAGS   = -Wall -Wextra -Wpedantic -std=c99 -O2 -I. \
           -D_WIN32_WINNT=0x0600
LDFLAGS  = -mconsole -lole32 -lsapi -lwinmm
OBJ_EXT  = o
TARGET   = SpeechToTextAux.exe
RM       = rm -f
RMDIR    = rm -rf
MKDIR    = mkdir -p

# Compilar un fuente .c -> .o
$(BUILD_DIR)/%.$(OBJ_EXT): $(SRC_DIR)/%.c
	@echo Compilando $<...
	$(MKDIR) $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -o $@ $<

# Enlazar
$(TARGET): $(addprefix $(BUILD_DIR)/, main.$(OBJ_EXT) recognizer.$(OBJ_EXT) console_utils.$(OBJ_EXT))
	@echo Enlazando $(TARGET)...
	$(CC) -o $@ $^ $(LDFLAGS)
	@echo.
	@echo "*** Compilación exitosa: $(TARGET) ***"

endif

# ---------------------------------------------------------------------------
#  Objetivos comunes
# ---------------------------------------------------------------------------

.PHONY: all clean rebuild

all: $(TARGET)

clean:
	-$(RMDIR) $(BUILD_DIR)
	-$(RM) $(TARGET) 2>nul || true

rebuild: clean all

# ---------------------------------------------------------------------------
#  Ayuda
# ---------------------------------------------------------------------------

help:
	@echo "SpeechToTextAux - Makefile"
	@echo.
	@echo "Uso:"
	@echo "  make TOOLCHAIN=MINGW           (MinGW-w64, por defecto)"
	@echo "  nmake /f Makefile TOOLCHAIN=MSVC  (MSVC)"
	@echo.
	@echo "Objetivos:"
	@echo "  all       - Compilar el programa (por defecto)"
	@echo "  clean     - Eliminar archivos generados"
	@echo "  rebuild   - Limpiar y recompilar"
	@echo "  help      - Mostrar esta ayuda"
