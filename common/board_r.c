/*
 * Copyright (c) 2011 The Chromium OS Authors.
 * (C) Copyright 2002-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <api.h>
/* TODO: can we just include all these headers whether needed or not? */
#if defined(CONFIG_CMD_BEDBUG)
#include <bedbug/type.h>
#endif
#include <command.h>
#include <console.h>
#include <dm.h>
#include <environment.h>
#include <fdtdec.h>
#include <ide.h>
#include <initcall.h>
#include <init_helpers.h>
#ifdef CONFIG_PS2KBD
#include <keyboard.h>
#endif
#if defined(CONFIG_CMD_KGDB)
#include <kgdb.h>
#endif
#include <malloc.h>
#include <mapmem.h>
#ifdef CONFIG_BITBANGMII
#include <miiphy.h>
#endif
#include <mmc.h>
#include <nand.h>
#include <of_live.h>
#include <onenand_uboot.h>
#include <scsi.h>
#include <serial.h>
#include <spi.h>
#include <stdio_dev.h>
#include <timer.h>
#include <trace.h>
#include <watchdog.h>
#ifdef CONFIG_ADDR_MAP
#include <asm/mmu.h>
#endif
#include <asm/sections.h>
#include <dm/root.h>
#include <linux/compiler.h>
#include <linux/err.h>
#include <efi_loader.h>
#ifdef CONFIG_FSL_FASTBOOT
#include <fsl_fastboot.h>
#endif
#if defined(CONFIG_ADVANTECH) || defined(CONFIG_ADVANTECH_MX8)
#include <version.h>
#include <spi_flash.h>

#define CONFIG_SPI_ENV_OFFSET	(768 * 1024)
#define CONFIG_SPI_ENV_SIZE	(8 * 1024)

struct boardcfg_t {
    unsigned char mac[6];
    unsigned char sn[10];
    unsigned char Manufacturing_Time[14];
};

static struct spi_flash *flash;
#endif /* CONFIG_ADVANTECH */

DECLARE_GLOBAL_DATA_PTR;

ulong monitor_flash_len;

__weak int board_flash_wp_on(void)
{
	/*
	 * Most flashes can't be detected when write protection is enabled,
	 * so provide a way to let U-Boot gracefully ignore write protected
	 * devices.
	 */
	return 0;
}

__weak void cpu_secondary_init_r(void)
{
}

static int initr_secondary_cpu(void)
{
	/*
	 * after non-volatile devices & environment is setup and cpu code have
	 * another round to deal with any initialization that might require
	 * full access to the environment or loading of some image (firmware)
	 * from a non-volatile device
	 */
	/* TODO: maybe define this for all archs? */
	cpu_secondary_init_r();

	return 0;
}

static int initr_trace(void)
{
#ifdef CONFIG_TRACE
	trace_init(gd->trace_buff, CONFIG_TRACE_BUFFER_SIZE);
#endif

	return 0;
}

static int initr_reloc(void)
{
	/* tell others: relocation done */
	gd->flags |= GD_FLG_RELOC | GD_FLG_FULL_MALLOC_INIT;

	return 0;
}

#ifdef CONFIG_ARM
/*
 * Some of these functions are needed purely because the functions they
 * call return void. If we change them to return 0, these stubs can go away.
 */
static int initr_caches(void)
{
	/* Enable caches */
	enable_caches();
	return 0;
}
#endif

__weak int fixup_cpu(void)
{
	return 0;
}

static int initr_reloc_global_data(void)
{
#ifdef __ARM__
	monitor_flash_len = _end - __image_copy_start;
#elif defined(CONFIG_NDS32) || defined(CONFIG_RISCV)
	monitor_flash_len = (ulong)&_end - (ulong)&_start;
#elif !defined(CONFIG_SANDBOX) && !defined(CONFIG_NIOS2)
	monitor_flash_len = (ulong)&__init_end - gd->relocaddr;
#endif
#if defined(CONFIG_MPC85xx) || defined(CONFIG_MPC86xx)
	/*
	 * The gd->cpu pointer is set to an address in flash before relocation.
	 * We need to update it to point to the same CPU entry in RAM.
	 * TODO: why not just add gd->reloc_ofs?
	 */
	gd->arch.cpu += gd->relocaddr - CONFIG_SYS_MONITOR_BASE;

	/*
	 * If we didn't know the cpu mask & # cores, we can save them of
	 * now rather than 'computing' them constantly
	 */
	fixup_cpu();
#endif
#ifdef CONFIG_SYS_EXTRA_ENV_RELOC
	/*
	 * Some systems need to relocate the env_addr pointer early because the
	 * location it points to will get invalidated before env_relocate is
	 * called.  One example is on systems that might use a L2 or L3 cache
	 * in SRAM mode and initialize that cache from SRAM mode back to being
	 * a cache in cpu_init_r.
	 */
	gd->env_addr += gd->relocaddr - CONFIG_SYS_MONITOR_BASE;
#endif
#ifdef CONFIG_OF_EMBED
	/*
	 * The fdt_blob needs to be moved to new relocation address
	 * incase of FDT blob is embedded with in image
	 */
	gd->fdt_blob += gd->reloc_off;
#endif
#ifdef CONFIG_EFI_LOADER
	efi_runtime_relocate(gd->relocaddr, NULL);
#endif

	return 0;
}

