/*
 * Copyright (C) 2014 Advantech Co. <risc-sw@advantech.com.tw>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <spl.h>
#include <asm/gpio.h>

extern const char version_string[];

#ifdef CONFIG_BOOT_SELECT
#define SABRESD_NANDF_CS0	IMX_GPIO_NR(6, 11) //GPIO6_11   
#define SABRESD_NANDF_CS1	IMX_GPIO_NR(6, 14) //GPIO6_14 
#define SABRESD_NANDF_CS2	IMX_GPIO_NR(6, 15) //GPIO6_15 
#endif

u32 mx6_boot_device = BOOT_DEVICE_NONE;

u32 spl_boot_mode(void)
{
    switch (mx6_boot_device) {
    case BOOT_DEVICE_MMC1:
    case BOOT_DEVICE_MMC2:
    case BOOT_DEVICE_MMC2_2:
    case BOOT_DEVICE_AUTO:
#ifdef CONFIG_SPL_FAT_SUPPORT
        return MMCSD_MODE_FAT;
#else
        return MMCSD_MODE_RAW;
#endif
    default:
        puts("spl: ERROR:  unknown device - can't select boot mode\n");
        hang();
    }
}

u32 spl_boot_device(void)
{
#ifdef CONFIG_BOOT_SELECT
	int board_cs0, board_cs1, board_cs2;

	gpio_request(SABRESD_NANDF_CS0, "Board_CS0");
	gpio_direction_input(SABRESD_NANDF_CS0);

	gpio_request(SABRESD_NANDF_CS1, "Board_CS1");
	gpio_direction_input(SABRESD_NANDF_CS1);	  

	gpio_request(SABRESD_NANDF_CS2, "Board_CS2");
	gpio_direction_input(SABRESD_NANDF_CS2); 

	board_cs0 = gpio_get_value(SABRESD_NANDF_CS0);
	board_cs1 = gpio_get_value(SABRESD_NANDF_CS1);
	board_cs2 = gpio_get_value(SABRESD_NANDF_CS2);

	printf("Boot_Switch: cs0=%d,cs1=%d,cs2=%d\n",board_cs0,board_cs1,board_cs2);
	//--------------------------------------------------------------------

	if((!board_cs2)&&(!board_cs1)&&(!board_cs0))		//Carrier SATA//
	{
		printf("booting from Carrier SATA\n");
		*(int *)0x22200000 = 0x02;

		mx6_boot_device = BOOT_DEVICE_SATA;
	}
	else if((!board_cs2)&&(!board_cs1)&&(board_cs0))	//Carrier SD Card//
	{	
		printf("booting from Carrier SD Card\n");
		*(int *)0x22200000 = 0x01;

		mx6_boot_device = BOOT_DEVICE_MMC1;
	}
	else if((!board_cs2)&&(board_cs1)&&(!board_cs0))	//Carrier eMMC Flash//
	{
		printf("booting from Carrier eMMC Flash\n");
	}
	else if((!board_cs2)&&(board_cs1)&&(board_cs0))		//Carrier SPI//
	{
		printf("booting from Carrier SPI\n");
		*(int *)0x22200000 = 0x04;

		mx6_boot_device = BOOT_DEVICE_SPI;
	}
	else if((board_cs2)&&(!board_cs1)&&(!board_cs0))	//Module device//
	{
		printf("booting from Module device\n");
	}
	else if((board_cs2)&&(!board_cs1)&&(board_cs0))		//Remote boot//
	{
		printf("booting from Remote boot\n");
	}
	else if((board_cs2)&&(board_cs1)&&(!board_cs0))		//Module eMMC Flash//
	{
		printf("booting from Module eMMC Flash\n");
		*(int *)0x22200000 = 0x03;

		mx6_boot_device = BOOT_DEVICE_MMC2;
	}
	else if((board_cs2)&&(board_cs1)&&(board_cs0))		//Module SPI//
	{
		printf("booting from Module SPI\n");
	}
	else
	{
		printf("booting not support\n");
	}
	//--------------------------------------------------------------------
#else
	/* We use BOOT_DEVICE_AUTO for auto boot device selection */
	mx6_boot_device = BOOT_DEVICE_AUTO;
#endif
	return mx6_boot_device;
}

#ifdef CONFIG_SPL_BOARD_INIT
void spl_board_init(void)
{
	char *pch,*s;
	char tmp[256];

	/* log adv version */
	pch=strchr(version_string,'2');
	if (pch!=NULL)
	{
		s=strchr(pch,' ');
		strncpy(tmp, pch, s-pch);
		tmp[s-pch]='\0';
		strcpy((void *)0x22300000, tmp);
	}

	/* forword memory size to uboot */
	//*(unsigned int *)0x22400000 = PHYS_SDRAM_1_SIZE;

	/* record ddr bit, 32 or 64 bit */
#ifdef CONFIG_DDR_32BIT 
	*(unsigned int *)0x22500000 = 32;
#else
	*(unsigned int *)0x22500000 = 64;
#endif
}
#endif
