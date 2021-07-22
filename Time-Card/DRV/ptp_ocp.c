#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/serial_8250.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/platform_data/i2c-ocores.h>
#include <linux/platform_device.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/spi/spi.h>
#include <linux/spi/xilinx_spi.h>
#include <linux/spi/altera.h>
#include <net/devlink.h>
#include <linux/i2c.h>
#include <linux/mtd/mtd.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>

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
};

#define PPS_STATUS_FILTER_ERR	BIT(0)
#define PPS_STATUS_SUPERV_ERR	BIT(1)

struct img_reg {
	u32	version;
};

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
	const char *name;
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
	struct ocp_resource	*res_tbl;
	spinlock_t		lock;
	struct mutex		mutex;
	struct ocp_reg __iomem	*reg;
	struct tod_reg __iomem	*tod;
	struct pps_reg __iomem	*pps_monitor;
	struct ptp_ocp_ext_src	*pps;
	struct ptp_ocp_ext_src	*phasemeter;
	struct ptp_ocp_ext_src	*ts0;
	struct ptp_ocp_ext_src	*ts1;
	struct ocp_art_osc_reg __iomem	*osc;
	struct img_reg __iomem	*image;
	struct ptp_clock	*ptp;
	struct ptp_clock_info	ptp_info;
	struct platform_device	*i2c_osc;
	struct platform_device	*spi_imu;
	struct platform_device	*spi_flash;
	struct platform_device	*i2c_flash;
	struct proc_dir_entry	*i2c_pde;
	struct clk_hw		*i2c_clk;
	struct devlink_health_reporter *health;
	struct proc_dir_entry	*proc;
	struct timer_list	watchdog;
	struct miscdevice	mro50;
	time64_t		gps_lost;
	int			id;
	int			n_irqs;
	int			gps_port;
	int			mac_port;	/* miniature atomic clock */
	u8			serial[6];
	int			flash_start;
	bool			has_serial;
	bool			pending_image;
};

struct ocp_resource {
	unsigned long offset;
	int size;
	int irq_vec;
	int (*setup)(struct ptp_ocp *bp, struct ocp_resource *r);
	void *extra;
	unsigned long bp_offset;
};

static void ptp_ocp_health_update(struct ptp_ocp *bp);
static int ptp_ocp_register_mem(struct ptp_ocp *bp, struct ocp_resource *r);
static int ptp_ocp_register_i2c(struct ptp_ocp *bp, struct ocp_resource *r);
static int ptp_ocp_register_spi(struct ptp_ocp *bp, struct ocp_resource *r);
static int ptp_ocp_register_serial(struct ptp_ocp *bp, struct ocp_resource *r);
static int ptp_ocp_register_ext(struct ptp_ocp *bp, struct ocp_resource *r);
static int ptp_ocp_fb_board_init(struct ptp_ocp *bp, struct ocp_resource *r);
static int ptp_ocp_art_board_init(struct ptp_ocp *bp, struct ocp_resource *r);
static irqreturn_t ptp_ocp_ts_irq(int irq, void *priv);
static irqreturn_t ptp_ocp_phase_irq(int irq, void *priv);
static irqreturn_t ptp_ocp_pps_irq(int irq, void *priv);
static int ptp_ocp_ts_enable(void *priv, bool enable);
static int ptp_ocp_phase_enable(void *priv, bool enable);
static int ptp_ocp_pps_enable(void *priv, bool enable);

#define bp_assign_entry(bp, res, val) ({				\
	uintptr_t addr = (uintptr_t)(bp) + (res)->bp_offset;		\
	*(typeof(val) *)addr = val;					\
})

#define OCP_RES_LOCATION(member) \
	.bp_offset = offsetof(struct ptp_ocp, member)

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
 * 6: SPI IMU (inertial measurement unit)
 * 7: I2C oscillator
 * 8: HWICAP
 * 9: SPI Flash
 * 10: phasemeter
 * 11: PPS
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
			.name = "ts0", .index = 1,
			.irq_fcn = ptp_ocp_ts_irq,
			.enable = ptp_ocp_ts_enable,
		},
	},
	{
		OCP_EXT_RESOURCE(ts1),
		.offset = 0x01020000, .size = 0x10000, .irq_vec = 2,
		.extra = &(struct ptp_ocp_ext_info) {
			.name = "ts1", .index = 2,
			.irq_fcn = ptp_ocp_ts_irq,
			.enable = ptp_ocp_ts_enable,
		},
	},
	{
		OCP_MEM_RESOURCE(pps_monitor),
		.offset = 0x01040000, .size = 0x10000,
	},
	{
		OCP_MEM_RESOURCE(tod),
		.offset = 0x01050000, .size = 0x10000,
	},
	{
		OCP_MEM_RESOURCE(image),
		.offset = 0x00020000, .size = 0x1000,
	},
