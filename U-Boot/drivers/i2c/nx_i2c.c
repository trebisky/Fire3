#include <common.h>
#include <errno.h>
#include <dm.h>
#include <i2c.h>
#include <asm/arch/nexell.h>
#include <asm/arch/reset.h>
#include <asm/arch/clk.h>
#include <asm/arch/nx_gpio.h>

#define I2C_WRITE       0
#define I2C_READ        1

#define I2C_OK          0
#define I2C_NOK         1
#define I2C_NACK        2
#define I2C_NOK_LA      3       /* Lost arbitration */
#define I2C_NOK_TOUT    4       /* time out */

#define I2CLC_FILTER	0x04	/* SDA filter on*/
#define I2CSTAT_BSY     0x20    /* Busy bit */
#define I2CSTAT_NACK    0x01    /* Nack bit */
#define I2CSTAT_ABT	0x08	/* Arbitration bit */
#define I2CCON_ACKGEN   0x80    /* Acknowledge generation */
#define I2CCON_IRENB	0x20	/* Interrupt Enable bit  */
#define I2CCON_IRPND    0x10    /* Interrupt pending bit */
#define I2C_MODE_MT     0xC0    /* Master Transmit Mode */
#define I2C_MODE_MR     0x80    /* Master Receive Mode */
#define I2C_START_STOP  0x20    /* START / STOP */
#define I2C_TXRX_ENA    0x10    /* I2C Tx/Rx enable */

#define I2C_TIMEOUT_MS	10      /* 10 ms */

#define I2C_M_NOSTOP	0x100

#ifndef CONFIG_MAX_I2C_NUM
#define CONFIG_MAX_I2C_NUM 3
#endif

DECLARE_GLOBAL_DATA_PTR;

struct nx_i2c_regs {
	uint     iiccon;
	uint     iicstat;
	uint     iicadd;
	uint     iicds;
	uint     iiclc;
};

struct nx_i2c_bus {
	uint bus_num;
	struct nx_i2c_regs *regs;
	uint speed;
	uint target_speed;
	uint sda_delay;
};

/* s5pxx18 i2c must be reset before enabled */
static void i2c_reset(int ch)
{
	int rst_id = RESET_ID_I2C0 + ch;

	nx_rstcon_setrst(rst_id, 0);
	nx_rstcon_setrst(rst_id, 1);
}

/* FIXME : this func will be removed after reset dm driver ported.
 * set mmc pad alternative func.
 */
static void set_i2c_pad_func(struct nx_i2c_bus *i2c)
{
	switch (i2c->bus_num) {
	case 0:
		nx_gpio_set_pad_function(3, 2, 1);
		nx_gpio_set_pad_function(3, 3, 1);
		break;
	case 1:
		nx_gpio_set_pad_function(3, 4, 1);
		nx_gpio_set_pad_function(3, 5, 1);
		break;
	case 2:
		nx_gpio_set_pad_function(3, 6, 1);
		nx_gpio_set_pad_function(3, 7, 1);
		break;
	}
}

static uint i2c_get_clkrate(struct nx_i2c_bus *bus)
{
	struct clk *clk;
	int index = bus->bus_num;
	char name[50] = {0, };

	sprintf(name, "%s.%d", DEV_NAME_I2C, index);
	clk = clk_get((const char *)name);
	if (!clk)
		return -1;

	return clk_get_rate(clk);
}

static uint i2c_set_clk(struct nx_i2c_bus *bus, uint enb)
{
	struct clk *clk;
	char name[50];

	sprintf(name, "%s.%d", DEV_NAME_I2C, bus->bus_num);
	clk = clk_get((const char *)name);
	if (!clk)
		return -1;

	if (enb) {
		clk_disable(clk);
		clk_enable(clk);
	} else {
		clk_disable(clk);
	}

	return 0;
}

/* get i2c module number from base address */
static uint i2c_get_busnum(struct nx_i2c_bus *bus)
{
	void *base_addr = (void *)PHY_BASEADDR_I2C0;
	int i;

	for (i = 0; i < CONFIG_MAX_I2C_NUM; i++) {
		if (base_addr == ((void *)bus->regs)) {
			bus->bus_num = i;
			return i;
		}
		base_addr += 0x1000;
	}

	return -1;
}

