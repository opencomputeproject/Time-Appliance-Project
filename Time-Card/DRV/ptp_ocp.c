// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2020 Facebook */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/serial_8250.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/platform_device.h>
#include <linux/platform_data/i2c-xiic.h>
#include <linux/platform_data/i2c-ocores.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/spi/spi.h>
#include <linux/spi/xilinx_spi.h>
#include <linux/spi/altera.h>
#include <net/devlink.h>
#include <linux/i2c.h>
#include <linux/mtd/mtd.h>
#include <linux/miscdevice.h>
#include <linux/version.h>

/*---------------------------------------------------------------------------*/
#ifndef MRO50_IOCTL_H
#define MRO50_IOCTL_H

#define MRO50_READ_FINE		_IOR('M', 1, u32 *)
#define MRO50_READ_COARSE	_IOR('M', 2, u32 *)
#define MRO50_ADJUST_FINE	_IOW('M', 3, u32)
#define MRO50_ADJUST_COARSE	_IOW('M', 4, u32)
#define MRO50_READ_TEMP		_IOR('M', 5, u32 *)
#define MRO50_READ_CTRL		_IOR('M', 6, u32 *)
#define MRO50_SAVE_COARSE	_IO('M', 7)

#endif /* MRO50_IOCTL_H */
/*---------------------------------------------------------------------------*/

#ifndef PCI_VENDOR_ID_FACEBOOK
#define PCI_VENDOR_ID_FACEBOOK 0x1d9b
#endif

#ifndef PCI_DEVICE_ID_FACEBOOK_TIMECARD
#define PCI_DEVICE_ID_FACEBOOK_TIMECARD 0x0400
#endif

#ifndef PCI_VENDOR_ID_OROLIA
#define PCI_VENDOR_ID_OROLIA 0x1ad7
#endif

#ifndef PCI_DEVICE_ID_OROLIA_ARTCARD
#define PCI_DEVICE_ID_OROLIA_ARTCARD 0xa000
#endif

static struct class timecard_class = {
	.owner		= THIS_MODULE,
	.name		= "timecard",
};

struct ocp_reg {
	u32	ctrl;
	u32	status;
	u32	select;
	u32	version;
	u32	time_ns;
	u32	time_sec;
	u32	__pad0[2];
	u32	adjust_ns;
	u32	adjust_sec;
	u32	__pad1[2];
	u32	offset_ns;
	u32	offset_window_ns;
	u32	__pad2[2];
	u32	drift_ns;
	u32	drift_window_ns;
	u32	__pad3[6];
	u32	servo_offset_p;
	u32	servo_offset_i;
	u32	servo_drift_p;
	u32	servo_drift_i;
};

#define OCP_CTRL_ENABLE		BIT(0)
#define OCP_CTRL_ADJUST_TIME	BIT(1)
#define OCP_CTRL_ADJUST_OFFSET	BIT(2)
#define OCP_CTRL_ADJUST_DRIFT	BIT(3)
#define OCP_CTRL_ADJUST_SERVO	BIT(8)
#define OCP_CTRL_READ_TIME_REQ	BIT(30)
#define OCP_CTRL_READ_TIME_DONE	BIT(31)

#define OCP_STATUS_IN_SYNC	BIT(0)
#define OCP_STATUS_IN_HOLDOVER	BIT(1)

#define OCP_SELECT_CLK_NONE	0
#define OCP_SELECT_CLK_REG	0xfe

struct tod_reg {
	u32	ctrl;
	u32	status;
	u32	uart_polarity;
	u32	version;
	u32	correction_sec;
	u32	__pad0[3];
	u32	uart_baud;
	u32	__pad1[3];
	u32	utc_status;
	u32	leap;
};

#define TOD_CTRL_PROTOCOL	BIT(28)
#define TOD_CTRL_DISABLE_FMT_A	BIT(17)
#define TOD_CTRL_DISABLE_FMT_B	BIT(16)
#define TOD_CTRL_ENABLE		BIT(0)
#define TOD_CTRL_GNSS_MASK	((1U << 4) - 1)
#define TOD_CTRL_GNSS_SHIFT	24

#define TOD_STATUS_UTC_MASK	0xff
#define TOD_STATUS_UTC_VALID	BIT(8)
#define TOD_STATUS_LEAP_VALID	BIT(16)

struct ts_reg {
	u32	enable;
	u32	error;
	u32	polarity;
	u32	version;
	u32	__pad0[4];
	u32	cable_delay;
	u32	__pad1[3];
	u32	intr;
	u32	intr_mask;
	u32	event_count;
	u32	__pad2[1];
	u32	ts_count;
	u32	time_ns;
	u32	time_sec;
	u32	data_width;
	u32	data;
};

struct pps_reg {
	u32	ctrl;
	u32	status;
	u32	__pad0[6];
	u32	cable_delay;
};

#define PPS_STATUS_FILTER_ERR	BIT(0)
#define PPS_STATUS_SUPERV_ERR	BIT(1)

struct img_reg {
	u32	version;
};

struct gpio_reg {
	u32	gpio1;
	u32	__pad0;
	u32	gpio2;
	u32	__pad1;
};

struct irig_master_reg {
	u32	ctrl;
	u32	status;
	u32	__pad0;
	u32	version;
	u32	adj_sec;
	u32	mode_ctrl;
};

#define IRIG_M_CTRL_ENABLE	BIT(0)

struct ptp_ocp_flash_info {
	const char *name;
	int pci_offset;
	int data_size;
	void *data;
};

struct ptp_ocp_i2c_info {
	const char *name;
	unsigned long fixed_rate;
	size_t data_size;
	void *data;
};

struct ptp_ocp_ext_info {
	int index;
	irqreturn_t (*irq_fcn)(int irq, void *priv);
	int (*enable)(void *priv, bool enable);
};

struct ptp_ocp_ext_src {
	void __iomem		*mem;
	struct ptp_ocp		*bp;
	struct ptp_ocp_ext_info	*info;
	int			irq_vec;
};

struct ptp_ocp {
	struct pci_dev		*pdev;
	struct device		dev;
	spinlock_t		lock;
	struct mutex		mutex;
	struct ocp_reg __iomem	*reg;
	struct tod_reg __iomem	*tod;
	struct pps_reg __iomem	*pps_to_ext;
	struct pps_reg __iomem	*pps_to_clk;
	struct gpio_reg __iomem	*sma;
	struct irig_master_reg 	__iomem *irig_out;
	struct ptp_ocp_ext_src	*pps;
	struct ptp_ocp_ext_src	*ts0;
	struct ptp_ocp_ext_src	*ts1;
	struct ptp_ocp_ext_src	*ts2;
	struct ocp_phase_reg	__iomem *phasemeter;
	struct ocp_art_osc_reg	__iomem *osc;
	struct img_reg __iomem	*image;
	struct ptp_clock	*ptp;
	struct ptp_clock_info	ptp_info;
	struct platform_device	*i2c_ctrl;
	struct platform_device	*spi_flash;
	struct clk_hw		*i2c_clk;
	struct timer_list	watchdog;
	const struct attribute_group **attr_groups;
	struct miscdevice	mro50;
	time64_t		gnss_lost;
	int			id;
	int			n_irqs;
	int			gnss_port;
	int			mac_port;	/* miniature atomic clock */
	u8			serial[6];
	int			flash_start;
	bool			has_serial;
};

struct ocp_resource {
	unsigned long offset;
	int size;
	int irq_vec;
	int (*setup)(struct ptp_ocp *bp, struct ocp_resource *r);
	void *extra;
	unsigned long bp_offset;
	const char * const name;
};

static int ptp_ocp_register_mem(struct ptp_ocp *bp, struct ocp_resource *r);
static int ptp_ocp_register_i2c(struct ptp_ocp *bp, struct ocp_resource *r);
static int ptp_ocp_register_spi(struct ptp_ocp *bp, struct ocp_resource *r);
static int ptp_ocp_register_serial(struct ptp_ocp *bp, struct ocp_resource *r);
static int ptp_ocp_register_ext(struct ptp_ocp *bp, struct ocp_resource *r);
static int ptp_ocp_fb_board_init(struct ptp_ocp *bp, struct ocp_resource *r);
static irqreturn_t ptp_ocp_ts_irq(int irq, void *priv);
static int ptp_ocp_ts_enable(void *priv, bool enable);

static int ptp_ocp_art_board_init(struct ptp_ocp *bp, struct ocp_resource *r);
static irqreturn_t ptp_ocp_art_pps_irq(int irq, void *priv);
static int ptp_ocp_art_pps_enable(void *priv, bool enable);

static const struct attribute_group *fb_timecard_groups[];
static const struct attribute_group *art_timecard_groups[];

#define bp_assign_entry(bp, res, val) ({				\
	uintptr_t addr = (uintptr_t)(bp) + (res)->bp_offset;		\
	*(typeof(val) *)addr = val;					\
})

#define OCP_RES_LOCATION(member) \
	.name = #member, .bp_offset = offsetof(struct ptp_ocp, member)

#define OCP_MEM_RESOURCE(member) \
	OCP_RES_LOCATION(member), .setup = ptp_ocp_register_mem

#define OCP_SERIAL_RESOURCE(member) \
	OCP_RES_LOCATION(member), .setup = ptp_ocp_register_serial

#define OCP_I2C_RESOURCE(member) \
	OCP_RES_LOCATION(member), .setup = ptp_ocp_register_i2c

