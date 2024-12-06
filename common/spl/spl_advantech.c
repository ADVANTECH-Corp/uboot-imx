/*
 * (C) Copyright 2010
 * Texas Instruments, <www.ti.com>
 *
 * Aneesh V <aneesh@ti.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <stdio.h>
#include <common.h>
#include <dm.h>
#include <spl.h>
#include <asm/u-boot.h>
#include <nand.h>
#include <fat.h>
#include <version.h>
#include <i2c.h>
#include <image.h>
#include <malloc.h>
#include <dm/root.h>
#include <linux/compiler.h>
#include <serial.h>
#include <hang.h>
#define DEBUG 1
DECLARE_GLOBAL_DATA_PTR;

#ifndef CONFIG_SYS_UBOOT_START
#define CONFIG_SYS_UBOOT_START	CONFIG_SYS_TEXT_BASE
#endif
#ifndef CONFIG_SYS_MONITOR_LEN
/* Unknown U-Boot size, let's assume it will not be more than 200 KB */
#define CONFIG_SYS_MONITOR_LEN	(200 * 1024)
#endif

u32 *boot_params_ptr = NULL;
struct spl_image_info spl_image;

#if defined(CONFIG_ADVANTECH)
#define SPL_VERSION_STRING "Adv-Boot SPL " PLAIN_VERSION " (" U_BOOT_DATE " - " \
                        U_BOOT_TIME ")\n"

const char __weak version_string[] = SPL_VERSION_STRING;
#endif
/* Define board data structure */
static struct bd_info bdata __attribute__ ((section(".data")));

/*
 * Default function to determine if u-boot or the OS should
 * be started. This implementation always returns 1.
 *
 * Please implement your own board specific funcion to do this.
 *
 * RETURN
 * 0 to not start u-boot
 * positive if u-boot should start
 */
#ifdef CONFIG_SPL_OS_BOOT
__weak int spl_start_uboot(void)
{
	puts("SPL: Please implement spl_start_uboot() for your board\n");
	puts("SPL: Direct Linux boot not active!\n");
	return 1;
}
#endif

/*
 * Weak default function for board specific cleanup/preparation before
 * Linux boot. Some boards/platforms might not need it, so just provide
 * an empty stub here.
 */
__weak void spl_board_prepare_for_linux(void)
{
	/* Nothing to do! */
}

void spl_set_header_raw_uboot(void)
{
	spl_image.size = CONFIG_SYS_MONITOR_LEN;
	spl_image.entry_point = CONFIG_SYS_UBOOT_START;
	spl_image.load_addr = CONFIG_SYS_TEXT_BASE;
	spl_image.os = IH_OS_U_BOOT;
	spl_image.name = "U-Boot";
}

void spl_parse_image_header_adv(const struct image_header *header)
{
	u32 header_size = sizeof(struct image_header);

	if (image_get_magic(header) == IH_MAGIC) {
		if (spl_image.flags & SPL_COPY_PAYLOAD_ONLY) {
			/*
			 * On some system (e.g. powerpc), the load-address and
			 * entry-point is located at address 0. We can't load
			 * to 0-0x40. So skip header in this case.
			 */
			spl_image.load_addr = image_get_load(header);
			spl_image.entry_point = image_get_ep(header);
			spl_image.size = image_get_data_size(header);
		} else {
			spl_image.entry_point = image_get_load(header);
			/* Load including the header */
			spl_image.load_addr = spl_image.entry_point -
				header_size;
			spl_image.size = image_get_data_size(header) +
				header_size;
		}
		spl_image.os = image_get_os(header);
		spl_image.name = image_get_name(header);
		debug("spl: payload image: %.*s load addr: 0x%lx size: %d\n",
			(int)sizeof(spl_image.name), spl_image.name,
			spl_image.load_addr, spl_image.size);
	} else {
#ifdef CONFIG_SPL_PANIC_ON_RAW_IMAGE
		/*
		 * CONFIG_SPL_PANIC_ON_RAW_IMAGE is defined when the
		 * code which loads images in SPL cannot guarantee that
		 * absolutely all read errors will be reported.
		 * An example is the LPC32XX MLC NAND driver, which
		 * will consider that a completely unreadable NAND block
		 * is bad, and thus should be skipped silently.
		 */
		panic("** no mkimage signature but raw image not supported");
#else
		/* Signature not found - assume u-boot.bin */
		debug("mkimage signature not found - ih_magic = %x\n",
			header->ih_magic);
		spl_set_header_raw_uboot();
#endif
	}
}