static int initr_serial(void)
{
	serial_initialize();
	return 0;
}

#if defined(CONFIG_PPC) || defined(CONFIG_M68K) || defined(CONFIG_MIPS)
static int initr_trap(void)
{
	/*
	 * Setup trap handlers
	 */
#if defined(CONFIG_PPC)
	trap_init(gd->relocaddr);
#else
	trap_init(CONFIG_SYS_SDRAM_BASE);
#endif
	return 0;
}
#endif

#ifdef CONFIG_ADDR_MAP
static int initr_addr_map(void)
{
	init_addr_map();

	return 0;
}
#endif

#ifdef CONFIG_POST
static int initr_post_backlog(void)
{
	post_output_backlog();
	return 0;
}
#endif

#if defined(CONFIG_SYS_INIT_RAM_LOCK) && defined(CONFIG_E500)
static int initr_unlock_ram_in_cache(void)
{
	unlock_ram_in_cache();	/* it's time to unlock D-cache in e500 */
	return 0;
}
#endif

#ifdef CONFIG_PCI
static int initr_pci(void)
{
#ifndef CONFIG_DM_PCI
	pci_init();
#endif

	return 0;
}
#endif

static int initr_barrier(void)
{
#ifdef CONFIG_PPC
	/* TODO: Can we not use dmb() macros for this? */
	asm("sync ; isync");
#endif
	return 0;
}

static int initr_malloc(void)
{
	ulong malloc_start;

#if CONFIG_VAL(SYS_MALLOC_F_LEN)
	debug("Pre-reloc malloc() used %#lx bytes (%ld KB)\n", gd->malloc_ptr,
	      gd->malloc_ptr / 1024);
#endif
	/* The malloc area is immediately below the monitor copy in DRAM */
	malloc_start = gd->relocaddr - TOTAL_MALLOC_LEN;
	mem_malloc_init((ulong)map_sysmem(malloc_start, TOTAL_MALLOC_LEN),
			TOTAL_MALLOC_LEN);
	return 0;
}

static int initr_console_record(void)
{
#if defined(CONFIG_CONSOLE_RECORD)
	return console_record_init();
#else
	return 0;
#endif
}

#ifdef CONFIG_SYS_NONCACHED_MEMORY
static int initr_noncached(void)
{
	noncached_init();
	return 0;
}
#endif

#ifdef CONFIG_OF_LIVE
static int initr_of_live(void)
{
	int ret;

	bootstage_start(BOOTSTAGE_ID_ACCUM_OF_LIVE, "of_live");
	ret = of_live_build(gd->fdt_blob, (struct device_node **)&gd->of_root);
	bootstage_accum(BOOTSTAGE_ID_ACCUM_OF_LIVE);
	if (ret)
		return ret;

	return 0;
}
#endif

#ifdef CONFIG_DM
static int initr_dm(void)
{
	int ret;

	/* Save the pre-reloc driver model and start a new one */
	gd->dm_root_f = gd->dm_root;
	gd->dm_root = NULL;
#ifdef CONFIG_TIMER
	gd->timer = NULL;
#endif
	bootstage_start(BOOTSTATE_ID_ACCUM_DM_R, "dm_r");
	ret = dm_init_and_scan(false);
	bootstage_accum(BOOTSTATE_ID_ACCUM_DM_R);
	if (ret)
		return ret;
#ifdef CONFIG_TIMER_EARLY
	ret = dm_timer_init();
	if (ret)
		return ret;
#endif

	return 0;
}
#endif

static int initr_bootstage(void)
{
	bootstage_mark_name(BOOTSTAGE_ID_START_UBOOT_R, "board_init_r");

	return 0;
}

__weak int power_init_board(void)
{
	return 0;
}

static int initr_announce(void)
{
	debug("Now running in RAM - U-Boot at: %08lx\n", gd->relocaddr);
	return 0;
}

#ifdef CONFIG_NEEDS_MANUAL_RELOC
static int initr_manual_reloc_cmdtable(void)
{
	fixup_cmdtable(ll_entry_start(cmd_tbl_t, cmd),
		       ll_entry_count(cmd_tbl_t, cmd));
	return 0;
}
#endif