#define OCP_SPI_RESOURCE(member) \
	OCP_RES_LOCATION(member), .setup = ptp_ocp_register_spi

#define OCP_EXT_RESOURCE(member) \
	OCP_RES_LOCATION(member), .setup = ptp_ocp_register_ext

/* This is the MSI vector mapping used.
 * 0: N/C
 * 1: TS0
 * 2: TS1
 * 3: GPS
 * 4: GPS2 (n/c)
 * 5: MAC
 * 6: TS2
 * 7: I2C controller
 * 8: HWICAP (notused)
 * 9: SPI Flash
 --
 * 10: n/c
 * 11: Orolia TS0
 * 12: Orolia TS1
 * 13: Orolia PPS
 */

static struct ocp_resource ocp_fb_resource[] = {
	{
		OCP_MEM_RESOURCE(reg),
		.offset = 0x01000000, .size = 0x10000,
	},
	{
		OCP_EXT_RESOURCE(ts0),
		.offset = 0x01010000, .size = 0x10000, .irq_vec = 1,
		.extra = &(struct ptp_ocp_ext_info) {
			.index = 0,
			.irq_fcn = ptp_ocp_ts_irq,
			.enable = ptp_ocp_ts_enable,
		},
	},
	{
		OCP_EXT_RESOURCE(ts1),
		.offset = 0x01020000, .size = 0x10000, .irq_vec = 2,
		.extra = &(struct ptp_ocp_ext_info) {
			.index = 1,
			.irq_fcn = ptp_ocp_ts_irq,
			.enable = ptp_ocp_ts_enable,
		},
	},
	{
		OCP_EXT_RESOURCE(ts2),
		.offset = 0x01060000, .size = 0x10000, .irq_vec = 6,
		.extra = &(struct ptp_ocp_ext_info) {
			.index = 2,
			.irq_fcn = ptp_ocp_ts_irq,
			.enable = ptp_ocp_ts_enable,
		},
	},
	{
		OCP_MEM_RESOURCE(pps_to_ext),
		.offset = 0x01030000, .size = 0x10000,
	},
	{
		OCP_MEM_RESOURCE(pps_to_clk),
		.offset = 0x01040000, .size = 0x10000,
	},
	{
		OCP_MEM_RESOURCE(tod),
		.offset = 0x01050000, .size = 0x10000,
	},
	{
		OCP_MEM_RESOURCE(irig_out),
		.offset = 0x01080000, .size = 0x10000,
	},
	{
		OCP_MEM_RESOURCE(image),
		.offset = 0x00020000, .size = 0x1000,
	},
	{
		OCP_MEM_RESOURCE(sma),
		.offset = 0x00140000, .size = 0x1000,
	},
	{
		OCP_I2C_RESOURCE(i2c_ctrl),
		.offset = 0x00150000, .size = 0x10000, .irq_vec = 7,
		.extra = &(struct ptp_ocp_i2c_info) {
			.name = "xiic-i2c",
			.fixed_rate = 50000000,
			.data_size = sizeof(struct xiic_i2c_platform_data),
			.data = &(struct xiic_i2c_platform_data) {
				.num_devices = 1,
				.devices = &(struct i2c_board_info) {
					I2C_BOARD_INFO("24mac402", 0x58),
				},
#if 0
				.num_devices = 2,
				.devices = (struct i2c_board_info[]) {
					{ I2C_BOARD_INFO("24c08", 0x50) },
					{ I2C_BOARD_INFO("24mac402", 0x58) },
				},
#endif
			},
		},
	},
	{
		OCP_SERIAL_RESOURCE(gnss_port),
		.offset = 0x00160000 + 0x1000, .irq_vec = 3,
	},
	{
		OCP_SERIAL_RESOURCE(mac_port),
		.offset = 0x00180000 + 0x1000, .irq_vec = 5,
	},
	{
		OCP_SPI_RESOURCE(spi_flash),
		.offset = 0x00310000, .size = 0x10000, .irq_vec = 9,
		.extra = &(struct ptp_ocp_flash_info) {
			.name = "xilinx_spi", .pci_offset = 0,
			.data_size = sizeof(struct xspi_platform_data),
			.data = &(struct xspi_platform_data) {
				.num_chipselect = 1,
				.bits_per_word = 8,
				.num_devices = 1,
				.devices = &(struct spi_board_info) {
					.modalias = "spi-nor",
				},
			},
		},
	},
	{
		.setup = ptp_ocp_fb_board_init,
	},
	{ }
};

struct ocp_art_osc_reg {
	u32	ctrl;
	u32	value;
	u32	adjust;
	u32	temp;
};
#define MRO50_CTRL_ENABLE		BIT(0)
#define MRO50_CTRL_LOCK			BIT(1)
#define MRO50_CTRL_READ_CMD		BIT(2)
#define MRO50_CTRL_READ_COARSE		BIT(3)
#define MRO50_CTRL_READ_DONE		BIT(4)
#define MRO50_CTRL_ADJUST_CMD		BIT(5)
#define MRO50_CTRL_ADJUST_COARSE	BIT(6)
#define MRO50_CTRL_SAVE_COARSE		BIT(7)

#define MRO50_CMD_READ		(MRO50_CTRL_ENABLE | MRO50_CTRL_READ_CMD)
#define MRO50_CMD_ADJUST	(MRO50_CTRL_ENABLE | MRO50_CTRL_ADJUST_CMD)

#define MRO50_OP_READ_FINE	MRO50_CMD_READ
#define MRO50_OP_READ_COARSE	(MRO50_CMD_READ | MRO50_CTRL_READ_COARSE)
#define MRO50_OP_ADJUST_FINE	MRO50_CMD_ADJUST
#define MRO50_OP_ADJUST_COARSE	(MRO50_CMD_ADJUST | MRO50_CTRL_ADJUST_COARSE)
#define MRO50_OP_SAVE_COARSE	(MRO50_CTRL_ENABLE | MRO50_CTRL_SAVE_COARSE)

struct ocp_art_pps_reg {
	u32	enable;
	u32	__pad0[11];
	u32	intr;
};

struct ocp_phase_reg {
	u32	__pad0;
	u32	phase_offset;
	u32	__pad1[9];
	u32	intr;
};

static struct ocp_resource ocp_art_resource[] = {
	{
		OCP_MEM_RESOURCE(reg),
		.offset = 0x01000000, .size = 0x10000,
	},
	{
		OCP_SERIAL_RESOURCE(gnss_port),
		.offset = 0x00160000 + 0x1000, .irq_vec = 3,
	},
	{
		OCP_MEM_RESOURCE(phasemeter),
		.offset = 0x00320000, .size = 0x30,
	},
	/* Timestamp associated with Internal PPS of the card */
	{
		OCP_EXT_RESOURCE(ts0),
		.offset = 0x00330000, .size = 0x20, .irq_vec = 11,
		.extra = &(struct ptp_ocp_ext_info) {
			.index = 0,
			.irq_fcn = ptp_ocp_ts_irq,
			.enable = ptp_ocp_ts_enable,
		},
	},
	{
		OCP_MEM_RESOURCE(osc),
		.offset = 0x00340000, .size = 0x20,
	},
	{
		OCP_SPI_RESOURCE(spi_flash),
		.offset = 0x00310000, .size = 0x10000, .irq_vec = 9,
		.extra = &(struct ptp_ocp_flash_info) {
			.name = "spi_altera", .pci_offset = 0,
			.data_size = sizeof(struct altera_spi_platform_data),
			.data = &(struct altera_spi_platform_data) {
				.num_chipselect = 1,
				.num_devices = 1,
				.devices = &(struct spi_board_info) {
					.modalias = "spi-nor",
				},
			},
		},
	},
	{
		OCP_I2C_RESOURCE(i2c_ctrl),
		.offset = 0x350000, .size = 0x100, .irq_vec = 4,
		.extra = &(struct ptp_ocp_i2c_info) {
			.name = "ocores-i2c",
			.fixed_rate = 400000,
#if 1
			.data_size = sizeof(struct ocores_i2c_platform_data),
			.data = &(struct ocores_i2c_platform_data) {
				.clock_khz = 125000,
				.bus_khz = 400,
				.num_devices = 1,
				.devices = &(struct i2c_board_info) {
					I2C_BOARD_INFO("24c08", 0x50),
				},
			},
#endif
		},
	},
	/* Timestamp associated with GNSS receiver PPS */
	{
		OCP_EXT_RESOURCE(ts1),
		.offset = 0x360000, .size = 0x20, .irq_vec = 12,
		.extra = &(struct ptp_ocp_ext_info) {
			.index = 1,
			.irq_fcn = ptp_ocp_ts_irq,
			.enable = ptp_ocp_ts_enable,
		},
	},
	{
		OCP_EXT_RESOURCE(pps),
		.offset = 0x00370000, .size = 0x20, .irq_vec = 13,
		.extra = &(struct ptp_ocp_ext_info) {
			.irq_fcn = ptp_ocp_art_pps_irq,
			.enable = ptp_ocp_art_pps_enable,
		},
	},
	{
		.setup = ptp_ocp_art_board_init,
	},
	{ }
};

static const struct pci_device_id ptp_ocp_pcidev_id[] = {
	{ PCI_DEVICE_DATA(FACEBOOK, TIMECARD, &ocp_fb_resource) },
	{ PCI_DEVICE_DATA(OROLIA, ARTCARD, &ocp_art_resource) },
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, ptp_ocp_pcidev_id);

