# Project Name
TARGET = SimpleSampler

# Sources
CPP_SOURCES = SimpleSampler.cpp

# Library Locations
LIBDAISY_DIR = ../../libDaisy/
DAISYSP_DIR = ../../DaisySP/


# Includes FatFS source files within project.
USE_FATFS = 1


# SDRAM Configuration
USE_SDRAM = 1

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
