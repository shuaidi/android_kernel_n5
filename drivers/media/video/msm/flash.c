
/* Copyright (c) 2009-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/leds-pmic8058.h>
#include <linux/pwm.h>
#include <linux/pmic8058-pwm.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <mach/pmic.h>
#include <mach/camera.h>
#include <mach/gpio.h>

struct i2c_client *sx150x_client;
struct timer_list timer_flash;
static struct msm_camera_sensor_info *sensor_data;
enum msm_cam_flash_stat{
	MSM_CAM_FLASH_OFF,
	MSM_CAM_FLASH_ON,
};

static struct i2c_client *sc628a_client;

static int32_t flash_i2c_txdata(struct i2c_client *client,
		unsigned char *txdata, int length)
{
	/* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, start */
	#ifdef CONFIG_FLASH_LM3642
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};
	#else
	struct i2c_msg msg[] = {
		{
			.addr = client->addr >> 1,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};
	#endif
	/* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, end */
	
	if (i2c_transfer(client->adapter, msg, 1) < 0) {
		CDBG("flash_i2c_txdata faild 0x%x\n", client->addr >> 1);
		return -EIO;
	}

	return 0;
}

static int32_t flash_i2c_write_b(struct i2c_client *client,
	uint8_t baddr, uint8_t bdata)
{
	int32_t rc = -EFAULT;
	unsigned char buf[2];
	if (!client)
		return  -ENOTSUPP;

	memset(buf, 0, sizeof(buf));
	buf[0] = baddr;
	buf[1] = bdata;

	rc = flash_i2c_txdata(client, buf, 2);
	if (rc < 0) {
		CDBG("i2c_write_b failed, addr = 0x%x, val = 0x%x!\n",
				baddr, bdata);
	}
	usleep_range(4000, 5000);

	return rc;
}

/* wangjianping 20121212 added for i2c read operation, start */
static int flash_i2c_rxdata(struct i2c_client *client,
	unsigned char *rxdata, int length)
{
	#ifdef CONFIG_FLASH_LM3642
	struct i2c_msg msgs[] = {
		{
			.addr  = client->addr,
			.flags = 0,
			.len   = 1,
			.buf   = rxdata,
		},
		{
			.addr  = client->addr,
			.flags = I2C_M_RD,
			.len   = 1,
			.buf   = rxdata,
		},
	};
	#else
	struct i2c_msg msgs[] = {
		{
			.addr  = client->addr >> 1,
			.flags = 0,
			.len   = 1,
			.buf   = rxdata,
		},
		{
			.addr  = client->addr >> 1,
			.flags = I2C_M_RD,
			.len   = 1,
			.buf   = rxdata,
		},
	};
	#endif
	
	if (i2c_transfer(client->adapter, msgs, 2) < 0) {
		CDBG("flash_i2c_rxdata failed!\n");
		return -EIO;
	}
	return 0;
}

static int32_t flash_i2c_read_b(struct i2c_client *client,
    uint8_t raddr, uint8_t *rdata, int rlen)
{
	int32_t rc = 0;
	unsigned char buf[1];
	if (!rdata)
		return -EIO;
	memset(buf, 0, sizeof(buf));
	buf[0] = raddr;
	
	rc = flash_i2c_rxdata(client, buf, rlen);
	if (rc < 0) {
		CDBG("%s 0x%x failed!\n", __func__, raddr);
		return rc;
	}
	*rdata = buf[0];
	return rc;
}
/* wangjianping 20121212 added for i2c read operation, start */

static const struct i2c_device_id sc628a_i2c_id[] = {
	{"sc628a", 0},
	{ }
};

static int sc628a_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int rc = 0;
	pr_err("sc628a_probe called!\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("i2c_check_functionality failed\n");
		goto probe_failure;
	}

	sc628a_client = client;

	pr_err("sc628a_probe success rc = %d\n", rc);
	return 0;

probe_failure:
	pr_err("sc628a_probe failed! rc = %d\n", rc);
	return rc;
}

static struct i2c_driver sc628a_i2c_driver = {
	.id_table = sc628a_i2c_id,
	.probe  = sc628a_i2c_probe,
	.remove = __exit_p(sc628a_i2c_remove),
	.driver = {
		.name = "sc628a",
	},
};

