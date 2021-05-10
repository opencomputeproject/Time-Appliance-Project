#define DEBUG

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/serial_8250.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/platform_device.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/spi/spi.h>
#include <linux/spi/xilinx_spi.h>
#include <linux/spi/altera.h>
#include <net/devlink.h>
#include <linux/i2c.h>
#include <linux/mtd/mtd.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>

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
	u32	__pad0;
	u32	version;
	u32	__pad1[60];
	u32	ctrl;
	u32	status;
	u32	__pad2[2];
	u32	intr;
	u32	intr_mask;
	u32	__pad3[2];
	u32	delay_req_rx_ns;
	u32	delay_req_rx_sec;
	u32	delay_req_tx_ns;
	u32	delay_req_tx_sec;
	u32	path_delay_req_rx_ns;
	u32	path_delay_req_rx_sec;
	u32	path_delay_req_tx_ns;
	u32	path_delay_req_tx_sec;
	u32	path_delay_resp_rx_ns;
	u32	path_delay_resp_rx_sec;
	u32	path_delay_resp_tx_ns;
	u32	path_delay_resp_tx_sec;
	u32	sync_rx_ns;
	u32	sync_rx_sec;
	u32	sync_tx_ns;
	u32	sync_tx_sec;
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

#define PHASEMETER_DEVICE_NAME "phase_error_gnss_mRO50"

#define PHASEMETER_OFFSET 0x4
#define PHASEMETER_IRQ_ENABLE 0x8
#define PHASEMETER_IRQ_ACK_OFFSET 0x28

#define to_phasemeter_device(p) container_of(p, struct phasemeter_device, misc)

struct phasemeter_device {
	void __iomem *reg;
	/* in ns */
	s32 phase_error;
	atomic_t data_available;
	/* the device can be opened only once */
	bool in_use;
	struct wait_queue_head data_wait_queue;
	struct miscdevice misc;

	/* lock for in_use, data_available, phase_error and the _reg fields */
	spinlock_t lock;
};

#define PPS_DEVICE_IRQ_ACK 0x10

struct ptp_ocp {
	struct pci_dev		*pdev;
	struct ocp_resource	*res;
	spinlock_t		lock;
	struct ocp_reg __iomem	*reg;
	struct tod_reg __iomem	*tod;
	struct pps_reg __iomem	*pps;
	struct ts_reg __iomem	*ts0;
	struct ts_reg __iomem	*ts1;
	struct img_reg __iomem	*image;
	struct ptp_clock	*ptp;
	struct ptp_clock_info	ptp_info;
	struct platform_device	*i2c_osc;
	struct platform_device	*spi_imu;
	struct platform_device	*spi_flash;
	struct proc_dir_entry	*i2c_pde;
	struct clk_hw		*i2c_clk;
	struct pps_device	*pps_device;
	void __iomem		*pps_device_reg;
	struct phasemeter_device *phasemeter;
	struct devlink_health_reporter *health;
	struct proc_dir_entry	*proc;
	struct timer_list	watchdog;
	time64_t		gps_lost;
	int			id;
	int			n_irqs;
	int			gps_port;
	int			mac_port;	/* miniature atomic clock */
	u8			serial[6];
	bool			has_serial;
	bool			pending_image;
};

struct ocp_resource {
	unsigned long offset;
	int size;
	int irq_vec;
	int (*setup)(struct ptp_ocp *bp, struct ocp_resource *res);
	void *extra;
	unsigned long bp_offset;
};

static void ptp_ocp_health_update(struct ptp_ocp *bp);
static int ptp_ocp_register_mem(struct ptp_ocp *bp, struct ocp_resource *res);
static int ptp_ocp_register_i2c(struct ptp_ocp *bp, struct ocp_resource *res);
static int ptp_ocp_register_spi(struct ptp_ocp *bp, struct ocp_resource *res);
static int ptp_ocp_register_spi_altera(struct ptp_ocp *bp, struct ocp_resource *res);
static int ptp_ocp_register_serial(struct ptp_ocp *bp,
				   struct ocp_resource *res);
