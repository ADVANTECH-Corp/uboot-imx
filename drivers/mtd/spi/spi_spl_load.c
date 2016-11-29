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

#ifdef CONFIG_SPL_OS_BOOT
/*
 * Load the kernel, check for a valid header we can parse, and if found load
 * the kernel and then device tree.
 */
static int spi_load_image_os(struct spi_flash *flash,
			     struct image_header *header)
{
	/* Read for a header, parse or error out. */
	spi_flash_read(flash, CONFIG_SYS_SPI_KERNEL_OFFS, 0x40,
		       (void *)header);

	if (image_get_magic(header) != IH_MAGIC)
		return -1;

	spl_parse_image_header(header);

	spi_flash_read(flash, CONFIG_SYS_SPI_KERNEL_OFFS,
		       spl_image.size, (void *)spl_image.load_addr);

	/* Read device tree. */
	spi_flash_read(flash, CONFIG_SYS_SPI_ARGS_OFFS,
		       CONFIG_SYS_SPI_ARGS_SIZE,
		       (void *)CONFIG_SYS_SPL_ARGS_ADDR);

	return 0;
}
#endif

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
int spl_spi_load_image(void)
{
	int err = 0;
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

	/* use CONFIG_SYS_TEXT_BASE as temporary storage area */
	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);

#ifdef CONFIG_SPL_OS_BOOT
	if (spl_start_uboot() || spi_load_image_os(flash, header))
#endif
	{
		/* Load u-boot, mkimage header is 64 bytes. */
		err = spi_flash_read(flash, CONFIG_SYS_SPI_U_BOOT_OFFS, 0x40,
				     (void *)header);
		if (err)
			return err;

		spl_parse_image_header(header);
		err = spi_flash_read(flash, CONFIG_SYS_SPI_U_BOOT_OFFS,
			       spl_image.size, (void *)spl_image.load_addr);
	}
#if defined (CONFIG_ADVANTECH) && defined(CONFIG_SPI_BOOT)
	printf("SPI read: dev %d:%d, size %d ... \n"
                , CONFIG_SPL_SPI_BUS, CONFIG_SPL_SPI_CS, spl_image.size);
#endif
	return err;
}