static struct i2c_client *tps61310_client;

static const struct i2c_device_id tps61310_i2c_id[] = {
	{"tps61310", 0},
	{ }
};

static int tps61310_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int rc = 0;
	CDBG("%s enter\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("i2c_check_functionality failed\n");
		goto probe_failure;
	}

	tps61310_client = client;

	rc = flash_i2c_write_b(tps61310_client, 0x01, 0x00);
	if (rc < 0) {
		tps61310_client = NULL;
		goto probe_failure;
	}

	CDBG("%s success! rc = %d\n", __func__, rc);
	return 0;

probe_failure:
	pr_err("%s failed! rc = %d\n", __func__, rc);
	return rc;
}

static struct i2c_driver tps61310_i2c_driver = {
	.id_table = tps61310_i2c_id,
	.probe  = tps61310_i2c_probe,
	.remove = __exit_p(tps61310_i2c_remove),
	.driver = {
		.name = "tps61310",
	},
};


/* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, start */
static struct i2c_client *lm3642_client;

static const struct i2c_device_id lm3642_i2c_id[] = {
	{"lm3642", 0},
	{ }
};


/*buffer content structure*/
/*|byte0 | byte1 |   byte 2-5      | byte 6-9 | */
/*=========================================*/
/*|   0    |    x    | register addr  |     data    | */
static ssize_t msm_flash_regs_set(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{    
    uint8_t   regs_addr;     
    uint8_t   data;   
    unsigned long int val;
    int32_t  rc = 0;	 
    
    val = simple_strtoul(buf, NULL, 16);
    
    printk("wjp:%s %ld\n", __func__, val);
    
    regs_addr = val >> 16;
    data = val;
    
    printk("wjp:lm3642_client->addr is 0x%x, %s\n", lm3642_client->addr, lm3642_client->name);
    printk("wjp:regs_addr=0x%x data=0x%x\n", regs_addr, data);
    
    rc = flash_i2c_write_b(lm3642_client, regs_addr, data);
        
    printk("wjp rc:%d\n", rc);
    if(rc<0)
    {
        printk("wjp:lm3642_client registers set FAIL!\n");
    }
    return count;
}

static DEVICE_ATTR(msm_flash_regs, S_IWUSR, NULL, msm_flash_regs_set);

static int lm3642_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int rc = 0;
    static int creat_sys_flag = 1;

	pr_err("lm3642_probe called!\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("i2c_check_functionality failed\n");
		goto probe_failure;
	}

	lm3642_client = client;

    if(creat_sys_flag){
        int err;
		creat_sys_flag = 0;
        err = device_create_file(&client->dev, &dev_attr_msm_flash_regs);    //ŽŽœšÊôÐÔÎÄŒþ        
    }
	
	pr_err("lm3642_probe success rc = %d\n", rc);
	return 0;

probe_failure:
	pr_err("lm3642_probe failed! rc = %d\n", rc);
	return rc;
}

//[ECID:000000] ZTEBSP wanghaifei 20121227 start, avoid power off leakage current
static int lm3462_led_en;
static void lm3462_i2c_shutdown(struct i2c_client * client)
{
	if (lm3462_led_en > 0)
		gpio_direction_output(lm3462_led_en, 0);
}
//[ECID:000000] ZTEBSP wanghaifei 20121227 end, avoid power off leakage current

static struct i2c_driver lm3642_i2c_driver = {
	.id_table = lm3642_i2c_id,
	.probe  = lm3642_i2c_probe,
	.remove = __exit_p(lm3642_i2c_remove),
	.shutdown = lm3462_i2c_shutdown, //[ECID:000000] ZTEBSP wanghaifei 20121227, avoid power off leakage current
	.driver = {
		.name = "lm3642",
	},
};
/* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, end */


static int config_flash_gpio_table(enum msm_cam_flash_stat stat,
			struct msm_camera_sensor_strobe_flash_data *sfdata)
{
	int rc = 0, i = 0;
	int msm_cam_flash_gpio_tbl[][2] = {
		{sfdata->flash_trigger, 1},
		{sfdata->flash_charge, 1},
		{sfdata->flash_charge_done, 0}
	};