/* Set SDA line delay */
static int nx_i2c_set_sda_delay(struct nx_i2c_bus *bus, ulong clkin)
{
	struct nx_i2c_regs *i2c = bus->regs;
	uint sda_delay = 0;

	if (bus->sda_delay) {
		sda_delay = clkin * bus->sda_delay;
		sda_delay = DIV_ROUND_UP(sda_delay, 1000000);
		sda_delay = DIV_ROUND_UP(sda_delay, 5);
		if (sda_delay > 3)
			sda_delay = 3;
		sda_delay |= I2CLC_FILTER;
	} else {
		sda_delay = 0;
	}

	sda_delay &= 0x7;
	writel(sda_delay, &i2c->iiclc);

	return 0;
}

/* Calculate the value of the divider and prescaler, set the bus speed. */
static int nx_i2c_set_bus_speed(struct udevice *dev, uint speed)
{
	struct nx_i2c_bus *bus = dev_get_priv(dev);
	struct nx_i2c_regs *i2c = bus->regs;
	unsigned long freq, pres = 16, div;

	freq = i2c_get_clkrate(bus);
	/* calculate prescaler and divisor values */
	if ((freq / pres / (16 + 1)) > speed)
		/* set prescaler to 512 */
		pres = 512;

	div = 0;
	while ((freq / pres / (div + 1)) > speed)
		div++;

	/* set prescaler, divisor according to freq, also set ACKGEN, IRQ */
	writel((div & 0x0F) | ((pres == 512) ? 0x40 : 0), &i2c->iiccon);

	/* init to SLAVE REVEIVE and set slaveaddr */
	writel(0, &i2c->iicstat);
	writel(0x00, &i2c->iicadd);
	/* program Master Transmit (and implicit STOP) */
	writel(I2C_MODE_MT | I2C_TXRX_ENA, &i2c->iicstat);

	bus->speed = bus->target_speed / (div * pres);

	return 0;
}

static void nx_i2c_set_clockrate(struct udevice *dev, uint speed)
{
	struct nx_i2c_bus *bus = dev_get_priv(dev);
	ulong clkin;

	nx_i2c_set_bus_speed(dev, speed);
	clkin = bus->speed;			/* the actual i2c speed */
	clkin /= 1000;				/* clkin now in Khz */
	nx_i2c_set_sda_delay(bus, clkin);
}

static void i2c_process_node(struct udevice *dev)
{
	struct nx_i2c_bus *bus = dev_get_priv(dev);
	const void *blob = gd->fdt_blob;

	int node;

	node = dev->of_offset;
	bus->target_speed = fdtdec_get_int(blob, node,
			"nexell,i2c-max-bus-freq", 0);
	bus->sda_delay = fdtdec_get_int(blob, node,
			"nexell,i2c-sda-delay", 0);
}

static int nx_i2c_probe(struct udevice *dev)
{
	struct nx_i2c_bus *bus = dev_get_priv(dev);

	/* get regs */
	bus->regs = (struct nx_i2c_regs *)dev_get_addr(dev);
	/* calc index */
	if (!i2c_get_busnum(bus)) {
		debug("not found i2c number!\n");
		return -1;
	}

	/* i2c optional node parsing */
	i2c_process_node(dev);
	if (!bus->target_speed)
		return -1;

	/* reset */
	i2c_reset(bus->bus_num);
	/* gpio pad */
	set_i2c_pad_func(bus);

	/* clock rate */
	i2c_set_clk(bus, 1);
	nx_i2c_set_clockrate(dev, bus->target_speed);
	i2c_set_clk(bus, 0);

	return 0;
}

/* i2c bus busy check */
static int i2c_is_busy(struct nx_i2c_regs *i2c)
{
	ulong start_time;
	start_time = get_timer(0);
	while (readl(&i2c->iicstat) & I2CSTAT_BSY) {
		if (get_timer(start_time) > I2C_TIMEOUT_MS) {
			debug("Timeout\n");
			return -I2C_NOK_TOUT;
		}
	}
	return 0;
}

