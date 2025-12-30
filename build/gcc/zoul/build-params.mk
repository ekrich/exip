# Specify compiler to be used
COMPILER = arm-none-eabi-gcc

# Specify ar tool to be used
ARCHIVER =  arm-none-eabi-ar

# Debug flags if any
DEBUGGING = #-O0 -g

# Warning flags
WARNING = -Wno-error -Wpacked -Wall -Wextra -Wpedantic -Wconversion -Wno-sign-conversion -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function

# Add additional CFLAGS if any 
ADDITIONAL_CFLAGS = -mcpu=cortex-m3 -mthumb -mlittle-endian -mabi=aapcs -ffunction-sections -fdata-sections -fno-strict-aliasing -std=c11 --specs=nano.specs --specs=nosys.specs

# add additional LDFLAGS if any
ADDITIONAL_LDFLAGS = --specs=nano.specs --specs=nosys.specs

# Whether to include the grammar generation module in the build
INCLUDE_GRAMMAR_GENERATION = false

# In case INCLUDE_GRAMMAR_GENERATION equals true; whether to
# support parsing of schema-mode EXI encoded XML Schemas
INCLUDE_SCHEMA_EXI_GRAMMAR_GENERATION = false

# CONTIKI must be set to point at the contiki-ng directory
# Default is adjacent to exip code (exip/build/gcc)
CONTIKI ?= ../../../../contiki-ng

# Alternatives could be MEMB or MMEM
MODULES += os/lib/heapmem

# Include CONTIKI setup
include $(CONTIKI)/Makefile.include
