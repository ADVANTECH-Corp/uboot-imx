/*
 * (C) Copyright 2014 Advantech Co. <risc-sw@advantech.com.tw>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <spl.h>
#include <asm/u-boot.h>
#include <asm/utils.h>
#include <sata.h>
#include <libata.h>
#include <version.h>

DECLARE_GLOBAL_DATA_PTR;

extern struct spl_image_info spl_image;

static int spl_sata_check_crc(unsigned int dev)
{
	u32 n;
	/* read crc file */
	char tag[512];
	char crc[512];
	
	n = sata_read(dev, 0x02, 1, (void *) 0x22100000);
	if(n != 1)
		return 1;
	
	memcpy(tag, (void *) 0x22100000, 512);
	//tag[9] = '\0';
	//printf("crc file %s\n", tag);
	
	/* make uboot crc */
	n = sata_read(dev, 0x03, 0x4b0, (void *) 0x22000000);
	if(n != 0x4b0)
		return 1;
	
	*(int *)0x21f00000 = crc32 (0, (const uchar *) 0x22000000, 0x96000);
	sprintf(crc, "%08x", *(int *)0x21f00000);
	//crc[9] = '\0';
	//printf("uboot crc %s\n", crc);
	
	/* verrify crc */
	if(memcmp(tag, crc, 8))
	{
		printf("spl: sata dev %d - crc error\n", dev);
		return 1;
	}

	return 0;
}

static int sata_load_image_raw(unsigned int dev)
{
	u32 image_size_sectors, err;
	const struct image_header *header;

	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE -
						sizeof(struct image_header));

	/* read image header to find the image size & load address */
	err = sata_read(dev,
			CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR, 1,
			(void *)header);

	if (err <= 0)
		goto end;

	spl_parse_image_header(header);

	/* convert size to sectors - round up */
	image_size_sectors = (spl_image.size + ATA_SECT_SIZE - 1) /
				ATA_SECT_SIZE;

	/* Read the header too to avoid extra memcpy */
	err = sata_read(dev,
			CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR,
			image_size_sectors, (void *)spl_image.load_addr);

	printf("SATA read: dev # %d block # %d, count %d ... ",
		dev, CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR, image_size_sectors);
end:
	if (err <= 0) {
		printf("spl: sata blk read err - %d\n", err);
		return 1;
	} else
		return 0;
}

int spl_sata_load_image(unsigned int dev)
{
	struct blk_desc *sata_dev_desc;
	int err;

	if (sata_initialize()) {
		printf("spl: sata_initialize failed\n");
		return 1;
	}

	sata_dev_desc = sata_get_dev(dev);
	if (!sata_dev_desc) {
		printf("spl: cannot find sata dev %d\n", dev);
		return 1;
	}

	err = spl_sata_check_crc(dev);
	if (err) {
		return 1;
	}

	return sata_load_image_raw(dev);
}
