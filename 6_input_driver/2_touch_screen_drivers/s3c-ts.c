/* linux/drivers/input/touchscreen/s3c-ts.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Copyright (c) 2004 Arnaud Patard <arnaud.patard@rtp-net.org>
 * iPAQ H1940 touchscreen support
 *
 * ChangeLog
 *
 * 2004-09-05: Herbert Potzl <herbert@13thfloor.at>
 *	- added clock (de-)allocation code
 *
 * 2005-03-06: Arnaud Patard <arnaud.patard@rtp-net.org>
 *      - h1940_ -> s3c24xx (this driver is now also used on the n30
 *        machines :P)
 *      - Debug messages are now enabled with the config option
 *        TOUCHSCREEN_S3C_DEBUG
 *      - Changed the way the value are read
 *      - Input subsystem should now work
 *      - Use ioremap and readl/writel
 *
 * 2005-03-23: Arnaud Patard <arnaud.patard@rtp-net.org>
 *      - Make use of some undocumented features of the touchscreen
 *        controller
 *
 * 2006-09-05: Ryu Euiyoul <ryu.real@gmail.com>
 *      - added power management suspend and resume code
 *
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
//#include <linux/semaphore.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */
#include <asm/io.h>
#include <asm/irq.h>
#include <mach/hardware.h>

#include <mach/regs-adc.h>
#include <mach/ts-s3c.h>
#include <mach/irqs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#define CONFIG_TOUCHSCREEN_S3C_DEBUG

/* For ts->dev.id.version */
#define S3C_TSVERSION	0x0101

#define WAIT4INT(x)  (((x)<<8) | \
    S3C_ADCTSC_YM_SEN | S3C_ADCTSC_YP_SEN | S3C_ADCTSC_XP_SEN | \
    S3C_ADCTSC_XY_PST(3))

#define AUTOPST	     (S3C_ADCTSC_YM_SEN | S3C_ADCTSC_YP_SEN | S3C_ADCTSC_XP_SEN | \
    S3C_ADCTSC_AUTO_PST | S3C_ADCTSC_XY_PST(0))


#define DEBUG_LVL    KERN_DEBUG

#ifdef CONFIG_HAS_EARLYSUSPEND
void ts_early_suspend(struct early_suspend *h);
void ts_late_resume(struct early_suspend *h);
#endif /* CONFIG_HAS_EARLYSUSPEND */

/* Touchscreen default configuration */
struct s3c_ts_mach_info s3c_ts_default_cfg __initdata = {
    .delay =		30000,
    .presc = 		49,
    .oversampling_shift = 	2,
    .resol_bit = 		10
};

/*
 * Definitions & global arrays.
 */
static char *s3c_ts_name = "S5P TouchScreen";
static void __iomem 		*ts_base;
static struct resource		*ts_mem;
static struct resource		*ts_irq;
static struct clk		*ts_clock;
static struct s3c_ts_info 	*ts;

static int downflag=0;//#if config ADC

#ifdef CONFIG_S5P_ADC
DECLARE_MUTEX(ADC_LOCK);//2.6.25.7 not 3.0.1
/* Indicate who is using the ADC controller */
#define LOCK_FREE		0
#define LOCK_TS			1
#define LOCK_ADC		2
static int adc_lock_id = LOCK_FREE;

#define	ADC_free()		(adc_lock_id == LOCK_FREE)
#define ADC_locked4TS()	(adc_lock_id == LOCK_TS)
static inline int s3c_ts_adc_lock(int id) {
        int ret;
        ret = down_trylock(&ADC_LOCK);
        if (!ret) {
                adc_lock_id = id;
        }

        return ret;
}

static inline void s3c_ts_adc_unlock(void) {
        adc_lock_id = 0;
        up(&ADC_LOCK);
}
static unsigned int _adccon, _adctsc, _adcdly;