static int ptp_ocp_register_pps(struct ptp_ocp *bp, struct ocp_resource * res);
static int ptp_ocp_register_phasemeter(struct ptp_ocp *bp, struct ocp_resource *res);


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

#define OCP_SPI_ALTERA_RESOURCE(member) \
	OCP_RES_LOCATION(member), .setup = ptp_ocp_register_spi_altera

#define OCP_PPS_RESOURCE(member) \
	OCP_RES_LOCATION(member), .setup = ptp_ocp_register_pps

#define OCP_PHASEMETER_RESOURCE(member) \
	OCP_RES_LOCATION(member), .setup = ptp_ocp_register_phasemeter

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
 * 10: Phasemeter
 * 11: PPS
 */

static struct spi_board_info ocp_spi_flash = {
	.modalias = "spi-nor",
	.bus_num = 1,			/* offset from PCI */
};

static struct ocp_resource ocp_fb_resource[] = {
	{
		OCP_MEM_RESOURCE(reg),
		.offset = 0x01000000, .size = 0x10000,
	},
	{
		OCP_MEM_RESOURCE(ts0),
		.offset = 0x01010000, .size = 0x10000,
	},
	{
		OCP_MEM_RESOURCE(ts1),
		.offset = 0x01020000, .size = 0x10000,
	},
	{
		OCP_MEM_RESOURCE(pps),
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
		.extra = &ocp_spi_flash,
	},
	{ }
};

#define MSI_VECTOR_PHASEMETER 10
#define MSI_VECTOR_PPS 11
static struct ocp_resource ocp_o2s_resource[] = {
	{
		OCP_MEM_RESOURCE(reg),
		.offset = 0x01000000, .size = 0x10000,
	},
	{
		OCP_SERIAL_RESOURCE(gps_port),
		.offset = 0x00160000 + 0x1000, .irq_vec = 3,
	},
	{
		OCP_SPI_ALTERA_RESOURCE(spi_flash),
		.offset = 0x00310000, .size = 0x10000, .irq_vec = 9,
		.extra = &ocp_spi_flash,
	},
	{
		OCP_PHASEMETER_RESOURCE(phasemeter),
		.offset = 0x00320000, .size = 0x30, .irq_vec = MSI_VECTOR_PHASEMETER
	},
	{
		OCP_PPS_RESOURCE(pps),
		.offset = 0x00330000, .size = 0x20, .irq_vec = MSI_VECTOR_PPS
	},
	{}
};

