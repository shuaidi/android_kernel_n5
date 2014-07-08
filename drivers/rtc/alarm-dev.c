/* drivers/rtc/alarm-dev.c
 *
 * Copyright (C) 2007-2009 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/android_alarm.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/wakelock.h>

#include <asm/mach/time.h>
//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,start.
//#include <linux/zte_hibernate.h>//enable hibernate function by macro ShangHai code.
//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,end.

#define ANDROID_ALARM_PRINT_INFO (1U << 0)
#define ANDROID_ALARM_PRINT_IO (1U << 1)
#define ANDROID_ALARM_PRINT_INT (1U << 2)
//[ECID:0000]ZTE_BSP maxiaoping 20121121 modify PLATFORM 8064 RTC alarm  for power_off charging,start.
extern int alarm_rpc_set(unsigned long seconds);
extern int clear_rtc_alarm(void);
//extern int get_rtc_alarm_status(void);
//[ECID:0000]ZTE_BSP maxiaoping 20121121 modify PLATFORM 8064 RTC alarm  for power_off charging,end.

static int debug_mask = ANDROID_ALARM_PRINT_INFO;
module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

#define pr_alarm(debug_level_mask, args...) \
	do { \
		if (debug_mask & ANDROID_ALARM_PRINT_##debug_level_mask) { \
			pr_info(args); \
		} \
	} while (0)

//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,start.
#define ANDROID_ALARM_WAKEUP_MASK ( \
	ANDROID_ALARM_RTC_WAKEUP_MASK | \
	ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP_MASK | \
	ANDROID_ALARM_POWEROFF_WAKEUP_MASK)	
//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,end.

/* support old usespace code */
#define ANDROID_ALARM_SET_OLD               _IOW('a', 2, time_t) /* set alarm */
#define ANDROID_ALARM_SET_AND_WAIT_OLD      _IOW('a', 3, time_t)

static int alarm_opened;
static DEFINE_SPINLOCK(alarm_slock);
static struct wake_lock alarm_wake_lock;
static DECLARE_WAIT_QUEUE_HEAD(alarm_wait_queue);
static uint32_t alarm_pending;
static uint32_t alarm_enabled;
static uint32_t wait_pending;

static struct alarm alarms[ANDROID_ALARM_TYPE_COUNT];
//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,start.
#ifdef CONFIG_ZTE_FIX_ALARM_SYNC
#define FROM_ANDROID_APP   0
#define FROM_MODEM_NETWORK 1    //time sync from network,not start by app
static int set_rtc_flag=FROM_MODEM_NETWORK; //first let all the time sync can notify alarmmanagerserver in app
/*this is a fix by zhengchao@20101008
 *this function is called by Hctosys.c when sync time from modem
 */
void fix_sync_alarm(void)
{
	if (set_rtc_flag==FROM_MODEM_NETWORK)
	{
		unsigned long flags;

		spin_lock_irqsave(&alarm_slock, flags);
		alarm_pending |= ANDROID_ALARM_TIME_CHANGE_MASK;
		wake_up(&alarm_wait_queue);
		spin_unlock_irqrestore(&alarm_slock, flags);
		printk("[alarm] time sync from modem,fix_sync_alarm\n");
	}

	set_rtc_flag=FROM_MODEM_NETWORK;

}

#endif
//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,end.