int X6410_adc_acquire_io(void) {
        int ret;

        ret = s3c_ts_adc_lock(LOCK_ADC);
        if (!ret) {
                //_adccon = readl(ts_base + S3C_ADCCON);
                //_adctsc = readl(ts_base + S3C_ADCTSC);
                //_adcdly = readl(ts_base + S3C_ADCDLY);

                //printk("forlinx debug****X6410_adc_acquire_io();.\n");


        }



        return ret;
}
EXPORT_SYMBOL(X6410_adc_acquire_io);

void X6410_adc_release_io(void) {
        //writel(_adccon, ts_base + S3C_ADCCON);
        //writel(_adctsc, ts_base + S3C_ADCTSC);
        //writel(_adcdly, ts_base + S3C_ADCDLY);
        //writel(WAIT4INT(0), ts_base + S3C_ADCTSC);

        s3c_ts_adc_unlock();

        //printk("forlinx debug*****X6410_adc_release_io();.\n");


}

EXPORT_SYMBOL(X6410_adc_release_io);

#endif// endif CONFIG_S5P_ADC

#ifdef CONFIG_ANDROID
  signed long pointercal[7] = {1,0,0,0,1,0,1};
  static struct proc_dir_entry *ts_proc_entry; //forlinx
#endif
static void touch_timer_fire(unsigned long data)
{
    unsigned long data0;
    unsigned long data1;
    int updown;
#ifdef CONFIG_S5P_ADC
        if (!ADC_locked4TS()) {
                /* Note: pen UP interrupt detected and handled, the lock is released,
                 * so do nothing in the timer which started by ADC ISR. */
                return;
        }
#endif //end CONFIG_S5P_ADC


#ifdef CONFIG_ANDROID
    int x,y;
    int a0,a1,a2,a3,a4,a5,a6;

    a0 = pointercal[0];
    a1 = pointercal[1];
    a2 = pointercal[2];
    a3 = pointercal[3];
    a4 = pointercal[4];
    a5 = pointercal[5];
    a6 = pointercal[6];

#endif /* CONFIG_ANDROID */

    //	a0=3404;
    //	a1=-2184768;
    //	a2=-2256;
    //	a3=0;
    //	a4=33924600;
    //	a5=65536632;
    //	a6=65536;
    data0 = readl(ts_base+S3C_ADCDAT0);
    data1 = readl(ts_base+S3C_ADCDAT1);

    updown = (!(data0 & S3C_ADCDAT0_UPDOWN)) && (!(data1 & S3C_ADCDAT1_UPDOWN));

    if (updown)
    {
        if (ts->count)
        {

#ifdef CONFIG_ANDROID	

            x=(int) ts->xp;
            y=(int) ts->yp;

            ts->xp=(long) ((a2+(a0*x)+(a1*y))/a6);
            ts->yp=(long) ((a5+(a3*x)+(a4*y))/a6);
         //   printk("x=%d, y=%d\n",(int) ts->xp,(int) ts->yp);

            if (ts->xp!=ts->xp_old || ts->yp!=ts->yp_old) {
                input_report_abs(ts->dev, ABS_X, ts->xp);
                input_report_abs(ts->dev, ABS_Y, ts->yp);
                input_report_abs(ts->dev, ABS_Z, 0);

                input_report_key(ts->dev, BTN_TOUCH, 1);
                input_report_abs(ts->dev, ABS_PRESSURE, 1);
                input_sync(ts->dev);
            }
            ts->xp_old=ts->xp;
            ts->yp_old=ts->yp;
#else /* CONFIG_ANDROID */
/*
            x=(int) ts->xp;
            y=(int) ts->yp;
            long  xp=(long) ((a2+(a0*x)+(a1*y))/a6);
            long  yp=(long) ((a5+(a3*x)+(a4*y))/a6);
            printk("xx=%ld, yy=%ld\n",xp,yp);
*/
         //   printk("x=%d,y=%d\n",(int) ts->xp,(int) ts->yp);

            if(downflag==0)
            {
               input_report_abs(ts->dev, ABS_X, ts->xp);
               input_report_abs(ts->dev, ABS_Y, ts->yp);

               input_report_key(ts->dev, BTN_TOUCH, 1);
               input_report_abs(ts->dev, ABS_PRESSURE, 1);
               input_sync(ts->dev);
	    }else
            {
		downflag=0;
	    }
#endif
        }

        ts->xp = 0;
        ts->yp = 0;
        ts->count = 0;

        writel(S3C_ADCTSC_PULL_UP_DISABLE | AUTOPST, ts_base+S3C_ADCTSC);
        writel(readl(ts_base+S3C_ADCCON) | S3C_ADCCON_ENABLE_START, ts_base+S3C_ADCCON);

    } else
    {
        ts->count = 0;
#ifdef CONFIG_ANDROID
        input_report_abs(ts->dev, ABS_X, ts->xp_old);
        input_report_abs(ts->dev, ABS_Y, ts->yp_old);
        input_report_abs(ts->dev, ABS_Z, 0);
#endif
        input_report_key(ts->dev, BTN_TOUCH, 0);

#ifndef CONFIG_ANDROID
        input_report_abs(ts->dev, ABS_PRESSURE, 0);
#endif
        input_sync(ts->dev);

        writel(WAIT4INT(0), ts_base+S3C_ADCTSC);
#ifdef CONFIG_S5P_ADC
        if (ADC_locked4TS()) 
        {
            s3c_ts_adc_unlock();
            //printk("forlinx debug*****s3c_ts_adc_unlock(); in touch_timer_fire.\n");
        }
#endif
    }
}

