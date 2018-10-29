/*
 * Name:webee210_lcd.c
 * Copyright (C) 2014 Webee.JY  (2483053468@qq.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/workqueue.h>

#include <asm/io.h>
#include <asm/div64.h>
#include <asm/uaccess.h>

#include <asm/mach/map.h>
#include <mach/regs-gpio.h>
#include <linux/fb.h>

#define VSPW       9   //4
#define VBPD       13  //17
#define LINEVAL    479  
#define VFPD       21  //26

#define HSPW       19    //4
#define HBPD       25   //40
#define HOZVAL     799   
#define HFPD       209   //214

#define LeftTopX     0
#define LeftTopY     0
#define RightBotX   799
#define RightBotY   479

static struct fb_info *webee_fbinfo;

/* LCD GPIO Pins */
static long unsigned long *gpf0con;
static long unsigned long *gpf1con;
static long unsigned long *gpf2con;
static long unsigned long *gpf3con;
static long unsigned long *gpd0con;
static long unsigned long *gpd0dat;
static long unsigned long *display_control;


/* LCD Controler Pins */
struct s5pv210_lcd_regs{
	volatile unsigned long vidcon0;
	volatile unsigned long vidcon1;
	volatile unsigned long vidcon2;
	volatile unsigned long vidcon3;
	
	volatile unsigned long vidtcon0;
	volatile unsigned long vidtcon1;
	volatile unsigned long vidtcon2;
	volatile unsigned long vidtcon3;
	
	volatile unsigned long wincon0;
	volatile unsigned long wincon1;
	volatile unsigned long wincon2;
	volatile unsigned long wincon3;
	volatile unsigned long wincon4;
	
	volatile unsigned long shadowcon;
	volatile unsigned long reserve1[2];
	
	volatile unsigned long vidosd0a;
	volatile unsigned long vidosd0b;
	volatile unsigned long vidosd0c;
};

struct clk	*lcd_clk;
static struct s5pv210_lcd_regs *lcd_regs;

static long unsigned long *vidw00add0b0;
static long unsigned long *vidw00add1b0;

static u32  pseudo_palette[16];


/* from pxafb.c */
static  unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

static int  webee210_lcdfb_setcolreg(unsigned regno,
			       unsigned red, unsigned green, unsigned blue,
			       unsigned transp, struct fb_info *info)
{
	unsigned int val;
	
	if (regno > 16) 
		return 1;
	
	/* 用red,green,blue三原色构造出val */
	val  = chan_to_field(red,   &info->var.red);
	val |= chan_to_field(green, &info->var.green);
	val |= chan_to_field(blue,  &info->var.blue);

	pseudo_palette[regno] = val;
		
	return 0;	
}

static struct fb_ops webee210_lcdfb_ops = 
{
	.owner			= THIS_MODULE,
	.fb_setcolreg	= webee210_lcdfb_setcolreg,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};

