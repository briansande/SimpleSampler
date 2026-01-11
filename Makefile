APP_TYPE=BOOT_SRAM
LDFLAGS = --specs=rdimon.specs -lrdimon

# Project Name
TARGET = SimpleSampler

# Sources
CPP_SOURCES = SimpleSampler.cpp \
              SampleLibrary.cpp \
              b3ReadWavFile.cpp \
              DisplayManager.cpp \
              Sequencer.cpp \
              Metronome.cpp \
              UIManager.cpp \
              Menus.cpp

# Library Locations
LIBDAISY_DIR = ../../libDaisy/
DAISYSP_DIR = ../../DaisySP/

# Suppress all warnings - only show errors
CFLAGS += -w
CXXFLAGS += -w


# Includes FatFS source files within project.
USE_FATFS = 1


# SDRAM Configuration
USE_SDRAM = 1

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