	if (stat == MSM_CAM_FLASH_ON) {
		for (i = 0; i < ARRAY_SIZE(msm_cam_flash_gpio_tbl); i++) {
			rc = gpio_request(msm_cam_flash_gpio_tbl[i][0],
							  "CAM_FLASH_GPIO");
			if (unlikely(rc < 0)) {
				pr_err("%s not able to get gpio\n", __func__);
				for (i--; i >= 0; i--)
					gpio_free(msm_cam_flash_gpio_tbl[i][0]);
				break;
			}
			if (msm_cam_flash_gpio_tbl[i][1])
				gpio_direction_output(
					msm_cam_flash_gpio_tbl[i][0], 0);
			else
				gpio_direction_input(
					msm_cam_flash_gpio_tbl[i][0]);
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(msm_cam_flash_gpio_tbl); i++) {
			gpio_direction_input(msm_cam_flash_gpio_tbl[i][0]);
			gpio_free(msm_cam_flash_gpio_tbl[i][0]);
		}
	}
	return rc;
}

int msm_camera_flash_current_driver(
	struct msm_camera_sensor_flash_current_driver *current_driver,
	unsigned led_state)
{
	int rc = 0;
#if defined CONFIG_LEDS_PMIC8058
	int idx;
	const struct pmic8058_leds_platform_data *driver_channel =
		current_driver->driver_channel;
	int num_leds = driver_channel->num_leds;

	CDBG("%s: led_state = %d\n", __func__, led_state);

	/* Evenly distribute current across all channels */
	switch (led_state) {
	case MSM_CAMERA_LED_OFF:
		for (idx = 0; idx < num_leds; ++idx) {
			rc = pm8058_set_led_current(
				driver_channel->leds[idx].id, 0);
			if (rc < 0)
				pr_err(
					"%s: FAIL name = %s, rc = %d\n",
					__func__,
					driver_channel->leds[idx].name,
					rc);
		}
		break;

	case MSM_CAMERA_LED_LOW:
		for (idx = 0; idx < num_leds; ++idx) {
			rc = pm8058_set_led_current(
				driver_channel->leds[idx].id,
				current_driver->low_current/num_leds);
			if (rc < 0)
				pr_err(
					"%s: FAIL name = %s, rc = %d\n",
					__func__,
					driver_channel->leds[idx].name,
					rc);
		}
		break;

	case MSM_CAMERA_LED_HIGH:
		for (idx = 0; idx < num_leds; ++idx) {
			rc = pm8058_set_led_current(
				driver_channel->leds[idx].id,
				current_driver->high_current/num_leds);
			if (rc < 0)
				pr_err(
					"%s: FAIL name = %s, rc = %d\n",
					__func__,
					driver_channel->leds[idx].name,
					rc);
		}
		break;
	case MSM_CAMERA_LED_INIT:
	case MSM_CAMERA_LED_RELEASE:
		break;

	default:
		rc = -EFAULT;
		break;
	}
	CDBG("msm_camera_flash_led_pmic8058: return %d\n", rc);
#endif /* CONFIG_LEDS_PMIC8058 */
	return rc;
}

int msm_camera_flash_led(
		struct msm_camera_sensor_flash_external *external,
		unsigned led_state)
{
	int rc = 0;

	CDBG("msm_camera_flash_led: %d\n", led_state);
	switch (led_state) {
	case MSM_CAMERA_LED_INIT:
		rc = gpio_request(external->led_en, "sgm3141");
		CDBG("MSM_CAMERA_LED_INIT: gpio_req: %d %d\n",
				external->led_en, rc);
		if (!rc)
			gpio_direction_output(external->led_en, 0);
		else
			return 0;

		rc = gpio_request(external->led_flash_en, "sgm3141");
		CDBG("MSM_CAMERA_LED_INIT: gpio_req: %d %d\n",
				external->led_flash_en, rc);
		if (!rc)
			gpio_direction_output(external->led_flash_en, 0);

			break;

	case MSM_CAMERA_LED_RELEASE:
		CDBG("MSM_CAMERA_LED_RELEASE\n");
		gpio_set_value_cansleep(external->led_en, 0);
		gpio_free(external->led_en);
		gpio_set_value_cansleep(external->led_flash_en, 0);
		gpio_free(external->led_flash_en);
		break;

	case MSM_CAMERA_LED_OFF:
		CDBG("MSM_CAMERA_LED_OFF\n");
		gpio_set_value_cansleep(external->led_en, 0);
		gpio_set_value_cansleep(external->led_flash_en, 0);
		break;

	case MSM_CAMERA_LED_LOW:
		CDBG("MSM_CAMERA_LED_LOW\n");
		gpio_set_value_cansleep(external->led_en, 1);
		gpio_set_value_cansleep(external->led_flash_en, 1);
		break;

	case MSM_CAMERA_LED_HIGH:
		CDBG("MSM_CAMERA_LED_HIGH\n");
		gpio_set_value_cansleep(external->led_en, 1);
		gpio_set_value_cansleep(external->led_flash_en, 1);
		break;

	default:
		rc = -EFAULT;
		break;
	}

	return rc;
}