static struct timer_list touch_timer =
        TIMER_INITIALIZER(touch_timer_fire, 0, 0);

static irqreturn_t stylus_updown(int irqno, void *param)
{
    unsigned long data0;
    unsigned long data1;
    int updown;
#ifdef CONFIG_S5P_ADC
        if (!ADC_locked4TS()) {
                if (s3c_ts_adc_lock(LOCK_TS)) {
                        /* Locking ADC controller failed */
			if(adc_lock_id!=0)//add by bluesky		
			  s3c_ts_adc_unlock();//add by bluesky
                        printk("Lock ADC failed, %d\n", adc_lock_id);
                        return IRQ_HANDLED;
                }

                //printk("forlinx debug***** s3c_ts_adc_lock(LOCK_TS); in stylus_updown.\n");
        }
#endif //CONFIG_S5P_ADC

    data0 = readl(ts_base+S3C_ADCDAT0);
    data1 = readl(ts_base+S3C_ADCDAT1);

    updown = (!(data0 & S3C_ADCDAT0_UPDOWN)) && (!(data1 & S3C_ADCDAT1_UPDOWN));

    //printk("****************down*****************\n");
    /* TODO we should never get an interrupt with updown set while
     * the timer is running, but maybe we ought to verify that the
     * timer isn't running anyways. */

    if (updown)
        touch_timer_fire(0);

    if (ts->s3c_adc_con == ADC_TYPE_2) {
        __raw_writel(0x0, ts_base+S3C_ADCCLRWK);
        __raw_writel(0x0, ts_base+S3C_ADCCLRINT);
    }

    return IRQ_HANDLED;
}
#ifdef CONFIG_ANDROID
static int ts_proc_write(struct file *file, const char *buffer,
                         unsigned long count, void *data)
{
    char buf[256];
    unsigned long len;
    char *p = (char *)buf;
    int i,j;
    int flag = 1;

    memset(buf,0,256);
    len = min_t(unsigned long, sizeof(buf) - 1, count);

    if (copy_from_user(buf, buffer, len))
        return count;

    j = 0;
    for(i=0;i<len;i++)
    {
        if(flag)
        {
            pointercal[j] = simple_strtol(p, &p, 10);

            j++;
            if(j>=7)
                break;

            flag = 0;
        }


        if(p[0] == ' ')
        {
            flag = 1;
        }
        if(p[0] == '\t')
        {
            flag = 1;
        }
        p++;
    }
    return count;

}
#endif
static irqreturn_t stylus_action(int irqno, void *param)
{
    unsigned long data0;
    unsigned long data1;
#ifdef CONFIG_S5P_ADC
        if (!ADC_locked4TS()) {
                if (ADC_free()) {
                        printk("Unexpected\n");

                        /* Clear ADC interrupt */
                        __raw_writel(0x0, ts_base + S3C_ADCCLRINT);
                }

                return IRQ_HANDLED;
        }
#endif

    data0 = readl(ts_base+S3C_ADCDAT0);
    data1 = readl(ts_base+S3C_ADCDAT1);

    if (ts->resol_bit == 12) {

        ts->xp += data0 & S3C_ADCDAT0_XPDATA_MASK_12BIT;

        ts->yp += data1 & S3C_ADCDAT1_YPDATA_MASK_12BIT;

    } else {

        ts->xp += data0 & S3C_ADCDAT0_XPDATA_MASK;
        ts->yp += data1 & S3C_ADCDAT1_YPDATA_MASK;

    }

    ts->count++;

    if (ts->count < (1<<ts->shift)) {
        writel(S3C_ADCTSC_PULL_UP_DISABLE | AUTOPST, ts_base+S3C_ADCTSC);
        writel(readl(ts_base+S3C_ADCCON) | S3C_ADCCON_ENABLE_START, ts_base+S3C_ADCCON);
    } else {
        mod_timer(&touch_timer, jiffies+5);
        writel(WAIT4INT(1), ts_base+S3C_ADCTSC);
    }

    if (ts->s3c_adc_con == ADC_TYPE_2) {
        __raw_writel(0x0, ts_base+S3C_ADCCLRWK);
        __raw_writel(0x0, ts_base+S3C_ADCCLRINT);
    }
    return IRQ_HANDLED;
}


