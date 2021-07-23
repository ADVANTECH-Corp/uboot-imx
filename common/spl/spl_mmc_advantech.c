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
#include <image.h>
#include <asm/u-boot.h>
#include <asm/utils.h>
#include <mmc.h>
#include <fat.h>
#include <version.h>
#include <u-boot/crc.h>
DECLARE_GLOBAL_DATA_PTR;

extern struct spl_image_info spl_image;


static int spl_mmc_check_crc(unsigned int dev,struct mmc *mmc)
{
#ifdef CONFIG_SPL_FAT_SUPPORT
	/* Needs to implement later */
#else
	u32 n;
	/* read crc file */
	char tag[512];
	char crc[512];
	
	n = mmc->block_dev.block_read(&mmc->block_dev, 0x02, 1, (void *) 0x22100000);
	if(n != 1)
		return 1;
	
	memcpy(tag, (void *) 0x22100000, 512);
	//tag[9] = '\0';
	//printf("crc file %s\n", tag);
	
	/* make uboot crc */
	n = mmc->block_dev.block_read(&mmc->block_dev, 0x03, 0x4b0, (void *) 0x22000000);
	if(n != 0x4b0)
		return 1;
	
	*(int *)0x21f00000 = crc32 (0, (const uchar *) 0x22000000, 0x96000);
	sprintf(crc, "%08x", *(int *)0x21f00000);
	//crc[9] = '\0';
	//printf("uboot crc %s\n", crc);
	
	/* verrify crc */
	if(memcmp(tag, crc, 8))
	{
		printf("spl: mmc dev %d - crc error\n", dev);
		return 1;
	}
#endif
	return 0;
}

static int mmc_load_image_raw(unsigned int dev, struct mmc *mmc)
{
	u32 image_size_sectors, err;
	const struct image_header *header;
	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE -
						sizeof(struct image_header));

	/* read image header to find the image size & load address */
	err = mmc->block_dev.block_read(&mmc->block_dev,
			CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR, 1,
			(void *)header);

	if (err <= 0)
		goto end;

	spl_parse_image_header_adv(header);

	/* convert size to sectors - round up */
	image_size_sectors = (spl_image.size + mmc->read_bl_len - 1) /
				mmc->read_bl_len;

	/* Read the header too to avoid extra memcpy */
	err = mmc->block_dev.block_read(&mmc->block_dev,
			CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR,
			image_size_sectors, (void *)spl_image.load_addr);

	printf("MMC read: dev # %d, block # %d,""count %d ... \n",
		dev, CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR, image_size_sectors);
end:
	if (err <= 0) {
		printf("spl: mmc blk read err - %d\n", err);
		return 1;
	} else
		return 0;
}

#ifdef CONFIG_SPL_FAT_SUPPORT
static int mmc_load_image_fat(struct mmc *mmc)
{
	s32 err;
	struct image_header *header;

	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE -
						sizeof(struct image_header));

	err = fat_register_device(&mmc->block_dev,
				CONFIG_SYS_MMC_SD_FAT_BOOT_PARTITION);
	if (err) {
		printf("spl: fat register err - %d\n", err);
		return 1;
	}

	err = file_fat_read(CONFIG_SPL_FAT_LOAD_PAYLOAD_NAME,
				(u8 *)header, sizeof(struct image_header));
	if (err <= 0)
		goto end;

	spl_parse_image_header(header);

	err = file_fat_read(CONFIG_SPL_FAT_LOAD_PAYLOAD_NAME,
				(u8 *)spl_image.load_addr, 0);

end:
	if (err <= 0) {
		printf("spl: error reading image %s, err - %d\n",
			CONFIG_SPL_FAT_LOAD_PAYLOAD_NAME, err);
		return 1;
	} else
		return 0;
}
#endif

int spl_mmc_load_image(unsigned int dev)
{
	struct mmc *mmc;
	int err;
	u32 boot_mode;

	mmc_initialize(gd->bd);

	mmc = find_mmc_device(dev);

	if (!mmc) {
		puts("spl: mmc device not found!!\n");
		return 1;
	}

	err = mmc_init(mmc);
	if (err) {
		printf("spl: mmc dev %d init failed\n", dev);
		return 1;
	}

	err = spl_mmc_check_crc(dev,mmc);
	if (err) {
		return 1;
	}

	boot_mode = spl_boot_mode();
	if (boot_mode == MMCSD_MODE_RAW) {
		debug("boot mode - RAW\n");
		return mmc_load_image_raw(dev, mmc);
#ifdef CONFIG_SPL_FAT_SUPPORT
	} else if (boot_mode == MMCSD_MODE_FAT) {
		debug("boot mode - FAT\n");
		return mmc_load_image_fat(mmc);
#endif
	} else {
		puts("spl: wrong MMC boot mode\n");
		return 1;
	}
}
