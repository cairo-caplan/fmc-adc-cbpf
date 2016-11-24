#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/byteorder.h>
#include "fmc-adc-100m14b4cha.h"
#include "fa-svec.h"
#include "vmebus.h"

#define VME_NO_ADDR_INCREMENT 1

/* FIXME: move to include again */
#ifndef lower_32_bits
#define lower_32_bits(n) ((u32)(n))
#endif /* lower_32_bits */

static void build_dma_desc(struct vme_dma *desc, unsigned long vme_addr,
			void *addr_dest, ssize_t len)
{
	struct vme_dma_attr *vme;
	struct vme_dma_attr *pci;

	memset(desc, 0, sizeof(struct vme_dma));

	vme = &desc->src;
	pci = &desc->dst;

	desc->dir	= VME_DMA_FROM_DEVICE;
	desc->length    = len;
	desc->novmeinc  = VME_NO_ADDR_INCREMENT;

	desc->ctrl.pci_block_size   = VME_DMA_BSIZE_4096;
	desc->ctrl.pci_backoff_time = VME_DMA_BACKOFF_0;
	desc->ctrl.vme_block_size   = VME_DMA_BSIZE_4096;
	desc->ctrl.vme_backoff_time = VME_DMA_BACKOFF_0;

	vme->data_width = VME_D32;
	vme->am         = VME_A24_USER_DATA_SCT;
	/*vme->am         = VME_A24_USER_MBLT;*/
	vme->addru	= upper_32_bits(vme_addr);
	vme->addrl	= lower_32_bits(vme_addr);

	pci->addru = upper_32_bits((unsigned long)addr_dest);
	pci->addrl = lower_32_bits((unsigned long)addr_dest);

}

/* Endianess */
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 0
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN 1
#endif

static int __get_endian(void)
{
	int i = 1;
	char *p = (char *)&i;

	if (p[0] == 1)
		return LITTLE_ENDIAN;
	else
		return BIG_ENDIAN;
}

static void __endianness(unsigned int byte_length, void *buffer)
{
	int i, size;
	uint32_t *ptr;

	/* CPU may be little endian, VME is big endian */
	if (__get_endian() == LITTLE_ENDIAN) {
		ptr = buffer;
		/* swap samples and trig timetag all seen as 32bits words */
		size = byte_length/4;
		for (i = 0; i < size; ++i, ++ptr)
			*ptr = __be32_to_cpu(*ptr);
	}

}

int fa_svec_dma_start(struct zio_cset *cset)
{
	struct fa_dev *fa = cset->zdev->priv_d;
	struct fa_svec_data *svec_data = fa->carrier_data;
	struct zio_channel *interleave = cset->interleave;
	struct zfad_block *fa_dma_block = interleave->priv_d;
	int i;
	struct vme_dma desc;    /* Vme driver DMA structure */
	unsigned long vme_addr;

	vme_addr = svec_data->vme_base + svec_data->fa_dma_ddr_data;

	/*
	 * write the data address in the ddr_addr register: this
	 * address has been computed after ACQ_END by looking to the
	 * trigger position see fa-irq.c::irq_acq_end.
	 * Be careful: the SVEC HW version expects an address of 32bits word
	 * therefore mem-offset in byte is translated into 32bit word
	 **/
	fa_writel(fa, svec_data->fa_dma_ddr_addr,
			&fa_svec_regfield[FA_DMA_DDR_ADDR],
			fa_dma_block[0].dev_mem_off/4);
	/* Execute DMA shot by shot */
	for (i = 0; i < fa->n_shots; ++i) {
		pr_debug("configure DMA descriptor shot %d "
			"vme addr: 0x%llx destination address: 0x%p len: %d\n",
			i, (long long)vme_addr, fa_dma_block[i].block->data,
			(int)fa_dma_block[i].block->datalen);
		build_dma_desc(&desc, vme_addr,
				fa_dma_block[i].block->data,
				fa_dma_block[i].block->datalen);

		if (vme_do_dma_kernel(&desc))
			return -1;
		__endianness(fa_dma_block[i].block->datalen,
			     fa_dma_block[i].block->data);
	}

	return 0;
}

void fa_svec_dma_done(struct zio_cset *cset)
{
	/* nothing special to do */
}

void fa_svec_dma_error(struct zio_cset *cset)
{
	struct fa_dev *fa = cset->zdev->priv_d;

	dev_err(&fa->fmc->dev,
		"DMA error. All acquisition lost\n");
}
