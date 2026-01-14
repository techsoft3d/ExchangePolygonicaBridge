#   
#   makefile to build TranslateToPGSolids on Linux 64 bit 
#   $Revision: #1 $
#

# Default values - can be overridden
FLAV=ddd_
ARCH=x64/linux/gcc

# Directory structure

PGLIB=pgogl

# Optional HOOPS Exchange include path - set EXCHANGE_INCLUDE_PATH environment variable or override on command line
EXCHANGE_INCLUDE_PATH  = /home/peter/HOOPS/Exchange/HOOPS_Exchange_2025.9.0/include
EXCHANGE_INCLUDE_FLAGS = $(if $(wildcard $(EXCHANGE_INCLUDE_PATH)),-I$(EXCHANGE_INCLUDE_PATH))

# Polygonica paths
PG_INSTALL_PATH=/home/peter/polygonica
PG_LIB_PATH=$(PG_INSTALL_PATH)/lib/$(ARCH)/$(FLAV)
PG_BIN_PATH=$(PG_INSTALL_PATH)/bin/$(ARCH)/$(FLAV)`
PG_INCLUDE_PATH = $(PG_INSTALL_PATH)/include

# Exchange Polygonica Bridge 
EXCHANGE_POLYGONICA_BRIDGE_INCLUDE_PATH = /home/peter/ExchangePolygonicaBridge/include
EXCHANGE_POLYGONICA_BRIDGE_INCLUDE_FLAGS = $(if $(wildcard $(EXCHANGE_POLYGONICA_BRIDGE_INCLUDE_PATH)),-I$(EXCHANGE_POLYGONICA_BRIDGE_INCLUDE_PATH))

# Compiler and linker flags
WARNING_FLAGS = $(if $(NOWARN),-w,-Wall -Wno-unused-variable -Wno-unused-function)
CXXFLAGS = -std=c++14 $(if $(DEBUGGER),-ggdb) -I$(PG_INCLUDE_PATH) $(EXCHANGE_INCLUDE_FLAGS) $(EXCHANGE_POLYGONICA_BRIDGE_INCLUDE_FLAGS) $(WARNING_FLAGS)
LDFLAGS = -L$(PG_LIB_PATH) -l$(PGLIB) -lm -ldl -lpthread
EXTRA_DEPENDENCIES_pgogl = -lX11 -lXft -lfontconfig -lGL 
KEY_OPTIONS = $(if $(KEY),-DPV_LICENSE='"$(KEY)"') $(if $(KEY_FILE),-DPV_LICENSE='$(shell sed -n 's@^.define *PV_LICENSE *@@p' $(KEY_FILE))') 

# Source files - excluding Windows-specific files
SOURCES = TranslateToPgSolids.cpp \
          pgwindow/WindowFactory.cpp \
          pgwindow/X11Window.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = TranslateToPgSolids

ifeq ($(ARCH),intel/heros5b/gcc810)
    EXTRA_DEPENDENCIES_pgogl += -m32 -mmmx -msse3 -mtune=generic
endif

help:
	@echo 'This makefile builds the TranslateToPgSolids application.'
	@echo 'Usage:'
	@echo '  make                    - Build TranslateToPgSolids'
	@echo '  make clean             - Clean build artifacts'
	@echo '  make run               - Build and run TranslateToPgSolids'
	@echo '  make debug             - Build and run with debugger'
	@echo ''
	@echo 'Options:'
	@echo '  FLAV=dos_              - Use different PG library flavour'
	@echo '  DEBUGGER=cgdb          - Use debugger when running'
	@echo '  EXCHANGE_INCLUDE_PATH=/path  - Set HOOPS Exchange include path'
	@echo '  NOWARN=1               - Suppress all compiler warnings'
	@echo ''
	@echo 'Examples:'
	@echo '  make FLAV=dos_'
	@echo '  make debug DEBUGGER=cgdb'
	@echo '  make run NOWARN=1'
	@echo '  make EXCHANGE_INCLUDE_PATH=/opt/hoops/include'

# Default target
all: $(TARGET)

# Create output directory
$(BIN):
	mkdir -p $@

# Build object files
%.o: %.cpp
	g++ $(CXXFLAGS) $(KEY_OPTIONS) -c $< -o $@

# Build the main executable
$(TARGET): $(OBJECTS)
	g++ $(OBJECTS) $(LDFLAGS) $(EXTRA_DEPENDENCIES_pgogl) -o $@ $(KEY_OPTIONS)

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Run the application
run: $(TARGET)
	@echo "=============================="
	@echo "Running TranslateToPgSolids"
	@echo "=============================="
	export LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:../hasp:$(PG_LIB_PATH) ; export POLYGONICA_DIR=.. ; ./$(TARGET)

# Debug the application
debug: $(TARGET)
	@echo "=============================="
	@echo "Debugging TranslateToPgSolids"
	@echo "=============================="
	export LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:../hasp:$(PG_LIB_PATH) ; export POLYGONICA_DIR=.. ; $(DEBUGGER) ./$(TARGET)

.PHONY: all clean run debug help