static const struct pci_device_id ptp_ocp_pcidev_id[] = {
	{ PCI_DEVICE_DATA(FACEBOOK, TIMECARD, &ocp_fb_resource) },
	{ PCI_DEVICE_DATA(OROLIA, ARTCARD, &ocp_o2s_resource) },
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

static const struct ptp_clock_info ptp_ocp_clock_info = {
	.owner		= THIS_MODULE,
	.name		= KBUILD_MODNAME,
	.max_adj	= 100000000,
	.gettimex64	= ptp_ocp_gettimex,
	.settime64	= ptp_ocp_settime,
	.adjtime	= ptp_ocp_adjtime,
	.adjfine	= ptp_ocp_null_adjfine,
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

	status = ioread32(&bp->pps->status);

	if (status & PPS_STATUS_SUPERV_ERR) {
		iowrite32(status, &bp->pps->status);
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
	size_t off, len, resid, wrote;
	struct erase_info erase;
	size_t base, blksz;
	int err;

	off = 0;
	base = 1024 * 4096;		/* Timecard.bin start address */
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

static struct platform_device *
ptp_ocp_spi_bus(struct pci_dev *pdev, struct ocp_resource *r, int id)
{
	struct xspi_platform_data spi_pxdata = {
		.num_chipselect = 1,
		.bits_per_word = 8,
	};
	struct resource res[2];
	unsigned long start;

	if (r->extra) {
		spi_pxdata.devices = r->extra;
		spi_pxdata.num_devices = 1;
		id += spi_pxdata.devices->bus_num;
	}

	start = pci_resource_start(pdev, 0) + r->offset;
	ptp_ocp_set_mem_resource(&res[0], start, r->size);
	ptp_ocp_set_irq_resource(&res[1], pci_irq_vector(pdev, r->irq_vec));

	return platform_device_register_resndata(&pdev->dev,
		"xilinx_spi", id, res, 2, &spi_pxdata, sizeof(spi_pxdata));
}

static int
ptp_ocp_register_spi(struct ptp_ocp *bp, struct ocp_resource *res)
{
	struct platform_device *p;
	int id;

	/* XXX hack to work around old FPGA */
	if (bp->n_irqs < 10) {
		dev_err(&bp->pdev->dev, "FPGA does not have SPI devices\n");
		return 0;
	}

	if (res->irq_vec > bp->n_irqs) {
		dev_err(&bp->pdev->dev, "spi device irq %d out of range\n",
			res->irq_vec);
		return 0;
	}

	id = pci_dev_id(bp->pdev) << 1;

	p = ptp_ocp_spi_bus(bp->pdev, res, id);
	if (IS_ERR(p))
		return PTR_ERR(p);

	bp_assign_entry(bp, res, p);

	return 0;
}

static struct platform_device *
ptp_ocp_spi_altera_bus(struct pci_dev *pdev, struct ocp_resource *r, int id)
{
	struct altera_spi_platform_data spi_altr_data = {
		.num_chipselect = 1,
		.num_devices = 1,
	};
	struct resource res[2];
	unsigned long start;

	if (r->extra) {
		spi_altr_data.devices = r->extra;
		spi_altr_data.num_devices = 1;
		id += spi_altr_data.devices->bus_num;
	}

	start = pci_resource_start(pdev, 0) + r->offset;
	ptp_ocp_set_mem_resource(&res[0], start, r->size);
	ptp_ocp_set_irq_resource(&res[1], pci_irq_vector(pdev, r->irq_vec));

	return platform_device_register_resndata(&pdev->dev,
		"spi_altera", id, res, 2, &spi_altr_data, sizeof(spi_altr_data));
}

static int
ptp_ocp_register_spi_altera(struct ptp_ocp *bp, struct ocp_resource *res)
{
	struct platform_device *p;
	int id;

	/* XXX hack to work around old FPGA */
	if (bp->n_irqs < 10) {
		dev_err(&bp->pdev->dev, "FPGA does not have SPI devices\n");
		return 0;
	}

	if (res->irq_vec > bp->n_irqs) {
		dev_err(&bp->pdev->dev, "spi device irq %d out of range\n",
			res->irq_vec);
		return 0;
	}

	id = pci_dev_id(bp->pdev) << 1;

	p = ptp_ocp_spi_altera_bus(bp->pdev, res, id);
	if (IS_ERR(p))
		return PTR_ERR(p);

	bp_assign_entry(bp, res, p);

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

	start = pci_resource_start(pdev, 0) + r->offset;
	ptp_ocp_set_mem_resource(&res[0], start, r->size);
	ptp_ocp_set_irq_resource(&res[1], pci_irq_vector(pdev, r->irq_vec));

	return platform_device_register_resndata(&pdev->dev, "xiic-i2c",
						 id, res, 2, NULL, 0);
}

static int
ptp_ocp_register_i2c(struct ptp_ocp *bp, struct ocp_resource *res)
{
	struct pci_dev *pdev = bp->pdev;
	struct platform_device *p;
	struct clk_hw *clk;
	char buf[32];
	int id;

	if (res->irq_vec > bp->n_irqs) {
		dev_err(&bp->pdev->dev, "i2c device irq %d out of range\n",
			res->irq_vec);
		return 0;
	}

	id = pci_dev_id(bp->pdev);

	sprintf(buf, "AXI.%d", id);
	clk = clk_hw_register_fixed_rate(&pdev->dev, buf, NULL, 0, 50000000);
	if (IS_ERR(clk))
		return PTR_ERR(clk);
	bp->i2c_clk = clk;

	sprintf(buf, "xiic-i2c.%d", id);
	devm_clk_hw_register_clkdev(&pdev->dev, clk, NULL, buf);
	p = ptp_ocp_i2c_bus(bp->pdev, res, id);
	if (IS_ERR(p))
		return PTR_ERR(p);

	bp_assign_entry(bp, res, p);

	return 0;
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
ptp_ocp_register_serial(struct ptp_ocp *bp, struct ocp_resource *res)
{
	int port;

	if (res->irq_vec > bp->n_irqs) {
		dev_err(&bp->pdev->dev, "serial device irq %d out of range\n",
			res->irq_vec);
		return 0;
	}

	port = ptp_ocp_serial_line(bp, res);
	if (port < 0)
		return port;

	bp_assign_entry(bp, res, port);

	return 0;
}

static struct pps_source_info pps_ktimer_info = {
	.name = "ocp-pps",
	.path = "/dev/pps-ocp-clock",
	.mode = PPS_CAPTUREASSERT | PPS_OFFSETASSERT |
			PPS_CANWAIT | PPS_TSFMT_TSPEC,
	.owner = THIS_MODULE,
};

irqreturn_t pps_irq_handler(int irq, void *dev_id) {
	struct ptp_ocp *bp;
	struct pps_event_time ts;

	/* Get the time stamp first */
	pps_get_ts(&ts);

	bp = (struct ptp_ocp *) dev_id;

	pps_event(bp->pps_device, &ts, PPS_CAPTUREASSERT, NULL);
	iowrite32(0, bp->pps_device_reg + PPS_DEVICE_IRQ_ACK);

	return IRQ_HANDLED;
}

static int
ptp_ocp_register_pps(struct ptp_ocp *bp, struct ocp_resource *res)
{
	int err;

	bp->pps_device_reg = ptp_ocp_get_mem(bp, res);
	err = pci_request_irq(bp->pdev, res->irq_vec, pps_irq_handler, 0, bp, KBUILD_MODNAME);
	if (err < 0) {
		printk(KERN_ALERT "%s: request_irq failed with %d\n",
		__func__, err);
		return -EINVAL;
	}

	bp->pps_device = pps_register_source(&pps_ktimer_info, PPS_CAPTUREASSERT | PPS_OFFSETASSERT);
	if (bp->pps_device == NULL) {
		return -EINVAL;
	}
	/* Activate PPS IRQ */
	iowrite32(1, bp->pps_device_reg);
	return 0;
}

static irqreturn_t
phasemeter_irq_handler(int irq, void* dev_id)
{
	struct ptp_ocp *bp;
	struct phasemeter_device *phasemeter;
	unsigned long flags;

	bp = (struct ptp_ocp *) dev_id;
	phasemeter = bp->phasemeter;

	spin_lock_irqsave(&phasemeter->lock, flags);

	phasemeter->phase_error = readl(phasemeter->reg);
	if (unlikely(abs(phasemeter->phase_error) > 500000000)) {
		pr_info("[%s] phase error is too big, reset issued\n",
			phasemeter->misc.name);
		writel(0, phasemeter->reg);
	} else {
		atomic_set(&phasemeter->data_available, true);
		wake_up_interruptible(&phasemeter->data_wait_queue);
	}

	/* Phasemeter ACK interrupt */
	iowrite32(0, phasemeter->reg + PHASEMETER_IRQ_ACK_OFFSET);
	spin_unlock_irqrestore(&phasemeter->lock, flags);

	return IRQ_HANDLED;
}

static int
phasemeter_device_open(struct inode *inode, struct file *file)
{
	struct miscdevice *misc;
	struct phasemeter_device *phasemeter;
	unsigned long flags;
	int ret;

	misc = file->private_data;
	phasemeter = to_phasemeter_device(misc);

	spin_lock_irqsave(&phasemeter->lock, flags);
	ret = phasemeter->in_use ? -EBUSY : 0;
	phasemeter->in_use = true;
	spin_unlock_irqrestore(&phasemeter->lock, flags);

	return ret;
}

static int
phasemeter_device_release(struct inode *inode, struct file *file)
{
	struct miscdevice *misc;
	struct phasemeter_device *phasemeter;
	unsigned long flags;

	misc = file->private_data;
	phasemeter = to_phasemeter_device(misc);

	spin_lock_irqsave(&phasemeter->lock, flags);
	phasemeter->in_use = false;
	spin_unlock_irqrestore(&phasemeter->lock, flags);

	return 0;
}

static ssize_t
phasemeter_device_read(struct file *file, char *buf,
		size_t nbytes, loff_t *ppos)
{
	ssize_t n;
	s32 data;
	struct miscdevice *misc;
	struct phasemeter_device *phasemeter;
	unsigned long flags;
	bool nonblock;
	int ret;

	misc = file->private_data;
	phasemeter = to_phasemeter_device(misc);
	nonblock = file->f_flags & O_NONBLOCK;
	if (!atomic_read(&phasemeter->data_available) && nonblock)
		return -EAGAIN;

	while (true) {
		ret = wait_event_interruptible(phasemeter->data_wait_queue,
				atomic_read(&phasemeter->data_available));
		if (ret == -ERESTARTSYS)
			return ret;

		spin_lock_irqsave(&phasemeter->lock, flags);

		if (atomic_read(&phasemeter->data_available))
			break;

		spin_unlock_irqrestore(&phasemeter->lock, flags);
	}

	data = phasemeter->phase_error;
	atomic_set(&phasemeter->data_available, false);
	spin_unlock_irqrestore(&phasemeter->lock, flags);

	n = put_user(data, (s32 *)buf);
	if (n != 0) {
		pr_warn("read: copy to user failed.\n");
		return -EFAULT;
	}
	return sizeof(data);
}

static ssize_t
phasemeter_device_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	ssize_t n;
	s32 phase_correction;
	struct miscdevice *misc;
	struct phasemeter_device *phasemeter;
	unsigned long flags;

	if (count != sizeof(phase_correction))
		return -EINVAL;

	misc = file->private_data;
	phasemeter = to_phasemeter_device(misc);

	n = get_user(phase_correction, (s32 *)buf);
	if (n != 0) {
		pr_warn("read: copy to user failed.\n");
		return -EFAULT;
	}

	spin_lock_irqsave(&phasemeter->lock, flags);
	/* phase correction unit is 5 nanoseconds */
	writel(phase_correction, phasemeter->reg + PHASEMETER_OFFSET);
	spin_unlock_irqrestore(&phasemeter->lock, flags);

	return sizeof(phase_correction);
}

static unsigned int
phasemeter_device_poll(struct file *file, poll_table *wait)
{
	unsigned int mask;
	struct miscdevice *misc;
	struct phasemeter_device *phasemeter;

	misc = file->private_data;
	phasemeter = to_phasemeter_device(misc);

	/* device is always writable */
	mask = POLLOUT | POLLWRNORM;
	poll_wait(file, &phasemeter->data_wait_queue, wait);
	if (atomic_read(&phasemeter->data_available))
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

const struct file_operations phasemeter_device_fops = {
		.owner = THIS_MODULE,
		.open = phasemeter_device_open,
		.release = phasemeter_device_release,
		.read = phasemeter_device_read,
		.write = phasemeter_device_write,
		.poll = phasemeter_device_poll,
};

static int
ptp_ocp_register_phasemeter(struct ptp_ocp *bp, struct ocp_resource *res)
{
	struct phasemeter_device * phasemeter;
	int err;

	/* Register misc device for phasemeter */
	phasemeter = devm_kzalloc(&bp->pdev->dev, sizeof(*phasemeter), GFP_KERNEL);
	if (phasemeter == NULL)
		return -ENOMEM;

	phasemeter->reg = ptp_ocp_get_mem(bp, res);
	atomic_set(&phasemeter->data_available, false);
	phasemeter->in_use = false;
	init_waitqueue_head(&phasemeter->data_wait_queue);
	spin_lock_init(&phasemeter->lock);

	phasemeter->misc = (struct miscdevice) {
		.minor = MISC_DYNAMIC_MINOR,
		.name = PHASEMETER_DEVICE_NAME,
		.fops = &phasemeter_device_fops,
		.parent = &bp->pdev->dev,
	};

	dev_info(&bp->pdev->dev, "Phasemeter device at %s", phasemeter->misc.name);

	err = misc_register(&phasemeter->misc);
	if (err) {
		kfree(phasemeter);
		phasemeter = NULL;
		dev_err(&bp->pdev->dev, "misc_register: %d\n", err);
		return -ENOMEM;
	}

	err = pci_request_irq(bp->pdev, res->irq_vec, phasemeter_irq_handler, 0, bp, KBUILD_MODNAME);
	if (err < 0) {
		printk(KERN_ALERT "%s: request_irq failed with %d\n",
		__func__, err);
		misc_deregister(&phasemeter->misc);
		kfree(phasemeter);
		phasemeter = NULL;
		return -ENOMEM;
	}

	/* Activate Phasemeter IRQ */
	iowrite32(1, phasemeter->reg + PHASEMETER_IRQ_ENABLE);

	bp->phasemeter = phasemeter;

	return 0;
}

static int
ptp_ocp_register_mem(struct ptp_ocp *bp, struct ocp_resource *res)
{
	void __iomem *mem, **ptr;
	mem = ptp_ocp_get_mem(bp, res);
	if (!mem)
		return -EINVAL;

	ptr = (void __iomem *)((uintptr_t)bp + res->bp_offset);
	*ptr = mem;

	return 0;
}

static int
ptp_ocp_register_resources(struct ptp_ocp *bp)
{
	struct ocp_resource *res;
	int err = 0;

	for (res = bp->res; res->setup; res++) {
		err = res->setup(bp, res);
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
	char buf[32];

	if (bp->gps_port != -1) {
		sprintf(buf, "/dev/ttyS%d", bp->gps_port);
		proc_symlink("ttyGPS", bp->proc, buf);
	}
	if (bp->mac_port != -1) {
		sprintf(buf, "/dev/ttyS%d", bp->mac_port);
		proc_symlink("ttyMAC", bp->proc, buf);
	}
	sprintf(buf, "/dev/ptp%d", ptp_clock_index(bp->ptp));
	proc_symlink("ptp", bp->proc, buf);

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
	if (bp->gps_port != -1)
		serial8250_unregister_port(bp->gps_port);
	if (bp->mac_port != -1)
		serial8250_unregister_port(bp->mac_port);
	if (bp->spi_imu)
		platform_device_unregister(bp->spi_imu);
	if (bp->spi_flash)
		platform_device_unregister(bp->spi_flash);
	if (bp->i2c_osc)
		platform_device_unregister(bp->i2c_osc);
	if (bp->i2c_clk)
		clk_hw_unregister_fixed_rate(bp->i2c_clk);
	if (bp->pps_device) {
		iowrite32(0, bp->pps_device_reg);
		pps_unregister_source(bp->pps_device);
		free_irq(pci_irq_vector(bp->pdev, MSI_VECTOR_PPS), bp);
	}
	if (bp->phasemeter) {
		iowrite32(0, bp->phasemeter->reg + PHASEMETER_IRQ_ENABLE);
		misc_deregister(&bp->phasemeter->misc);
		free_irq(pci_irq_vector(bp->pdev, MSI_VECTOR_PHASEMETER), bp);
	}
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
	struct pci_device_id *card_id;
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

	card_id = pci_match_id(id, pdev);

	bp->ptp_info = ptp_ocp_clock_info;
	spin_lock_init(&bp->lock);
	bp->gps_port = -1;
	bp->mac_port = -1;
	bp->id = -1;
	bp->pdev = pdev;
	bp->res = (struct ocp_resource *)card_id->driver_data;

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
	err = pci_alloc_irq_vectors(pdev, 1, 16, PCI_IRQ_MSI);
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

	err = ptp_ocp_init_clock(bp);
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