#if defined(CONFIG_MTD_NOR_FLASH)
static int initr_flash(void)
{
	ulong flash_size = 0;
	bd_t *bd = gd->bd;

	puts("Flash: ");

	if (board_flash_wp_on())
		printf("Uninitialized - Write Protect On\n");
	else
		flash_size = flash_init();

	print_size(flash_size, "");
#ifdef CONFIG_SYS_FLASH_CHECKSUM
	/*
	 * Compute and print flash CRC if flashchecksum is set to 'y'
	 *
	 * NOTE: Maybe we should add some WATCHDOG_RESET()? XXX
	 */
	if (env_get_yesno("flashchecksum") == 1) {
		const uchar *flash_base = (const uchar *)CONFIG_SYS_FLASH_BASE;

		printf("  CRC: %08X", crc32(0,
					    flash_base,
					    flash_size));
	}
#endif /* CONFIG_SYS_FLASH_CHECKSUM */
	putc('\n');

	/* update start of FLASH memory    */
#ifdef CONFIG_SYS_FLASH_BASE
	bd->bi_flashstart = CONFIG_SYS_FLASH_BASE;
#endif
	/* size of FLASH memory (final value) */
	bd->bi_flashsize = flash_size;

#if defined(CONFIG_SYS_UPDATE_FLASH_SIZE)
	/* Make a update of the Memctrl. */
	update_flash_size(flash_size);
#endif

#if defined(CONFIG_OXC) || defined(CONFIG_RMU)
	/* flash mapped at end of memory map */
	bd->bi_flashoffset = CONFIG_SYS_TEXT_BASE + flash_size;
#elif CONFIG_SYS_MONITOR_BASE == CONFIG_SYS_FLASH_BASE
	bd->bi_flashoffset = monitor_flash_len;	/* reserved area for monitor */
#endif
	return 0;
}
#endif

#if defined(CONFIG_PPC) && !defined(CONFIG_DM_SPI)
static int initr_spi(void)
{
	/* PPC does this here */
#ifdef CONFIG_SPI
#if !defined(CONFIG_ENV_IS_IN_EEPROM)
	spi_init_f();
#endif
	spi_init_r();
#endif
	return 0;
}
#endif

#ifdef CONFIG_CMD_NAND
/* go init the NAND */
static int initr_nand(void)
{
	puts("NAND:  ");
	nand_init();
	printf("%lu MiB\n", nand_size() / 1024);
	return 0;
}
#endif

#if defined(CONFIG_CMD_ONENAND)
/* go init the NAND */
static int initr_onenand(void)
{
	puts("NAND:  ");
	onenand_init();
	return 0;
}
#endif

#ifdef CONFIG_MMC
static int initr_mmc(void)
{
	puts("MMC:   ");
	mmc_initialize(gd->bd);
	return 0;
}
#endif

/*
 * Tell if it's OK to load the environment early in boot.
 *
 * If CONFIG_OF_CONTROL is defined, we'll check with the FDT to see
 * if this is OK (defaulting to saying it's OK).
 *
 * NOTE: Loading the environment early can be a bad idea if security is
 *       important, since no verification is done on the environment.
 *
 * @return 0 if environment should not be loaded, !=0 if it is ok to load
 */
static int should_load_env(void)
{
#ifdef CONFIG_OF_CONTROL
	return fdtdec_get_config_int(gd->fdt_blob, "load-environment", 1);
#elif defined CONFIG_DELAY_ENVIRONMENT
	return 0;
#else
	return 1;
#endif
}

static int initr_env(void)
{
	/* initialize environment */
	if (should_load_env())
		env_relocate();
	else
		set_default_env(NULL);
#ifdef CONFIG_OF_CONTROL
	env_set_addr("fdtcontroladdr", gd->fdt_blob);
#endif

	/* Initialize from environment */
	load_addr = env_get_ulong("loadaddr", 16, load_addr);

	return 0;
}

#ifdef CONFIG_SYS_BOOTPARAMS_LEN
static int initr_malloc_bootparams(void)
{
	gd->bd->bi_boot_params = (ulong)malloc(CONFIG_SYS_BOOTPARAMS_LEN);
	if (!gd->bd->bi_boot_params) {
		puts("WARNING: Cannot allocate space for boot parameters\n");
		return -ENOMEM;
	}
	return 0;
}
#endif

static int initr_jumptable(void)
{
	jumptable_init();
	return 0;
}

#if defined(CONFIG_API)
static int initr_api(void)
{
	/* Initialize API */
	api_init();
	return 0;
}
#endif

/* enable exceptions */
#ifdef CONFIG_ARM
static int initr_enable_interrupts(void)
{
	enable_interrupts();
	return 0;
}
#endif