static DEFINE_MUTEX(ptp_ocp_lock);
static DEFINE_IDR(ptp_ocp_idr);

struct ocp_selector {
	const char *name;
	int value;
};

static struct ocp_selector ptp_ocp_clock[] = {
	{ .name = "NONE",	.value = 0 },
	{ .name = "TOD",	.value = 1 },
	{ .name = "IRIG",	.value = 2 },
	{ .name = "PPS",	.value = 3 },
	{ .name = "PTP",	.value = 4 },
	{ .name = "RTC",	.value = 5 },
	{ .name = "DCF",	.value = 6 },
	{ .name = "REGS",	.value = 0xfe },
	{ .name = "EXT",	.value = 0xff },
	{ }
};

static struct ocp_selector ptp_ocp_sma_in[] = {
	{ .name = "10Mhz",	.value = 0x00 },
	{ .name = "PPS1",	.value = 0x01 },
	{ .name = "PPS2",	.value = 0x02 },
	{ .name = "TS1",	.value = 0x04 },
	{ .name = "TS2",	.value = 0x08 },
	{ .name = "IRIG",	.value = 0x10 },
	{ .name = "DCF",	.value = 0x20 },
	{ }
};

static struct ocp_selector ptp_ocp_sma_out[] = {
	{ .name = "10Mhz",	.value = 0x00 },
	{ .name = "FPGA",	.value = 0x01 },
	{ .name = "MAC",	.value = 0x02 },
	{ .name = "GNSS",	.value = 0x04 },
	{ .name = "GNSS2",	.value = 0x08 },
	{ .name = "IRIG",	.value = 0x10 },
	{ .name = "DCF",	.value = 0x20 },
	{ }
};

static const char *
select_name_from_val(struct ocp_selector *tbl, int val)
{
	int i;

	for (i = 0; tbl[i].name; i++)
		if (tbl[i].value == val)
			return tbl[i].name;
	return NULL;
}

static int
select_val_from_name(struct ocp_selector *tbl, const char *name)
{
	const char *select;
	int i;

	for (i = 0; tbl[i].name; i++) {
		select = tbl[i].name;
		if (!strncasecmp(name, select, strlen(select)))
			return tbl[i].value;
	}
	return -EINVAL;
}

static ssize_t
select_table_show(struct ocp_selector *tbl, char *buf)
{
	ssize_t count;
	int i;

	count = 0;
	for (i = 0; tbl[i].name; i++)
		count += sysfs_emit_at(buf, count, "%s ", tbl[i].name);
	if (count)
		count--;
	count += sysfs_emit_at(buf, count, "\n");
	return count;
}

static int
__ptp_ocp_gettime_locked(struct ptp_ocp *bp, struct timespec64 *ts,
			 struct ptp_system_timestamp *sts)
{
	u32 ctrl, time_sec, time_ns;
	int i;

	ctrl = ioread32(&bp->reg->ctrl);
	ctrl |= OCP_CTRL_READ_TIME_REQ;

	ptp_read_system_prets(sts);
	iowrite32(ctrl, &bp->reg->ctrl);

	for (i = 0; i < 100; i++) {
		ctrl = ioread32(&bp->reg->ctrl);
		if (ctrl & OCP_CTRL_READ_TIME_DONE)
			break;
	}
	ptp_read_system_postts(sts);

	time_ns = ioread32(&bp->reg->time_ns);
	time_sec = ioread32(&bp->reg->time_sec);

	ts->tv_sec = time_sec;
	ts->tv_nsec = time_ns;

	return ctrl & OCP_CTRL_READ_TIME_DONE ? 0 : -ETIMEDOUT;
}

static int
ptp_ocp_gettimex(struct ptp_clock_info *ptp_info, struct timespec64 *ts,
		 struct ptp_system_timestamp *sts)
{
	struct ptp_ocp *bp = container_of(ptp_info, struct ptp_ocp, ptp_info);
	unsigned long flags;
	int err;

	spin_lock_irqsave(&bp->lock, flags);
	err = __ptp_ocp_gettime_locked(bp, ts, sts);
	spin_unlock_irqrestore(&bp->lock, flags);

	return err;
}

static void
__ptp_ocp_settime_locked(struct ptp_ocp *bp, const struct timespec64 *ts)
{
	u32 ctrl, time_sec, time_ns;
	u32 select;

	time_ns = ts->tv_nsec;
	time_sec = ts->tv_sec;

	select = ioread32(&bp->reg->select);
	iowrite32(OCP_SELECT_CLK_REG, &bp->reg->select);

	iowrite32(time_ns, &bp->reg->adjust_ns);
	iowrite32(time_sec, &bp->reg->adjust_sec);

	ctrl = ioread32(&bp->reg->ctrl);
	ctrl |= OCP_CTRL_ADJUST_TIME;
	iowrite32(ctrl, &bp->reg->ctrl);

	/* restore clock selection */
	iowrite32(select >> 16, &bp->reg->select);
}

static int
ptp_ocp_settime(struct ptp_clock_info *ptp_info, const struct timespec64 *ts)
{
	struct ptp_ocp *bp = container_of(ptp_info, struct ptp_ocp, ptp_info);
	unsigned long flags;

	if (ioread32(&bp->reg->status) & OCP_STATUS_IN_SYNC)
		return 0;

	spin_lock_irqsave(&bp->lock, flags);
	__ptp_ocp_settime_locked(bp, ts);
	spin_unlock_irqrestore(&bp->lock, flags);

	return 0;
}

static int
ptp_ocp_adjtime(struct ptp_clock_info *ptp_info, s64 delta_ns)
{
	struct ptp_ocp *bp = container_of(ptp_info, struct ptp_ocp, ptp_info);
	struct timespec64 ts;
	unsigned long flags;
	int err;

	if (ioread32(&bp->reg->status) & OCP_STATUS_IN_SYNC)
		return 0;

	spin_lock_irqsave(&bp->lock, flags);
	err = __ptp_ocp_gettime_locked(bp, &ts, NULL);
	if (likely(!err)) {
		timespec64_add_ns(&ts, delta_ns);
		__ptp_ocp_settime_locked(bp, &ts);
	}
	spin_unlock_irqrestore(&bp->lock, flags);

	return err;
}

static int
ptp_ocp_null_adjfine(struct ptp_clock_info *ptp_info, long scaled_ppm)
{
	if (scaled_ppm == 0)
		return 0;

	return -EOPNOTSUPP;
}

static int
ptp_ocp_adjphase(struct ptp_clock_info *ptp_info, s32 phase_ns)
{
	struct ptp_ocp *bp = container_of(ptp_info, struct ptp_ocp, ptp_info);
	struct ocp_phase_reg __iomem *reg = bp->phasemeter;
	unsigned long flags;

	if (!bp->phasemeter)
		return -EOPNOTSUPP;

	spin_lock_irqsave(&bp->lock, flags);
	iowrite32(phase_ns, &reg->phase_offset);
	spin_unlock_irqrestore(&bp->lock, flags);

	return 0;
}

static int
ptp_ocp_enable(struct ptp_clock_info *ptp_info, struct ptp_clock_request *rq,
	       int on)
{
	struct ptp_ocp *bp = container_of(ptp_info, struct ptp_ocp, ptp_info);
	struct ptp_ocp_ext_src *ext = NULL;
	int err;

	switch (rq->type) {
	case PTP_CLK_REQ_EXTTS:
		switch (rq->extts.index) {
		case 0:
			ext = bp->ts0;
			break;
		case 1:
			ext = bp->ts1;
			break;
		case 2:
			ext = bp->ts2;
			break;
		}
		break;
	case PTP_CLK_REQ_PPS:
		ext = bp->pps;
		break;
	default:
		return -EOPNOTSUPP;
	}

	err = -ENXIO;
	if (ext)
		err = ext->info->enable(ext, on);

	return err;
}

static const struct ptp_clock_info ptp_ocp_clock_info = {
	.owner		= THIS_MODULE,
	.name		= KBUILD_MODNAME,
	.max_adj	= 100000000,
	.gettimex64	= ptp_ocp_gettimex,
	.settime64	= ptp_ocp_settime,
	.adjtime	= ptp_ocp_adjtime,
	.adjfine	= ptp_ocp_null_adjfine,
	.adjphase	= ptp_ocp_adjphase,
	.enable		= ptp_ocp_enable,
	.pps		= true,
	.n_ext_ts	= 3,
};

static void
__ptp_ocp_clear_drift_locked(struct ptp_ocp *bp)
{
	u32 ctrl, select;

	select = ioread32(&bp->reg->select);
	iowrite32(OCP_SELECT_CLK_REG, &bp->reg->select);

	iowrite32(0, &bp->reg->drift_ns);

	ctrl = ioread32(&bp->reg->ctrl);
	ctrl |= OCP_CTRL_ADJUST_DRIFT;
	iowrite32(ctrl, &bp->reg->ctrl);

	/* restore clock selection */
	iowrite32(select >> 16, &bp->reg->select);
}