static int  __init webee210_lcd_init(void)
{
	/* 1.分配一个fb_info */
	webee_fbinfo = framebuffer_alloc(0 , NULL);

	/* 2. 设置 */
	/* 2.1 设置固定的参数 */
	strcpy(webee_fbinfo->fix.id, "webee210_lcd");
	webee_fbinfo->fix.smem_len = 800 * 480 * 32/8;
	webee_fbinfo->fix.type = FB_TYPE_PACKED_PIXELS;
	webee_fbinfo->fix.visual = FB_VISUAL_TRUECOLOR;
	webee_fbinfo->fix.line_length = 800 * 32/8;

	
	/* 2.2 设置可变的参数 */
	webee_fbinfo->var.xres = 800;
	webee_fbinfo->var.yres = 480;
	webee_fbinfo->var.xres_virtual   = 800;
	webee_fbinfo->var.yres_virtual   = 480;
	webee_fbinfo->var.bits_per_pixel = 32;
	
	/*RGB:888*/
	webee_fbinfo->var.red.offset = 16;
	webee_fbinfo->var.red.length = 8;
	
	webee_fbinfo->var.green.offset = 8;
	webee_fbinfo->var.green.length = 8;
	
	webee_fbinfo->var.blue.offset = 0;
	webee_fbinfo->var.blue.length = 8;
	
	webee_fbinfo->var.activate = FB_ACTIVATE_NOW	;

	/* 2.3 设置操作函数 */
	webee_fbinfo->fbops = &webee210_lcdfb_ops;
	
	/* 2.4 其他的设置 */
	/* 2.4.1 设置显存的大小 */
	webee_fbinfo->screen_size =  800 * 480 * 32/8;	

	/* 2.4.2 设置调色板 */
	webee_fbinfo->pseudo_palette = pseudo_palette;

	/* 2.4.3 设置显存的虚拟起始地址 */
	webee_fbinfo->screen_base = dma_alloc_writecombine(NULL,
			webee_fbinfo->fix.smem_len, (u32*)&(webee_fbinfo->fix.smem_start), GFP_KERNEL);
	

	/* 3. 硬件相关的操作 */
	/* 3.1 获取lcd时钟，使能时钟 */
	lcd_clk = clk_get(NULL, "lcd");
	if (!lcd_clk || IS_ERR(lcd_clk)) {
		printk(KERN_INFO "failed to get lcd clock source\n");
	}
	clk_enable(lcd_clk);

	/* 3.2 配置GPIO用于LCD */
	gpf0con = ioremap(0xE0200120, 4);
	gpf1con = ioremap(0xE0200140, 4);
	gpf2con = ioremap(0xE0200160, 4);
	gpf3con = ioremap(0xE0200180, 4);
	gpd0con = ioremap(0xE02000A0, 4);
	gpd0dat = ioremap(0xE02000A4, 4);
	display_control = ioremap(0xe0107008, 4);

	/* 设置相关GPIO引脚用于LCD */
	*gpf0con = 0x22222222;
	*gpf1con = 0x22222222;
	*gpf2con = 0x22222222;
	*gpf3con = 0x22222222;

	/* 使能LCD本身 */
	*gpd0con |= 1<<4;
	*gpd0dat |= 1<<1;

	/* 显示路径的选择, 0b10: RGB=FIMD I80=FIMD ITU=FIMD */
	*display_control  = 2<<0;

	/* 3.3 映射LCD控制器对应寄存器 */	
	lcd_regs = ioremap(0xF8000000, sizeof(struct s5pv210_lcd_regs));	
	vidw00add0b0 = ioremap(0xF80000A0, 4);
	vidw00add1b0 = ioremap(0xF80000D0, 4);
	
	lcd_regs->vidcon0 &= ~((3<<26) | (1<<18) | (0xff<<6)  | (1<<2));   
	lcd_regs->vidcon0 |= ((5<<6) | (1<<4) );

	lcd_regs->vidcon1 &= ~(1<<7);    		/* 在vclk的下降沿获取数据 */
	lcd_regs->vidcon1 |= ((1<<6) | (1<<5));  /* HSYNC极性反转, VSYNC极性反转 */

	lcd_regs->vidtcon0 = (VBPD << 16) | (VFPD << 8) | (VSPW << 0);
	lcd_regs->vidtcon1 = (HBPD << 16) | (HFPD << 8) | (HSPW << 0);
	lcd_regs->vidtcon2 = (LINEVAL << 11) | (HOZVAL << 0);
	lcd_regs->wincon0 &= ~(0xf << 2);
	lcd_regs->wincon0 |= (0xB<<2)|(1<<15);
	lcd_regs->vidosd0a = (LeftTopX<<11) | (LeftTopY << 0);
	lcd_regs->vidosd0b = (RightBotX<<11) | (RightBotY << 0);
	lcd_regs->vidosd0c = (LINEVAL + 1) * (HOZVAL + 1);

    *vidw00add0b0 = webee_fbinfo->fix.smem_start;  
    *vidw00add1b0 = webee_fbinfo->fix.smem_start + webee_fbinfo->fix.smem_len;  

	lcd_regs->shadowcon = 0x1;  /* 使能通道0 */	
	lcd_regs->vidcon0  |= 0x3;  /* 开启总控制器 */	
	lcd_regs->wincon0 |= 1;     /* 开启窗口0 */

	
	/*4.注册*/
	register_framebuffer(webee_fbinfo);

	return 0;
}
static void __exit webee210_lcd_exit(void)
{
	unregister_framebuffer(webee_fbinfo);
	dma_free_writecombine(NULL,  webee_fbinfo->fix.smem_len, webee_fbinfo->screen_base, webee_fbinfo->fix.smem_start);
	iounmap(gpf0con);
	iounmap(gpf1con);
	iounmap(gpf2con);
	iounmap(gpf3con);
	iounmap(gpd0con);
	iounmap(gpd0dat);
	iounmap(display_control);
	iounmap(lcd_regs);
	iounmap(vidw00add0b0);
	iounmap(vidw00add1b0);
	framebuffer_release(webee_fbinfo);
}

module_init(webee210_lcd_init);
module_exit(webee210_lcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("webee");
MODULE_DESCRIPTION("Lcd driver for webee210 board");