#if 0
	{
		OCP_SPI_RESOURCE(spi_imu),
		.offset = 0x00140000, .size = 0x10000, .irq_vec = 6,
		/* IMU is a BNO085, needs out of tree linux driver BNO055 */
	},
#endif
	{
		OCP_I2C_RESOURCE(i2c_osc),
		.offset = 0x00150000, .size = 0x10000, .irq_vec = 7,
		.extra = &(struct ptp_ocp_i2c_info) {
			.name = "xiic-i2c",
			.fixed_rate = 500000,
		},
	},
	{
		OCP_SERIAL_RESOURCE(gps_port),
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
	u32	__pad0[3];
	u32	intr;
};

struct ocp_phase_reg {
	u32	phase_error;
	u32	phase_offset;
	u32	enable;
	u32	__pad1[7];
	u32	intr;
};

static struct ocp_resource ocp_art_resource[] = {
	{
		OCP_MEM_RESOURCE(reg),
		.offset = 0x01000000, .size = 0x10000,
	},
	{
		OCP_SERIAL_RESOURCE(gps_port),
		.offset = 0x00160000 + 0x1000, .irq_vec = 3,
	},
	{
		OCP_EXT_RESOURCE(phasemeter),
		.offset = 0x00320000, .size = 0x30, .irq_vec = 10,
		.extra = &(struct ptp_ocp_ext_info) {
			.name = "phasemeter", .index = 0,
			.irq_fcn = ptp_ocp_phase_irq,
			.enable = ptp_ocp_phase_enable,
		},
	},
	{
		OCP_EXT_RESOURCE(pps),
		.offset = 0x00330000, .size = 0x20, .irq_vec = 11,
		.extra = &(struct ptp_ocp_ext_info) {
			.name = "pps",
			.irq_fcn = ptp_ocp_pps_irq,
			.enable = ptp_ocp_pps_enable,
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
		OCP_I2C_RESOURCE(i2c_flash),
		.offset = 0x350000, .size = 0x100, .irq_vec = 4,
		.extra = &(struct ptp_ocp_i2c_info) {
			.name = "ocores-i2c",
			.fixed_rate = 400000,
			.data_size = sizeof(struct ocores_i2c_platform_data),
			.data = &(struct ocores_i2c_platform_data) {
				.clock_khz = 125000,
				.bus_khz = 400,
				.num_devices = 1,
				.devices = &(struct i2c_board_info) {
					.addr = 0x50,
					.type = "24c08",
				},
			},
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

static struct {
	const char *name;
	int value;
} ptp_ocp_clock[] = {
	{ .name = "NONE",	.value = 0 },
	{ .name = "TOD",	.value = 1 },
	{ .name = "IRIG",	.value = 2 },
	{ .name = "PPS",	.value = 3 },
	{ .name = "PTP",	.value = 4 },
	{ .name = "RTC",	.value = 5 },
	{ .name = "DCF",	.value = 6 },
	{ .name = "REGS",	.value = 0xfe },
	{ .name = "EXT",	.value = 0xff },
};

static const char *
ptp_ocp_clock_name_from_val(int val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ptp_ocp_clock); i++)
		if (ptp_ocp_clock[i].value == val)
			return ptp_ocp_clock[i].name;
	return NULL;
}

static int
ptp_ocp_clock_val_from_name(const char *name)
{
	const char *clk;
	int i;

	for (i = 0; i < ARRAY_SIZE(ptp_ocp_clock); i++) {
		clk = ptp_ocp_clock[i].name;
		if (!strncasecmp(name, clk, strlen(clk)))
			return ptp_ocp_clock[i].value;
	}
	return -EINVAL;
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

	dev_info(&bp->pdev->dev, "settime to: %lld.%ld\n",
		 ts->tv_sec, ts->tv_nsec);

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

	dev_info(&bp->pdev->dev, "adjtime , adjust by: %lld\n", delta_ns);

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
	struct ptp_ocp *bp = container_of(ptp_info, struct ptp_ocp, ptp_info);

	if (scaled_ppm == 0)
		return 0;

	dev_info(&bp->pdev->dev, "adjfine, scaled by: %ld\n", scaled_ppm);

	return -EOPNOTSUPP;
}

static int
ptp_ocp_adjphase(struct ptp_clock_info *ptp_info, s32 phase_ns)
{
	struct ptp_ocp *bp = container_of(ptp_info, struct ptp_ocp, ptp_info);
	struct ocp_phase_reg __iomem *reg = bp->phasemeter->mem;
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
			ext = bp->phasemeter;
			break;
		case 1:
			ext = bp->ts0;
			break;
		case 2:
			ext = bp->ts1;
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

	status = ioread32(&bp->pps_monitor->status);

	if (status & PPS_STATUS_SUPERV_ERR) {
		iowrite32(status, &bp->pps_monitor->status);
		if (!bp->gps_lost) {
			spin_lock_irqsave(&bp->lock, flags);
			__ptp_ocp_clear_drift_locked(bp);
			spin_unlock_irqrestore(&bp->lock, flags);
			bp->gps_lost = ktime_get_real_seconds();
			ptp_ocp_health_update(bp);
		}

	} else if (bp->gps_lost) {
		bp->gps_lost = 0;
		ptp_ocp_health_update(bp);
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
	static const char *proto_name[] = {
		"NMEA", "NMEA_ZDA", "NMEA_RMC", "NMEA_none",
		"UBX", "UBX_UTC", "UBX_LS", "UBX_none"
	};
	static const char *gnss_name[] = {
		"ALL", "COMBINED", "GPS", "GLONASS", "GALILEO", "BEIDOU",
	};
	u32 version, ctrl, reg;
	int idx;

	version = ioread32(&bp->tod->version);
	dev_info(&bp->pdev->dev, "TOD Version %d.%d.%d\n",
		version >> 24, (version >> 16) & 0xff, version & 0xffff);

#if 1
	ctrl = ioread32(&bp->tod->ctrl);
	ctrl |= TOD_CTRL_PROTOCOL | TOD_CTRL_ENABLE;
	ctrl &= ~(TOD_CTRL_DISABLE_FMT_A | TOD_CTRL_DISABLE_FMT_B);
	iowrite32(ctrl, &bp->tod->ctrl);
#endif

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

	dev = device_find_child(&bp->i2c_osc->dev, NULL, ptp_ocp_firstchild);
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
		ptp_ocp_clock_name_from_val(select >> 16),
		ptp_clock_index(bp->ptp));

	if (bp->tod)
		ptp_ocp_tod_info(bp);
}

static const struct devlink_param ptp_ocp_devlink_params[] = {
};

static void
ptp_ocp_devlink_set_params_init_values(struct devlink *devlink)
{
}

static int
ptp_ocp_devlink_register(struct devlink *devlink, struct device *dev)
{
	int err;

	err = devlink_register(devlink, dev);
	if (err)
		return err;

	err = devlink_params_register(devlink, ptp_ocp_devlink_params,
				      ARRAY_SIZE(ptp_ocp_devlink_params));
	ptp_ocp_devlink_set_params_init_values(devlink);
	if (err)
		goto out;
	devlink_params_publish(devlink);

	return 0;

out:
	devlink_unregister(devlink);
	return err;
}

static void
ptp_ocp_devlink_unregister(struct devlink *devlink)
{
	devlink_params_unregister(devlink, ptp_ocp_devlink_params,
				  ARRAY_SIZE(ptp_ocp_devlink_params));
	devlink_unregister(devlink);
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
	int err;

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

	bp->pending_image = true;

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

	if (bp->pending_image) {
		err = devlink_info_version_stored_put(req,
						      "timecard", "pending");
		if (err)
			return err;
	}

	if (bp->image) {
		u32 ver = bp->image->version;
		if (ver & 0xffff) {
			sprintf(buf, "%d", ver);
			err = devlink_info_version_running_put(req,
					"timecard", buf);
		} else {
			sprintf(buf, "%d", ver >> 16);
			err = devlink_info_version_running_put(req,
					"golden flash", buf);
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

static int
ptp_ocp_health_diagnose(struct devlink_health_reporter *reporter,
			struct devlink_fmsg *fmsg,
			struct netlink_ext_ack *extack)
{
	struct ptp_ocp *bp = devlink_health_reporter_priv(reporter);
	char buf[32];
	int err;

	if (!bp->gps_lost)
		return 0;

	sprintf(buf, "%ptT", &bp->gps_lost);
	err = devlink_fmsg_string_pair_put(fmsg, "Lost sync at", buf);
	if (err)
		return err;

	return 0;
}

static void
ptp_ocp_health_update(struct ptp_ocp *bp)
{
	int state;

	state = bp->gps_lost ? DEVLINK_HEALTH_REPORTER_STATE_ERROR
			     : DEVLINK_HEALTH_REPORTER_STATE_HEALTHY;

	if (bp->gps_lost)
		devlink_health_report(bp->health, "No GPS signal", NULL);

	devlink_health_reporter_state_update(bp->health, state);
}

static const struct devlink_health_reporter_ops ptp_ocp_health_ops = {
	.name = "gps_sync",
	.diagnose = ptp_ocp_health_diagnose,
};

static void
ptp_ocp_devlink_health_register(struct devlink *devlink)
{
	struct ptp_ocp *bp = devlink_priv(devlink);
	struct devlink_health_reporter *r;

	r = devlink_health_reporter_create(devlink, &ptp_ocp_health_ops, 0, bp);
	if (IS_ERR(r))
		dev_err(&bp->pdev->dev, "Failed to create reporter, err %ld\n",
			PTR_ERR(r));
	bp->health = r;
}

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

	/* XXX hack to work around old FPGA */
	if (bp->n_irqs < 10) {
		dev_err(&bp->pdev->dev, "FPGA does not have SPI devices\n");
		return 0;
	}

	if (r->irq_vec > bp->n_irqs) {
		dev_err(&bp->pdev->dev, "spi device irq %d out of range\n",
			r->irq_vec);
		return 0;
	}

	start = pci_resource_start(pdev, 0) + r->offset;
	ptp_ocp_set_mem_resource(&res[0], start, r->size);
	ptp_ocp_set_irq_resource(&res[1], pci_irq_vector(pdev, r->irq_vec));

	info = r->extra;
	id = pci_dev_id(pdev) << 1;
	id += info->pci_offset;

	p = platform_device_register_resndata(&pdev->dev,
		info->name, id, res, 2, info->data, info->data_size);
	if (IS_ERR(p))
		return PTR_ERR(p);

	bp_assign_entry(bp, r, p);

	return 0;
}

/* XXX
 * Can set platform data (xiic_i2c_platform_data),
 *   which lists the devices on the I2C bus; these are added at reg time.
 *
 * Expected devices:
 *  OSC:
 *    0x30: unknown, returns all 0xff
 *    0x50: unknown, returns all 0xff
 *    0x58: extended eeprom.  Looks like OUI data at addr 0x9A; so 24mac402 ?
 *	    can only read 2 bytes at a time.  timeout otherwise.
 */
static struct platform_device *
ptp_ocp_i2c_bus(struct pci_dev *pdev, struct ocp_resource *r, int id)
{
	struct resource res[2];
	unsigned long start;
	struct ptp_ocp_i2c_info *info;

	info = r->extra;
	start = pci_resource_start(pdev, 0) + r->offset;
	ptp_ocp_set_mem_resource(&res[0], start, r->size);
	ptp_ocp_set_irq_resource(&res[1], pci_irq_vector(pdev, r->irq_vec));

	return platform_device_register_resndata(&pdev->dev, info->name,
						 id, res, 2, info->data, info->data_size);
}

static int
ptp_ocp_register_i2c(struct ptp_ocp *bp, struct ocp_resource *r)
{
	struct pci_dev *pdev = bp->pdev;
	struct platform_device *p;
	struct clk_hw *clk;
	struct ptp_ocp_i2c_info *info;
	char buf[32];
	int id;

	info = r->extra;
	if (!info) {
		return -EINVAL;
	}

	if (r->irq_vec > bp->n_irqs) {
		dev_err(&bp->pdev->dev, "i2c device irq %d out of range\n",
			r->irq_vec);
		return 0;
	}

	id = pci_dev_id(bp->pdev);

	sprintf(buf, "AXI.%d", id);
	clk = clk_hw_register_fixed_rate(&pdev->dev, buf, NULL, 0, info->fixed_rate);
	if (IS_ERR(clk))
		return PTR_ERR(clk);
	bp->i2c_clk = clk;

	sprintf(buf, "xiic-i2c.%d", id);
	devm_clk_hw_register_clkdev(&pdev->dev, clk, NULL, buf);
	p = ptp_ocp_i2c_bus(bp->pdev, r, id);
	if (IS_ERR(p))
		return PTR_ERR(p);

	bp_assign_entry(bp, r, p);

	return 0;
}

static irqreturn_t
ptp_ocp_phase_irq(int irq, void *priv)
{
	struct ptp_ocp_ext_src *ext = priv;
	struct ocp_phase_reg __iomem *reg = ext->mem;
	struct ptp_clock_event ev;
	s32 phase_error;

	phase_error = ioread32(&reg->phase_error);
	if (phase_error > 500000000) {
		iowrite32(0, &reg->phase_error);
		phase_error = 0;
	}


#ifdef PTP_CLOCK_EXTTSUSR
	ev.type = PTP_CLOCK_EXTTSUSR;
	ev.index = ext->info->index;
	ev.data = phase_error;
	pps_get_ts(&ev.pps_times);
#else
	ev.type = PTP_CLOCK_EXTTS;
	ev.index = ext->info->index;
	ev.timestamp = phase_error;
#endif

	ptp_clock_event(ext->bp->ptp, &ev);

	iowrite32(0, &reg->intr);

	return IRQ_HANDLED;
}

static int
ptp_ocp_phase_enable(void *priv, bool enable)
{
	struct ptp_ocp_ext_src *ext = priv;
	struct ocp_phase_reg __iomem *reg = ext->mem;

	iowrite32(enable, &reg->enable);

	return 0;
}

static irqreturn_t
ptp_ocp_pps_irq(int irq, void *priv)
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
ptp_ocp_pps_enable(void *priv, bool enable)
{
	struct ptp_ocp_ext_src *ext = priv;
	struct ocp_art_pps_reg __iomem *reg = ext->mem;

	iowrite32(enable, &reg->enable);

	return 0;
}

static irqreturn_t
ptp_ocp_ts_irq(int irq, void *priv)
{
	struct ptp_ocp_ext_src *ext = priv;
	struct ts_reg __iomem *reg = ext->mem;
	struct ptp_clock_event ev;

	/* XXX "should not happen", but is happening on jlemon's box */
	if (!ioread32(&reg->intr)) {
		dev_err(&ext->bp->pdev->dev,
			"%s: ocp%d.%s irq %d when not set\n",
			__func__, ext->bp->id, ext->info->name, irq);
		return IRQ_HANDLED;
	}

#ifdef PTP_CLOCK_EXTTSUSR
	ev.type = PTP_CLOCK_EXTTSUSR;
	ev.index = ext->info->index;
	ev.pps_times.ts_real.tv_sec = ioread32(&reg->time_sec);
	ev.pps_times.ts_real.tv_nsec = ioread32(&reg->time_ns);
	ev.data = ioread32(&reg->ts_count);
#else
	ev.type = PTP_CLOCK_EXTTS;
	ev.index = ext->info->index;
	ev.pps_times.ts_real.tv_sec = ioread32(&reg->time_sec);
	ev.pps_times.ts_real.tv_nsec = ioread32(&reg->time_ns);
#endif

	ptp_clock_event(ext->bp->ptp, &ev);

{
	u32 en, mask, intr;
	en = ioread32(&reg->enable);
	mask = ioread32(&reg->intr_mask);
	intr = ioread32(&reg->intr);
	pr_err("EXT IRQ: %d ocp%d.%s en:%d mask:%d intr:%d\n",
		irq, ext->bp->id, ext->info->name, en, mask, intr);
}

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

{
	u32 en, mask, intr;
	en = ioread32(&reg->enable);
	mask = ioread32(&reg->intr_mask);
	intr = ioread32(&reg->intr);
	pr_err("EXT SET: %s ocp%d.%s en:%d mask:%d intr:%d\n",
		enable ? "ON " : "OFF",
		ext->bp->id, ext->info->name, en, mask, intr);
}

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

	err = pci_request_irq(pdev, r->irq_vec, ext->info->irq_fcn, 0,
			      ext, "ocp%d.%s", bp->id, ext->info->name);
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
	 * the the serial port device claim and release the pci resource.
	 */
	memset(&uart, 0, sizeof(uart));
	uart.port.dev = &pdev->dev;
	uart.port.iotype = UPIO_MEM;
	uart.port.regshift = 2;
	uart.port.mapbase = pci_resource_start(pdev, 0) + r->offset;
//	uart.port.membase = bp->base + r->offset;
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

	if (r->irq_vec > bp->n_irqs) {
		dev_err(&bp->pdev->dev, "serial device irq %d out of range\n",
			r->irq_vec);
		return 0;
	}

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

	return ptp_ocp_init_clock(bp);
}

static int
__ptp_ocp_mro50_wait_cmd(struct ocp_art_osc_reg *reg, u32 done)
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
__ptp_ocp_mro50_write_locked(struct ocp_art_osc_reg *reg, u32 ctrl, u32 val)
{
	iowrite32(val, &reg->adjust);
	iowrite32(ctrl, &reg->ctrl);

	return __ptp_ocp_mro50_wait_cmd(reg, MRO50_CTRL_ADJUST_CMD);
}

static int
__ptp_ocp_mro50_read_locked(struct ocp_art_osc_reg *reg, u32 ctrl, u32 *val)
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

const struct file_operations ptp_ocp_mro50_fops = {
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
	return ptp_ocp_register_mro50(bp);
}

static int
ptp_ocp_register_resources(struct ptp_ocp *bp)
{
	struct ocp_resource *r;
	int err = 0;

	for (r = bp->res_tbl; r->setup; r++) {
		err = r->setup(bp, r);
		if (err)
			break;
	}
	return err;
}

static int
ptp_ocp_proc_serial(struct seq_file *seq, void *v)
{
	struct ptp_ocp *bp = seq->private;

	if (!bp->has_serial)
		ptp_ocp_get_serial_number(bp);

	seq_printf(seq, "%pM\n", bp->serial);
	return 0;
}

static int
ptp_ocp_proc_gps_sync(struct seq_file *seq, void *v)
{
	struct ptp_ocp *bp = seq->private;

	if (bp->gps_lost)
		seq_printf(seq, "LOST @ %ptT\n", &bp->gps_lost);
	else
		seq_printf(seq, "SYNC\n");
	return 0;
}

static int
ptp_ocp_procfs_init(struct ptp_ocp *bp)
{
	char buf[32];
	int err;

	mutex_lock(&ptp_ocp_lock);
	err = idr_alloc(&ptp_ocp_idr, bp, 0, 0, GFP_KERNEL);
	mutex_unlock(&ptp_ocp_lock);
	if (err < 0) {
		dev_err(&bp->pdev->dev, "idr_alloc failed: %d\n", err);
		return err;
	}
	bp->id = err;

	sprintf(buf, "driver/ocp%d", bp->id);
	bp->proc = proc_mkdir(buf, NULL);

	return 0;
}

static ssize_t
ptp_ocp_proc_source_write(struct file *file, const char __user *user_buf,
			  size_t nbytes, loff_t *ppos)
{
	struct ptp_ocp *bp = PDE_DATA(file_inode(file));
	unsigned long flags;
	char buf[16];
	size_t sz;
	int val;

	sz = min_t(ssize_t, nbytes, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, sz))
		return -EFAULT;
	buf[sz] = 0;

	val = ptp_ocp_clock_val_from_name(buf);
	if (val < 0)
		return val;

	spin_lock_irqsave(&bp->lock, flags);
	iowrite32(val, &bp->reg->select);
	spin_unlock_irqrestore(&bp->lock, flags);

	return nbytes;
}

static int
ptp_ocp_proc_source_show(struct seq_file *seq, void *v)
{
	struct ptp_ocp *bp = seq->private;
	const char *p;
	u32 select;

	select = ioread32(&bp->reg->select);
	p = ptp_ocp_clock_name_from_val(select >> 16);
	seq_printf(seq, "%s\n", p);

	return 0;
}

static int
ptp_ocp_proc_source_open(struct inode *inode, struct file *file)
{
	return single_open(file, ptp_ocp_proc_source_show, PDE_DATA(inode));
}

static const struct proc_ops ptp_ocp_proc_source_ops = {
	.proc_open	= ptp_ocp_proc_source_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_write	= ptp_ocp_proc_source_write,
	.proc_release	= single_release,
};

static int
ptp_ocp_procfs_complete(struct ptp_ocp *bp)
{
	struct pps_device *pps;
	char buf[32];

	if (bp->gps_port != -1) {
		sprintf(buf, "/dev/ttyS%d", bp->gps_port);
		proc_symlink("ttyGPS", bp->proc, buf);
	}
	if (bp->mac_port != -1) {
		sprintf(buf, "/dev/ttyS%d", bp->mac_port);
		proc_symlink("ttyMAC", bp->proc, buf);
	}
	if (bp->mro50.name) {
		sprintf(buf, "/dev/%s", bp->mro50.name);
		proc_symlink("mro50", bp->proc, buf);
	}
	sprintf(buf, "/dev/ptp%d", ptp_clock_index(bp->ptp));
	proc_symlink("ptp", bp->proc, buf);

	pps = pps_lookup_dev(bp->ptp);
	if (pps) {
		sprintf(buf, "/dev/%s", dev_name(pps->dev));
		proc_symlink("pps", bp->proc, buf);
	}

	proc_create_single_data("serial",
				0, bp->proc, ptp_ocp_proc_serial, bp);
	proc_create_single_data("gps_state",
				0, bp->proc, ptp_ocp_proc_gps_sync, bp);
	proc_create_data("clock_source",
			 0644, bp->proc, &ptp_ocp_proc_source_ops, bp);

	return 0;
}

static int
ptp_ocp_procfs_del(struct ptp_ocp *bp)
{

	mutex_lock(&ptp_ocp_lock);
	idr_remove(&ptp_ocp_idr, bp->id);
	mutex_unlock(&ptp_ocp_lock);

	proc_remove(bp->proc);

	return 0;
}

static void
ptp_resource_summary(struct ptp_ocp *bp)
{
	struct device *dev = &bp->pdev->dev;

	if (bp->image) {
		u32 ver = bp->image->version;
		dev_info(dev, "version %x\n", ver);
		if (ver & 0xffff)
			dev_info(dev, "regular image, version %d\n",
				 ver & 0xffff);
		else
			dev_info(dev, "golden image, version %d\n",
				 ver >> 16);
	}
	if (bp->gps_port != -1)
		dev_info(dev, "GPS @ /dev/ttyS%d  115200\n", bp->gps_port);
	if (bp->mac_port != -1)
		dev_info(dev, "MAC @ /dev/ttyS%d   57600\n", bp->mac_port);
}

static void
ptp_ocp_detach(struct ptp_ocp *bp)
{

	if (timer_pending(&bp->watchdog))
		del_timer_sync(&bp->watchdog);
	if (bp->id != -1)
		ptp_ocp_procfs_del(bp);
	if (bp->ts0)
		ptp_ocp_unregister_ext(bp->ts0);
	if (bp->ts1)
		ptp_ocp_unregister_ext(bp->ts1);
	if (bp->phasemeter)
		ptp_ocp_unregister_ext(bp->phasemeter);
	if (bp->pps)
		ptp_ocp_unregister_ext(bp->pps);
	if (bp->gps_port != -1)
		serial8250_unregister_port(bp->gps_port);
	if (bp->mac_port != -1)
		serial8250_unregister_port(bp->mac_port);
	if (bp->spi_imu)
		platform_device_unregister(bp->spi_imu);
	if (bp->spi_flash)
		platform_device_unregister(bp->spi_flash);
	if (bp->i2c_flash)
		platform_device_unregister(bp->i2c_flash);
	if (bp->i2c_osc)
		platform_device_unregister(bp->i2c_osc);
	if (bp->i2c_clk)
		clk_hw_unregister_fixed_rate(bp->i2c_clk);
	if (bp->mro50.name)
		ptp_ocp_unregister_mro50(&bp->mro50);
	if (bp->n_irqs)
		pci_free_irq_vectors(bp->pdev);
	if (bp->ptp)
		ptp_clock_unregister(bp->ptp);
	if (bp->health)
		devlink_health_reporter_destroy(bp->health);
}

static int
ptp_ocp_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct devlink *devlink;
	struct ptp_ocp *bp;
	int err;

	devlink = devlink_alloc(&ptp_ocp_devlink_ops, sizeof(*bp));
	if (!devlink) {
		dev_err(&pdev->dev, "devlink_alloc failed\n");
		return -ENOMEM;
	}

	err = ptp_ocp_devlink_register(devlink, &pdev->dev);
	if (err)
		goto out_free;

	bp = devlink_priv(devlink);

	bp->ptp_info = ptp_ocp_clock_info;
	spin_lock_init(&bp->lock);
	mutex_init(&bp->mutex);
	bp->gps_port = -1;
	bp->mac_port = -1;
	bp->id = -1;
	bp->pdev = pdev;
	bp->res_tbl = (struct ocp_resource *)id->driver_data;

	pci_set_drvdata(pdev, bp);

	err = pci_enable_device(pdev);
	if (err) {
		dev_err(&pdev->dev, "pci_enable_device\n");
		goto out_unregister;
	}

	/* XXX	--  temporary compat mode.
	 * Older FPGA firmware only returns 2 irq's.
	 * allow this - if not all of the IRQ's are returned, skip the
	 * extra devices and just register the clock.
	 */
	err = pci_alloc_irq_vectors(pdev, 1, 12, PCI_IRQ_MSI | PCI_IRQ_MSIX);
	if (err < 0) {
		dev_err(&pdev->dev, "alloc_irq_vectors err: %d\n", err);
		goto out;
	}
	bp->n_irqs = err;
	pci_set_master(pdev);

	err = ptp_ocp_procfs_init(bp);
	if (err)
		goto out;

	err = ptp_ocp_register_resources(bp);
	if (err)
		goto out;

	bp->ptp = ptp_clock_register(&bp->ptp_info, &pdev->dev);
	if (IS_ERR(bp->ptp)) {
		err = PTR_ERR(bp->ptp);
		dev_err(&pdev->dev, "ptp_clock_register: %d\n", err);
		bp->ptp = 0;
		goto out;
	}

	err = ptp_ocp_procfs_complete(bp);
	if (err)
		goto out;

	ptp_ocp_info(bp);
	ptp_resource_summary(bp);
	ptp_ocp_devlink_health_register(devlink);

	return 0;

out:
	ptp_ocp_detach(bp);
	pci_disable_device(pdev);
out_unregister:
	pci_set_drvdata(pdev, NULL);
	ptp_ocp_devlink_unregister(devlink);
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
	pci_disable_device(pdev);
	pci_set_drvdata(pdev, NULL);

	ptp_ocp_devlink_unregister(devlink);
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
	struct device *dev = data;
	struct ptp_ocp *bp;
	char buf[32];
	bool add;

	switch (action) {
	case BUS_NOTIFY_ADD_DEVICE:
	case BUS_NOTIFY_DEL_DEVICE:
		add = action == BUS_NOTIFY_ADD_DEVICE;
		break;
	default:
		return 0;
	}

	if (!i2c_verify_adapter(dev))
		return 0;

	sprintf(buf, "/sys/bus/i2c/devices/%s", dev_name(dev));

	while ((dev = dev->parent))
		if (dev->driver && !strcmp(dev->driver->name, KBUILD_MODNAME))
			goto found;
	return 0;

found:
	bp = dev_get_drvdata(dev);
	if (add)
		bp->i2c_pde = proc_symlink("i2c", bp->proc, buf);
	else {
		proc_remove(bp->i2c_pde);
		bp->i2c_pde = 0;
	}

	return 0;
}

static struct notifier_block ptp_ocp_i2c_notifier = {
	.notifier_call = ptp_ocp_i2c_notifier_call,
};

static int __init
ptp_ocp_init(void)
{
	int err;

	err = bus_register_notifier(&i2c_bus_type, &ptp_ocp_i2c_notifier);
	return pci_register_driver(&ptp_ocp_driver);
}

static void __exit
ptp_ocp_fini(void)
{
	bus_unregister_notifier(&i2c_bus_type, &ptp_ocp_i2c_notifier);
	pci_unregister_driver(&ptp_ocp_driver);
}

module_init(ptp_ocp_init);
module_exit(ptp_ocp_fini);

MODULE_DESCRIPTION("OpenCompute TimeCard driver");
MODULE_LICENSE("GPL v2");