static void
ptp_ocp_watchdog(struct timer_list *t)
{
	struct ptp_ocp *bp = from_timer(bp, t, watchdog);
	unsigned long flags;
	u32 status;

	status = ioread32(&bp->pps_to_clk->status);

	if (status & PPS_STATUS_SUPERV_ERR) {
		iowrite32(status, &bp->pps_to_clk->status);
		if (!bp->gnss_lost) {
			spin_lock_irqsave(&bp->lock, flags);
			__ptp_ocp_clear_drift_locked(bp);
			spin_unlock_irqrestore(&bp->lock, flags);
			bp->gnss_lost = ktime_get_real_seconds();
		}

	} else if (bp->gnss_lost) {
		bp->gnss_lost = 0;
	}

	mod_timer(&bp->watchdog, jiffies + HZ);
}

static int
ptp_ocp_init_clock(struct ptp_ocp *bp)
{
	struct timespec64 ts;
	bool sync;
	u32 ctrl;

	/* make sure clock is enabled */
	ctrl = ioread32(&bp->reg->ctrl);
	ctrl |= OCP_CTRL_ENABLE;
	iowrite32(ctrl, &bp->reg->ctrl);

	/* NO DRIFT Correction */
	/* offset_p:i 1/8, offset_i: 1/16, drift_p: 0, drift_i: 0 */
	iowrite32(0x2000, &bp->reg->servo_offset_p);
	iowrite32(0x1000, &bp->reg->servo_offset_i);
	iowrite32(0,	  &bp->reg->servo_drift_p);
	iowrite32(0,	  &bp->reg->servo_drift_i);

	/* latch servo values */
	ctrl |= OCP_CTRL_ADJUST_SERVO;
	iowrite32(ctrl, &bp->reg->ctrl);

	if ((ioread32(&bp->reg->ctrl) & OCP_CTRL_ENABLE) == 0) {
		dev_err(&bp->pdev->dev, "clock not enabled\n");
		return -ENODEV;
	}

	sync = ioread32(&bp->reg->status) & OCP_STATUS_IN_SYNC;
	if (!sync) {
		ktime_get_real_ts64(&ts);
		ptp_ocp_settime(&bp->ptp_info, &ts);
	}
	if (!ptp_ocp_gettimex(&bp->ptp_info, &ts, NULL))
		dev_info(&bp->pdev->dev, "Time: %lld.%ld, %s\n",
			 ts.tv_sec, ts.tv_nsec,
			 sync ? "in-sync" : "UNSYNCED");

	timer_setup(&bp->watchdog, ptp_ocp_watchdog, 0);
	mod_timer(&bp->watchdog, jiffies + HZ);

	return 0;
}

static void
ptp_ocp_tod_info(struct ptp_ocp *bp)
{
	static const char * const proto_name[] = {
		"NMEA", "NMEA_ZDA", "NMEA_RMC", "NMEA_none",
		"UBX", "UBX_UTC", "UBX_LS", "UBX_none"
	};
	static const char * const gnss_name[] = {
		"ALL", "COMBINED", "GPS", "GLONASS", "GALILEO", "BEIDOU",
	};
	u32 version, ctrl, reg;
	int idx;

	version = ioread32(&bp->tod->version);
	dev_info(&bp->pdev->dev, "TOD Version %d.%d.%d\n",
		 version >> 24, (version >> 16) & 0xff, version & 0xffff);

	ctrl = ioread32(&bp->tod->ctrl);
	ctrl |= TOD_CTRL_PROTOCOL | TOD_CTRL_ENABLE;
	ctrl &= ~(TOD_CTRL_DISABLE_FMT_A | TOD_CTRL_DISABLE_FMT_B);
	iowrite32(ctrl, &bp->tod->ctrl);

	ctrl = ioread32(&bp->tod->ctrl);
	idx = ctrl & TOD_CTRL_PROTOCOL ? 4 : 0;
	idx += (ctrl >> 16) & 3;
	dev_info(&bp->pdev->dev, "control: %x\n", ctrl);
	dev_info(&bp->pdev->dev, "TOD Protocol %s %s\n", proto_name[idx],
		 ctrl & TOD_CTRL_ENABLE ? "enabled" : "");

	idx = (ctrl >> TOD_CTRL_GNSS_SHIFT) & TOD_CTRL_GNSS_MASK;
	if (idx < ARRAY_SIZE(gnss_name))
		dev_info(&bp->pdev->dev, "GNSS %s\n", gnss_name[idx]);

	reg = ioread32(&bp->tod->status);
	dev_info(&bp->pdev->dev, "status: %x\n", reg);

	reg = ioread32(&bp->tod->correction_sec);
	dev_info(&bp->pdev->dev, "correction: %d\n", reg);

	reg = ioread32(&bp->tod->utc_status);
	dev_info(&bp->pdev->dev, "utc_status: %x\n", reg);
	dev_info(&bp->pdev->dev, "utc_offset: %d  valid:%d  leap_valid:%d\n",
		 reg & TOD_STATUS_UTC_MASK, reg & TOD_STATUS_UTC_VALID ? 1 : 0,
		 reg & TOD_STATUS_LEAP_VALID ? 1 : 0);
}

static int
ptp_ocp_firstchild(struct device *dev, void *data)
{
	return 1;
}

static int
ptp_ocp_read_i2c(struct i2c_adapter *adap, u8 addr, u8 reg, u8 sz, u8 *data)
{
	struct i2c_msg msgs[2] = {
		{
			.addr = addr,
			.len = 1,
			.buf = &reg,
		},
		{
			.addr = addr,
			.flags = I2C_M_RD,
			.len = 2,
			.buf = data,
		},
	};
	int err;
	u8 len;

	/* xiic-i2c for some stupid reason only does 2 byte reads. */
	while (sz) {
		len = min_t(u8, sz, 2);
		msgs[1].len = len;
		err = i2c_transfer(adap, msgs, 2);
		if (err != msgs[1].len)
			return err;
		msgs[1].buf += len;
		reg += len;
		sz -= len;
	}
	return 0;
}

static void
ptp_ocp_get_serial_number(struct ptp_ocp *bp)
{
	struct i2c_adapter *adap;
	struct device *dev;
	int err;

	if (!bp->i2c_ctrl)
		return;

	dev = device_find_child(&bp->i2c_ctrl->dev, NULL, ptp_ocp_firstchild);
	if (!dev) {
		dev_err(&bp->pdev->dev, "Can't find I2C adapter\n");
		return;
	}

	adap = i2c_verify_adapter(dev);
	if (!adap) {
		dev_err(&bp->pdev->dev, "device '%s' isn't an I2C adapter\n",
			dev_name(dev));
		goto out;
	}

	err = ptp_ocp_read_i2c(adap, 0x58, 0x9A, 6, bp->serial);
	if (err) {
		dev_err(&bp->pdev->dev, "could not read eeprom: %d\n", err);
		goto out;
	}

	bp->has_serial = true;

out:
	put_device(dev);
}

static void
ptp_ocp_info(struct ptp_ocp *bp)
{
	u32 version, select;

	version = ioread32(&bp->reg->version);
	select = ioread32(&bp->reg->select);
	dev_info(&bp->pdev->dev, "Version %d.%d.%d, clock %s, device ptp%d\n",
		 version >> 24, (version >> 16) & 0xff, version & 0xffff,
		 select_name_from_val(ptp_ocp_clock, select >> 16),
		 ptp_clock_index(bp->ptp));

	if (bp->tod)
		ptp_ocp_tod_info(bp);
}

static struct device *
ptp_ocp_find_flash(struct ptp_ocp *bp)
{
	struct device *dev, *last;

	last = NULL;
	dev = &bp->spi_flash->dev;

	while ((dev = device_find_child(dev, NULL, ptp_ocp_firstchild))) {
		if (!strcmp("mtd", dev_bus_name(dev)))
			break;
		put_device(last);
		last = dev;
	}
	put_device(last);

	return dev;
}

static int
ptp_ocp_devlink_flash(struct devlink *devlink, struct device *dev,
		      const struct firmware *fw)
{
	struct mtd_info *mtd = dev_get_drvdata(dev);
	struct ptp_ocp *bp = devlink_priv(devlink);
	size_t off, len, resid, wrote;
	struct erase_info erase;
	size_t base, blksz;
	int err = 0;

	off = 0;
	base = bp->flash_start;
	blksz = 4096;
	resid = fw->size;

	while (resid) {
		devlink_flash_update_status_notify(devlink, "Flashing",
						   NULL, off, fw->size);

		len = min_t(size_t, resid, blksz);
		erase.addr = base + off;
		erase.len = blksz;

		err = mtd_erase(mtd, &erase);
		if (err)
			goto out;

		err = mtd_write(mtd, base + off, len, &wrote, &fw->data[off]);
		if (err)
			goto out;

		off += blksz;
		resid -= len;
	}
out:
	return err;
}

static int
ptp_ocp_devlink_flash_update(struct devlink *devlink,
			     struct devlink_flash_update_params *params,
			     struct netlink_ext_ack *extack)
{
	struct ptp_ocp *bp = devlink_priv(devlink);
	struct device *dev;
	const char *msg;
	int err;

	dev = ptp_ocp_find_flash(bp);
	if (!dev) {
		dev_err(&bp->pdev->dev, "Can't find Flash SPI adapter\n");
		return -ENODEV;
	}

	devlink_flash_update_status_notify(devlink, "Preparing to flash",
					   NULL, 0, 0);

	err = ptp_ocp_devlink_flash(devlink, dev, params->fw);

	msg = err ? "Flash error" : "Flash complete";
	devlink_flash_update_status_notify(devlink, msg, NULL, 0, 0);

