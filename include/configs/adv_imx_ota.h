#define CONFIG_ADV_OTA_SUPPORT

#define CONFIG_CMD_BOOTA
#define CONFIG_SUPPORT_RAW_INITRD
#define CONFIG_ANDROID_RECOVERY
#define CONFIG_BCB_SUPPORT

#ifdef CONFIG_ADVANTECH
#define CONFIG_USB_FUNCTION_FASTBOOT
#define CONFIG_CMD_FASTBOOT
#define CONFIG_ANDROID_BOOT_IMAGE
#define CONFIG_FASTBOOT_STORAGE_MMC
#define CONFIG_FSL_FASTBOOT
#define CONFIG_FASTBOOT_BUF_ADDR   CONFIG_SYS_LOAD_ADDR
#define CONFIG_FASTBOOT_BUF_SIZE   0x19000000
#endif /* CONFIG_ADVANTECH */