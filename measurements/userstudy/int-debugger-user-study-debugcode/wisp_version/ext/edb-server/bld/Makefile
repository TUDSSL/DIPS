EXEC = edb-server

OBJECTS = \
	main.o \

DEPS += \
	libmsp \
	libedbserver \

# libedb is not a full dependency, We only need the headers
override CFLAGS += -I$(LIB_ROOT)/libedb/src/include

## Config local to this app executable
export CONFIG_ROUTE_ACLK_TO_PIN ?= 0
export CONFIG_CLOCK_TEST_MODE ?= 0

# Configure the build here (see Makefile.config, by default all flags are unset)
export CONFIG_ENABLE_WATCHPOINTS = 1
export CONFIG_TARGET_UART = 1
export CONFIG_ENABLE_DEBUG_MODE = 1
export CONFIG_PULL_DOWN_ON_COMM_LINES = 1

export CONFIG_HOST_UART = 1
export CONFIG_SYSTICK = 1
export CONFIG_ENABLE_WATCHPOINT_STREAM = 1
export CONFIG_ENABLE_VOLTAGE_STREAM = 1
export CONFIG_ENABLE_TARGET_SIDE_DEBUG_MODE = 1
export CONFIG_POWER_TARGET_IN_DEBUG_MODE = 1
export CONFIG_FETCH_INTERRUPT_CONTEXT = 1
export CONFIG_BOOT_LED = 1
export CONFIG_MAIN_LOOP_LED = 1
export CONFIG_DEBUG_MODE_LED = 1
export CONFIG_ERROR_LED = 1
export CONFIG_CHARGE_MANIP = 1
export CONFIG_TARGET_UART_PUSH = 1

export CONFIG_DEV_CONSOLE = 1

export LIBMSP_XT1_FREQ = 32678ull
export LIBMSP_XT1_CAP = 12
export LIBMSP_XT2_FREQ = 12000000ull
export LIBMSP_CLOCK_SOURCE = DCO
export LIBMSP_DCO_REF_SOURCE = XT2
export LIBMSP_DCO_REF_CLOCK_DIV = 4
export LIBMSP_DCO_FREQ = 24000000ull

export LIBMSP_STARTUP_VOLTAGE_WORKAROUNDS = disable-pmm

export LIBMSPSOFTUART_BAUDRATE = 115200
export LIBMSPSOFTUART_CLOCK_FREQ = $(LIBMSP_DCO_FREQ)

ifeq ($(BOARD),edb)
export LIBMSPSOFTUART_PORT_TX = 1.1
export LIBMSPSOFTUART_TIMER = A.0.0
else ifeq ($(BOARD),edb-1.1)
export LIBMSPSOFTUART_PORT_TX = 1.7
export LIBMSPSOFTUART_TIMER = A.1.0
else
$(error Uknown BOARD)
endif # BOARD

include ../Makefile.config

# To build the dependee that uses the headers, we need the config macros here
include $(LIB_ROOT)/libmsp/bld/Makefile.config

export CONFIG_PRINTF_LIB = libmspsoftuart
include $(MAKER_ROOT)/Makefile.console