static long alarm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rv = 0;
	unsigned long flags;
	struct timespec new_alarm_time;
	struct timespec new_rtc_time;
	struct timespec tmp_time;
	enum android_alarm_type alarm_type = ANDROID_ALARM_IOCTL_TO_TYPE(cmd);
	uint32_t alarm_type_mask = 1U << alarm_type;

	if (alarm_type >= ANDROID_ALARM_TYPE_COUNT)
		return -EINVAL;

	if (ANDROID_ALARM_BASE_CMD(cmd) != ANDROID_ALARM_GET_TIME(0)) {
		if ((file->f_flags & O_ACCMODE) == O_RDONLY)
			return -EPERM;
		if (file->private_data == NULL &&
		    cmd != ANDROID_ALARM_SET_RTC) {
			spin_lock_irqsave(&alarm_slock, flags);
			if (alarm_opened) {
				spin_unlock_irqrestore(&alarm_slock, flags);
				return -EBUSY;
			}
			alarm_opened = 1;
			file->private_data = (void *)1;
			spin_unlock_irqrestore(&alarm_slock, flags);
		}
	}

	switch (ANDROID_ALARM_BASE_CMD(cmd)) {
	case ANDROID_ALARM_CLEAR(0):
		spin_lock_irqsave(&alarm_slock, flags);
		pr_alarm(IO, "alarm %d clear\n", alarm_type);
		alarm_try_to_cancel(&alarms[alarm_type]);
		if (alarm_pending) {
			alarm_pending &= ~alarm_type_mask;
			if (!alarm_pending && !wait_pending)
				wake_unlock(&alarm_wake_lock);
		}
		alarm_enabled &= ~alarm_type_mask;
		spin_unlock_irqrestore(&alarm_slock, flags);
		//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,start.
		if (alarm_type == ANDROID_ALARM_POWEROFF_WAKEUP)
		{
		printk("PM_DEBUG_MXP:alarm type %d clear\n",alarm_type);
		clear_rtc_alarm();
		}
		//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,end.
		break;

	case ANDROID_ALARM_SET_OLD:
	case ANDROID_ALARM_SET_AND_WAIT_OLD:
		if (get_user(new_alarm_time.tv_sec, (int __user *)arg)) {
			rv = -EFAULT;
			goto err1;
		}
		new_alarm_time.tv_nsec = 0;
		goto from_old_alarm_set;

	case ANDROID_ALARM_SET_AND_WAIT(0):
	case ANDROID_ALARM_SET(0):
		if (copy_from_user(&new_alarm_time, (void __user *)arg,
		    sizeof(new_alarm_time))) {
			rv = -EFAULT;
			goto err1;
		}
from_old_alarm_set:
		spin_lock_irqsave(&alarm_slock, flags);
		pr_alarm(IO, "alarm %d set %ld.%09ld\n", alarm_type,
			new_alarm_time.tv_sec, new_alarm_time.tv_nsec);
		alarm_enabled |= alarm_type_mask;
		alarm_start_range(&alarms[alarm_type],
			timespec_to_ktime(new_alarm_time),
			timespec_to_ktime(new_alarm_time));
		spin_unlock_irqrestore(&alarm_slock, flags);
		//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,start.
		if(alarm_type == ANDROID_ALARM_POWEROFF_WAKEUP)
		{
		//printk("PM_DEBUG_MXP: set_rtc_alarm.\n");
		alarm_rpc_set(new_alarm_time.tv_sec);
		//printk("PM_DEBUG_MXP: set_rtc_alarm finish.\n");
		}
		//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,end.
		if (ANDROID_ALARM_BASE_CMD(cmd) != ANDROID_ALARM_SET_AND_WAIT(0)
		    && cmd != ANDROID_ALARM_SET_AND_WAIT_OLD)
			break;
		/* fall though */
	case ANDROID_ALARM_WAIT:
		spin_lock_irqsave(&alarm_slock, flags);
		pr_alarm(IO, "alarm wait\n");
		if (!alarm_pending && wait_pending) {
			wake_unlock(&alarm_wake_lock);
			wait_pending = 0;
		}
		spin_unlock_irqrestore(&alarm_slock, flags);
		rv = wait_event_interruptible(alarm_wait_queue, alarm_pending);
		if (rv)
			goto err1;
		spin_lock_irqsave(&alarm_slock, flags);
		rv = alarm_pending;
		//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,start.
		//printk("PM_DEBUG_MXP:ANDROID_ALARM_WAIT return value rv %d\n",rv);
		//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,end.
		wait_pending = 1;
		alarm_pending = 0;
		spin_unlock_irqrestore(&alarm_slock, flags);
		break;
	case ANDROID_ALARM_SET_RTC:
		//printk("PM_DEBUG_MXP: ANDROID_ALARM_SET_RTC start .\n");
		if (copy_from_user(&new_rtc_time, (void __user *)arg,
		    sizeof(new_rtc_time))) {
			rv = -EFAULT;
			goto err1;
		}
	//[ECID:0000] ZTEBSP maxiaoping 20130204 for APQ8064 platform ui timer synnc problem,start.
	#ifdef CONFIG_ZTE_FIX_ALARM_SYNC
		printk("[alarm] time sync from APP\n");

		//Setting this flag means sync operation comes from APP 
		set_rtc_flag = FROM_ANDROID_APP;
	#endif
	//[ECID:0000] ZTEBSP maxiaoping 20130204 for APQ8064 platform ui timer synnc problem,end.
		//printk("PM_DEBUG_MXP: user new_rtc_time.tv_sec = %ld.\n",new_rtc_time.tv_sec);
		rv = alarm_set_rtc(new_rtc_time);
		spin_lock_irqsave(&alarm_slock, flags);
		alarm_pending |= ANDROID_ALARM_TIME_CHANGE_MASK;
		wake_up(&alarm_wait_queue);
		spin_unlock_irqrestore(&alarm_slock, flags);
		//printk("PM_DEBUG_MXP: ANDROID_ALARM_SET_RTC end .\n");
		if (rv < 0)
			goto err1;
		break;
	case ANDROID_ALARM_GET_TIME(0):
		switch (alarm_type) {
		case ANDROID_ALARM_RTC_WAKEUP:
		case ANDROID_ALARM_RTC:
		//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,start.
		case ANDROID_ALARM_POWEROFF_WAKEUP:
		//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,end.
			getnstimeofday(&tmp_time);
			break;
		case ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP:
		case ANDROID_ALARM_ELAPSED_REALTIME:
			tmp_time =
				ktime_to_timespec(alarm_get_elapsed_realtime());
			break;
		case ANDROID_ALARM_TYPE_COUNT:
		case ANDROID_ALARM_SYSTEMTIME:
			ktime_get_ts(&tmp_time);
			break;
		}
		if (copy_to_user((void __user *)arg, &tmp_time,
		    sizeof(tmp_time))) {
			rv = -EFAULT;
			goto err1;
		}
		break;

	default:
		rv = -EINVAL;
		goto err1;
	}