	put_device(dev);
	return err;
}

static int
ptp_ocp_devlink_info_get(struct devlink *devlink, struct devlink_info_req *req,
			 struct netlink_ext_ack *extack)
{
	struct ptp_ocp *bp = devlink_priv(devlink);
	char buf[32];
	int err;

	err = devlink_info_driver_name_put(req, KBUILD_MODNAME);
	if (err)
		return err;

	if (bp->image) {
		u32 ver = ioread32(&bp->image->version);

		if (ver & 0xffff) {
			sprintf(buf, "%d", ver);
			err = devlink_info_version_running_put(req,
							       "fw",
							       buf);
		} else {
			sprintf(buf, "%d", ver >> 16);
			err = devlink_info_version_running_put(req,
							       "loader",
							       buf);
		}
		if (err)
			return err;
	}

	if (!bp->has_serial)
		ptp_ocp_get_serial_number(bp);

	if (bp->has_serial) {
		sprintf(buf, "%pM", bp->serial);
		err = devlink_info_serial_number_put(req, buf);
		if (err)
			return err;
	}

	return 0;
}

static const struct devlink_ops ptp_ocp_devlink_ops = {
	.flash_update = ptp_ocp_devlink_flash_update,
	.info_get = ptp_ocp_devlink_info_get,
};

static void __iomem *
__ptp_ocp_get_mem(struct ptp_ocp *bp, unsigned long start, int size)
{
	struct resource res = DEFINE_RES_MEM_NAMED(start, size, "ptp_ocp");

	return devm_ioremap_resource(&bp->pdev->dev, &res);
}

static void __iomem *
ptp_ocp_get_mem(struct ptp_ocp *bp, struct ocp_resource *r)
{
	unsigned long start;

	start = pci_resource_start(bp->pdev, 0) + r->offset;
	return __ptp_ocp_get_mem(bp, start, r->size);
}

static void
ptp_ocp_set_irq_resource(struct resource *res, int irq)
{
	struct resource r = DEFINE_RES_IRQ(irq);
	*res = r;
}

static void
ptp_ocp_set_mem_resource(struct resource *res, unsigned long start, int size)
{
	struct resource r = DEFINE_RES_MEM(start, size);
	*res = r;
}

static int
ptp_ocp_register_spi(struct ptp_ocp *bp, struct ocp_resource *r)
{
	struct ptp_ocp_flash_info *info;
	struct pci_dev *pdev = bp->pdev;
	struct platform_device *p;
	struct resource res[2];
	unsigned long start;
	int id;

	start = pci_resource_start(pdev, 0) + r->offset;
	ptp_ocp_set_mem_resource(&res[0], start, r->size);
	ptp_ocp_set_irq_resource(&res[1], pci_irq_vector(pdev, r->irq_vec));

	info = r->extra;
	id = pci_dev_id(pdev) << 1;
	id += info->pci_offset;

	p = platform_device_register_resndata(&pdev->dev, info->name, id,
					      res, 2, info->data,
					      info->data_size);
	if (IS_ERR(p))
		return PTR_ERR(p);

	bp_assign_entry(bp, r, p);

	return 0;
}

static struct platform_device *
ptp_ocp_i2c_bus(struct pci_dev *pdev, struct ocp_resource *r, int id)
{
	struct ptp_ocp_i2c_info *info;
	struct resource res[2];
	unsigned long start;

	info = r->extra;
	start = pci_resource_start(pdev, 0) + r->offset;
	ptp_ocp_set_mem_resource(&res[0], start, r->size);
	ptp_ocp_set_irq_resource(&res[1], pci_irq_vector(pdev, r->irq_vec));

	return platform_device_register_resndata(&pdev->dev, info->name,
						 id, res, 2,
						 info->data, info->data_size);
}

static int
ptp_ocp_register_i2c(struct ptp_ocp *bp, struct ocp_resource *r)
{
	struct pci_dev *pdev = bp->pdev;
	struct ptp_ocp_i2c_info *info;
	struct platform_device *p;
	struct clk_hw *clk;
	char buf[32];
	int id;

	info = r->extra;
	id = pci_dev_id(bp->pdev);

	sprintf(buf, "AXI.%d", id);
	clk = clk_hw_register_fixed_rate(&pdev->dev, buf, NULL, 0,
					 info->fixed_rate);
	if (IS_ERR(clk))
		return PTR_ERR(clk);
	bp->i2c_clk = clk;

	sprintf(buf, "%s.%d", info->name, id);
	devm_clk_hw_register_clkdev(&pdev->dev, clk, NULL, buf);
	p = ptp_ocp_i2c_bus(bp->pdev, r, id);
	if (IS_ERR(p))
		return PTR_ERR(p);

	bp_assign_entry(bp, r, p);

	return 0;
}

static irqreturn_t
ptp_ocp_ts_irq(int irq, void *priv)
{
	struct ptp_ocp_ext_src *ext = priv;
	struct ts_reg __iomem *reg = ext->mem;
	struct ptp_clock_event ev;
	u32 sec, nsec;

	/* XXX should fix API - this converts s/ns -> ts -> s/ns */
	sec = ioread32(&reg->time_sec);
	nsec = ioread32(&reg->time_ns);

	ev.type = PTP_CLOCK_EXTTS;
	ev.index = ext->info->index;
	ev.timestamp = sec * 1000000000ULL + nsec;

	ptp_clock_event(ext->bp->ptp, &ev);

	iowrite32(1, &reg->intr);	/* write 1 to ack */

	return IRQ_HANDLED;
}

static int
ptp_ocp_ts_enable(void *priv, bool enable)
{
	struct ptp_ocp_ext_src *ext = priv;
	struct ts_reg __iomem *reg = ext->mem;

	if (enable) {
		iowrite32(1, &reg->enable);
		iowrite32(1, &reg->intr_mask);
		iowrite32(1, &reg->intr);
	} else {
		iowrite32(0, &reg->intr_mask);
		iowrite32(0, &reg->enable);
	}

	return 0;
}

static irqreturn_t
ptp_ocp_art_pps_irq(int irq, void *priv)
{
	struct ptp_ocp_ext_src *ext = priv;
	struct ocp_art_pps_reg __iomem *reg = ext->mem;
	struct ptp_clock_event ev;

	ev.type = PTP_CLOCK_PPS;
	ptp_clock_event(ext->bp->ptp, &ev);

	iowrite32(0, &reg->intr);

	return IRQ_HANDLED;
}

static int
ptp_ocp_art_pps_enable(void *priv, bool enable)
{
	struct ptp_ocp_ext_src *ext = priv;
	struct ocp_art_pps_reg __iomem *reg = ext->mem;

	iowrite32(enable, &reg->enable);

	return 0;
}

static void
ptp_ocp_unregister_ext(struct ptp_ocp_ext_src *ext)
{
	ext->info->enable(ext, false);
	pci_free_irq(ext->bp->pdev, ext->irq_vec, ext);
	kfree(ext);
}

static int
ptp_ocp_register_ext(struct ptp_ocp *bp, struct ocp_resource *r)
{
	struct pci_dev *pdev = bp->pdev;
	struct ptp_ocp_ext_src *ext;
	int err;

	ext = kzalloc(sizeof(*ext), GFP_KERNEL);
	if (!ext)
		return -ENOMEM;

	err = -EINVAL;
	ext->mem = ptp_ocp_get_mem(bp, r);
	if (!ext->mem)
		goto out;

	ext->bp = bp;
	ext->info = r->extra;
	ext->irq_vec = r->irq_vec;

	err = pci_request_irq(pdev, r->irq_vec, ext->info->irq_fcn, NULL,
			      ext, "ocp%d.%s", bp->id, r->name);
	if (err) {
		dev_err(&pdev->dev, "Could not get irq %d\n", r->irq_vec);
		goto out;
	}

	bp_assign_entry(bp, r, ext);

	return 0;

out:
	kfree(ext);
	return err;
}

static int
ptp_ocp_serial_line(struct ptp_ocp *bp, struct ocp_resource *r)
{
	struct pci_dev *pdev = bp->pdev;
	struct uart_8250_port uart;

	/* Setting UPF_IOREMAP and leaving port.membase unspecified lets
	 * the serial port device claim and release the pci resource.
	 */
	memset(&uart, 0, sizeof(uart));
	uart.port.dev = &pdev->dev;
	uart.port.iotype = UPIO_MEM;
	uart.port.regshift = 2;
	uart.port.mapbase = pci_resource_start(pdev, 0) + r->offset;
	uart.port.irq = pci_irq_vector(pdev, r->irq_vec);
	uart.port.uartclk = 50000000;
	uart.port.flags = UPF_FIXED_TYPE | UPF_IOREMAP;
	uart.port.type = PORT_16550A;

	return serial8250_register_8250_port(&uart);
}

static int
ptp_ocp_register_serial(struct ptp_ocp *bp, struct ocp_resource *r)
{
	int port;

	port = ptp_ocp_serial_line(bp, r);
	if (port < 0)
		return port;

	bp_assign_entry(bp, r, port);

	return 0;
}

static int
ptp_ocp_register_mem(struct ptp_ocp *bp, struct ocp_resource *r)
{
	void __iomem *mem;

	mem = ptp_ocp_get_mem(bp, r);
	if (!mem)
		return -EINVAL;

	bp_assign_entry(bp, r, mem);

	return 0;
}