#ifdef CONFIG_CMD_NET
static int initr_ethaddr(void)
{
	bd_t *bd = gd->bd;

	/* kept around for legacy kernels only ... ignore the next section */
	eth_env_get_enetaddr("ethaddr", bd->bi_enetaddr);
#ifdef CONFIG_HAS_ETH1
	eth_env_get_enetaddr("eth1addr", bd->bi_enet1addr);
#endif
#ifdef CONFIG_HAS_ETH2
	eth_env_get_enetaddr("eth2addr", bd->bi_enet2addr);
#endif
#ifdef CONFIG_HAS_ETH3
	eth_env_get_enetaddr("eth3addr", bd->bi_enet3addr);
#endif
#ifdef CONFIG_HAS_ETH4
	eth_env_get_enetaddr("eth4addr", bd->bi_enet4addr);
#endif
#ifdef CONFIG_HAS_ETH5
	eth_env_get_enetaddr("eth5addr", bd->bi_enet5addr);
#endif
	return 0;
}

#if defined(CONFIG_ADVANTECH) || defined(CONFIG_ADVANTECH_MX8)
#define XMK_STR(x)	#x
#define MK_STR(x)	XMK_STR(x)

static int get_eth0_mac(void)
{
	int rc = 0;
	struct boardcfg_t boardcfg;
	char print_buf[32];
	uint64_t macaddr = 0;

	if(spi_flash_read(flash, CONFIG_SPI_ENV_OFFSET + 8*CONFIG_SPI_ENV_SIZE, sizeof(boardcfg), &boardcfg)==0) {

		/*printf("offset=%d\n", CONFIG_SPI_ENV_OFFSET+ 8*CONFIG_SPI_ENV_SIZE);*/

		/*printf("0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X\n",
						boardcfg.mac[0],
						boardcfg.mac[1],
						boardcfg.mac[2],
						boardcfg.mac[3],
						boardcfg.mac[4],
						boardcfg.mac[5]);*/


		macaddr = ((uint64_t)boardcfg.mac[0] << 40)
			+ ((uint64_t)boardcfg.mac[1] << 32)
			+ ((uint64_t)boardcfg.mac[2] << 24)
			+ ((uint64_t)boardcfg.mac[3] << 16)
			+ ((uint64_t)boardcfg.mac[4] << 8)
			+ boardcfg.mac[5];
		/* printf ("MAC addr =%012llX\n", macaddr); */

		if( (macaddr==0) || (macaddr==0xFFFFFFFFFFFFull) ) {
			printf("eth0 MAC address is invailed !!\n");
			sprintf(print_buf,"0x00:0x04:0x9F:0x01:0x30:0xE0");
			printf("Use default MAC adderss:%s\n",print_buf);
			env_set("ethaddr",print_buf);
			return rc;
		}
	} else {
		printf("SPI Read fail!!\n");
		rc = -1;
	}

	if (rc==0) {
		sprintf(print_buf, "0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X",
						boardcfg.mac[0],
						boardcfg.mac[1],
						boardcfg.mac[2],
						boardcfg.mac[3],
						boardcfg.mac[4],
						boardcfg.mac[5]);
		printf ("eth0 MAC addr = %s\n", print_buf);

		if( (env_get("ethaddr") == NULL) ||
			(strcmp (env_get("ethaddr"),print_buf) != 0) ||
			(strcmp (env_get("ethaddr"),MK_STR(CONFIG_ETHADDR)) == 0) ) {
			env_set("ethaddr", print_buf);
		}
	}

	return rc;
}

#ifdef CONFIG_HAS_ETH1
static int get_eth1_mac(void)
{
	int rc = 0;
	struct boardcfg_t boardcfg;
	char print_buf[32];
	uint64_t macaddr = 0;

	if(spi_flash_read(flash, CONFIG_SPI_ENV_OFFSET + 8*CONFIG_SPI_ENV_SIZE + 1024, sizeof(boardcfg), &boardcfg)==0) {

		/*printf("offset=%d\n", CONFIG_SPI_ENV_OFFSET+ 8*CONFIG_SPI_ENV_SIZE);*/

		/*printf("0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X\n",
						boardcfg.mac[0],
						boardcfg.mac[1],
						boardcfg.mac[2],
						boardcfg.mac[3],
						boardcfg.mac[4],
						boardcfg.mac[5]);*/


		macaddr = ((uint64_t)boardcfg.mac[0] << 40)
			+ ((uint64_t)boardcfg.mac[1] << 32)
			+ ((uint64_t)boardcfg.mac[2] << 24)
			+ ((uint64_t)boardcfg.mac[3] << 16)
			+ ((uint64_t)boardcfg.mac[4] << 8)
			+ boardcfg.mac[5];
		/* printf ("MAC addr =%012llX\n", macaddr); */

		if( (macaddr==0) || (macaddr==0xFFFFFFFFFFFFull) ) {
			printf("eth1 MAC address is invailed !!\n");
			sprintf(print_buf,"0x00:0x04:0x9F:0x01:0x30:0xE0");
			printf("Use default MAC adderss:%s\n",print_buf);
			env_set("eth1addr",print_buf);
			return rc;
		}
	} else {
		printf("SPI Read fail!!\n");
		rc = -1;
	}

	if (rc==0) {
		sprintf(print_buf, "0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X",
						boardcfg.mac[0],
						boardcfg.mac[1],
						boardcfg.mac[2],
						boardcfg.mac[3],
						boardcfg.mac[4],
						boardcfg.mac[5]);
		printf ("eth1 MAC addr = %s\n", print_buf);

		if( (env_get("eth1addr") == NULL) ||
			(strcmp (env_get("eth1addr"),print_buf) != 0) ||
			(strcmp (env_get("eth1addr"),MK_STR(CONFIG_ETH1ADDR)) == 0) ) {
			env_set("eth1addr", print_buf);
		}
	}

	return rc;
}
#endif

