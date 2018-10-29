/* linux/arch/arm/mach-s5pv210/mach-smdkv210.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/dm9000.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/device.h>
#include <linux/dm9000.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/pwm_backlight.h>
#include <linux/platform_data/s3c-hsotg.h>

#include <asm/hardware/vic.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <video/platform_lcd.h>
#include <video/samsung_fimd.h>

#include <mach/map.h>
#include <mach/regs-clock.h>

#include <plat/regs-serial.h>
#include <plat/regs-srom.h>
#include <plat/gpio-cfg.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/adc.h>
#include <linux/platform_data/touchscreen-s3c2410.h>
#include <linux/platform_data/ata-samsung_cf.h>
#include <linux/platform_data/i2c-s3c2410.h>
#include <plat/keypad.h>
#include <plat/pm.h>
#include <plat/fb.h>
#include <plat/s5p-time.h>
#include <plat/backlight.h>
#include <plat/mfc.h>
#include <plat/clock.h>
#include <linux/gpio_keys.h>	/* Add by Webee */

#include "common.h"

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define SMDKV210_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define SMDKV210_ULCON_DEFAULT	S3C2410_LCON_CS8

#define SMDKV210_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

static struct s3c2410_uartcfg smdkv210_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= SMDKV210_UCON_DEFAULT,
		.ulcon		= SMDKV210_ULCON_DEFAULT,
		.ufcon		= SMDKV210_UFCON_DEFAULT,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= SMDKV210_UCON_DEFAULT,
		.ulcon		= SMDKV210_ULCON_DEFAULT,
		.ufcon		= SMDKV210_UFCON_DEFAULT,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= SMDKV210_UCON_DEFAULT,
		.ulcon		= SMDKV210_ULCON_DEFAULT,
		.ufcon		= SMDKV210_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= SMDKV210_UCON_DEFAULT,
		.ulcon		= SMDKV210_ULCON_DEFAULT,
		.ufcon		= SMDKV210_UFCON_DEFAULT,
	},
};

static struct s3c_ide_platdata smdkv210_ide_pdata __initdata = {
	.setup_gpio	= s5pv210_ide_setup_gpio,
};

static uint32_t smdkv210_keymap[] __initdata = {
	/* KEY(row, col, keycode) */
	KEY(0, 3, KEY_1), KEY(0, 4, KEY_2), KEY(0, 5, KEY_3),
	KEY(0, 6, KEY_4), KEY(0, 7, KEY_5),
	KEY(1, 3, KEY_A), KEY(1, 4, KEY_B), KEY(1, 5, KEY_C),
	KEY(1, 6, KEY_D), KEY(1, 7, KEY_E)
};

static struct matrix_keymap_data smdkv210_keymap_data __initdata = {
	.keymap		= smdkv210_keymap,
	.keymap_size	= ARRAY_SIZE(smdkv210_keymap),
};

static struct samsung_keypad_platdata smdkv210_keypad_data __initdata = {
	.keymap_data	= &smdkv210_keymap_data,
	.rows		= 8,
	.cols		= 8,
};

////////////////////////////////////////////////////////////////////
static struct resource smdkv210_dm9000_resources[] = {
	[0] = DEFINE_RES_MEM(0x88001000, 1),
	[1] = DEFINE_RES_MEM(0x88001000 + 0x300c, 1),
	[2] = DEFINE_RES_NAMED(IRQ_EINT(7), 1, NULL, IORESOURCE_IRQ \
				| IORESOURCE_IRQ_HIGHLEVEL),
};

static struct dm9000_plat_data smdkv210_dm9000_platdata = {
	.flags		= DM9000_PLATF_16BITONLY | DM9000_PLATF_NO_EEPROM,
	.dev_addr	= { 0x00, 0x09, 0xc0, 0xff, 0xec, 0x48 },
};

static struct platform_device smdkv210_dm9000 = {
	.name		= "dm9000",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(smdkv210_dm9000_resources),
	.resource	= smdkv210_dm9000_resources,
	.dev		= {
		.platform_data	= &smdkv210_dm9000_platdata,
	},
};
/////////////////////////////////////////////////////////////////////

static void smdkv210_lte480wv_set_power(struct plat_lcd_data *pd,
					unsigned int power)
{
	if (power) {
#if !defined(CONFIG_BACKLIGHT_PWM)
		gpio_request_one(S5PV210_GPD0(3), GPIOF_OUT_INIT_HIGH, "GPD0");
		gpio_free(S5PV210_GPD0(3));
#endif

		/* fire nRESET on power up */
		gpio_request_one(S5PV210_GPH0(6), GPIOF_OUT_INIT_HIGH, "GPH0");

		gpio_set_value(S5PV210_GPH0(6), 0);
		mdelay(10);

		gpio_set_value(S5PV210_GPH0(6), 1);
		mdelay(10);

		gpio_free(S5PV210_GPH0(6));
	} else {
#if !defined(CONFIG_BACKLIGHT_PWM)
		gpio_request_one(S5PV210_GPD0(3), GPIOF_OUT_INIT_LOW, "GPD0");
		gpio_free(S5PV210_GPD0(3));
#endif
	}
}

