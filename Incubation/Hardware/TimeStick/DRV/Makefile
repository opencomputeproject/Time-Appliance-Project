TARGET	= ax_usb_nic
KDIR	:= /lib/modules/$(shell uname -r)/build
PWD	= $(shell pwd)

ENABLE_IOCTL_DEBUG = n
ENABLE_AUTODETACH_FUNC = n
ENABLE_MAC_PASS = n
ENABLE_DWC3_ENHANCE = n
ENABLE_INT_POLLING = n
ENABLE_AUTOSUSPEND = n
ENABLE_TX_TASKLET = n
ENABLE_RX_TASKLET = n
ENABLE_PTP_FUNC = y
ENABLE_PTP_DEBUG = n
ENABLE_QUEUE_PRIORITY = n

obj-m := $(TARGET).o
$(TARGET)-objs := ax_main.o ax88179_178a.o ax88179a_772d.o
EXTRA_CFLAGS = -fno-pie
TOOL_EXTRA_CFLAGS = -Werror

ifeq ($(ENABLE_IOCTL_DEBUG), y)
	EXTRA_CFLAGS += -DENABLE_IOCTL_DEBUG
	TOOL_EXTRA_CFLAGS += -DENABLE_IOCTL_DEBUG
endif

ifeq ($(ENABLE_AUTODETACH_FUNC), y)
	EXTRA_CFLAGS += -DENABLE_AUTODETACH_FUNC
endif

ifeq ($(ENABLE_MAC_PASS), y)
	EXTRA_CFLAGS += -DENABLE_MAC_PASS
endif

ifeq ($(ENABLE_DWC3_ENHANCE), y)
	EXTRA_CFLAGS += -DENABLE_DWC3_ENHANCE
ifeq ($(ENABLE_INT_POLLING), n)
	EXTRA_CFLAGS += -DENABLE_INT_POLLING
endif
endif

ifeq ($(ENABLE_AUTOSUSPEND), y)
	EXTRA_CFLAGS += -DENABLE_AUTOSUSPEND
endif

ifeq ($(ENABLE_TX_TASKLET), y)
	EXTRA_CFLAGS += -DENABLE_TX_TASKLET
endif
ifeq ($(ENABLE_RX_TASKLET), y)
	EXTRA_CFLAGS += -DENABLE_RX_TASKLET
endif

ifeq ($(ENABLE_PTP_FUNC), y)
	$(TARGET)-objs += ax_ptp.o
	EXTRA_CFLAGS += -DENABLE_PTP_FUNC
ifeq ($(ENABLE_PTP_DEBUG), y)
	EXTRA_CFLAGS += -DENABLE_PTP_DEBUG
endif
endif

ifeq ($(ENABLE_QUEUE_PRIORITY), y)
	EXTRA_CFLAGS += -DENABLE_QUEUE_PRIORITY
endif

	EXTRA_CFLAGS += -DENABLE_AX88279
ifeq ($(ENABLE_MACSEC_FUNC), y)
	$(TARGET)-objs += ax_macsec.o
	EXTRA_CFLAGS += -DENABLE_MACSEC_FUNC
endif
	EXTRA_CFLAGS += -DENABLE_AX88279_MINIP_2_5G


ifneq (,$(filter $(SUBLEVEL),14 15 16 17 18 19 20 21))
MDIR	= kernel/drivers/usb/net
else
MDIR	= kernel/drivers/net/usb
endif

all:
	make -C $(KDIR) M=$(PWD) modules
	$(CC) $(TOOL_EXTRA_CFLAGS) ax88179_programmer.c -o ax88179_programmer
	$(CC) $(TOOL_EXTRA_CFLAGS) ax88179a_programmer.c -o ax88179b_179a_772d_programmer
	$(CC) $(TOOL_EXTRA_CFLAGS) ax88279_programmer.c -o ax88279_programmer
	$(CC) $(TOOL_EXTRA_CFLAGS) ax88179a_ieee.c -o ax88179b_179a_772d_ieee
	$(CC) $(TOOL_EXTRA_CFLAGS) axcmd.c -o axcmd

install:
ifneq (,$(wildcard /lib/modules/$(shell uname -r)/$(MDIR)/ax88179_178a.ko))
	gzip /lib/modules/$(shell uname -r)/$(MDIR)/ax88179_178a.ko
endif
	make -C $(KDIR) M=$(PWD) INSTALL_MOD_DIR=$(MDIR) modules_install
	depmod -a

uninstall:
ifneq (,$(wildcard /lib/modules/$(shell uname -r)/$(MDIR)/$(TARGET).ko))
	rm -f /lib/modules/$(shell uname -r)/$(MDIR)/$(TARGET).ko
endif
	depmod -a

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -rf *_programmer *_ieee axcmd .tmp_versions
