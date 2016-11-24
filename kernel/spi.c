/*
 * SPI access to fine-delay internals
 *
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2 as published by the Free Software Foundation or, at your
 * option, any later version.
 */

#include <linux/jiffies.h>
#include <linux/io.h>
#include <linux/delay.h>
#include "fmc-adc-100m14b4cha.h"

/* SPI register */
#define FA_SPI_RX(x) (x * 4)
#define FA_SPI_TX(x) (x * 4)
#define FA_SPI_CTRL 0x10
#define FA_SPI_DIV 0x14
#define FA_SPI_CS 0x18

/* SPI control register fields mask */
#define FA_SPI_CTRL_CHAR_LEN	0x007F
#define FA_SPI_CTRL_GO		0x0100 /* go/busy */
#define FA_SPI_CTRL_BUSY	0x0100 /* go/busy */
#define FA_SPI_CTRL_Rx_NEG	0x0200
#define FA_SPI_CTRL_Tx_NEG	0x0400
#define FA_SPI_CTRL_LSB		0x0800
#define FA_SPI_CTRL_IE		0x1000
#define FA_SPI_CTRL_ASS		0x2000


int fa_spi_xfer(struct fa_dev *fa, int cs, int num_bits,
		uint32_t tx, uint32_t *rx)
{
	uint32_t regval;
	unsigned long j = jiffies + HZ;
	int err = 0;

	/* Put out value (LSB-aligned) in the T0 register (bits 0..31) */
	fmc_writel(fa->fmc, tx, fa->fa_spi_base + FA_SPI_TX(0));
	/* Configure SPI controller */
	regval = FA_SPI_CTRL_ASS |	/* Automatic Slave Select*/
		FA_SPI_CTRL_Tx_NEG |	/* Change on falling edge */
		num_bits;		/* In CHAR_LEN field */
	fmc_writel(fa->fmc, regval, fa->fa_spi_base + FA_SPI_CTRL);
	/* Set Chip Select */
	fmc_writel(fa->fmc, (1 << cs), fa->fa_spi_base + FA_SPI_CS);
	/* Start transfer */
	fmc_writel(fa->fmc, regval | FA_SPI_CTRL_GO,
		   fa->fa_spi_base + FA_SPI_CTRL);
	/* Wait transfer complete */
	while (fmc_readl(fa->fmc, fa->fa_spi_base + FA_SPI_CTRL)
	       & FA_SPI_CTRL_BUSY) {
		if (jiffies > j) {
			dev_err(&fa->fmc->dev, "SPI transfer error\n");
			err = -EIO;
			goto out;
		}
	}
	/* Transfer compleate, read data */
	regval = fmc_readl(fa->fmc, fa->fa_spi_base + FA_SPI_RX(0));
	if (rx)
		*rx = regval;
out:
	/* Clear Chip Select */
	fmc_writel(fa->fmc, 0, fa->fa_spi_base + FA_SPI_CTRL);

	return err;
}


int fa_spi_init(struct fa_dev *fa)
{
	uint32_t tx, rx;
	int i;

	/* Divider must be 100, according to firmware guide */
	fmc_writel(fa->fmc, 100, fa->fa_spi_base + FA_SPI_DIV);

	/* Force 2's complement data output (register 1, bit 5) */
	fa_spi_xfer(fa, FA_SPI_SS_ADC, 16, (1 << 8) | (1 << 5), &rx);

	if (0) {
		/* Check the current configuration of the ADC chip */
		for (i = 0; i < 5; i++) {
			tx = 0x8000 | (i << 8);
			fa_spi_xfer(fa, FA_SPI_SS_ADC, 16, tx, &rx);
			dev_dbg(&fa->fmc->dev, "LTC register %02x: 0x%02x\n",
				i, rx & 0xff);
		}
	}
	return 0;
}

void fa_spi_exit(struct fa_dev *fa)
{
	/* nothing to do */
}