int boardcfg_get_mac(void)
{
	int rc = 0;

	flash = spi_flash_probe(CONFIG_SF_DEFAULT_BUS, CONFIG_SF_DEFAULT_CS,
				CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE);
	if (!flash)
		return -1;

	rc = get_eth0_mac();
#ifdef CONFIG_HAS_ETH1
	rc = get_eth1_mac();
#endif

	return rc;
}
#endif /* CONFIG_ADVANTECH */
#endif /* CONFIG_CMD_NET */

#ifdef CONFIG_CMD_KGDB
static int initr_kgdb(void)
{
	puts("KGDB:  ");
	kgdb_init();
	return 0;
}
#endif

#if defined(CONFIG_LED_STATUS)
static int initr_status_led(void)
{
#if defined(CONFIG_LED_STATUS_BOOT)
	status_led_set(CONFIG_LED_STATUS_BOOT, CONFIG_LED_STATUS_BLINKING);
#else
	status_led_init();
#endif
	return 0;
}
#endif

#if defined(CONFIG_SCSI) && !defined(CONFIG_DM_SCSI)
static int initr_scsi(void)
{
	puts("SCSI:  ");
	scsi_init();

	return 0;
}
#endif

#ifdef CONFIG_BITBANGMII
static int initr_bbmii(void)
{
	bb_miiphy_init();
	return 0;
}
#endif

#ifdef CONFIG_CMD_NET
static int initr_net(void)
{
	puts("Net:   ");
	eth_initialize();
#if defined(CONFIG_RESET_PHY_R)
	debug("Reset Ethernet PHY\n");
	reset_phy();
#endif
	return 0;
}
#endif

#ifdef CONFIG_POST
static int initr_post(void)
{
	post_run(NULL, POST_RAM | post_bootmode_get(0));
	return 0;
}
#endif

#if defined(CONFIG_CMD_PCMCIA) && !defined(CONFIG_IDE)
static int initr_pcmcia(void)
{
	puts("PCMCIA:");
	pcmcia_init();
	return 0;
}
#endif

#if defined(CONFIG_IDE)
static int initr_ide(void)
{
	puts("IDE:   ");
#if defined(CONFIG_START_IDE)
	if (board_start_ide())
		ide_init();
#else
	ide_init();
#endif
	return 0;
}
#endif

#if defined(CONFIG_PRAM)
/*
 * Export available size of memory for Linux, taking into account the
 * protected RAM at top of memory
 */
int initr_mem(void)
{
	ulong pram = 0;
	char memsz[32];

	pram = env_get_ulong("pram", 10, CONFIG_PRAM);
	sprintf(memsz, "%ldk", (long int)((gd->ram_size / 1024) - pram));
	env_set("mem", memsz);

	return 0;
}
#endif

#ifdef CONFIG_CMD_BEDBUG
static int initr_bedbug(void)
{
	bedbug_init();

	return 0;
}
#endif

#ifdef CONFIG_PS2KBD
static int initr_kbd(void)
{
	puts("PS/2:  ");
	kbd_init();
	return 0;
}
#endif

#if defined(AVB_RPMB) && !defined(CONFIG_SPL)
extern int init_avbkey(void);
static int initr_avbkey(void)
{
	return init_avbkey();
}
#endif

#ifdef CONFIG_FSL_FASTBOOT
static int initr_fastboot_setup(void)
{
	fastboot_setup();
	return 0;
}

static int initr_check_fastboot(void)
{
	fastboot_run_bootmode();
	return 0;
}
#endif

#ifdef CONFIG_IMX_TRUSTY_OS
extern void tee_setup(void);
static int initr_tee_setup(void)
{
	tee_setup();
	return 0;
}
#endif

#ifdef CONFIG_ADVANTECH
int check_emmc_exist(void)
{
        struct mmc *mmc = find_mmc_device(CONFIG_EMMC_DEV_NUM);
        int result=0;
        if (mmc) {
                result = mmc_init(mmc);
                if(result != 0) {
                        printf("*** emmc cannot init, emmc device doesn't exist\n");
                        return 0;
                }
                return 1;
        } else{
                return 0;
        }
}