__weak void __noreturn jump_to_image_no_args(struct spl_image_info *spl_image)
{
	typedef void __noreturn (*image_entry_noargs_t)(void);

	image_entry_noargs_t image_entry =
			(image_entry_noargs_t) spl_image->entry_point;

	debug("image entry point: 0x%lX\n", spl_image->entry_point);
	image_entry();
}

#ifdef CONFIG_SPL_RAM_DEVICE
static void spl_ram_load_image(void)
{
	const struct image_header *header;

	/*
	 * Get the header.  It will point to an address defined by handoff
	 * which will tell where the image located inside the flash. For
	 * now, it will temporary fixed to address pointed by U-Boot.
	 */
	header = (struct image_header *)
		(CONFIG_SYS_TEXT_BASE -	sizeof(struct image_header));

	spl_parse_image_header_adv(header);
}
#endif

void board_init_r(gd_t *dummy1, ulong dummy2)
{
	u32 boot_device;
	debug(">>spl:board_init_r()\n");

#if defined(CONFIG_SYS_SPL_MALLOC_START)
	mem_malloc_init(CONFIG_SYS_SPL_MALLOC_START,
			CONFIG_SYS_SPL_MALLOC_SIZE);
	gd->flags |= GD_FLG_FULL_MALLOC_INIT;
#elif defined(CONFIG_SYS_MALLOC_F_LEN)
	gd->malloc_limit = gd->malloc_base + CONFIG_SYS_MALLOC_F_LEN;
	gd->malloc_ptr = 0;
#endif
#ifdef CONFIG_SPL_DM
	dm_init_and_scan(true);
#endif

#ifndef CONFIG_PPC
	/*
	 * timer_init() does not exist on PPC systems. The timer is initialized
	 * and enabled (decrementer) in interrupt_init() here.
	 */
	timer_init();
#endif

#ifdef CONFIG_SPL_BOARD_INIT
	spl_board_init();
#endif

	boot_device = spl_boot_device();
	debug("boot device - %d\n", boot_device);
	switch (boot_device) {
#if defined(CONFIG_ADVANTECH)
	case BOOT_DEVICE_AUTO:
		printf("BOOT_DEVICE_AUTO\n");
#ifdef CONFIG_SPL_MMC_SUPPORT
		/* 1. SD card */
		if (!spl_mmc_load_image(0)) {
			printf("booting from SD\n");
			*(int *)0x22200000 = 0x01;
		} else
#endif
#ifdef CONFIG_TWO_SD_BOOT
		/* 5. Carrier SD card for ROM-7421 */
		if (!spl_mmc_load_image(2)) {
			printf("booting from Carrier SD\n");
			*(int *)0x22200000 = 0x05;
		} else
#endif
#ifdef CONFIG_SPL_SATA_SUPPORT
		/* 2. SATA disk */
		if (!spl_sata_load_image(0)) {
			printf("booting from SATA\n");
			*(int *)0x22200000 = 0x02;
		} else
#endif
#ifdef CONFIG_SPL_MMC_SUPPORT
		/* 3. eMMC flash */
		if(!spl_mmc_load_image(1)) {
			printf("booting from iNAND\n");
			*(int *)0x22200000 = 0x03;
		} else
#endif
		{
			printf("SPL: No bootable device is found!\n");
			hang();
		}
		break;
#ifdef CONFIG_SPL_SATA_SUPPORT
	case BOOT_DEVICE_SATA:
		printf("BOOT_DEVICE_SATA\n");
		spl_sata_load_image(0);
		break;
#endif
#endif /* CONFIG_ADVANTECH */
#ifdef CONFIG_SPL_RAM_DEVICE
	case BOOT_DEVICE_RAM:
		printf("BOOT_DEVICE_RAM\n");
		spl_ram_load_image();
		break;
#endif
#ifdef CONFIG_SPL_MMC_SUPPORT
#if defined(CONFIG_ADVANTECH)
	case BOOT_DEVICE_MMC1:
		printf("BOOT_DEVICE_MMC1\n");
		spl_mmc_load_image(0);
		break;
	case BOOT_DEVICE_MMC2:
		printf("BOOT_DEVICE_MMC2\n");
		spl_mmc_load_image(1);
		break;
	case BOOT_DEVICE_MMC2_2:
		printf("BOOT_DEVICE_MMC2_2\n");
		/* Do nothing */
		break;
#else
	case BOOT_DEVICE_MMC1:
	case BOOT_DEVICE_MMC2:
	case BOOT_DEVICE_MMC2_2:
		printf("BOOT_DEVICE_MMC\n");
		spl_mmc_load_image();
		break;
#endif
#endif
#ifdef CONFIG_SPL_NAND_SUPPORT
	case BOOT_DEVICE_NAND:
		printf("BOOT_DEVICE_NAND\n");
		spl_nand_load_image();
		break;
#endif
#ifdef CONFIG_SPL_ONENAND_SUPPORT
	case BOOT_DEVICE_ONENAND:
		printf("BOOT_DEVICE_ONENAND\n");
		spl_onenand_load_image();
		break;
#endif
#ifdef CONFIG_SPL_NOR_SUPPORT
	case BOOT_DEVICE_NOR:
		printf("BOOT_DEVICE_NOR\n");
		spl_nor_load_image();
		break;
#endif
#ifdef CONFIG_SPL_YMODEM_SUPPORT
	case BOOT_DEVICE_UART:
		printf("BOOT_DEVICE_UART\n");
		spl_ymodem_load_image();
		break;
#endif
#ifdef CONFIG_SPL_SPI_SUPPORT
	case BOOT_DEVICE_SPI:
		printf("BOOT_DEVICE_SPI\n");
		spl_spi_load_image();
		break;
#endif
#ifdef CONFIG_SPL_ETH_SUPPORT
	case BOOT_DEVICE_CPGMAC:
		printf("BOOT_DEVICE_CPGMAC\n");
#ifdef CONFIG_SPL_ETH_DEVICE
		spl_net_load_image(CONFIG_SPL_ETH_DEVICE);
#else
		spl_net_load_image(NULL);
#endif
		break;
#endif
#ifdef CONFIG_SPL_USBETH_SUPPORT
	case BOOT_DEVICE_USBETH:
		printf("BOOT_DEVICE_USBETH\n");
		spl_net_load_image("usb_ether");
		break;
#endif
	default:
		printf("SPL: Un-supported Boot Device\n");
		hang();
	}

	switch (spl_image.os) {
	case IH_OS_U_BOOT:
		printf("Jumping to U-Boot\n");
		break;
#ifdef CONFIG_SPL_OS_BOOT
	case IH_OS_LINUX:
		printf("Jumping to Linux\n");
		spl_board_prepare_for_linux();
		jump_to_image_linux((void *)CONFIG_SYS_SPL_ARGS_ADDR);
#endif
	default:
		printf("Unsupported OS image.. Jumping nevertheless..\n");
	}
	jump_to_image_no_args(&spl_image);
}