static struct plat_lcd_data smdkv210_lcd_lte480wv_data = {
	.set_power	= smdkv210_lte480wv_set_power,
};

static struct platform_device smdkv210_lcd_lte480wv = {
	.name			= "platform-lcd",
	.dev.parent		= &s3c_device_fb.dev,
	.dev.platform_data	= &smdkv210_lcd_lte480wv_data,
};

static struct s3c_fb_pd_win smdkv210_fb_win0 = {
	.max_bpp	= 32,
	.default_bpp	= 24,
	.xres		= 800,
	.yres		= 480,
};

static struct fb_videomode smdkv210_lcd_timing = {
	.left_margin	= 13,
	.right_margin	= 8,
	.upper_margin	= 7,
	.lower_margin	= 5,
	.hsync_len	= 3,
	.vsync_len	= 1,
	.xres		= 800,
	.yres		= 480,
};

static struct s3c_fb_platdata smdkv210_lcd0_pdata __initdata = {
	.win[0]		= &smdkv210_fb_win0,
	.vtiming	= &smdkv210_lcd_timing,
	.vidcon0	= VIDCON0_VIDOUT_RGB | VIDCON0_PNRMODE_RGB,
	.vidcon1	= VIDCON1_INV_HSYNC | VIDCON1_INV_VSYNC,
	.setup_gpio	= s5pv210_fb_gpio_setup_24bpp,
};

/* USB OTG */
static struct s3c_hsotg_plat smdkv210_hsotg_pdata;

////////////////////////////////
#define S5PV210_PA_NAND                 (0xB0E00000)
#define S5PV210_SZ_NAND                 SZ_1M

static struct resource s3c_nand_resource[] = {
	[0] = {
		.start	= S5PV210_PA_NAND,
		.end	= S5PV210_PA_NAND + S5PV210_SZ_NAND - 1,
		.flags	= IORESOURCE_MEM,
	}
};

struct platform_device s3c_device_nand = {
	.name		= "webee210-nand",
	.id			= -1,
	.num_resources	= ARRAY_SIZE(s3c_nand_resource),
	.resource	= s3c_nand_resource,
};
//////////////////////////////////

/***********************Add by Webee********************************/

static struct gpio_keys_button webee210_buttons[] = {
	{
		.gpio		= S5PV210_GPH2(0),		/* S1 */
		.code		= KEY_A,
		.desc		= "Button 1",
		.active_low	= 1,
	},
	{
		.gpio		= S5PV210_GPH2(1),		/* S2 */
		.code		= KEY_B,
		.desc		= "Button 2",
		.active_low	= 1,
	},
	{
		.gpio		= S5PV210_GPH2(2),		/* S3 */
		.code		= KEY_C,
		.desc		= "Button 3",
		.active_low	= 1,
	},
	{
		.gpio		= S5PV210_GPH2(3),		/* S4 */
		.code		= KEY_L,
		.desc		= "Button 4",
		.active_low	= 1,
	},
	{
		.gpio		= S5PV210_GPH3(0),		/* S5 */
		.code		= KEY_S,
		.desc		= "Button 5",
		.active_low	= 1,
	},
	{
		.gpio		= S5PV210_GPH3(1),		/* S6 */
		.code		= KEY_ENTER,
		.desc		= "Button 6",
		.active_low	= 1,
	},
	{
		.gpio		= S5PV210_GPH3(2),		/* S7 */
		.code		= KEY_LEFTSHIFT,
		.desc		= "Button 7",
		.active_low	= 1,
	},
	{
		.gpio		= S5PV210_GPH3(3),		/* S8 */
		.code		= KEY_DELETE,
		.desc		= "Button 8",
		.active_low	= 1,
	},
};

static struct gpio_keys_platform_data webee210_button_data = {
	.buttons	= webee210_buttons,
	.nbuttons	= ARRAY_SIZE(webee210_buttons),
};

/* 设备相关的信息会保存在pdev->dev.platform_data中 */
static struct platform_device webee210_button_device = {
	.name		= "gpio-keys",
	.id		= -1,
	.dev		= {
		.platform_data	= &webee210_button_data,
	}
};

/***********************Add by Webee********************************/