int msm_camera_flash_external(
	struct msm_camera_sensor_flash_external *external,
	unsigned led_state)
{
	int rc = 0;

	pr_err("%s,----------- flash_id is %d,led_state is %d \n", __func__, external->flash_id,led_state);

	switch (led_state) {

	case MSM_CAMERA_LED_INIT:
		if (external->flash_id == MAM_CAMERA_EXT_LED_FLASH_SC628A) {
			if (!sc628a_client) {
				rc = i2c_add_driver(&sc628a_i2c_driver);
				if (rc < 0 || sc628a_client == NULL) {
					pr_err("sc628a_i2c_driver add failed\n");
					rc = -ENOTSUPP;
					return rc;
				}
			}
		} else if (external->flash_id ==
			MAM_CAMERA_EXT_LED_FLASH_TPS61310) {
			if (!tps61310_client) {
				rc = i2c_add_driver(&tps61310_i2c_driver);
				if (rc < 0 || tps61310_client == NULL) {
					pr_err("tps61310_i2c_driver add failed\n");
					rc = -ENOTSUPP;
					return rc;
				}
			}
		}
        /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, start */
        else if (external->flash_id ==
        			MAM_CAMERA_EXT_LED_FLASH_LM3642) {
        			if (!lm3642_client) {
						uint8_t flag_reg = 0;
						pr_err("lm3642_i2c_driver add client \r\n");
        				rc = i2c_add_driver(&lm3642_i2c_driver);
        				if (rc < 0 || lm3642_client == NULL) {
        					pr_err("lm3642_i2c_driver add failed, rc:%d \r\n", rc);
        					rc = -ENOTSUPP;
        					return rc;
        				}
					lm3462_led_en = external->led_en;//[ECID:000000] ZTEBSP wanghaifei 20121227, for shutdown
                        /* read flag register */
                        rc = flash_i2c_read_b(lm3642_client, 0x0B, &flag_reg, 1);
                        if(rc < 0)
                            pr_err("%s read flag register failed, rc = %d \n",__func__, rc);
						printk("%s read flag register = %d \n",__func__, flag_reg);
        			}
        		}
        /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, end */		
		else {
			pr_err("Flash id not supported\n");
			rc = -ENOTSUPP;
			return rc;
		}

#if defined(CONFIG_GPIO_SX150X) || defined(CONFIG_GPIO_SX150X_MODULE)
		if (external->expander_info && !sx150x_client) {
			struct i2c_adapter *adapter =
			i2c_get_adapter(external->expander_info->bus_id);
			if (adapter)
				sx150x_client = i2c_new_device(adapter,
					external->expander_info->board_info);
			if (!sx150x_client || !adapter) {
				pr_err("sx150x_client is not available\n");
				rc = -ENOTSUPP;
				if (sc628a_client) {
					i2c_del_driver(&sc628a_i2c_driver);
					sc628a_client = NULL;
				}
				if (tps61310_client) {
					i2c_del_driver(&tps61310_i2c_driver);
					tps61310_client = NULL;
				}
                /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, start */
                if (lm3642_client) {
                    i2c_del_driver(&lm3642_i2c_driver);
                    lm3642_client = NULL;
                }
                /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, end */
				return rc;
			}
			i2c_put_adapter(adapter);
		}
#endif
		if (sc628a_client)
			rc = gpio_request(external->led_en, "sc628a");
		if (tps61310_client)
			rc = gpio_request(external->led_en, "tps61310");
        /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, start */
        if (lm3642_client) {
            rc = gpio_request(external->led_en, "lm3642");
        }
        /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, end */

        // zte-modify, 20130206 fuyipeng modify for flash +++
        if (rc) {
            gpio_free(external->led_en);
            
            if (sc628a_client)
                rc = gpio_request(external->led_en, "sc628a");
            if (tps61310_client)
                rc = gpio_request(external->led_en, "tps61310");
            if (lm3642_client) {
                rc = gpio_request(external->led_en, "lm3642");
            }
        }
        // zte-modify, 20130206 fuyipeng modify for flash ---

		if (!rc) {
			gpio_direction_output(external->led_en, 0);
		} else {
			goto error;
		}

		if (sc628a_client)
			rc = gpio_request(external->led_flash_en, "sc628a");
		if (tps61310_client)
			rc = gpio_request(external->led_flash_en, "tps61310");
        /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, start */
        if (lm3642_client) {
            rc = gpio_request(external->led_flash_en, "lm3642");
        }
        /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, end */

		if (!rc) {
			gpio_direction_output(external->led_flash_en, 0);
			break;
		}

		gpio_set_value_cansleep(external->led_en, 0);//zhangzhao
		gpio_free(external->led_en);
		pr_err("Flash INIT GPIO  ERROR---------\n");
		
error:
		pr_err("%s gpio request failed\n", __func__);
        // zte-modify, 20130206 fuyipeng modify for flash +++
		gpio_free(external->led_en);
        // zte-modify, 20130206 fuyipeng modify for flash ---
        
		if (sc628a_client) {
			i2c_del_driver(&sc628a_i2c_driver);
			sc628a_client = NULL;
		}
		if (tps61310_client) {
			i2c_del_driver(&tps61310_i2c_driver);
			tps61310_client = NULL;
		}
        /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, start */
        if (lm3642_client) {
            i2c_del_driver(&lm3642_i2c_driver);
            lm3642_client = NULL;        
		}
        /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, end */
		
		break;

	case MSM_CAMERA_LED_RELEASE:
        /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, start */
        if (sc628a_client || tps61310_client || lm3642_client)
        {
        /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, end */
			gpio_set_value_cansleep(external->led_en, 0);
			gpio_free(external->led_en);
			gpio_set_value_cansleep(external->led_flash_en, 0);
			gpio_free(external->led_flash_en);
			if (sc628a_client) {
				i2c_del_driver(&sc628a_i2c_driver);
				sc628a_client = NULL;
			}
			if (tps61310_client) {
				i2c_del_driver(&tps61310_i2c_driver);
				tps61310_client = NULL;
			}
            /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, start */
            if (lm3642_client) {
                i2c_del_driver(&lm3642_i2c_driver);
                lm3642_client = NULL;        
    		}
            /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, end */
			
		}
#if defined(CONFIG_GPIO_SX150X) || defined(CONFIG_GPIO_SX150X_MODULE)
		if (external->expander_info && sx150x_client) {
			i2c_unregister_device(sx150x_client);
			sx150x_client = NULL;
		}
#endif
		break;

	case MSM_CAMERA_LED_OFF:
		pr_err("-----------000000000--------flash in standy mode\n");
		usleep_range(2000, 3000);
		if (sc628a_client)
			//rc = flash_i2c_write_b(sc628a_client, 0x02, 0x00);
			rc = flash_i2c_write_b(sc628a_client, 0x04, 0xa0);//ZHANGZHAO
		if (tps61310_client)
			rc = flash_i2c_write_b(tps61310_client, 0x01, 0x00);
        /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, start */
        if (lm3642_client) {
			rc = flash_i2c_write_b(lm3642_client, 0x0A, 0x00);       
        }
        /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, end */
	
		//gpio_set_value_cansleep(external->led_en, 0);
		gpio_set_value_cansleep(external->led_en, 0);
		gpio_set_value_cansleep(external->led_flash_en, 0);
		break;

	case MSM_CAMERA_LED_LOW:
		gpio_set_value_cansleep(external->led_en, 1);
		usleep_range(2000, 3000);
		if (sc628a_client)
			{
			//rc = flash_i2c_write_b(sc628a_client, 0x02, 0x06);
			rc = flash_i2c_write_b(sc628a_client, 0x04, 0xaa);//ZHANGZHAO
			}
		if (tps61310_client)
			rc = flash_i2c_write_b(tps61310_client, 0x01, 0x86);
        /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, start */
        if (lm3642_client) {
            gpio_set_value_cansleep(external->led_flash_en, 0);  /* set Flash pin low */
			rc = flash_i2c_write_b(lm3642_client, 0x06, 0x00);   //0x30
			/* Bit[7] RFU; Bits[6:4] Torch Current; Bits[3:0] Flash Current */
			rc = flash_i2c_write_b(lm3642_client, 0x09, 0x00); // fuyipeng modify
			rc = flash_i2c_write_b(lm3642_client, 0x0A, 0x12);  	/*0x1F*/	
        }
        /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, end */
		
		usleep_range(2000, 3000);
		break;

	case MSM_CAMERA_LED_HIGH:
		gpio_set_value_cansleep(external->led_en, 1);
		usleep_range(2000, 3000);
		if (sc628a_client)
			{
			rc = flash_i2c_write_b(sc628a_client, 0x04, 0xab);
			}
		if (tps61310_client)
			rc = flash_i2c_write_b(tps61310_client, 0x01, 0x8B);
        /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, start */
        if (lm3642_client) {
            gpio_set_value_cansleep(external->led_en, 0);  /* set Tx pin low */
            gpio_set_value_cansleep(external->led_flash_en, 1);			
			msleep(400);
			//printk("wjp MSM_CAMERA_LED_HIGH \n");			
			rc = flash_i2c_write_b(lm3642_client, 0x08, 0x11); 
			/* Bit[7] RFU; Bits[6:4] Torch Current; Bits[3:0] Flash Current */
			rc = flash_i2c_write_b(lm3642_client, 0x09, 0x07); // fuyipeng modify   //0x49   
			rc = flash_i2c_write_b(lm3642_client, 0x0A, 0x23);  /*0x1F*/
        }
        /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, end */
		
		usleep_range(2000, 3000);
		//gpio_set_value_cansleep(external->led_en, 1);
		//gpio_set_value_cansleep(external->led_flash_en, 1);
		break;

	default:
		rc = -EFAULT;
		break;
	}
	return rc;
}

