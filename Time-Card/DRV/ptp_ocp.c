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

static const struct pci_device_id ptp_ocp_pcidev_id[] = {
	{ PCI_DEVICE(0x1d9b, 0x0400) },
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, ptp_ocp_pcidev_id);

enum ocp_res_name {
	OCP_RES_BRIDGE,
	OCP_RES_CLOCK,
	OCP_RES_TOD,
	OCP_RES_GPS,
	OCP_RES_MAC,
	OCP_RES_OSC,
	OCP_RES_IMU,
};

struct ocp_resource {
	unsigned long offset;
	int size;
	int irq_vec;
};

/* This is the MSI vector mapping used.
 * 0: N/C
 * 1: TS0
 * 2: TS1
 * 3: GPS
 * 4: GPS2 (n/c)
 * 5: MAC
 * 6: I2C IMU (inertial measurement unit)
 * 7: I2C oscillator
 */

static struct ocp_resource ocp_resource[] = {
	[OCP_RES_BRIDGE] = {
		.offset = 0x00010000, .size = 0x01000,
	},
	[OCP_RES_CLOCK] = {
		.offset = 0x01000000, .size = 0x10000,
	},
	[OCP_RES_TOD] = {
		.offset = 0x01050000, .size = 0x10000,
	},
	[OCP_RES_IMU] = {
		.offset = 0x00140000, .size = 0x10000, .irq_vec = 6,
	},
	[OCP_RES_OSC] = {
		.offset = 0x00150000, .size = 0x10000, .irq_vec = 7,
	},
	[OCP_RES_GPS] = {
		.offset = 0x00160000 + 0x1000, .irq_vec = 3,
	},
	[OCP_RES_MAC] = {
		.offset = 0x00180000 + 0x1000, .irq_vec = 5,
	},
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
};

#define OCP_CTRL_ENABLE		BIT(0)
#define OCP_CTRL_ADJUST_TIME	BIT(1)
#define OCP_CTRL_ADJUST_OFFSET	BIT(2)
#define OCP_CTRL_READ_TIME_REQ	BIT(30)
#define OCP_CTRL_READ_TIME_DONE	BIT(31)

#define OCP_STATUS_IN_SYNC	BIT(0)

#define OCP_SELECT_CLK_NONE	0
#define OCP_SELECT_CLK_REG 	6

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

struct ptp_ocp {
	struct pci_dev		*pdev;
	spinlock_t		lock;
	void __iomem		*base;
	struct ocp_reg __iomem	*reg;
	struct tod_reg __iomem	*tod;
	struct ptp_clock	*ptp;
	struct ptp_clock_info	ptp_info;
	struct platform_device 	*osc_i2c;
	struct platform_device 	*imu_i2c;
	struct clk_hw		*i2c_clk;
	int			n_irqs;
	int 			gps_port;
	int 			mac_port;	/* miniature atomic clock */
};

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