static struct s3c_ts_mach_info *s3c_ts_get_platdata(struct device *dev)
{
    if (dev->platform_data != NULL)
        return (struct s3c_ts_mach_info *)dev->platform_data;

    return &s3c_ts_default_cfg;
}

/*
 * The functions for inserting/removing us as a module.
 */
static int __init s3c_ts_probe(struct platform_device *pdev)
{
    struct resource *res;
    struct device *dev;
    struct input_dev *input_dev;
    struct s3c_ts_mach_info *s3c_ts_cfg;
    int ret, size, err;
    int irq_flags = 0;

    dev = &pdev->dev;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (res == NULL) {
        dev_err(dev, "no memory resource specified\n");
        return -ENOENT;
    }

    size = (res->end - res->start) + 1;

    printk("*********res->start=%x, res->end=%x size=%d, pdev-name=%s\n",res->start,res->end, size, pdev->name);


    ts_mem = request_mem_region(res->start, size, pdev->name);
    if (ts_mem == NULL) {
        dev_err(dev, "failed to get memory region\n");
        ret = -ENOENT;
        goto err_req;
    }

    ts_base = ioremap(res->start, size);
    if (ts_base == NULL) {
        dev_err(dev, "failed to ioremap() region\n");
        ret = -EINVAL;
        goto err_map;
    }

    ts_clock = clk_get(&pdev->dev, "adc");
    if (IS_ERR(ts_clock)) {
        dev_err(dev, "failed to find adc clock source\n");
        ret = PTR_ERR(ts_clock);
        goto err_clk;
    }

    clk_enable(ts_clock);

    s3c_ts_cfg = s3c_ts_get_platdata(&pdev->dev);
    if ((s3c_ts_cfg->presc&0xff) > 0)
        writel(S3C_ADCCON_PRSCEN | S3C_ADCCON_PRSCVL(s3c_ts_cfg->presc&0xFF),\
               ts_base+S3C_ADCCON);
    else
        writel(0, ts_base+S3C_ADCCON);

    /* Initialise registers */
    if ((s3c_ts_cfg->delay&0xffff) > 0)
        writel(s3c_ts_cfg->delay & 0xffff, ts_base+S3C_ADCDLY);

    if (s3c_ts_cfg->resol_bit == 12) {
        switch (s3c_ts_cfg->s3c_adc_con) {
        case ADC_TYPE_2:
            writel(readl(ts_base+S3C_ADCCON)|S3C_ADCCON_RESSEL_12BIT, ts_base+S3C_ADCCON);
            break;

        case ADC_TYPE_1:
            writel(readl(ts_base+S3C_ADCCON)|S3C_ADCCON_RESSEL_12BIT_1, ts_base+S3C_ADCCON);
            break;

        default:
            dev_err(dev, "Touchscreen over this type of AP isn't supported !\n");
            break;
        }
    }

    writel(WAIT4INT(0), ts_base+S3C_ADCTSC);

    ts = kzalloc(sizeof(struct s3c_ts_info), GFP_KERNEL);

    input_dev = input_allocate_device();
    if (!input_dev) {
        ret = -ENOMEM;
        goto err_alloc;
    }

    ts->dev = input_dev;

    ts->dev->evbit[0] = ts->dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
    ts->dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

    if (s3c_ts_cfg->resol_bit==12) {

        input_set_abs_params(ts->dev, ABS_X, 0, 800, 0, 0);
        input_set_abs_params(ts->dev, ABS_Y, 0, 480, 0, 0);
        //  input_set_abs_params(ts->dev, ABS_Z, 0, 0, 0, 0);

        set_bit(0,ts->dev->evbit);
        set_bit(1,ts->dev->evbit);
        set_bit(2,ts->dev->evbit);
        set_bit(3,ts->dev->evbit);
        set_bit(5,ts->dev->evbit);

        set_bit(0,ts->dev->relbit);
        set_bit(1,ts->dev->relbit);

        set_bit(0,ts->dev->absbit);
        set_bit(1,ts->dev->absbit);
        set_bit(2,ts->dev->absbit);

        set_bit(0,ts->dev->swbit);

        for (err=0; err < 512; err++)
            set_bit(err,ts->dev->keybit);

        input_event(ts->dev,5,0,1);

    } else {
        input_set_abs_params(ts->dev, ABS_X, 0, 0x3FF, 0, 0);
        input_set_abs_params(ts->dev, ABS_Y, 0, 0x3FF, 0, 0);
    }

    input_set_abs_params(ts->dev, ABS_PRESSURE, 0, 1, 0, 0);

    sprintf(ts->phys, "input(ts)");

    ts->dev->name = s3c_ts_name;
    ts->dev->phys = ts->phys;
    ts->dev->id.bustype = BUS_RS232;
    ts->dev->id.vendor = 0xDEAD;
    ts->dev->id.product = 0xBEEF;
    ts->dev->id.version = S3C_TSVERSION;

    ts->shift = s3c_ts_cfg->oversampling_shift;
    ts->resol_bit = s3c_ts_cfg->resol_bit;
    ts->s3c_adc_con = s3c_ts_cfg->s3c_adc_con;

#ifdef CONFIG_HAS_EARLYSUSPEND
    ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    ts->early_suspend.suspend = ts_early_suspend;
    ts->early_suspend.resume =ts_late_resume;
    register_early_suspend(&ts->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */

    /* For IRQ_PENDUP */
    ts_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (ts_irq == NULL) {
        dev_err(dev, "no irq resource specified\n");
        ret = -ENOENT;
        goto err_irq;
    }

    ret = request_irq(ts_irq->start, stylus_updown, irq_flags, "s3c_updown", ts);
    if (ret != 0) {
        dev_err(dev, "s3c_ts.c: Could not allocate ts IRQ_PENDN !\n");
        ret = -EIO;
        goto err_irq;
    }

    /* For IRQ_ADC */
    ts_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
    if (ts_irq == NULL) {
        dev_err(dev, "no irq resource specified\n");
        ret = -ENOENT;
        goto err_irq;
    }

    ret = request_irq(ts_irq->start, stylus_action, irq_flags| IRQF_SHARED, "s3c_action", ts);
    if (ret != 0) {
        dev_err(dev, "s3c_ts.c: Could not allocate ts IRQ_ADC !\n");
        ret =  -EIO;
        goto err_irq;
    }

    printk(KERN_INFO "%s got loaded successfully : %d bits\n", s3c_ts_name, s3c_ts_cfg->resol_bit);

    /* All went ok, so register to the input system */
    ret = input_register_device(ts->dev);
    if (ret) {
        dev_err(dev, "s3c_ts.c: Could not register input device(touchscreen)!\n");
        ret = -EIO;
        goto fail;
    }


    printk(KERN_INFO "TSSEL : %d\n", 0x01 & (readl(ts_base+S3C_ADCCON)>>17));

#ifdef CONFIG_ANDROID
    ts_proc_entry = create_proc_entry("driver/micc_ts", 0, NULL);
    if (ts_proc_entry) {
        ts_proc_entry->write_proc = ts_proc_write;
    }
#endif
    return 0;

fail:
    free_irq(ts_irq->start, ts->dev);
    free_irq(ts_irq->end, ts->dev);

err_irq:
    input_free_device(input_dev);
    kfree(ts);

err_alloc:
    clk_disable(ts_clock);
    clk_put(ts_clock);

err_clk:
    iounmap(ts_base);

err_map:
    release_resource(ts_mem);
    kfree(ts_mem);

err_req:
    return ret;
}

static int s3c_ts_remove(struct platform_device *dev)
{
    printk(KERN_INFO "s3c_ts_remove() of TS called !\n");

    disable_irq(IRQ_ADC);
    disable_irq(IRQ_PENDN);

    free_irq(IRQ_PENDN, ts->dev);
    free_irq(IRQ_ADC, ts->dev);

    if (ts_clock) {
        clk_disable(ts_clock);
        clk_put(ts_clock);
        ts_clock = NULL;
    }

#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&ts->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */

    input_unregister_device(ts->dev);
    iounmap(ts_base);

    return 0;
}

#ifdef CONFIG_PM
static unsigned int adccon, adctsc, adcdly;

static int s3c_ts_suspend(struct platform_device *dev, pm_message_t state)
{
    adccon = readl(ts_base+S3C_ADCCON);
    adctsc = readl(ts_base+S3C_ADCTSC);
    adcdly = readl(ts_base+S3C_ADCDLY);

    disable_irq(IRQ_ADC);
    disable_irq(IRQ_PENDN);

    clk_disable(ts_clock);

    return 0;
}

static int s3c_ts_resume(struct platform_device *pdev)
{
    clk_enable(ts_clock);

    writel(adccon, ts_base+S3C_ADCCON);
    writel(adctsc, ts_base+S3C_ADCTSC);
    writel(adcdly, ts_base+S3C_ADCDLY);
    writel(WAIT4INT(0), ts_base+S3C_ADCTSC);

    enable_irq(IRQ_ADC);
    enable_irq(IRQ_PENDN);

    return 0;
}
#else
#define s3c_ts_suspend NULL
#define s3c_ts_resume  NULL
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
void ts_early_suspend(struct early_suspend *h)
{
    struct s3c_ts_info *ts;
    ts = container_of(h, struct s3c_ts_info, early_suspend);
    s3c_ts_suspend(NULL, PMSG_SUSPEND); // platform_device is now used
}

void ts_late_resume(struct early_suspend *h)
{
    struct s3c_ts_info *ts;
    ts = container_of(h, struct s3c_ts_info, early_suspend);
    s3c_ts_resume(NULL); // platform_device is now used
}
#endif /* CONFIG_HAS_EARLYSUSPEND */

static struct platform_driver s3c_ts_driver = {
    .probe          = s3c_ts_probe,
    .remove         = s3c_ts_remove,
    .suspend        = s3c_ts_suspend,
    .resume         = s3c_ts_resume,
    .driver		= {
        .owner	= THIS_MODULE,
        .name	= "s3c-ts",
    },
};

static char banner[] __initdata = KERN_INFO "S5P Touchscreen driver, (c) 2008 Samsung Electronics\n";

static int __init s3c_ts_init(void)
{
    printk(banner);
    return platform_driver_register(&s3c_ts_driver);
}

static void __exit s3c_ts_exit(void)
{
    platform_driver_unregister(&s3c_ts_driver);
}

module_init(s3c_ts_init);
module_exit(s3c_ts_exit);

MODULE_AUTHOR("Samsung AP");
MODULE_DESCRIPTION("S5P touchscreen driver");
MODULE_LICENSE("GPL");