static int msm_camera_flash_pwm(
	struct msm_camera_sensor_flash_pwm *pwm,
	unsigned led_state)
{
	int rc = 0;
	int PWM_PERIOD = USEC_PER_SEC / pwm->freq;

	static struct pwm_device *flash_pwm;

	if (!flash_pwm) {
		flash_pwm = pwm_request(pwm->channel, "camera-flash");
		if (flash_pwm == NULL || IS_ERR(flash_pwm)) {
			pr_err("%s: FAIL pwm_request(): flash_pwm=%p\n",
			       __func__, flash_pwm);
			flash_pwm = NULL;
			return -ENXIO;
		}
	}

	switch (led_state) {
	case MSM_CAMERA_LED_LOW:
		rc = pwm_config(flash_pwm,
			(PWM_PERIOD/pwm->max_load)*pwm->low_load,
			PWM_PERIOD);
		if (rc >= 0)
			rc = pwm_enable(flash_pwm);
		break;

	case MSM_CAMERA_LED_HIGH:
		rc = pwm_config(flash_pwm,
			(PWM_PERIOD/pwm->max_load)*pwm->high_load,
			PWM_PERIOD);
		if (rc >= 0)
			rc = pwm_enable(flash_pwm);
		break;

	case MSM_CAMERA_LED_OFF:
		pwm_disable(flash_pwm);
		break;
	case MSM_CAMERA_LED_INIT:
	case MSM_CAMERA_LED_RELEASE:
		break;

	default:
		rc = -EFAULT;
		break;
	}
	return rc;
}