/* irq enable/disable functions */
static void i2c_enable_irq(struct nx_i2c_regs *i2c)
{
	unsigned int reg;

	reg = readl(&i2c->iiccon);
	reg |= I2CCON_IRENB;
	writel(reg, &i2c->iiccon);
}

/* irq clear function */
static void i2c_clear_irq(struct nx_i2c_regs *i2c)
{
	clrbits_le32(&i2c->iiccon, I2CCON_IRPND);
}

/* ack enable functions */
static void i2c_enable_ack(struct nx_i2c_regs *i2c)
{
	unsigned int reg;

	reg = readl(&i2c->iiccon);
	reg |= I2CCON_ACKGEN;
	writel(reg, &i2c->iiccon);
}

static void i2c_send_stop(struct nx_i2c_regs *i2c)
{
	unsigned int reg;

	/* Send STOP. */
	reg = readl(&i2c->iicstat);
	reg |= I2C_MODE_MR | I2C_TXRX_ENA;
	reg &= (~I2C_START_STOP);
	writel(reg, &i2c->iicstat);
	i2c_clear_irq(i2c);
}

static int wait_for_xfer(struct nx_i2c_regs *i2c)
{
	unsigned long start_time = get_timer(0);

	do {
		if (readl(&i2c->iiccon) & I2CCON_IRPND)
			return (readl(&i2c->iicstat) & I2CSTAT_NACK) ?
				I2C_NACK : I2C_OK;
	} while (get_timer(start_time) < I2C_TIMEOUT_MS);

	return I2C_NOK_TOUT;
}

static int i2c_transfer(struct nx_i2c_regs *i2c,
		uchar cmd_type,
		uchar chip,
		uchar addr[],
		uchar addr_len,
		uchar data[],
		unsigned short data_len,
		uint seq)
{
	uint status;
	int i = 0, result;

	if (data == 0 || data_len == 0) {
		/*Don't support data transfer of no length or to address 0 */
		debug("i2c_transfer: bad call\n");
		return I2C_NOK;
	}

	i2c_enable_irq(i2c);
	i2c_enable_ack(i2c);

	/* Get the slave chip address going */
	writel(chip, &i2c->iicds);
	status = I2C_TXRX_ENA | I2C_START_STOP;
	if ((cmd_type == I2C_WRITE) || (addr && addr_len))
		status |= I2C_MODE_MT;
	else
		status |= I2C_MODE_MR;
	writel(status, &i2c->iicstat);
	if (seq)
		i2c_clear_irq(i2c);

	/* Wait for chip address to transmit. */
	result = wait_for_xfer(i2c);
	if (result != I2C_OK)
		goto bailout;

	/* If register address needs to be transmitted - do it now. */
	if (addr && addr_len) {  /* register addr */
		while ((i < addr_len) && (result == I2C_OK)) {
			writel(addr[i++], &i2c->iicds);
			i2c_clear_irq(i2c);
			result = wait_for_xfer(i2c);
		}

		i = 0;
		if (result != I2C_OK)
			goto bailout;
	}

	switch (cmd_type) {
	case I2C_WRITE:
		while ((i < data_len) && (result == I2C_OK)) {
			writel(data[i++], &i2c->iicds);
			i2c_clear_irq(i2c);
			result = wait_for_xfer(i2c);
		}
		break;
	case I2C_READ:
		if (addr && addr_len) {
			/*
			 * Register address has been sent, now send slave chip
			 * address again to start the actual read transaction.
			 */
			writel(chip, &i2c->iicds);

			/* Generate a re-START. */
			writel(I2C_MODE_MR | I2C_TXRX_ENA
					| I2C_START_STOP, &i2c->iicstat);
			i2c_clear_irq(i2c);
			result = wait_for_xfer(i2c);
			if (result != I2C_OK)
				goto bailout;
		}

		while ((i < data_len) && (result == I2C_OK)) {
			/* disable ACK for final READ */
			if (i == data_len - 1)
				clrbits_le32(&i2c->iiccon
						, I2CCON_ACKGEN);

			i2c_clear_irq(i2c);
			result = wait_for_xfer(i2c);
			data[i++] = readb(&i2c->iicds);
		}

		if (result == I2C_NACK)
			result = I2C_OK; /* Normal terminated read. */
		break;

	default:
		debug("i2c_transfer: bad call\n");
		result = I2C_NOK;
		break;
	}

bailout:
	return result;
}