/*
 * This requires UART clocks to be enabled.  In order for this to work the
 * caller must ensure that the gd pointer is valid.
 */
void preloader_console_init(void)
{
	gd->bd = &bdata;
	gd->baudrate = CONFIG_BAUDRATE;

	serial_init();		/* serial communications setup */

	gd->have_console = 1;

#if defined(CONFIG_ADVANTECH)
	puts("\n\n" SPL_VERSION_STRING);
#else
	puts("\nU-Boot SPL " PLAIN_VERSION " (" U_BOOT_DATE " - " \
			U_BOOT_TIME ")\n");
#endif
#ifdef CONFIG_SPL_DISPLAY_PRINT
	spl_display_print();
#endif
}

/**
 * spl_relocate_stack_gd() - Relocate stack ready for board_init_r() execution
 *
 * Sometimes board_init_f() runs with a stack in SRAM but we want to use SDRAM
 * for the main board_init_r() execution. This is typically because we need
 * more stack space for things like the MMC sub-system.
 *
 * This function calculates the stack position, copies the global_data into
 * place and returns the new stack position. The caller is responsible for
 * setting up the sp register.
 *
 * @return new stack location, or 0 to use the same stack
 */
ulong spl_relocate_stack_gd(void)
{
#ifdef CONFIG_SPL_STACK_R
	gd_t *new_gd;
	ulong ptr;

	/* Get stack position: use 8-byte alignment for ABI compliance */
	ptr = CONFIG_SPL_STACK_R - sizeof(gd_t);
	ptr &= ~7;
	new_gd = (gd_t *)ptr;
	memcpy(new_gd, (void *)gd, sizeof(gd_t));
	gd = new_gd;

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	return ptr;
#else
	return 0;
#endif
}
