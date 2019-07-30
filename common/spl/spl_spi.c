/*
 * Copyright (C) 2011 OMICRON electronics GmbH
 *
 * based on drivers/mtd/nand/nand_spl_load.c
 *
 * Copyright (C) 2011
 * Heiko Schocher, DENX Software Engineering, hs@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <spi.h>
#include <spi_flash.h>
#include <errno.h>
#include <spl.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_SPL_OS_BOOT
/*
 * Load the kernel, check for a valid header we can parse, and if found load
 * the kernel and then device tree.
 */
static int spi_load_image_os(struct spl_image_info *spl_image,
			     struct spi_flash *flash,
			     struct image_header *header)
{
	int err;

	/* Read for a header, parse or error out. */
	spi_flash_read(flash, CONFIG_SYS_SPI_KERNEL_OFFS, 0x40,
		       (void *)header);

	if (image_get_magic(header) != IH_MAGIC)
		return -1;

	err = spl_parse_image_header(spl_image, header);
	if (err)
		return err;

	spi_flash_read(flash, CONFIG_SYS_SPI_KERNEL_OFFS,
		       spl_image->size, (void *)spl_image->load_addr);

	/* Read device tree. */
	spi_flash_read(flash, CONFIG_SYS_SPI_ARGS_OFFS,
		       CONFIG_SYS_SPI_ARGS_SIZE,
		       (void *)CONFIG_SYS_SPL_ARGS_ADDR);

	return 0;
}
#endif

#ifdef CONFIG_SYS_SPI_U_BOOT_OFFS
unsigned long  __weak spl_spi_get_uboot_raw_sector(struct spi_flash *flash)
{
	return CONFIG_SYS_SPI_U_BOOT_OFFS;
}
#endif

#ifdef CONFIG_PARSE_CONTAINER
int __weak spi_load_image_parse_container(struct spl_image_info *spl_image,
					  struct spi_flash *flash,
					  unsigned long offset)
{
	return -EINVAL;
}
#endif

static ulong spl_spi_fit_read(struct spl_load_info *load, ulong sector,
			      ulong count, void *buf)
{
	struct spi_flash *flash = load->dev;
	ulong ret;

	ret = spi_flash_read(flash, sector, count, buf);
	if (!ret)
		return count;
	else
		return 0;
}

#if defined (CONFIG_ADVANTECH) && defined(CONFIG_SPI_BOOT)
static int spl_spi_check_crc(struct spi_flash *flash)
{
        /* read crc file */
        char tag[512];
        char crc[512];

        if(spi_flash_read(flash, 2*512, 512, (void *) 0x22100000) == 0)
        {
                //printf("SPI Read success!\n");
        }
        else
        {
                printf("SPI Read fail!!\n");
                return 1;
        }

        memcpy(tag, (void *) 0x22100000, 512);
        //tag[9] = '\0';
        //printf("crc file %s\n", tag);

        /* make uboot crc */
        if(spi_flash_read(flash, 3*512, 0x96000, (void *) 0x22000000) == 0)
        {
                //printf("SPI Read success!\n");
        }
        else
        {
                printf("SPI Read fail!!\n");
                return 1;
        }

        *(int *)0x21f00000 = crc32 (0, (const uchar *) 0x22000000, 0x96000);
        sprintf(crc, "%08x", *(int *)0x21f00000);
        //crc[9] = '\0';
        //printf("uboot crc %s\n", crc);

        /* verrify crc */
        if(memcmp(tag, crc, 8))
        {
                printf("spl: spi dev - crc error\n");
                return 1;
        }

        return 0;
}
#endif

/*
 * The main entry for SPI booting. It's necessary that SDRAM is already
 * configured and available since this code loads the main U-Boot image
 * from SPI into SDRAM and starts it from there.
 */
static int spl_spi_load_image(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev)
{
	int err = 0;
	unsigned payload_offs = 0;
	struct spi_flash *flash;
	struct image_header *header;

	/*
	 * Load U-Boot image from SPI flash into RAM
	 */
#if defined (CONFIG_ADVANTECH) && defined(CONFIG_SPI_BOOT)
	flash = spi_flash_probe(CONFIG_SPL_SPI_BUS, CONFIG_SPL_SPI_CS,
                                CONFIG_SF_DEFAULT_SPEED, SPI_MODE_3);
#else
	flash = spi_flash_probe(CONFIG_SF_DEFAULT_BUS,
				CONFIG_SF_DEFAULT_CS,
				CONFIG_SF_DEFAULT_SPEED,
				CONFIG_SF_DEFAULT_MODE);
#endif
	if (!flash) {
		puts("SPI probe failed.\n");
		return -ENODEV;
	}

#if defined (CONFIG_ADVANTECH) && defined(CONFIG_SPI_BOOT)
        err = spl_spi_check_crc(flash);
        if (err)
		return err;
#endif

	payload_offs = spl_spi_get_uboot_raw_sector(flash);

	/* use CONFIG_SYS_TEXT_BASE as temporary storage area */
	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);

#if CONFIG_IS_ENABLED(OF_CONTROL) && !CONFIG_IS_ENABLED(OF_PLATDATA)
	payload_offs = fdtdec_get_config_int(gd->fdt_blob,
					     "u-boot,spl-payload-offset",
					     payload_offs);
#endif

#ifdef CONFIG_SPL_OS_BOOT
	if (spl_start_uboot() || spi_load_image_os(spl_image, flash, header))
#endif
	{
		/* Load u-boot, mkimage header is 64 bytes. */
		err = spi_flash_read(flash, payload_offs, 0x40,
				     (void *)header);
		if (err) {
			debug("%s: Failed to read from SPI flash (err=%d)\n",
			      __func__, err);
			return err;
		}

		if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) &&
			image_get_magic(header) == FDT_MAGIC) {
			struct spl_load_info load;

			debug("Found FIT\n");
			load.dev = flash;
			load.priv = NULL;
			load.filename = NULL;
			load.bl_len = 1;
			load.read = spl_spi_fit_read;
			err = spl_load_simple_fit(spl_image, &load,
						  payload_offs,
						  header);
		} else {
#ifdef CONFIG_PARSE_CONTAINER
			err = spi_load_image_parse_container(spl_image,
							     flash,
							     payload_offs);
#else
			err = spl_parse_image_header(spl_image, header);
			if (err)
				return err;
			err = spi_flash_read(flash, payload_offs,
					     spl_image->size,
					     (void *)spl_image->load_addr);
#endif
		}
	}

#if defined (CONFIG_ADVANTECH) && defined(CONFIG_SPI_BOOT)
	printf("SPI read: dev %d:%d, size %d ... \n"
                , CONFIG_SPL_SPI_BUS, CONFIG_SPL_SPI_CS, spl_image.size);
#endif

	return err;
}
/* Use priorty 1 so that boards can override this */
SPL_LOAD_IMAGE_METHOD("SPI", 1, BOOT_DEVICE_SPI, spl_spi_load_image);
