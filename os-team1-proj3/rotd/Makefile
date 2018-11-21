CROSS_COMPILE = arm-linux-gnueabi-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
CFLAGS := -W -Wall

all: rotd

rotd: rotd.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f rotd.o rotd

.PHONY: clean

# -------------------------------------------------------------------------
#  Installation targets below this line

TARGET_DIR = /data/misc

%_emu: ADB_OPT = -e
%_emu: ADB_TGT = Android Emulator
%_dev: ADB_OPT = -d
%_dev: ADB_TGT = Android Device

install_%: $(all)
	@echo [INSTALL] moving test binaries to $(ADB_TGT) [$(TARGET_DIR)]...
	@adb $(ADB_OPT) remount
	@adb $(ADB_OPT) push test $(TARGET_DIR)
	@adb $(ADB_OPT) push infinite $(TARGET_DIR)
	@adb $(ADB_OPT) shell chmod 6755 $(TARGET_DIR)/test
	@adb $(ADB_OPT) shell chmod 6755 $(TARGET_DIR)/infinite