static int
ptp_ocp_check_clock(struct ptp_ocp *bp)
{
	struct timespec64 ts;
	bool sync;
	u32 ctrl;

	/* make sure clock is enabled */
	ctrl = ioread32(&bp->reg->ctrl);
	ctrl |= OCP_CTRL_ENABLE;
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

static void
ptp_ocp_info(struct ptp_ocp *bp)
{
	static const char *clock_name[] = {
		"NO", "TOD", "IRIG", "PPS", "PTP", "RTC", "REGS", "EXT"
	};
	u32 version, select;

	version = ioread32(&bp->reg->version);
	select = ioread32(&bp->reg->select);
	dev_info(&bp->pdev->dev, "Version %d.%d.%d, clock %s, device ptp%d\n",
		version >> 24, (version >> 16) & 0xff, version & 0xffff,
		clock_name[select & 7],
		ptp_clock_index(bp->ptp));

	ptp_ocp_tod_info(bp);
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

/* XXX
 * Can set platform data (xiic_i2c_platform_data),
 *   which lists the devices on the I2C bus; these are added at reg time.
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
ptp_ocp_register_i2c(struct ptp_ocp *bp)
{
	struct pci_dev *pdev = bp->pdev;
	struct platform_device *p;
	struct clk_hw *clk;
	char buf[32];
	int id;

	id = pci_dev_id(bp->pdev) << 1;

	sprintf(buf, "AXI.%d", id);
	clk = clk_hw_register_fixed_rate(&pdev->dev, buf, NULL, 0, 50000000);
	if (IS_ERR(clk))
		return PTR_ERR(clk);
	bp->i2c_clk = clk;

	sprintf(buf, "xiic-i2c.%d", id);
	devm_clk_hw_register_clkdev(&pdev->dev, clk, NULL, buf);
	p = ptp_ocp_i2c_bus(bp->pdev, &ocp_resource[OCP_RES_OSC], id);
	if (IS_ERR(p))
		return PTR_ERR(p);
	bp->osc_i2c = p;
	id++;

	sprintf(buf, "xiic-i2c.%d", id);
	devm_clk_hw_register_clkdev(&pdev->dev, clk, NULL, buf);
	p = ptp_ocp_i2c_bus(bp->pdev, &ocp_resource[OCP_RES_IMU], id);
	if (IS_ERR(p))
		return PTR_ERR(p);
	bp->imu_i2c = p;
	id++;

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
ptp_ocp_register_serial(struct ptp_ocp *bp)
{
	struct pci_dev *pdev = bp->pdev;
	int err;

	err = ptp_ocp_serial_line(bp, &ocp_resource[OCP_RES_GPS]);
	if (err < 0)
		goto out;
	bp->gps_port = err;

	err = ptp_ocp_serial_line(bp, &ocp_resource[OCP_RES_MAC]);
	if (err < 0)
		goto out;
	bp->mac_port = err;

	dev_info(&pdev->dev, "GPS @ /dev/ttyS%d  115200\n", bp->gps_port);
	dev_info(&pdev->dev, "MAC @ /dev/ttyS%d   57600\n", bp->mac_port);

	return 0;

out:
	dev_err(&pdev->dev, "Failure path, err: %d\n", err);
	return err;
}

static void
ptp_ocp_detach(struct ptp_ocp *bp)
{

	if (bp->gps_port > 0)
		serial8250_unregister_port(bp->gps_port);
	if (bp->mac_port > 0)
		serial8250_unregister_port(bp->mac_port);
	if (bp->osc_i2c)
		platform_device_unregister(bp->osc_i2c);
	if (bp->imu_i2c)
		platform_device_unregister(bp->imu_i2c);
	if (bp->i2c_clk)
		clk_hw_unregister_fixed_rate(bp->i2c_clk);
	if (bp->n_irqs)
		pci_free_irq_vectors(bp->pdev);
	if (bp->ptp)
		ptp_clock_unregister(bp->ptp);
}

static int
ptp_ocp_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct ptp_ocp *bp;
	int err;

	bp = kzalloc(sizeof(*bp), GFP_KERNEL);
	if (!bp) {
		dev_err(&pdev->dev, "kzalloc\n");
		return -ENOMEM;
	}
	bp->pdev = pdev;
	pci_set_drvdata(pdev, bp);

	err = pci_enable_device(pdev);
	if (err) {
		dev_err(&pdev->dev, "pci_enable_device\n");
		goto out_free;
	}

	bp->reg = ptp_ocp_get_mem(bp, &ocp_resource[OCP_RES_CLOCK]);
	if (!bp->reg)
		goto out;

	bp->tod = ptp_ocp_get_mem(bp, &ocp_resource[OCP_RES_TOD]);
	if (!bp->tod)
		goto out;

	bp->ptp_info = ptp_ocp_clock_info;
	spin_lock_init(&bp->lock);
	bp->gps_port = -1;
	bp->mac_port = -1;

	/* XXX  --  temporary compat mode.
	 * Older FPGA firmware only returns 2 irq's, not 8.
	 * allow this - if 8 IRQ's are not returned, skip the
	 * serial devices and just register the clock.
	 */
	err = pci_alloc_irq_vectors(pdev, 1, 8, PCI_IRQ_MSI);
	if (err < 0) {
		dev_err(&pdev->dev, "alloc_irq_vectors err: %d\n", err);
		goto out;
	}
	bp->n_irqs = err;
	pci_set_master(pdev);

	if (bp->n_irqs != 8) {
		dev_err(&pdev->dev, "Only %d IRQs, no I2C or serial\n",
			bp->n_irqs);
	} else {
		err = ptp_ocp_register_serial(bp);
		if (err)
			goto out;

		err = ptp_ocp_register_i2c(bp);
		if (err)
			goto out;
	}

	err = ptp_ocp_check_clock(bp);
	if (err)
		goto out;

	bp->ptp = ptp_clock_register(&bp->ptp_info, &pdev->dev);
	if (IS_ERR(bp->ptp)) {
		err = PTR_ERR(bp->ptp);
		dev_err(&pdev->dev, "ptp_clock_register: %d\n", err);
		bp->ptp = 0;
		goto out;
	}

	ptp_ocp_info(bp);

	return 0;

out:
	ptp_ocp_detach(bp);
	pci_disable_device(pdev);
out_free:
	kfree(bp);

	return err;
}

static void
ptp_ocp_remove(struct pci_dev *pdev)
{
	struct ptp_ocp *bp = pci_get_drvdata(pdev);

	ptp_ocp_detach(bp);
	pci_disable_device(pdev);
	pci_set_drvdata(pdev, NULL);
	kfree(bp);
}

static struct pci_driver ptp_ocp_driver = {
	.name		= KBUILD_MODNAME,
	.id_table	= ptp_ocp_pcidev_id,
	.probe		= ptp_ocp_probe,
	.remove		= ptp_ocp_remove,
};

static int __init
ptp_ocp_init(void)
{
	int err;

	err = pci_register_driver(&ptp_ocp_driver);
	return err;
}

static void __exit
ptp_ocp_fini(void)
{
	pci_unregister_driver(&ptp_ocp_driver);
}

module_init(ptp_ocp_init);
module_exit(ptp_ocp_fini);

MODULE_DESCRIPTION("OpenCompute TimeCard driver");
MODULE_LICENSE("GPL v2");
