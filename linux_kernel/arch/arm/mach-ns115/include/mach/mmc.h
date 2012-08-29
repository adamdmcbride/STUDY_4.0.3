#ifndef	__NS115_SDIO_H
#define	__NS115_SDIO_H

#include <linux/types.h>
#include <linux/device.h>
#include <linux/mmc/host.h>

#define	MAX_SLOTS	4

#define	SD_CARD		1
#define	MMC_CARD	2
#define	EMMC_CARD	4
#define	SDIO_CARD	8

struct device;
struct mmc_host;

struct ns115_mmc_platform_data {
	/* link to device */
	struct device *dev;
	u8		nr_slots;
	unsigned int	host_max_freq;
	unsigned int 	detect_delay_ms;
	unsigned int 	gpio;

	struct evatronix_sdio_slot_data {
		/* have to do for DMA per slot basis */
		struct device slot_dev;

		u8	ctype;
		u8	bus_width;
		u32	ocr_avail;
		u32	max_freq;
		u32	freq;
	} slots[MAX_SLOTS];
};

#endif
