PROTOCOL_DIR = protocol
BMP_DIR = $(PROTOCOL_DIR)/bmp
OPT_DEFS += -UNO_DEBUG
CFLAGS += -DMOUSE_EXTENDED_REPORT

SRC := $(filter-out $(QUANTUM_DIR)/dynamic_keymap.c, $(SRC))
SRC := $(filter-out $(QUANTUM_DIR)/encoder.c, $(SRC))
# Override get_tapping_term of VIAL
# $(INTERMEDIATE_OUTPUT)/quantum/vial.o: CFLAGS += -Dget_tapping_term=get_tapping_term_vial_default

SRC += $(BMP_DIR)/gcc_startup_nrf52840.S
SRC += $(BMP_DIR)/crc16.c
SRC += $(BMP_DIR)/bmp.c
SRC += $(BMP_DIR)/bmp_vial.c
SRC += $(BMP_DIR)/bmp_flash.c
SRC += $(BMP_DIR)/bmp_indicator_led.c
SRC += $(BMP_DIR)/bmp_file.c
SRC += $(BMP_DIR)/bmp_settings.c
SRC += $(BMP_DIR)/dynamic_keymap.c
SRC += $(BMP_DIR)/process_record_bmp.c
SRC += $(BMP_DIR)/state_controller.c
SRC += $(BMP_DIR)/host_driver.c
SRC += $(BMP_DIR)/via_qmk.c
SRC += $(BMP_DIR)/matrix/matrix.c
SRC += $(BMP_DIR)/matrix/matrix_basic.c
SRC += $(BMP_DIR)/matrix/matrix_duplex.c
SRC += $(BMP_DIR)/matrix/matrix_lpme.c
SRC += $(BMP_DIR)/matrix/matrix_74hc164.c
SRC += $(BMP_DIR)/matrix/lpme.c
SRC += $(BMP_DIR)/matrix/bmp_debounce.c
SRC += $(BMP_DIR)/cli/cli.c
SRC += $(BMP_DIR)/cli/microshell/core/microshell.c
SRC += $(BMP_DIR)/cli/microshell/core/mscore.c
SRC += $(BMP_DIR)/cli/microshell/util/mscmd.c
SRC += $(BMP_DIR)/cli/microshell/util/msopt.c
SRC += $(BMP_DIR)/cli/microshell/util/ntlibc.c
SRC += $(BMP_DIR)/cli/xmodem.c
SRC += $(BMP_DIR)/key_override/bmp_key_override.c
SRC += $(BMP_DIR)/key_override/jp_key_on_us_os_override.c
SRC += $(BMP_DIR)/key_override/us_key_on_jp_os_override.c

ifeq ($(strip $(ENCODER_ENABLE)), yes)
	SRC += $(BMP_DIR)/bmp_encoder.c
endif

LDFLAGS += -L$(TMK_PATH)/$(BMP_DIR)

VPATH += $(TMK_PATH)/$(PROTOCOL_DIR)
VPATH += $(TMK_PATH)/$(BMP_DIR)
VPATH += $(TMK_PATH)/$(BMP_DIR)/matrix
VPATH += $(TMK_PATH)/$(BMP_DIR)/cli
VPATH += $(TMK_PATH)/$(BMP_DIR)/cli/microshell
VPATH += $(TMK_PATH)/$(BMP_DIR)/cli/microshell/core
VPATH += $(TMK_PATH)/$(BMP_DIR)/cli/microshell/util
VPATH += $(TMK_PATH)/$(BMP_DIR)/key_override

$(BMP_DIR)/bmp_settings.c: | $(INTERMEDIATE_OUTPUT)/src/vial_generated_custom_menu_definition.h
$(INTERMEDIATE_OUTPUT)/src/vial_generated_custom_menu_definition.h: $(KEYMAP_PATH)/vial.json
	python3 util/vial_generate_custom_menu_id.py $(KEYMAP_PATH)/vial.json $(INTERMEDIATE_OUTPUT)/src/vial_generated_custom_menu_definition.h