int msm_camera_flash_pmic(
	struct msm_camera_sensor_flash_pmic *pmic,
	unsigned led_state)
{
	int rc = 0;

	switch (led_state) {
	case MSM_CAMERA_LED_OFF:
		rc = pmic->pmic_set_current(pmic->led_src_1, 0);
		if (pmic->num_of_src > 1)
			rc = pmic->pmic_set_current(pmic->led_src_2, 0);
		break;

	case MSM_CAMERA_LED_LOW:
		rc = pmic->pmic_set_current(pmic->led_src_1,
				pmic->low_current);
		if (pmic->num_of_src > 1)
			rc = pmic->pmic_set_current(pmic->led_src_2, 0);
		break;

	case MSM_CAMERA_LED_HIGH:
		rc = pmic->pmic_set_current(pmic->led_src_1,
			pmic->high_current);
		if (pmic->num_of_src > 1)
			rc = pmic->pmic_set_current(pmic->led_src_2,
				pmic->high_current);
		break;

	case MSM_CAMERA_LED_INIT:
	case MSM_CAMERA_LED_RELEASE:
		 break;

	default:
		rc = -EFAULT;
		break;
	}
	CDBG("flash_set_led_state: return %d\n", rc);

	return rc;
}

int32_t msm_camera_flash_set_led_state(
	struct msm_camera_sensor_flash_data *fdata, unsigned led_state)
{
	int32_t rc;

