/*
 * Some utility functions not supported in the current version of fmc-bus.
 *
 * Copyright (C) 2012-2013 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Author: Alessandro Rubini <rubini@gnudd.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2 as published by the Free Software Foundation or, at your
 * option, any later version.
*/

#include <linux/fmc.h>
#include <linux/fmc-sdb.h>
#include <linux/err.h>
#include <asm/byteorder.h>

/* Finds index-th SDB device that matches (vid/did) pair. */
signed long fmc_find_sdb_device_ext(struct sdb_array *tree,
				    uint64_t vid, uint32_t did, int index,
				    unsigned long *sz)
{
	signed long res = -ENODEV;
	union sdb_record *r;
	struct sdb_product *p;
	struct sdb_component *c;
	int i, n = tree->len;
	uint64_t last, first;
	int ci = 0;

	/* FIXME: what if the first interconnect is not at zero? */
	for (i = 0; i < n; i++) {
		r = &tree->record[i];
		c = &r->dev.sdb_component;
		p = &c->product;

		if (!IS_ERR(tree->subtree[i]))
			res = fmc_find_sdb_device(tree->subtree[i],
						  vid, did, sz);

		/* FIXME: this index SHOULD be recursive, too */
		if (ci == index && res >= 0)
			return res + tree->baseaddr;
		if (r->empty.record_type != sdb_type_device)
			continue;
		if (__be64_to_cpu(p->vendor_id) != vid)
			continue;
		if (__be32_to_cpu(p->device_id) != did)
			continue;
		/* found */
		last = __be64_to_cpu(c->addr_last);
		first = __be64_to_cpu(c->addr_first);
		if (sz)
			*sz = (typeof(*sz)) (last + 1 - first);
		if (ci == index)
			return first + tree->baseaddr;
		ci++;
	}
	return res;
}