/* FB specific board initializers; last "resource" registered. */
static int
ptp_ocp_fb_board_init(struct ptp_ocp *bp, struct ocp_resource *r)
{
	bp->flash_start = 1024 * 4096;
	bp->attr_groups = fb_timecard_groups;

	return ptp_ocp_init_clock(bp);
}

static void
ptp_ocp_irig_out(struct ptp_ocp *bp, bool enable)
{
	u32 ctrl;
	bool on;

	ctrl = ioread32(&bp->irig_out->ctrl);
	on = ctrl & IRIG_M_CTRL_ENABLE;
	if (on ^ enable) {
		ctrl &= ~IRIG_M_CTRL_ENABLE;
		ctrl |= enable ? IRIG_M_CTRL_ENABLE : 0;
		iowrite32(ctrl, &bp->irig_out->ctrl);
	}
}

static int
__ptp_ocp_mro50_wait_cmd(struct ocp_art_osc_reg __iomem *reg, u32 done)
{
	u32 ctrl;
	int i;

	for (i = 0; i < 100; i++) {
		ctrl = ioread32(&reg->ctrl);
		if (ctrl & done)
			break;
		usleep_range(100, 1000);
	}
	return ctrl & done ? 0 : -ETIMEDOUT;
}

static int
__ptp_ocp_mro50_write_locked(struct ocp_art_osc_reg __iomem *reg,
			     u32 ctrl, u32 val)
{
	iowrite32(val, &reg->adjust);
	iowrite32(ctrl, &reg->ctrl);

	return __ptp_ocp_mro50_wait_cmd(reg, MRO50_CTRL_ADJUST_CMD);
}

static int
__ptp_ocp_mro50_read_locked(struct ocp_art_osc_reg __iomem *reg,
			    u32 ctrl, u32 *val)
{
	int err;

	iowrite32(ctrl, &reg->ctrl);
	err = __ptp_ocp_mro50_wait_cmd(reg, MRO50_CTRL_READ_DONE);
	if (!err)
		*val = ioread32(&reg->value);

	return err;
}

static int
ptp_ocp_mro50_read(struct ptp_ocp *bp, u32 ctrl, u32 *val)
{
	int err;

	mutex_lock(&bp->mutex);
	err = __ptp_ocp_mro50_read_locked(bp->osc, ctrl, val);
	mutex_unlock(&bp->mutex);

	return err;
}

static int
ptp_ocp_mro50_write(struct ptp_ocp *bp, u32 ctrl, u32 val)
{
	int err;

	mutex_lock(&bp->mutex);
	err = __ptp_ocp_mro50_write_locked(bp->osc, ctrl, val);
	mutex_unlock(&bp->mutex);

	return err;
}

static long
ptp_ocp_mro50_ioctl(struct file *file, unsigned cmd, unsigned long arg)
{
	struct miscdevice *mro50 = file->private_data;
	struct ptp_ocp *bp = container_of(mro50, struct ptp_ocp, mro50);
	u32 val;
	int err;

	switch (cmd) {
	case MRO50_READ_FINE:
		err = ptp_ocp_mro50_read(bp, MRO50_OP_READ_FINE, &val);
		break;
	case MRO50_READ_COARSE:
		err = ptp_ocp_mro50_read(bp, MRO50_OP_READ_COARSE, &val);
		break;
	case MRO50_READ_TEMP:
		val = ioread32(&bp->osc->temp);
		err = 0;
		break;
	case MRO50_READ_CTRL:
		val = ioread32(&bp->osc->ctrl);
		err = 0;
		break;
	case MRO50_ADJUST_FINE:
		if (get_user(val, (u32 __user *)arg))
			return -EFAULT;
		return ptp_ocp_mro50_write(bp, MRO50_OP_ADJUST_FINE, val);
	case MRO50_ADJUST_COARSE:
		if (get_user(val, (u32 __user *)arg))
			return -EFAULT;
		return ptp_ocp_mro50_write(bp, MRO50_OP_ADJUST_COARSE, val);
	case MRO50_SAVE_COARSE:
		mutex_lock(&bp->mutex);
		iowrite32(MRO50_OP_SAVE_COARSE, &bp->osc->ctrl);
		mutex_unlock(&bp->mutex);
		return 0;
	default:
		return -ENOTTY;
	}

	if (!err && put_user(val, (int __user *)arg))
		err = -EFAULT;

	return err;
}

static const struct file_operations ptp_ocp_mro50_fops = {
	.owner =		THIS_MODULE,
	.unlocked_ioctl =	ptp_ocp_mro50_ioctl,
};

static void
ptp_ocp_unregister_mro50(struct miscdevice *mro50)
{
	struct ptp_ocp *bp = container_of(mro50, struct ptp_ocp, mro50);

	iowrite32(0, &bp->osc->ctrl);

	misc_deregister(mro50);
	kfree(mro50->name);
}

static int
ptp_ocp_register_mro50(struct ptp_ocp *bp)
{
	struct miscdevice *mro50 = &bp->mro50;
	char *name;
	int len;
	int err;

	len = strlen("mro50.X") + 1;

	name = kmalloc(len, GFP_KERNEL);
	if (!name)
		return -ENOMEM;
	snprintf(name, len, "mro50.%d", bp->id);

	mro50->minor = MISC_DYNAMIC_MINOR;
	mro50->fops = &ptp_ocp_mro50_fops;
	mro50->name = name;

	err = misc_register(mro50);
	if (err)
		goto out;

	iowrite32(MRO50_CTRL_ENABLE, &bp->osc->ctrl);

	return 0;

out:
	kfree(name);
	return err;
}

/* ART specific board initializers; last "resource" registered. */
static int
ptp_ocp_art_board_init(struct ptp_ocp *bp, struct ocp_resource *r)
{
	bp->flash_start = 0x1000000;
	bp->attr_groups = art_timecard_groups;

	return ptp_ocp_register_mro50(bp);
}

static bool
ptp_ocp_allow_irq(struct ptp_ocp *bp, struct ocp_resource *r)
{
	bool allow = !r->irq_vec || r->irq_vec < bp->n_irqs;

	if (!allow)
		dev_err(&bp->pdev->dev, "irq %d out of range, skipping %s\n",
			r->irq_vec, r->name);
	return allow;
}

static int
ptp_ocp_register_resources(struct ptp_ocp *bp, kernel_ulong_t driver_data)
{
	struct ocp_resource *r, *table;
	int err = 0;

	table = (struct ocp_resource *)driver_data;
	for (r = table; r->setup; r++) {
		if (!ptp_ocp_allow_irq(bp, r))
			continue;
		err = r->setup(bp, r);
		if (err)
			break;
	}
	return err;
}

static ssize_t
serialnum_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ptp_ocp *bp = dev_get_drvdata(dev);

	if (!bp->has_serial)
		ptp_ocp_get_serial_number(bp);

	return sysfs_emit(buf, "%pM\n", bp->serial);
}
static DEVICE_ATTR_RO(serialnum);

static ssize_t
gnss_sync_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ptp_ocp *bp = dev_get_drvdata(dev);
	ssize_t ret;

	if (bp->gnss_lost)
		ret = sysfs_emit(buf, "LOST @ %ptT\n", &bp->gnss_lost);
	else
		ret = sysfs_emit(buf, "SYNC\n");

	return ret;
}
static DEVICE_ATTR_RO(gnss_sync);

static ssize_t
sma_show_output(u32 val, char *buf, int default_idx)
{
	const char *name;

	name = select_name_from_val(ptp_ocp_sma_out, val);
	if (!name)
		name = ptp_ocp_sma_out[default_idx].name;
	return sysfs_emit(buf, "%s\n", name);
}

static ssize_t
sma_show_inputs(u32 val, char *buf)
{
	const char *name;
	ssize_t count;
	int i;

	count = 0;
	for (i = 0; i < ARRAY_SIZE(ptp_ocp_sma_in); i++) {
		if (val & ptp_ocp_sma_in[i].value) {
			name = ptp_ocp_sma_in[i].name;
			count += sysfs_emit_at(buf, count, "%s ", name);
		}
	}
	if (count)
		count--;
	count += sysfs_emit_at(buf, count, "\n");
	return count;
}

static int
sma_parse_inputs(const char *buf)
{
	int i, count;
	char **argv;
	int ret;

	argv = argv_split(GFP_KERNEL, buf, &count);
	if (!argv)
		return -ENOMEM;

	ret = -EINVAL;
	if (!count)
		goto out;

	ret = 0;
	for (i = 0; i < count; i++)
		ret |= select_val_from_name(ptp_ocp_sma_in, argv[i]);

out:
	argv_free(argv);
	return ret;
}

static ssize_t
sma1_out_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ptp_ocp *bp = dev_get_drvdata(dev);
	u32 val;

	val = ioread32(&bp->sma->gpio2) & 0x3f;
	return sma_show_output(val, buf, 0);
}

static ssize_t
sma2_out_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ptp_ocp *bp = dev_get_drvdata(dev);
	u32 val;

	val = (ioread32(&bp->sma->gpio2) >> 16) & 0x3f;
	return sma_show_output(val, buf, 1);
}

static void
__ctrl_irig_out(struct ptp_ocp *bp, u32 val)
{
	ptp_ocp_irig_out(bp, val & 0x00100010);
}