	//pr_err("%s, --------------- %d\n", __func__, fdata->flash_src->flash_sr_type);

	if (fdata->flash_type != MSM_CAMERA_FLASH_LED ||
		fdata->flash_src == NULL)
		return -ENODEV;

	switch (fdata->flash_src->flash_sr_type) {
	case MSM_CAMERA_FLASH_SRC_PMIC:
		rc = msm_camera_flash_pmic(&fdata->flash_src->_fsrc.pmic_src,
			led_state);
		break;

	case MSM_CAMERA_FLASH_SRC_PWM:
		rc = msm_camera_flash_pwm(&fdata->flash_src->_fsrc.pwm_src,
			led_state);
		break;

	case MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER:
		rc = msm_camera_flash_current_driver(
			&fdata->flash_src->_fsrc.current_driver_src,
			led_state);
		break;

	case MSM_CAMERA_FLASH_SRC_EXT:
		rc = msm_camera_flash_external(
			&fdata->flash_src->_fsrc.ext_driver_src,
			led_state);
		break;

	case MSM_CAMERA_FLASH_SRC_LED1:
		rc = msm_camera_flash_led(
				&fdata->flash_src->_fsrc.ext_driver_src,
				led_state);
		break;

	default:
		rc = -ENODEV;
		break;
	}

	return rc;
}

static int msm_strobe_flash_xenon_charge(int32_t flash_charge,
		int32_t charge_enable, uint32_t flash_recharge_duration)
{
	gpio_set_value_cansleep(flash_charge, charge_enable);
	if (charge_enable) {
		timer_flash.expires = jiffies +
			msecs_to_jiffies(flash_recharge_duration);
		/* add timer for the recharge */
		if (!timer_pending(&timer_flash))
			add_timer(&timer_flash);
	} else
		del_timer_sync(&timer_flash);
	return 0;
}

static void strobe_flash_xenon_recharge_handler(unsigned long data)
{
	unsigned long flags;
	struct msm_camera_sensor_strobe_flash_data *sfdata =
		(struct msm_camera_sensor_strobe_flash_data *)data;

	spin_lock_irqsave(&sfdata->timer_lock, flags);
	msm_strobe_flash_xenon_charge(sfdata->flash_charge, 1,
		sfdata->flash_recharge_duration);
	spin_unlock_irqrestore(&sfdata->timer_lock, flags);

	return;
}

static irqreturn_t strobe_flash_charge_ready_irq(int irq_num, void *data)
{
	struct msm_camera_sensor_strobe_flash_data *sfdata =
		(struct msm_camera_sensor_strobe_flash_data *)data;

	/* put the charge signal to low */
	gpio_set_value_cansleep(sfdata->flash_charge, 0);

	return IRQ_HANDLED;
}

static int msm_strobe_flash_xenon_init(
	struct msm_camera_sensor_strobe_flash_data *sfdata)
{
	unsigned long flags;
	int rc = 0;