int board_set_boot_device(void)
{
	char buf[256];
	char advboot_version[128];
	char uboot_version[128];
	char *pch,*s;
	 
	int dev = (*(int *)0x22200000);
	int emmc_exist = 0;

	/* log uboot version */
	strncpy(advboot_version, (void *)0x22300000, 128);

	if ((strstr(advboot_version,"advantech") == NULL) && (strstr(advboot_version,"LIV") == NULL))
#ifdef CONFIG_ANDROID_SUPPORT
		strcpy(advboot_version, "NA");
#else
		strcpy(advboot_version, "");
#endif

	pch=strchr(version_string,'2');
	if (pch!=NULL)
	{
		s=strchr(pch,' ');
		strncpy(uboot_version, pch, s-pch);
		uboot_version[s-pch]='\0';
	}

	sprintf(buf, "advboot_version=%s uboot_version=%s", advboot_version, uboot_version);
#ifdef CONFIG_ANDROID_SUPPORT
	env_set("bootargs_adv", buf);
#else
	env_set("bootargs", buf);
#endif

	// check emmc exists or not
	emmc_exist = check_emmc_exist();

	switch(dev)
	{
#ifdef CONFIG_ANDROID_SUPPORT
	/* Android */
		case 1:
		default:
			/* booting from SD*/
			printf("booting from SD\n");
			sprintf(buf, "%s androidboot.selinux=disabled", env_get("bootargs_adv"));
			env_set("bootargs_adv", buf);
			env_set("bootargs", "console=ttymxc0,115200");
			env_set("fastboot_dev", "mmc0");
			env_set("bootcmd", "boota mmc0");
			break;
		case 2:
			/* booting from SATA*/
			printf("booting from SATA\n");
			sprintf(buf, "%s androidboot.selinux=disabled androidboot.fs=sata", env_get("bootargs_adv"));
			env_set("bootargs_adv", buf);
			env_set("fastboot_dev", "sata");
			env_set("bootcmd", "boota sata");
			break;
		case 3:
			/* booting from iNAND*/
			printf("booting from iNAND\n");
			sprintf(buf, "%s androidboot.selinux=disabled androidboot.fs=emmc", env_get("bootargs_adv"));
			env_set("bootargs_adv", buf);
			env_set("fastboot_dev", "mmc1");
			env_set("bootcmd", "boota mmc1");
			break;
#ifdef CONFIG_SPI_BOOT_SUPPORT
		case 4:
			/* booting from SPI*/
			printf("booting from SPI -> kernel boot form EMMC\n");
			sprintf(buf, "%s androidboot.selinux=disabled androidboot.fs=emmc", env_get("bootargs_adv"));
			env_set("bootargs_adv", buf);
			if(emmc_exist) {
				env_set("fastboot_dev", "mmc1");
				env_set("bootcmd", "boota mmc1");
			}
			break;
#endif
#else
	/* Linux */
		case 1:
		default:
			/* booting from SD*/
			printf("booting from SD\n");
			env_set("mmcdev", "0");
			if(emmc_exist) {
				sprintf(buf, "/dev/mmcblk1p2 rootwait rw");
				env_set("mmcroot",buf);
			}
			break;
		case 2:
			/* booting from SATA*/
			printf("booting from SATA\n");
			sprintf(buf, "fatload sata 0:1 ${loadaddr} ${image}");
			env_set("loadimage", buf);
			sprintf(buf, "fatload sata 0:1 ${fdt_addr} ${fdt_file}");
			env_set("loadfdt", buf);
			sprintf(buf, "fatload sata 0:1 ${loadaddr} ${script}");
			env_set("loadbootscript", buf);
			sprintf(buf, "/dev/sda2 rootwait rw");
			env_set("sataroot", buf);
			sprintf(buf, "setenv bootargs console=${console},${baudrate} ${smp} root=${sataroot} ${bootargs}");
			env_set("sataargs", buf);
			sprintf(buf, "dcache off; sata init; run loadimage; run loadbootscript; run sataargs; run loadfdt; bootz ${loadaddr} - ${fdt_addr}");
			env_set("bootcmd", buf);
			break;
		case 3:
			/* booting from iNAND*/
			printf("booting from iNAND\n");
			env_set("mmcdev", MK_STR(CONFIG_EMMC_DEV_NUM));
			break;
#ifdef CONFIG_SPI_BOOT
		case 4:
			/* booting from SPI*/
			printf("booting from SPI -> kernel boot form EMMC\n");
			if(emmc_exist) 	env_set("mmcdev", MK_STR(CONFIG_EMMC_DEV_NUM));
			break;
#endif
#if defined(USDHC2_CD_GPIO) && defined(USDHC3_CD_GPIO)
		case 5:
			/* booting from Carrier SD*/
			printf("booting from Carrier SD\n");
			env_set("mmcdev", MK_STR(CONFIG_CARRIERSD_DEV_NUM));
			sprintf(buf, "/dev/mmcblk2p2 rootwait rw");
			env_set("mmcroot",buf);
                        break;
#endif
#endif /*CONFIG_ANDROID_SUPPORT*/
	}

	/*record ddr bit, 32 or 64 bit*/
	sprintf(buf, "%d", *(unsigned int *)0x22500000);
	env_set("ddr_bit", buf);

	return 0;
}
#endif /* CONFIG_ADVANTECH */