static ssize_t
sma1_out_store(struct device *dev, struct device_attribute *attr,
	      const char *buf, size_t count)
{
	struct ptp_ocp *bp = dev_get_drvdata(dev);
	unsigned long flags;
	u32 gpio;
	int val;

	val = select_val_from_name(ptp_ocp_sma_out, buf);
	if (val < 0)
		return val;

	spin_lock_irqsave(&bp->lock, flags);
	gpio = ioread32(&bp->sma->gpio2);
	gpio = (gpio & 0xffff0000) | val;
	__ctrl_irig_out(bp, gpio);
	iowrite32(gpio, &bp->sma->gpio2);
	spin_unlock_irqrestore(&bp->lock, flags);

	return count;
}
static DEVICE_ATTR_RW(sma1_out);

static ssize_t
sma2_out_store(struct device *dev, struct device_attribute *attr,
	      const char *buf, size_t count)
{
	struct ptp_ocp *bp = dev_get_drvdata(dev);
	unsigned long flags;
	u32 gpio;
	int val;

	val = select_val_from_name(ptp_ocp_sma_out, buf);
	if (val < 0)
		return val;

	spin_lock_irqsave(&bp->lock, flags);
	gpio = ioread32(&bp->sma->gpio2);
	gpio = (gpio & 0xffff) | (val << 16);
	__ctrl_irig_out(bp, gpio);
	iowrite32(gpio, &bp->sma->gpio2);
	spin_unlock_irqrestore(&bp->lock, flags);

	return count;
}
static DEVICE_ATTR_RW(sma2_out);

static ssize_t
sma3_in_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ptp_ocp *bp = dev_get_drvdata(dev);
	u32 val;

	val = ioread32(&bp->sma->gpio1) & 0x3f;
	if (val == 0)
		return sysfs_emit(buf, "%s\n", ptp_ocp_sma_in[0].name);
	return sma_show_inputs(val, buf);
}

static ssize_t
sma4_in_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ptp_ocp *bp = dev_get_drvdata(dev);
	u32 val;

	val = (ioread32(&bp->sma->gpio1) >> 16) & 0x3f;
	return sma_show_inputs(val, buf);
}

static ssize_t
sma3_in_store(struct device *dev, struct device_attribute *attr,
	      const char *buf, size_t count)
{
	struct ptp_ocp *bp = dev_get_drvdata(dev);
	unsigned long flags;
	u32 gpio;
	int val;

	val = sma_parse_inputs(buf);
	if (val < 0)
		return val;

	spin_lock_irqsave(&bp->lock, flags);
	gpio = ioread32(&bp->sma->gpio1);
	gpio = (gpio & 0xffff0000) | val;
	iowrite32(gpio, &bp->sma->gpio1);
	spin_unlock_irqrestore(&bp->lock, flags);

	return count;
}
static DEVICE_ATTR_RW(sma3_in);

static ssize_t
sma4_in_store(struct device *dev, struct device_attribute *attr,
	      const char *buf, size_t count)
{
	struct ptp_ocp *bp = dev_get_drvdata(dev);
	unsigned long flags;
	u32 gpio;
	int val;

	val = sma_parse_inputs(buf);
	if (val < 0)
		return val;

	spin_lock_irqsave(&bp->lock, flags);
	gpio = ioread32(&bp->sma->gpio1);
	gpio = (gpio & 0xffff) | (val << 16);
	iowrite32(gpio, &bp->sma->gpio1);
	spin_unlock_irqrestore(&bp->lock, flags);

	return count;
}
static DEVICE_ATTR_RW(sma4_in);

static ssize_t
available_sma_inputs_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	return select_table_show(ptp_ocp_sma_in, buf);
}
static DEVICE_ATTR_RO(available_sma_inputs);

static ssize_t
available_sma_outputs_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	return select_table_show(ptp_ocp_sma_out, buf);
}
static DEVICE_ATTR_RO(available_sma_outputs);

static ssize_t
external_pps_cable_delay_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct ptp_ocp *bp = dev_get_drvdata(dev);
	u32 val;

	val = ioread32(&bp->pps_to_ext->cable_delay);
	return sysfs_emit(buf, "%d\n", val);
}

static ssize_t
external_pps_cable_delay_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct ptp_ocp *bp = dev_get_drvdata(dev);
	unsigned long flags;
	int err;
	u16 val;

	err = kstrtou16(buf, 0, &val);
	if (err)
		return err;

	spin_lock_irqsave(&bp->lock, flags);
	iowrite32(val, &bp->pps_to_ext->cable_delay);
	spin_unlock_irqrestore(&bp->lock, flags);

	return count;
}
static DEVICE_ATTR_RW(external_pps_cable_delay);

static ssize_t
internal_pps_cable_delay_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct ptp_ocp *bp = dev_get_drvdata(dev);
	u32 val;

	val = ioread32(&bp->pps_to_clk->cable_delay);
	return sysfs_emit(buf, "%d\n", val);
}

static ssize_t
internal_pps_cable_delay_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct ptp_ocp *bp = dev_get_drvdata(dev);
	unsigned long flags;
	int err;
	u16 val;

	err = kstrtou16(buf, 0, &val);
	if (err)
		return err;

	spin_lock_irqsave(&bp->lock, flags);
	iowrite32(val, &bp->pps_to_clk->cable_delay);
	spin_unlock_irqrestore(&bp->lock, flags);

	return count;
}
static DEVICE_ATTR_RW(internal_pps_cable_delay);

static ssize_t
clock_source_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ptp_ocp *bp = dev_get_drvdata(dev);
	const char *p;
	u32 select;

	select = ioread32(&bp->reg->select);
	p = select_name_from_val(ptp_ocp_clock, select >> 16);

	return sysfs_emit(buf, "%s\n", p);
}

static ssize_t
clock_source_store(struct device *dev, struct device_attribute *attr,
		   const char *buf, size_t count)
{
	struct ptp_ocp *bp = dev_get_drvdata(dev);
	unsigned long flags;
	int val;

	val = select_val_from_name(ptp_ocp_clock, buf);
	if (val < 0)
		return val;

	spin_lock_irqsave(&bp->lock, flags);
	iowrite32(val, &bp->reg->select);
	spin_unlock_irqrestore(&bp->lock, flags);

	return count;
}
static DEVICE_ATTR_RW(clock_source);

static ssize_t
available_clock_sources_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	return select_table_show(ptp_ocp_clock, buf);
}
static DEVICE_ATTR_RO(available_clock_sources);

static struct attribute *fb_timecard_attrs[] = {
	&dev_attr_serialnum.attr,
	&dev_attr_gnss_sync.attr,
	&dev_attr_clock_source.attr,
	&dev_attr_available_clock_sources.attr,
	&dev_attr_external_pps_cable_delay.attr,
	&dev_attr_internal_pps_cable_delay.attr,
	&dev_attr_sma1_out.attr,
	&dev_attr_sma2_out.attr,
	&dev_attr_sma3_in.attr,
	&dev_attr_sma4_in.attr,
	&dev_attr_available_sma_inputs.attr,
	&dev_attr_available_sma_outputs.attr,
	NULL,
};
ATTRIBUTE_GROUPS(fb_timecard);

static struct attribute *art_timecard_attrs[] = {
	&dev_attr_serialnum.attr,
	NULL,
};
ATTRIBUTE_GROUPS(art_timecard);

static void
ptp_ocp_dev_release(struct device *dev)
{
	struct ptp_ocp *bp = dev_get_drvdata(dev);

	mutex_lock(&ptp_ocp_lock);
	idr_remove(&ptp_ocp_idr, bp->id);
	mutex_unlock(&ptp_ocp_lock);
}

static int
ptp_ocp_device_init(struct ptp_ocp *bp, struct pci_dev *pdev)
{
	int err;

	mutex_lock(&ptp_ocp_lock);
	err = idr_alloc(&ptp_ocp_idr, bp, 0, 0, GFP_KERNEL);
	mutex_unlock(&ptp_ocp_lock);
	if (err < 0) {
		dev_err(&pdev->dev, "idr_alloc failed: %d\n", err);
		return err;
	}
	bp->id = err;

	bp->ptp_info = ptp_ocp_clock_info;
	spin_lock_init(&bp->lock);
	mutex_init(&bp->mutex);
	bp->gnss_port = -1;
	bp->mac_port = -1;
	bp->pdev = pdev;

	device_initialize(&bp->dev);
	dev_set_name(&bp->dev, "ocp%d", bp->id);
	bp->dev.class = &timecard_class;
	bp->dev.parent = &pdev->dev;
	bp->dev.release = ptp_ocp_dev_release;
	dev_set_drvdata(&bp->dev, bp);

	err = device_add(&bp->dev);
	if (err) {
		dev_err(&bp->dev, "device add failed: %d\n", err);
		goto out;
	}

	pci_set_drvdata(pdev, bp);

	return 0;

out:
	ptp_ocp_dev_release(&bp->dev);
	put_device(&bp->dev);
	return err;
}

static void
ptp_ocp_symlink(struct ptp_ocp *bp, struct device *child, const char *link)
{
	struct device *dev = &bp->dev;

	if (sysfs_create_link(&dev->kobj, &child->kobj, link))
		dev_err(dev, "%s symlink failed\n", link);
}

static void
ptp_ocp_link_child(struct ptp_ocp *bp, const char *name, const char *link)
{
	struct device *dev, *child;

	dev = &bp->pdev->dev;

	child = device_find_child_by_name(dev, name);
	if (!child) {
		dev_err(dev, "Could not find device %s\n", name);
		return;
	}

	ptp_ocp_symlink(bp, child, link);
	put_device(child);
}

