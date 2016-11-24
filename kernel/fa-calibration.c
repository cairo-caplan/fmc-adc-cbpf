/*
 * EEPROM calibration block retreival code for fa-dev
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2 as published by the Free Software Foundation or, at your
 * option, any later version.
 */

#include "fmc-adc-100m14b4cha.h"

/* This identity calibration is used as default */
static struct fa_calib_stanza fa_identity_calib = {
	.offset = { 0, },
	.gain = {0x8000, 0x8000, 0x8000, 0x8000},
	.temperature = 50 * 100, /* 50 celsius degrees */
};
/* Max difference from identity thing */
#define FA_CALIB_MAX_DELTA_OFFSET	0x1000
#define FA_CALIB_MAX_DELTA_GAIN		0x1000
#define FA_CALIB_MAX_DELTA_TEMP		(40 * 100) /* 10-90 celsius */

/* Actual verification code */
static int fa_verify_calib_stanza(struct device *msgdev, char *name, int r,
				    struct fa_calib_stanza *cal,
				    struct fa_calib_stanza *iden)
{
	int i, err = 0;

	for (i = 0; i < ARRAY_SIZE(cal->offset); i++) {
		if (abs(cal->offset[i] - iden->offset[i])
		    > FA_CALIB_MAX_DELTA_OFFSET) {
			dev_dbg(msgdev, "wrong offset 0x%x\n", cal->offset[i]);
			err++;
		}
		if (abs((s16)(cal->gain[i] - iden->gain[i]))
		    > FA_CALIB_MAX_DELTA_GAIN) {
			dev_dbg(msgdev, "wrong gain   0x%x\n", cal->gain[i]);
			err++;
		}
	}
	if (abs((s16)(cal->temperature - iden->temperature))
	    > FA_CALIB_MAX_DELTA_TEMP) {
		dev_dbg(msgdev, "wrong temper 0x%x\n", cal->temperature);
		err++;
	}
	if (err)
		dev_dbg(msgdev, "%i errors in %s calibration, range %i\n",
			err, name, r);
	return err;
}

static void fa_verify_calib(struct device *msgdev,
			      struct fa_calib *calib,
			      struct fa_calib_stanza *identity)
{
	int i, err = 0;

	for (i = 0; i < ARRAY_SIZE(calib->adc); i++) {
		err += fa_verify_calib_stanza(msgdev, "adc", i,
						calib->adc + i, identity);
		err += fa_verify_calib_stanza(msgdev, "dac", i,
						calib->dac + i, identity);
	}
	if (!err)
		return;

	dev_info(msgdev, "Invalid calibration in EEPROM (%i errors)\n", err);
	dev_info(msgdev, "Using identity calibration\n");
	for (i = 0; i < ARRAY_SIZE(calib->adc); i++) {
		calib->adc[i] = *identity;
		calib->dac[i] = *identity;
	}
}

static void fa_endian_calib(struct fa_calib *calib)
{
	int i;
	uint16_t *p = (void *)calib;

	/* We know for sure that our structure is only made of 16bit fields */
	for (i = 0; i < sizeof(*calib) / sizeof(uint16_t); i++)
		le16_to_cpus(p + i); /* s == in situ */
}


void fa_read_eeprom_calib(struct fa_dev *fa)
{
	/* Retrieve calibration data from the eeprom, then verify it */
	memcpy(&fa->calib, fa->fmc->eeprom + FA_CAL_OFFSET, sizeof(fa->calib));
	fa_endian_calib(&fa->calib);
	fa_verify_calib(&fa->fmc->dev, &fa->calib, &fa_identity_calib);
	dev_info(&fa->fmc->dev, "%s succeeds.\n", __func__);
}