static int nx_i2c_read(struct udevice *dev, uchar chip, uint addr,
		uint alen, uchar *buffer, uint len, uint seq)
{
	struct nx_i2c_bus *i2c;
	uchar xaddr[4];
	int ret;

	i2c = dev_get_priv(dev);
	if (!i2c)
		return -EFAULT;

	if (alen > 4) {
		debug("I2C read: addr len %d not supported\n", alen);
		return -EADDRNOTAVAIL;
	}

	if (alen > 0)
		xaddr[0] = (addr >> 24) & 0xFF;

	if (alen > 0) {
		xaddr[0] = (addr >> 24) & 0xFF;
		xaddr[1] = (addr >> 16) & 0xFF;
		xaddr[2] = (addr >> 8) & 0xFF;
		xaddr[3] = addr & 0xFF;
	}

	ret = i2c_transfer(i2c->regs, I2C_READ, chip << 1,
			&xaddr[4-alen], alen, buffer, len, seq);

	if (ret) {
		debug("I2C read failed %d\n", ret);
		return -EIO;
	}

	return 0;
}

static int nx_i2c_write(struct udevice *dev, uchar chip, uint addr,
		uint alen, uchar *buffer, uint len, uint seq)
{
	struct nx_i2c_bus *i2c;
	uchar xaddr[4];
	int ret;

	i2c = dev_get_priv(dev);
	if (!i2c)
		return -EFAULT;

	if (alen > 4) {
		debug("I2C write: addr len %d not supported\n", alen);
		return -EINVAL;
	}

	if (alen > 0) {
		xaddr[0] = (addr >> 24) & 0xFF;
		xaddr[1] = (addr >> 16) & 0xFF;
		xaddr[2] = (addr >> 8) & 0xFF;
		xaddr[3] = addr & 0xFF;
	}

	ret = i2c_transfer(i2c->regs, I2C_WRITE, chip << 1,
			&xaddr[4 - alen], alen, buffer, len, seq);
	if (ret) {
		debug("I2C write failed %d\n", ret);
		return -EIO;
	}

	return 0;
}

static int nx_i2c_xfer(struct udevice *dev, struct i2c_msg *msg, int nmsgs)
{
	struct nx_i2c_bus *bus = dev_get_priv(dev);
	struct nx_i2c_regs *i2c = bus->regs;
	int ret;
	int i;

	/* The power loss by the clock, only during on/off. */
	i2c_set_clk(bus, 1);

	/* Bus State(Busy) check  */
	ret = i2c_is_busy(i2c);
	if (ret < 0)
		return ret;

	for (i = 0; i < nmsgs; msg++, i++) {
		if (msg->flags & I2C_M_RD) {
			ret = nx_i2c_read(dev, msg->addr, 0, 0, msg->buf,
					msg->len, i);
		} else {
			ret = nx_i2c_write(dev, msg->addr, 0, 0, msg->buf,
					msg->len, i);
		}

		if (ret) {
			printf("i2c_xfer: error sending\n");
			return -EREMOTEIO;
		}
	}
	/* Send Stop */
	i2c_send_stop(i2c);
	i2c_set_clk(bus, 0);

	return ret ? -EREMOTEIO : 0;
};

static const struct dm_i2c_ops nx_i2c_ops = {
	.xfer		= nx_i2c_xfer,
	.set_bus_speed	= nx_i2c_set_bus_speed,
};

static const struct udevice_id nx_i2c_ids[] = {
	{ .compatible = "nexell,s5pxx18-i2c" },
	{ }
};

U_BOOT_DRIVER(i2c_nexell) = {
	.name		= "i2c_nexell",
	.id		= UCLASS_I2C,
	.of_match	= nx_i2c_ids,
	.probe		= nx_i2c_probe,
	.priv_auto_alloc_size	= sizeof(struct nx_i2c_bus),
	.ops		= &nx_i2c_ops,
};