static int
ptp_ocp_complete(struct ptp_ocp *bp)
{
	struct pps_device *pps;
	char buf[32];

	if (bp->gnss_port != -1) {
		sprintf(buf, "ttyS%d", bp->gnss_port);
		ptp_ocp_link_child(bp, buf, "ttyGNSS");
	}
	if (bp->mac_port != -1) {
		sprintf(buf, "ttyS%d", bp->mac_port);
		ptp_ocp_link_child(bp, buf, "ttyMAC");
	}
	sprintf(buf, "ptp%d", ptp_clock_index(bp->ptp));
	ptp_ocp_link_child(bp, buf, "ptp");

	pps = pps_lookup_dev(bp->ptp);
	if (pps)
		ptp_ocp_symlink(bp, pps->dev, "pps");

	if (device_add_groups(&bp->dev, bp->attr_groups))
		pr_err("device add groups failed\n");

	return 0;
}

static void
ptp_ocp_resource_summary(struct ptp_ocp *bp)
{
	struct device *dev = &bp->pdev->dev;

	if (bp->image) {
		u32 ver = ioread32(&bp->image->version);

		dev_info(dev, "version %x\n", ver);
		if (ver & 0xffff)
			dev_info(dev, "regular image, version %d\n",
				 ver & 0xffff);
		else
			dev_info(dev, "golden image, version %d\n",
				 ver >> 16);
	}
	if (bp->gnss_port != -1)
		dev_info(dev, "GNSS @ /dev/ttyS%d 115200\n", bp->gnss_port);
	if (bp->mac_port != -1)
		dev_info(dev, "MAC @ /dev/ttyS%d   57600\n", bp->mac_port);
}

static void
ptp_ocp_detach_sysfs(struct ptp_ocp *bp)
{
	struct device *dev = &bp->dev;

	sysfs_remove_link(&dev->kobj, "ttyGNSS");
	sysfs_remove_link(&dev->kobj, "ttyMAC");
	sysfs_remove_link(&dev->kobj, "ptp");
	sysfs_remove_link(&dev->kobj, "pps");
	device_remove_groups(dev, bp->attr_groups);
}

static void
ptp_ocp_detach(struct ptp_ocp *bp)
{
	ptp_ocp_detach_sysfs(bp);
	if (timer_pending(&bp->watchdog))
		del_timer_sync(&bp->watchdog);
	if (bp->ts0)
		ptp_ocp_unregister_ext(bp->ts0);
	if (bp->ts1)
		ptp_ocp_unregister_ext(bp->ts1);
	if (bp->ts2)
		ptp_ocp_unregister_ext(bp->ts2);
	if (bp->pps)
		ptp_ocp_unregister_ext(bp->pps);
	if (bp->gnss_port != -1)
		serial8250_unregister_port(bp->gnss_port);
	if (bp->mac_port != -1)
		serial8250_unregister_port(bp->mac_port);
	if (bp->spi_flash)
		platform_device_unregister(bp->spi_flash);
	if (bp->i2c_ctrl)
		platform_device_unregister(bp->i2c_ctrl);
	if (bp->i2c_clk)
		clk_hw_unregister_fixed_rate(bp->i2c_clk);
	if (bp->mro50.name)
		ptp_ocp_unregister_mro50(&bp->mro50);
	if (bp->n_irqs)
		pci_free_irq_vectors(bp->pdev);
	if (bp->ptp)
		ptp_clock_unregister(bp->ptp);
	device_unregister(&bp->dev);
}

/* XXX out of tree hack - if this symbol is defined, kernel is using new API */
#ifdef DEVLINK_PARAM_GENERIC_ENABLE_ETH_NAME
#define DEVLINK_NEW_API 1
#endif

static inline struct devlink *
ptp_ocp_devlink_alloc(const struct devlink_ops *ops, size_t priv_size,
		      struct device *dev)
{
#ifdef DEVLINK_NEW_API
	return devlink_alloc(ops, priv_size, dev);
#else
	return devlink_alloc(ops, priv_size);
#endif
}

static inline int
ptp_ocp_devlink_register(struct devlink *devlink, struct device *dev)
{
#ifdef DEVLINK_NEW_API
	return devlink_register(devlink);
#else
	return devlink_register(devlink, dev);
#endif
}

static int
ptp_ocp_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct devlink *devlink;
	struct ptp_ocp *bp;
	int err;

	devlink = ptp_ocp_devlink_alloc(&ptp_ocp_devlink_ops, sizeof(*bp),
					&pdev->dev);
	if (!devlink) {
		dev_err(&pdev->dev, "devlink_alloc failed\n");
		return -ENOMEM;
	}

	err = ptp_ocp_devlink_register(devlink, &pdev->dev);
	if (err)
		goto out_free;

	err = pci_enable_device(pdev);
	if (err) {
		dev_err(&pdev->dev, "pci_enable_device\n");
		goto out_unregister;
	}

	bp = devlink_priv(devlink);
	err = ptp_ocp_device_init(bp, pdev);
	if (err)
		goto out_disable;

	/* compat mode.
	 * Older FPGA firmware only returns 2 irq's.
	 * allow this - if not all of the IRQ's are returned, skip the
	 * extra devices and just register the clock.
	 */
	err = pci_alloc_irq_vectors(pdev, 1, 16, PCI_IRQ_MSI | PCI_IRQ_MSIX);
	if (err < 0) {
		dev_err(&pdev->dev, "alloc_irq_vectors err: %d\n", err);
		goto out;
	}
	bp->n_irqs = err;
	pci_set_master(pdev);

	err = ptp_ocp_register_resources(bp, id->driver_data);
	if (err)
		goto out;

	bp->ptp = ptp_clock_register(&bp->ptp_info, &pdev->dev);
	if (IS_ERR(bp->ptp)) {
		err = PTR_ERR(bp->ptp);
		dev_err(&pdev->dev, "ptp_clock_register: %d\n", err);
		bp->ptp = NULL;
		goto out;
	}

	err = ptp_ocp_complete(bp);
	if (err)
		goto out;

	ptp_ocp_info(bp);
	ptp_ocp_resource_summary(bp);

	return 0;

out:
	ptp_ocp_detach(bp);
	pci_set_drvdata(pdev, NULL);
out_disable:
	pci_disable_device(pdev);
out_unregister:
	devlink_unregister(devlink);
out_free:
	devlink_free(devlink);

	return err;
}

static void
ptp_ocp_remove(struct pci_dev *pdev)
{
	struct ptp_ocp *bp = pci_get_drvdata(pdev);
	struct devlink *devlink = priv_to_devlink(bp);

	ptp_ocp_detach(bp);
	pci_set_drvdata(pdev, NULL);
	pci_disable_device(pdev);

	devlink_unregister(devlink);
	devlink_free(devlink);
}

static struct pci_driver ptp_ocp_driver = {
	.name		= KBUILD_MODNAME,
	.id_table	= ptp_ocp_pcidev_id,
	.probe		= ptp_ocp_probe,
	.remove		= ptp_ocp_remove,
};

static int
ptp_ocp_i2c_notifier_call(struct notifier_block *nb,
			  unsigned long action, void *data)
{
	struct device *dev, *child = data;
	struct ptp_ocp *bp;
	bool add;

	switch (action) {
	case BUS_NOTIFY_ADD_DEVICE:
	case BUS_NOTIFY_DEL_DEVICE:
		add = action == BUS_NOTIFY_ADD_DEVICE;
		break;
	default:
		return 0;
	}

	if (!i2c_verify_adapter(child))
		return 0;

	dev = child;
	while ((dev = dev->parent))
		if (dev->driver && !strcmp(dev->driver->name, KBUILD_MODNAME))
			goto found;
	return 0;

found:
	bp = dev_get_drvdata(dev);
	if (add)
		ptp_ocp_symlink(bp, child, "i2c");
	else
		sysfs_remove_link(&bp->dev.kobj, "i2c");

	return 0;
}

static struct notifier_block ptp_ocp_i2c_notifier = {
	.notifier_call = ptp_ocp_i2c_notifier_call,
};

static int __init
ptp_ocp_init(void)
{
	const char *what;
	int err;

	what = "timecard class";
	err = class_register(&timecard_class);
	if (err)
		goto out;

	what = "i2c notifier";
	err = bus_register_notifier(&i2c_bus_type, &ptp_ocp_i2c_notifier);
	if (err)
		goto out_notifier;

	what = "ptp_ocp driver";
	err = pci_register_driver(&ptp_ocp_driver);
	if (err)
		goto out_register;

	return 0;

out_register:
	bus_unregister_notifier(&i2c_bus_type, &ptp_ocp_i2c_notifier);
out_notifier:
	class_unregister(&timecard_class);
out:
	pr_err(KBUILD_MODNAME ": failed to register %s: %d\n", what, err);
	return err;
}

static void __exit
ptp_ocp_fini(void)
{
	bus_unregister_notifier(&i2c_bus_type, &ptp_ocp_i2c_notifier);
	pci_unregister_driver(&ptp_ocp_driver);
	class_unregister(&timecard_class);
}

module_init(ptp_ocp_init);
module_exit(ptp_ocp_fini);

MODULE_DESCRIPTION("OpenCompute TimeCard driver");
MODULE_LICENSE("GPL v2");
