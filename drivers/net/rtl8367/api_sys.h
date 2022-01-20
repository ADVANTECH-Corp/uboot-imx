#ifndef __RTK_API_SYS_H__
#define __RTK_API_SYS_H__
#include <asm-generic/gpio.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/arch/imx8mp_pins.h>
#include <asm/mach-imx/gpio.h>


#include <common.h>
#include <cpu_func.h>
#include <dm.h>
#include <env.h>
#include <malloc.h>
#include <memalign.h>
#include <miiphy.h>
#include <net.h>
#include <netdev.h>
#include <power/regulator.h>

#include <asm/io.h>
#include <linux/errno.h>
#include <linux/compiler.h>

typedef struct  switch_test_info_s
{
    int test_port_number;
    int test_full_100;
    
}switch_test_info_t;
void adv_set_i2c(unsigned scl,unsigned sda);
void rtl8367_switch_init(switch_test_info_t test_info);

/*
 * Data Type Declaration
 */


#endif /* __RTK_API_SYS_H__ */