static int run_main_loop(void)
{
#ifdef CONFIG_SANDBOX
	sandbox_main_loop_init();
#endif
	/* main_loop() can return to retry autoboot, if so just run it again */
	for (;;)
		main_loop();
	return 0;
}

/*
 * Over time we hope to remove these functions with code fragments and
 * stub functions, and instead call the relevant function directly.
 *
 * We also hope to remove most of the driver-related init and do it if/when
 * the driver is later used.
 *
 * TODO: perhaps reset the watchdog in the initcall function after each call?
 */
static init_fnc_t init_sequence_r[] = {
	initr_trace,
	initr_reloc,
	/* TODO: could x86/PPC have this also perhaps? */
#ifdef CONFIG_ARM
	initr_caches,
	/* Note: For Freescale LS2 SoCs, new MMU table is created in DDR.
	 *	 A temporary mapping of IFC high region is since removed,
	 *	 so environmental variables in NOR flash is not available
	 *	 until board_init() is called below to remap IFC to high
	 *	 region.
	 */
#endif
	initr_reloc_global_data,
#if defined(CONFIG_SYS_INIT_RAM_LOCK) && defined(CONFIG_E500)
	initr_unlock_ram_in_cache,
#endif
	initr_barrier,
	initr_malloc,
	log_init,
	initr_bootstage,	/* Needs malloc() but has its own timer */
	initr_console_record,
#ifdef CONFIG_SYS_NONCACHED_MEMORY
	initr_noncached,
#endif
	bootstage_relocate,
#ifdef CONFIG_OF_LIVE
	initr_of_live,
#endif
#ifdef CONFIG_DM
	initr_dm,
#endif
#if defined(CONFIG_ARM) || defined(CONFIG_NDS32) || defined(CONFIG_RISCV)
	board_init,	/* Setup chipselects */
#endif
	/*
	 * TODO: printing of the clock inforamtion of the board is now
	 * implemented as part of bdinfo command. Currently only support for
	 * davinci SOC's is added. Remove this check once all the board
	 * implement this.
	 */
#ifdef CONFIG_CLOCKS
	set_cpu_clk_info, /* Setup clock information */
#endif
#ifdef CONFIG_EFI_LOADER
	efi_memory_init,
#endif
	stdio_init_tables,
	initr_serial,
	initr_announce,
	INIT_FUNC_WATCHDOG_RESET
#ifdef CONFIG_NEEDS_MANUAL_RELOC
	initr_manual_reloc_cmdtable,
#endif
#if defined(CONFIG_PPC) || defined(CONFIG_M68K) || defined(CONFIG_MIPS)
	initr_trap,
#endif
#ifdef CONFIG_ADDR_MAP
	initr_addr_map,
#endif
#if defined(CONFIG_BOARD_EARLY_INIT_R)
	board_early_init_r,
#endif
	INIT_FUNC_WATCHDOG_RESET
#ifdef CONFIG_POST
	initr_post_backlog,
#endif
	INIT_FUNC_WATCHDOG_RESET
#if defined(CONFIG_PCI) && defined(CONFIG_SYS_EARLY_PCI_INIT)
	/*
	 * Do early PCI configuration _before_ the flash gets initialised,
	 * because PCU resources are crucial for flash access on some boards.
	 */
	initr_pci,
#endif
#ifdef CONFIG_ARCH_EARLY_INIT_R
	arch_early_init_r,
#endif
	power_init_board,
#ifdef CONFIG_MTD_NOR_FLASH
	initr_flash,
#endif
	INIT_FUNC_WATCHDOG_RESET
#if defined(CONFIG_PPC) || defined(CONFIG_M68K) || defined(CONFIG_X86)
	/* initialize higher level parts of CPU like time base and timers */
	cpu_init_r,
#endif
#ifdef CONFIG_PPC
	initr_spi,
#endif
#ifdef CONFIG_CMD_NAND
	initr_nand,
#endif
#ifdef CONFIG_CMD_ONENAND
	initr_onenand,
#endif
#ifdef CONFIG_MMC
	initr_mmc,
#endif
	initr_env,
#ifdef CONFIG_SYS_BOOTPARAMS_LEN
	initr_malloc_bootparams,
#endif
	INIT_FUNC_WATCHDOG_RESET
	initr_secondary_cpu,
#if defined(CONFIG_ID_EEPROM) || defined(CONFIG_SYS_I2C_MAC_OFFSET)
	mac_read_from_eeprom,
#endif
	INIT_FUNC_WATCHDOG_RESET
#if defined(CONFIG_PCI) && !defined(CONFIG_SYS_EARLY_PCI_INIT)
	/*
	 * Do pci configuration
	 */
	initr_pci,
#endif
	stdio_add_devices,
	initr_jumptable,
#ifdef CONFIG_API
	initr_api,
#endif
	console_init_r,		/* fully init console as a device */
#ifdef CONFIG_DISPLAY_BOARDINFO_LATE
	console_announce_r,
	show_board_info,
#endif
#ifdef CONFIG_ARCH_MISC_INIT
	arch_misc_init,		/* miscellaneous arch-dependent init */
#endif
#ifdef CONFIG_MISC_INIT_R
	misc_init_r,		/* miscellaneous platform-dependent init */
#endif
	INIT_FUNC_WATCHDOG_RESET
#ifdef CONFIG_CMD_KGDB
	initr_kgdb,
#endif
	interrupt_init,
#ifdef CONFIG_ARM
	initr_enable_interrupts,
#endif
#if defined(CONFIG_MICROBLAZE) || defined(CONFIG_M68K)
	timer_init,		/* initialize timer */
#endif
#if defined(CONFIG_LED_STATUS)
	initr_status_led,
#endif
	/* PPC has a udelay(20) here dating from 2002. Why? */
#ifdef CONFIG_CMD_NET
	initr_ethaddr,
#if defined(CONFIG_ADVANTECH) || defined(CONFIG_ADVANTECH_MX8)
	boardcfg_get_mac,	/* Get MAC address from SPI */
#endif
#endif
#ifdef CONFIG_BOARD_LATE_INIT
	board_late_init,
#endif
#ifdef CONFIG_ADVANTECH
	board_set_boot_device,
#endif
#ifdef CONFIG_FSL_FASTBOOT
	initr_fastboot_setup,
#endif
#if defined(CONFIG_SCSI) && !defined(CONFIG_DM_SCSI)
	INIT_FUNC_WATCHDOG_RESET
	initr_scsi,
#endif
#ifdef CONFIG_BITBANGMII
	initr_bbmii,
#endif
#ifdef CONFIG_CMD_NET
	INIT_FUNC_WATCHDOG_RESET
	initr_net,
#endif
#ifdef CONFIG_POST
	initr_post,
#endif
#if defined(CONFIG_CMD_PCMCIA) && !defined(CONFIG_IDE)
	initr_pcmcia,
#endif
#if defined(CONFIG_IDE)
	initr_ide,
#endif
#ifdef CONFIG_LAST_STAGE_INIT
	INIT_FUNC_WATCHDOG_RESET
	/*
	 * Some parts can be only initialized if all others (like
	 * Interrupts) are up and running (i.e. the PC-style ISA
	 * keyboard).
	 */
	last_stage_init,
#endif
#ifdef CONFIG_CMD_BEDBUG
	INIT_FUNC_WATCHDOG_RESET
	initr_bedbug,
#endif
#if defined(CONFIG_PRAM)
	initr_mem,
#endif
#ifdef CONFIG_PS2KBD
	initr_kbd,
#endif
#if defined(AVB_RPMB) && !defined(CONFIG_SPL)
	initr_avbkey,
#endif
#ifdef CONFIG_IMX_TRUSTY_OS
	initr_tee_setup,
#endif
#ifdef CONFIG_FSL_FASTBOOT
	initr_check_fastboot,
#endif
	run_main_loop,
};

void board_init_r(gd_t *new_gd, ulong dest_addr)
{
	/*
	 * Set up the new global data pointer. So far only x86 does this
	 * here.
	 * TODO(sjg@chromium.org): Consider doing this for all archs, or
	 * dropping the new_gd parameter.
	 */
#if CONFIG_IS_ENABLED(X86_64)
	arch_setup_gd(new_gd);
#endif

#ifdef CONFIG_NEEDS_MANUAL_RELOC
	int i;
#endif

#if !defined(CONFIG_X86) && !defined(CONFIG_ARM) && !defined(CONFIG_ARM64)
	gd = new_gd;
#endif
	gd->flags &= ~GD_FLG_LOG_READY;

#ifdef CONFIG_NEEDS_MANUAL_RELOC
	for (i = 0; i < ARRAY_SIZE(init_sequence_r); i++)
		init_sequence_r[i] += gd->reloc_off;
#endif

	if (initcall_run_list(init_sequence_r))
		hang();

	/* NOTREACHED - run_main_loop() does not return */
	hang();
}