err1:
	return rv;
}

static int alarm_open(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static int alarm_release(struct inode *inode, struct file *file)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&alarm_slock, flags);
	if (file->private_data != 0) {
		for (i = 0; i < ANDROID_ALARM_TYPE_COUNT; i++) {
			uint32_t alarm_type_mask = 1U << i;
			if (alarm_enabled & alarm_type_mask) {
				pr_alarm(INFO, "alarm_release: clear alarm, "
					"pending %d\n",
					!!(alarm_pending & alarm_type_mask));
				alarm_enabled &= ~alarm_type_mask;
			}
			spin_unlock_irqrestore(&alarm_slock, flags);
			alarm_cancel(&alarms[i]);
			spin_lock_irqsave(&alarm_slock, flags);
		}
		if (alarm_pending | wait_pending) {
			if (alarm_pending)
				pr_alarm(INFO, "alarm_release: clear "
					"pending alarms %x\n", alarm_pending);
			wake_unlock(&alarm_wake_lock);
			wait_pending = 0;
			alarm_pending = 0;
		}
		alarm_opened = 0;
	}
	spin_unlock_irqrestore(&alarm_slock, flags);
	return 0;
}
//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,start.
//enable in ShangHai code for hibernate
#if 0
void clear_hb_alarm(void);
int is_hb_alarm_expire(void);
#endif
//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,end.

static void alarm_triggered(struct alarm *alarm)
{
	unsigned long flags;
	uint32_t alarm_type_mask = 1U << alarm->type;

	//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,start.
	//pr_debug("PM_DEBUG_MXP:alarm_triggered type %d\n",alarm->type);
	//printk("PM_DEBUG_MXP:alarm_triggered type %d\n",alarm->type);
	//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,end.
	
	pr_alarm(INT, "alarm_triggered type %d\n", alarm->type);
	spin_lock_irqsave(&alarm_slock, flags);
	if (alarm_enabled & alarm_type_mask) {
		wake_lock_timeout(&alarm_wake_lock, 5 * HZ);
		alarm_enabled &= ~alarm_type_mask;
		alarm_pending |= alarm_type_mask;
		wake_up(&alarm_wait_queue);
		
		//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,start.
		//enable in ShangHai code for hibernate
		#if 0
		if (is_hb_alarm_expire()) {
			clear_hb_alarm();
			alarm_wakeup_hibernate();
		}
		#endif
		//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,end.
	}
	spin_unlock_irqrestore(&alarm_slock, flags);
}

static const struct file_operations alarm_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = alarm_ioctl,
	.open = alarm_open,
	.release = alarm_release,
};

static struct miscdevice alarm_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "alarm",
	.fops = &alarm_fops,
};

static int __init alarm_dev_init(void)
{
	int err;
	int i;

	err = misc_register(&alarm_device);
	if (err)
		return err;

	//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,start.
	for (i = 0; i < ANDROID_ALARM_TYPE_COUNT; i++)
	{
		alarm_init(&alarms[i], i, alarm_triggered);
		//pr_debug("PM_DEBUG_MXP: alarm_init: type %d",i);
		//printk("PM_DEBUG_MXP: alarm_init: type %d.\n",i);
		//pr_info("pm debug: alarm_init: type %d",i);
	}
	//[ECID:0000]ZTE_BSP maxiaoping 20121030 modify PLATFORM 8064 RTC alarm driver for power_off alarm,end.
	
	wake_lock_init(&alarm_wake_lock, WAKE_LOCK_SUSPEND, "alarm");

	return 0;
}

static void  __exit alarm_dev_exit(void)
{
	misc_deregister(&alarm_device);
	wake_lock_destroy(&alarm_wake_lock);
}

module_init(alarm_dev_init);
module_exit(alarm_dev_exit);