static struct platform_device *smdkv210_devices[] __initdata = {
	&s3c_device_nand,			
	&s3c_device_adc,
	&s3c_device_cfcon,
	&s3c_device_fb,
	&s3c_device_hsmmc0,
	&s3c_device_hsmmc1,
	&s3c_device_hsmmc2,
	&s3c_device_hsmmc3,
	&s3c_device_i2c0,
	&s3c_device_i2c1,
	&s3c_device_i2c2,
	&s3c_device_rtc,
	&s3c_device_ts,
	&s3c_device_usb_hsotg,
	&s3c_device_wdt,
	&s5p_device_fimc0,
	&s5p_device_fimc1,
	&s5p_device_fimc2,
	&s5p_device_fimc_md,
	&s5p_device_jpeg,
	&s5p_device_mfc,
	&s5p_device_mfc_l,
	&s5p_device_mfc_r,
	&s5pv210_device_ac97,
	&s5pv210_device_iis0,
	&s5pv210_device_spdif,
	&samsung_asoc_idma,
	&samsung_device_keypad,
	&smdkv210_dm9000,				
	&smdkv210_lcd_lte480wv,
	&webee210_button_device,		/* Add by Webee  */
};
/*
static void __init smdkv210_dm9000_init(void)
{
	unsigned int tmp;

	gpio_request(S5PV210_MP01(5), "nCS5");
	s3c_gpio_cfgpin(S5PV210_MP01(5), S3C_GPIO_SFN(2));
	gpio_free(S5PV210_MP01(5));

	tmp = (5 << S5P_SROM_BCX__TACC__SHIFT);
	__raw_writel(tmp, S5P_SROM_BC5);

	tmp = __raw_readl(S5P_SROM_BW);
	tmp &= (S5P_SROM_BW__CS_MASK << S5P_SROM_BW__NCS5__SHIFT);
	tmp |= (1 << S5P_SROM_BW__NCS5__SHIFT);
	__raw_writel(tmp, S5P_SROM_BW);
}
*/
///////////////////////////////////////////////
#include <sound/wm8960.h>
static struct wm8960_data wm8960_pdata = {
	.capless		= 0,
	.dres			= WM8960_DRES_400R,
};
static struct i2c_board_info smdkv210_i2c_devs0[] __initdata = {
	{ I2C_BOARD_INFO("24c08", 0x50), },     /* Samsung S524AD0XD1 */
//	{
//		I2C_BOARD_INFO("wm8960", 0x34),
//		.platform_data = &wm8960_pdata,
//	},
};
/////////////////////////////////////////////
#include <linux/input/edt-ft5x06.h>

static struct i2c_board_info smdkv210_i2c_devs1[] __initdata = {
	/* To Be Updated */
	//////////////////////////
         {
                I2C_BOARD_INFO("edt_ft5x06", (0x70 >> 1)),
         },


	/////////////////////////////
	
};

static struct i2c_board_info smdkv210_i2c_devs2[] __initdata = {
	/* To Be Updated */
};

/* LCD Backlight data */
static struct samsung_bl_gpio_info smdkv210_bl_gpio_info = {
	.no = S5PV210_GPD0(3),
	.func = S3C_GPIO_SFN(2),
};

static struct platform_pwm_backlight_data smdkv210_bl_data = {
	.pwm_id = 3,
	.pwm_period_ns = 1000,
};

static void __init smdkv210_map_io(void)
{
	s5pv210_init_io(NULL, 0);
	s3c24xx_init_clocks(clk_xusbxti.rate);
	s3c24xx_init_uarts(smdkv210_uartcfgs, ARRAY_SIZE(smdkv210_uartcfgs));
	s5p_set_timer_source(S5P_PWM2, S5P_PWM4);
}

static void __init smdkv210_reserve(void)
{
	s5p_mfc_reserve_mem(0x43000000, 8 << 20, 0x51000000, 8 << 20);
}

static void __init smdkv210_machine_init(void)
{
	s3c_pm_init();

	//smdkv210_dm9000_init();

	samsung_keypad_set_platdata(&smdkv210_keypad_data);
	s3c24xx_ts_set_platdata(NULL);

	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);
	s3c_i2c2_set_platdata(NULL);
	i2c_register_board_info(0, smdkv210_i2c_devs0,
			ARRAY_SIZE(smdkv210_i2c_devs0));
	i2c_register_board_info(1, smdkv210_i2c_devs1,
			ARRAY_SIZE(smdkv210_i2c_devs1));
	i2c_register_board_info(2, smdkv210_i2c_devs2,
			ARRAY_SIZE(smdkv210_i2c_devs2));

	s3c_ide_set_platdata(&smdkv210_ide_pdata);

	s3c_fb_set_platdata(&smdkv210_lcd0_pdata);

	samsung_bl_set(&smdkv210_bl_gpio_info, &smdkv210_bl_data);

	s3c_hsotg_set_platdata(&smdkv210_hsotg_pdata);

	platform_add_devices(smdkv210_devices, ARRAY_SIZE(smdkv210_devices));
}

MACHINE_START(SMDKV210, "SMDKV210")
	/* Maintainer: Kukjin Kim <kgene.kim@samsung.com> */
	.atag_offset	= 0x100,
	.init_irq	= s5pv210_init_irq,
	.handle_irq	= vic_handle_irq,
	.map_io		= smdkv210_map_io,
	.init_machine	= smdkv210_machine_init,
	.timer		= &s5p_timer,
	.restart	= s5pv210_restart,
	.reserve	= &smdkv210_reserve,
MACHINE_END