	spin_lock_irqsave(&sfdata->spin_lock, flags);
	if (!sfdata->state) {

		rc = config_flash_gpio_table(MSM_CAM_FLASH_ON, sfdata);
		if (rc < 0) {
			pr_err("%s: gpio_request failed\n", __func__);
			goto go_out;
		}
		rc = request_irq(sfdata->irq, strobe_flash_charge_ready_irq,
			IRQF_TRIGGER_RISING, "charge_ready", sfdata);
		if (rc < 0) {
			pr_err("%s: request_irq failed %d\n", __func__, rc);
			goto go_out;
		}

		spin_lock_init(&sfdata->timer_lock);
		/* setup timer */
		init_timer(&timer_flash);
		timer_flash.function = strobe_flash_xenon_recharge_handler;
		timer_flash.data = (unsigned long)sfdata;
	}
	sfdata->state++;
go_out:
	spin_unlock_irqrestore(&sfdata->spin_lock, flags);

	return rc;
}

static int msm_strobe_flash_xenon_release
(struct msm_camera_sensor_strobe_flash_data *sfdata, int32_t final_release)
{
	unsigned long flags;

	spin_lock_irqsave(&sfdata->spin_lock, flags);
	if (sfdata->state > 0) {
		if (final_release)
			sfdata->state = 0;
		else
			sfdata->state--;

		if (!sfdata->state) {
			free_irq(sfdata->irq, sfdata);
			config_flash_gpio_table(MSM_CAM_FLASH_OFF, sfdata);
			if (timer_pending(&timer_flash))
				del_timer_sync(&timer_flash);
		}
	}
	spin_unlock_irqrestore(&sfdata->spin_lock, flags);
	return 0;
}

static void msm_strobe_flash_xenon_fn_init
	(struct msm_strobe_flash_ctrl *strobe_flash_ptr)
{
	strobe_flash_ptr->strobe_flash_init =
				msm_strobe_flash_xenon_init;
	strobe_flash_ptr->strobe_flash_charge =
				msm_strobe_flash_xenon_charge;
	strobe_flash_ptr->strobe_flash_release =
				msm_strobe_flash_xenon_release;
}

int msm_strobe_flash_init(struct msm_sync *sync, uint32_t sftype)
{
	int rc = 0;
	switch (sftype) {
	case MSM_CAMERA_STROBE_FLASH_XENON:
		if (sync->sdata->strobe_flash_data) {
			msm_strobe_flash_xenon_fn_init(&sync->sfctrl);
			rc = sync->sfctrl.strobe_flash_init(
			sync->sdata->strobe_flash_data);
		} else
			return -ENODEV;
		break;
	default:
		rc = -ENODEV;
	}
	return rc;
}

int msm_strobe_flash_ctrl(struct msm_camera_sensor_strobe_flash_data *sfdata,
	struct strobe_flash_ctrl_data *strobe_ctrl)
{
	int rc = 0;
	switch (strobe_ctrl->type) {
	case STROBE_FLASH_CTRL_INIT:
		if (!sfdata)
			return -ENODEV;
		rc = msm_strobe_flash_xenon_init(sfdata);
		break;
	case STROBE_FLASH_CTRL_CHARGE:
		rc = msm_strobe_flash_xenon_charge(sfdata->flash_charge,
			strobe_ctrl->charge_en,
			sfdata->flash_recharge_duration);
		break;
	case STROBE_FLASH_CTRL_RELEASE:
		if (sfdata)
			rc = msm_strobe_flash_xenon_release(sfdata, 0);
		break;
	default:
		pr_err("Invalid Strobe Flash State\n");
		rc = -EINVAL;
	}
	return rc;
}

int msm_flash_ctrl(struct msm_camera_sensor_info *sdata,
	struct flash_ctrl_data *flash_info)
{
	int rc = 0;
	sensor_data = sdata;
	pr_err("%s, -------------flash mode %d\n", __func__, flash_info->flashtype);
	switch (flash_info->flashtype) {
	case LED_FLASH:
		rc = msm_camera_flash_set_led_state(sdata->flash_data,
			flash_info->ctrl_data.led_state);
			break;
	case STROBE_FLASH:
		rc = msm_strobe_flash_ctrl(sdata->strobe_flash_data,
			&(flash_info->ctrl_data.strobe_ctrl));
		break;
	default:
		pr_err("Invalid Flash MODE\n");
		rc = -EINVAL;
	}
	return rc;
}
