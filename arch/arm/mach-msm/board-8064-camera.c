/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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

#include <linux/i2c.h>
#include <linux/gpio.h>

#include <asm/mach-types.h>

#include <mach/camera.h>
#include <mach/msm_bus_board.h>
#include <mach/gpiomux.h>

#include "devices.h"
#include "board-8064.h"

#ifdef CONFIG_MSM_CAMERA

static struct gpiomux_setting cam_settings[] = {
	{
		.func = GPIOMUX_FUNC_GPIO, /*suspend*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},

	{
		.func = GPIOMUX_FUNC_1, /*active 1*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*active 2*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_2, /*active 3*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_5, /*active 4*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_6, /*active 5*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_2, /*active 6*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_3, /*active 7*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*i2c suspend*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_KEEPER,
	},

	{
		.func = GPIOMUX_FUNC_9, /*active 9*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_A, /*active 10*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_6, /*active 11*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_4, /*active 12*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

};

//shihuiqin++
static struct msm_gpiomux_config apq8064_cam_common_configs[] = {
#if 0
	{
		.gpio = 1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#endif
	{
		.gpio = 2,     /*cam_mclk2*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[12],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = 3,      //shihuiqin FLASH_EN    
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#if 0
	{
		.gpio = 4,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#endif
	{
		.gpio = 5,     //shihuiqin CAM_MCLK1
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[1],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = 34,   //shihuiqin  FLASH_CONT_EN
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#if 0
	{
		.gpio = 107,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#endif
	{
		.gpio = 10,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[9],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
	{
		.gpio = 11,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[10],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
#if 0
	{
		.gpio = 12,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[11],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
	{
		.gpio = 13,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[11],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
#endif
};
//shihuiqin--

/* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, start */
#ifdef CONFIG_FLASH_LM3642
#define VFE_CAMIF_TIMER1_GPIO 3
#define VFE_CAMIF_TIMER2_GPIO PM8921_GPIO_PM_TO_SYS(30)

static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_EXT,
	._fsrc.ext_driver_src.led_en = VFE_CAMIF_TIMER2_GPIO,  /* connects to TX pin */
	._fsrc.ext_driver_src.led_flash_en = VFE_CAMIF_TIMER1_GPIO, /* connects to STROBE pin */
	._fsrc.ext_driver_src.flash_id = MAM_CAMERA_EXT_LED_FLASH_LM3642,
};

#else
#define VFE_CAMIF_TIMER1_GPIO 34 //zhangzhao for flash
#define VFE_CAMIF_TIMER2_GPIO PM8921_GPIO_PM_TO_SYS(30) //zhangzhao for flash

static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_EXT,
	._fsrc.ext_driver_src.led_en = VFE_CAMIF_TIMER1_GPIO,
	._fsrc.ext_driver_src.led_flash_en = VFE_CAMIF_TIMER2_GPIO,
	._fsrc.ext_driver_src.flash_id = MAM_CAMERA_EXT_LED_FLASH_SC628A,
};
#endif
/* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, end */

static struct msm_gpiomux_config apq8064_cam_2d_configs[] = {
};

static struct msm_bus_vectors cam_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_preview_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 27648000,
		.ib  = 110592000,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_video_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 140451840,
		.ib  = 561807360,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 206807040,
		.ib  = 488816640,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 274423680,
		.ib  = 1097694720,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 540000000,
		.ib  = 1350000000,
	},
};

static struct msm_bus_vectors cam_zsl_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 302071680,
		.ib  = 1208286720,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 540000000,
		.ib  = 1350000000,
	},
};

static struct msm_bus_paths cam_bus_client_config[] = {
	{
		ARRAY_SIZE(cam_init_vectors),
		cam_init_vectors,
	},
	{
		ARRAY_SIZE(cam_preview_vectors),
		cam_preview_vectors,
	},
	{
		ARRAY_SIZE(cam_video_vectors),
		cam_video_vectors,
	},
	{
		ARRAY_SIZE(cam_snapshot_vectors),
		cam_snapshot_vectors,
	},
	{
		ARRAY_SIZE(cam_zsl_vectors),
		cam_zsl_vectors,
	},
};

static struct msm_bus_scale_pdata cam_bus_client_pdata = {
		cam_bus_client_config,
		ARRAY_SIZE(cam_bus_client_config),
		.name = "msm_camera",
};

static struct msm_camera_device_platform_data msm_camera_csi_device_data[] = {
	{
		.csid_core = 0,
		.is_vpe    = 1,
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
	{
		.csid_core = 1,
		.is_vpe    = 1,
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
};

static struct camera_vreg_t apq_8064_back_cam_vreg[] = {
	{"cam_vdig", REG_LDO, 1200000, 1200000, 105000},
	{"cam_vio", REG_VS, 0, 0, 0},
	{"cam_vana", REG_LDO, 2800000, 2850000, 85600},
	{"cam_vaf", REG_LDO, 2800000, 2850000, 300000},
};

static struct camera_vreg_t apq_8064_front_cam_vreg[] = {
	{"cam_vio", REG_VS, 0, 0, 0},
	{"cam_vana", REG_LDO, 2800000, 2850000, 85600},
	{"cam_vdig", REG_LDO, 1200000, 1200000, 105000},
	{"cam_vaf", REG_LDO, 2800000, 2850000, 300000},
};
#if defined (CONFIG_PROJECT_P864A10) || defined(CONFIG_PROJECT_P864H01)//zhangzhao added synaptics for 865a10 2012-8-23
#define CAML_RSTN PM8921_GPIO_PM_TO_SYS(29)// zhangzhao modified for 8064 BACK CMAER RESET
#define CAMR_RSTN 34 

//back cam
#define CAML_PWD_ZTE PM8921_GPIO_PM_TO_SYS(27)//34 zhangzhao modified for 8064 back CAMERA STANDBY  NO CONNECT
#define CAML_RSTN_ZTE PM8921_GPIO_PM_TO_SYS(29) //34 zhangzhao modified for 8064 back CAMERA RESET
#define CAML_1V2_GPIO_EN PM8921_GPIO_PM_TO_SYS(33) //34 zhangzhao modified for 8064a10 back camera 1.2V enable 2012-8-22

#else
#define CAML_RSTN PM8921_GPIO_PM_TO_SYS(29)// zhangzhao modified for 8064 BACK CMAER RESET
#define CAMR_RSTN PM8921_GPIO_PM_TO_SYS(31) // zhangzhao modified for 8064 BACK CMAER RESET

//back cam
#define CAML_PWD_ZTE PM8921_GPIO_PM_TO_SYS(27)//34 zhangzhao modified for 8064 back CAMERA STANDBY  NO CONNECT
#define CAML_RSTN_ZTE PM8921_GPIO_PM_TO_SYS(29) //34 zhangzhao modified for 8064 back CAMERA RESET
#define CAML_1V2_GPIO_EN PM8921_GPIO_PM_TO_SYS(24) //34 zhangzhao modified for 8064 back camera 1.2V enable
#endif
//fornt ca
#define CAMR_PWD_ZTE PM8921_GPIO_PM_TO_SYS(28)//34 zhangzhao modified for 8064 FRONT CAMERA STANDBY
#define CAMR_RSTN_ZTE PM8921_GPIO_PM_TO_SYS(31) //34 zhangzhao modified for 8064 FRONT CAMERA RESET

static struct gpio apq8064_common_cam_gpio[] = {
};

static struct gpio apq8064_back_cam_gpio[] = {
	{5, GPIOF_DIR_IN, "CAMIF_MCLK"},
	{CAML_RSTN, GPIOF_DIR_OUT, "CAM_RESET"},
};

static struct msm_gpio_set_tbl apq8064_back_cam_gpio_set_tbl[] = {
	{CAML_RSTN, GPIOF_OUT_INIT_LOW, 10000},
	{CAML_RSTN, GPIOF_OUT_INIT_HIGH, 10000},
};

/*zhangzhao add for p864a20 evb*/
static struct gpio apq8064_back_cam_gpio_zte[] = {
	{5, GPIOF_DIR_IN, "CAMIF_MCLK"},
	{CAML_1V2_GPIO_EN, GPIOF_DIR_OUT, "CAM_1V2_EN"},//zhangzhao add 1v2
	//{CAML_PWD_ZTE, GPIOF_DIR_OUT, "CAM_PWD"},//zhangzhao add  pwd
	{CAML_RSTN_ZTE, GPIOF_DIR_OUT, "CAM_RESET"},
	
};

static struct msm_gpio_set_tbl apq8064_back_cam_gpio_set_tbl_zte[] = {
	{CAML_1V2_GPIO_EN, GPIOF_OUT_INIT_HIGH, 20000},
	//{CAML_PWD_ZTE, GPIOF_OUT_INIT_LOW, 10000},
	{CAML_RSTN_ZTE, GPIOF_OUT_INIT_LOW, 10000},
	{CAML_RSTN_ZTE, GPIOF_OUT_INIT_HIGH, 10000},
};
/*zhangzhao add for p864a20 evb*/
static struct msm_camera_gpio_conf apq8064_back_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = apq8064_cam_2d_configs,
	.cam_gpiomux_conf_tbl_size = ARRAY_SIZE(apq8064_cam_2d_configs),
	.cam_gpio_common_tbl = apq8064_common_cam_gpio,
	.cam_gpio_common_tbl_size = ARRAY_SIZE(apq8064_common_cam_gpio),
	.cam_gpio_req_tbl = apq8064_back_cam_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(apq8064_back_cam_gpio),
	.cam_gpio_set_tbl = apq8064_back_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(apq8064_back_cam_gpio_set_tbl),
};

/*zhangzhao add for p864a20 evb start*/

static struct msm_camera_gpio_conf apq8064_back_cam_gpio_conf_zte = {
	.cam_gpiomux_conf_tbl = apq8064_cam_2d_configs,
	.cam_gpiomux_conf_tbl_size = ARRAY_SIZE(apq8064_cam_2d_configs),
	.cam_gpio_common_tbl = apq8064_common_cam_gpio,
	.cam_gpio_common_tbl_size = ARRAY_SIZE(apq8064_common_cam_gpio),
	.cam_gpio_req_tbl = apq8064_back_cam_gpio_zte,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(apq8064_back_cam_gpio_zte),
	.cam_gpio_set_tbl = apq8064_back_cam_gpio_set_tbl_zte,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(apq8064_back_cam_gpio_set_tbl_zte),
};

static struct gpio apq8064_front_cam_gpio[] = {
	{2, GPIOF_DIR_IN, "CAMIF_MCLK"},
	//{12, GPIOF_DIR_IN, "CAMIF_I2C_DATA"},
	//{13, GPIOF_DIR_IN, "CAMIF_I2C_CLK"},
	{CAMR_RSTN, GPIOF_DIR_OUT, "CAM_RESET"},
};



static struct msm_gpio_set_tbl apq8064_front_cam_gpio_set_tbl[] = {
	{CAMR_RSTN, GPIOF_OUT_INIT_LOW, 10000},
	{CAMR_RSTN, GPIOF_OUT_INIT_HIGH, 10000},
};

static struct gpio apq8064_front_cam_gpio_zte[] = {
	{2, GPIOF_DIR_IN, "CAMIF_MCLK"},
//	{12, GPIOF_DIR_IN, "CAMIF_I2C_DATA"},
	//{13, GPIOF_DIR_IN, "CAMIF_I2C_CLK"},
	{CAMR_PWD_ZTE, GPIOF_DIR_OUT, "CAM_STANDBY"},//34 zhangzhao modified for 8064 FRONT CAMERA STANDBY	
	{CAMR_RSTN_ZTE, GPIOF_DIR_OUT, "CAM2_RESET"},
};

static struct msm_gpio_set_tbl apq8064_front_cam_gpio_set_tbl_zte[] = {
	{CAMR_PWD_ZTE, GPIOF_OUT_INIT_LOW, 10000},//34 zhangzhao modified for 8064 FRONT CAMERA STANDBY	
	{CAMR_RSTN_ZTE, GPIOF_OUT_INIT_LOW, 10000},
	{CAMR_RSTN_ZTE, GPIOF_OUT_INIT_HIGH, 10000},
};
/*zhangzhao add for p864a20 evb end*/
static struct msm_camera_gpio_conf apq8064_front_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = apq8064_cam_2d_configs,
	.cam_gpiomux_conf_tbl_size = ARRAY_SIZE(apq8064_cam_2d_configs),
	.cam_gpio_common_tbl = apq8064_common_cam_gpio,
	.cam_gpio_common_tbl_size = ARRAY_SIZE(apq8064_common_cam_gpio),
	.cam_gpio_req_tbl = apq8064_front_cam_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(apq8064_front_cam_gpio),
	.cam_gpio_set_tbl = apq8064_front_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(apq8064_front_cam_gpio_set_tbl),
};
/*zhangzhao add for p864a20 evb start*/

static struct msm_camera_gpio_conf apq8064_front_cam_gpio_conf_zte = {
	.cam_gpiomux_conf_tbl = apq8064_cam_2d_configs,
	.cam_gpiomux_conf_tbl_size = ARRAY_SIZE(apq8064_cam_2d_configs),
	.cam_gpio_common_tbl = apq8064_common_cam_gpio,
	.cam_gpio_common_tbl_size = ARRAY_SIZE(apq8064_common_cam_gpio),
	.cam_gpio_req_tbl = apq8064_front_cam_gpio_zte,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(apq8064_front_cam_gpio_zte),
	.cam_gpio_set_tbl = apq8064_front_cam_gpio_set_tbl_zte,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(apq8064_front_cam_gpio_set_tbl_zte),
};
/*zhangzhao add for p864a20 evb end*/


static struct msm_camera_i2c_conf apq8064_back_cam_i2c_conf = {
	.use_i2c_mux = 1,
	.mux_dev = &msm8960_device_i2c_mux_gsbi4,
	.i2c_mux_mode = MODE_L,
};

static struct i2c_board_info msm_act_main_cam_i2c_info = {
	I2C_BOARD_INFO("msm_actuator", 0x11),
};

static struct msm_actuator_info msm_act_main_cam_0_info = {
	.board_info     = &msm_act_main_cam_i2c_info,
	.cam_name   = MSM_ACTUATOR_MAIN_CAM_0,
	.bus_id         = APQ_8064_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
};

static struct i2c_board_info msm_act_main_cam1_i2c_info = {
	I2C_BOARD_INFO("msm_actuator", 0x18),
};

static struct msm_actuator_info msm_act_main_cam_1_info = {
	.board_info     = &msm_act_main_cam1_i2c_info,
	.cam_name       = MSM_ACTUATOR_MAIN_CAM_1,
	.bus_id         = APQ_8064_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
};


static struct msm_camera_i2c_conf apq8064_front_cam_i2c_conf = {
	.use_i2c_mux = 1,
	.mux_dev = &msm8960_device_i2c_mux_gsbi4,
	.i2c_mux_mode = MODE_L,
};

static struct msm_camera_sensor_flash_data flash_imx074 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
	.flash_src	= &msm_flash_src
};

static struct msm_camera_csi_lane_params imx074_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0xF,
};

static struct msm_camera_sensor_platform_info sensor_board_info_imx074 = {
	.mount_angle	= 90,
	.cam_vreg = apq_8064_back_cam_vreg,
	.num_vreg = ARRAY_SIZE(apq_8064_back_cam_vreg),
	.gpio_conf = &apq8064_back_cam_gpio_conf,
	.i2c_conf = &apq8064_back_cam_i2c_conf,
	.csi_lane_params = &imx074_csi_lane_params,
};

static struct i2c_board_info imx074_eeprom_i2c_info = {
	I2C_BOARD_INFO("imx074_eeprom", 0x34 << 1),
};

static struct msm_eeprom_info imx074_eeprom_info = {
	.board_info     = &imx074_eeprom_i2c_info,
	.bus_id         = APQ_8064_GSBI4_QUP_I2C_BUS_ID,
};

static struct msm_camera_sensor_info msm_camera_sensor_imx074_data = {
	.sensor_name	= "imx074",
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx074,
	.sensor_platform_info = &sensor_board_info_imx074,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
	.actuator_info = &msm_act_main_cam_0_info,
	.eeprom_info = &imx074_eeprom_info,
};

static struct msm_camera_csi_lane_params imx091_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0xF,
};

static struct camera_vreg_t apq_8064_imx091_vreg[] = {
	{"cam_vana", REG_LDO, 2800000, 2850000, 85600},
	{"cam_vaf", REG_LDO, 2800000, 2850000, 300000},
//[ECID:000000] ZTEBSP wanghaifei start 20121025, for P864H01
#ifdef CONFIG_PROJECT_P864H01
	{"cam_dvdd", REG_LDO, 1200000, 1200000, 105000},
#endif
//[ECID:000000] ZTEBSP wanghaifei end 20121025, for P864H01
	{"cam_vio", REG_VS, 0, 0, 0},
#if defined(CONFIG_PROJECT_P864A20) || defined(CONFIG_PROJECT_P864G02)
	{"cam_vdig", REG_VS, 0,0,0},    //ZTEBSP yaotong 20121102
#endif
};

static struct msm_camera_sensor_flash_data flash_imx091 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
	.flash_src	= &msm_flash_src
};

static struct msm_camera_sensor_platform_info sensor_board_info_imx091 = {
	.mount_angle	= 90,/*[ECID:617001755692] zhangzhao for imx091 rggb camera  */
	.cam_vreg = apq_8064_imx091_vreg,
	.num_vreg = ARRAY_SIZE(apq_8064_imx091_vreg),
	.gpio_conf = &apq8064_back_cam_gpio_conf_zte,//zhangzhao modified
	.i2c_conf = &apq8064_back_cam_i2c_conf,
	.csi_lane_params = &imx091_csi_lane_params,
};
/* zhangzhao 2013-1-15 */
static struct i2c_board_info imx091_eeprom_i2c_info = {
	I2C_BOARD_INFO("imx091_eeprom", 0x21),
};

static struct msm_eeprom_info imx091_eeprom_info = {
	.board_info     = &imx091_eeprom_i2c_info,
	.bus_id         = APQ_8064_GSBI4_QUP_I2C_BUS_ID,
};
/* zhangzhao 2013-1-15 */
static struct msm_camera_sensor_info msm_camera_sensor_imx091_data = {
	.sensor_name	= "imx091",
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx091,
	.sensor_platform_info = &sensor_board_info_imx091,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
	.actuator_info = &msm_act_main_cam_1_info,
	.eeprom_info = &imx091_eeprom_info,
};

static struct camera_vreg_t apq_8064_s5k3l1yx_vreg[] = {
	{"cam_vdig", REG_LDO, 1200000, 1200000, 105000},
	{"cam_vana", REG_LDO, 2800000, 2850000, 85600},
	{"cam_vio", REG_VS, 0, 0, 0},
	{"cam_vaf", REG_LDO, 2800000, 2850000, 300000},
};

static struct msm_camera_sensor_flash_data flash_s5k3l1yx = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_csi_lane_params s5k3l1yx_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0xF,
};

static struct msm_camera_sensor_platform_info sensor_board_info_s5k3l1yx = {
	.mount_angle	= 90,
	.cam_vreg = apq_8064_s5k3l1yx_vreg,
	.num_vreg = ARRAY_SIZE(apq_8064_s5k3l1yx_vreg),
	.gpio_conf = &apq8064_back_cam_gpio_conf,
	.i2c_conf = &apq8064_back_cam_i2c_conf,
	.csi_lane_params = &s5k3l1yx_csi_lane_params,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3l1yx_data = {
	.sensor_name	= "s5k3l1yx",
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_s5k3l1yx,
	.sensor_platform_info = &sensor_board_info_s5k3l1yx,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
};

static struct camera_vreg_t apq_8064_mt9m114_vreg[] = {
	{"cam_vio", REG_VS, 0, 0, 0},
	{"cam_vdig", REG_LDO, 1200000, 1200000, 105000},
	{"cam_vana", REG_LDO, 2800000, 2850000, 85600},
	{"cam_vaf", REG_LDO, 2800000, 2850000, 300000},
};

static struct msm_camera_sensor_flash_data flash_mt9m114 = {
	.flash_type = MSM_CAMERA_FLASH_NONE
};

static struct msm_camera_csi_lane_params mt9m114_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_board_info_mt9m114 = {
	.mount_angle = 90,
	.cam_vreg = apq_8064_mt9m114_vreg,
	.num_vreg = ARRAY_SIZE(apq_8064_mt9m114_vreg),
	.gpio_conf = &apq8064_front_cam_gpio_conf,
	.i2c_conf = &apq8064_front_cam_i2c_conf,
	.csi_lane_params = &mt9m114_csi_lane_params,
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9m114_data = {
	.sensor_name = "mt9m114",
	.pdata = &msm_camera_csi_device_data[1],
	.flash_data = &flash_mt9m114,
	.sensor_platform_info = &sensor_board_info_mt9m114,
	.csi_if = 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
};

static struct msm_camera_sensor_flash_data flash_ov2720 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_csi_lane_params ov2720_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov2720 = {
	.mount_angle	= 0,
	.cam_vreg = apq_8064_front_cam_vreg,
	.num_vreg = ARRAY_SIZE(apq_8064_front_cam_vreg),
	.gpio_conf = &apq8064_front_cam_gpio_conf,
	.i2c_conf = &apq8064_front_cam_i2c_conf,
	.csi_lane_params = &ov2720_csi_lane_params,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov2720_data = {
	.sensor_name	= "ov2720",
	.pdata	= &msm_camera_csi_device_data[1],
	.flash_data	= &flash_ov2720,
	.sensor_platform_info = &sensor_board_info_ov2720,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
};
/**************************************************************/
/*zhangzhao add for p864a20 evb start*/
static struct camera_vreg_t apq_8064_mt9d115_vreg[] = {
	{"cam_vio", REG_VS, 0, 0, 0},
	{"cam_vana", REG_LDO, 2800000, 2850000, 85600},
};

static struct msm_camera_sensor_flash_data flash_mt9d115 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_csi_lane_params mt9d115_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_board_info_mt9d115 = {
	.mount_angle	= 0,
	.cam_vreg = apq_8064_mt9d115_vreg,
	.num_vreg = ARRAY_SIZE(apq_8064_mt9d115_vreg),
	.gpio_conf = &apq8064_front_cam_gpio_conf_zte,
	.i2c_conf = &apq8064_front_cam_i2c_conf,
	.csi_lane_params = &mt9d115_csi_lane_params,
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9d115_data ={
	.sensor_name	= "mt9d115",
	.pdata	= &msm_camera_csi_device_data[1],
	.flash_data	= &flash_mt9d115,
	.sensor_platform_info = &sensor_board_info_mt9d115,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
};
/*zhangzhao add for p864a20 evb end*/

/**************************************************************/

/**************************************************************/
/*zhangzhao add for p864a20 evb start*/
static struct camera_vreg_t apq_8064_ov9740_vreg[] = {
	{"cam_vio", REG_VS, 0, 0, 0},
	{"cam_vana", REG_LDO, 2800000, 2850000, 85600},
};

static struct msm_camera_sensor_flash_data flash_ov9740 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_csi_lane_params ov9740_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov9740 = {
	.mount_angle	= 270,
	.cam_vreg = apq_8064_ov9740_vreg,
	.num_vreg = ARRAY_SIZE(apq_8064_ov9740_vreg),
	.gpio_conf = &apq8064_front_cam_gpio_conf_zte,
	.i2c_conf = &apq8064_front_cam_i2c_conf,
	.csi_lane_params = &ov9740_csi_lane_params,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov9740_data ={
	.sensor_name	= "ov9740",
	.pdata	= &msm_camera_csi_device_data[1],
	.flash_data	= &flash_ov9740,
	.sensor_platform_info = &sensor_board_info_ov9740,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
};
/**************************************************************/

/*ZTEBSP yaotong 20121010 add for sony imx132 start*/

static struct camera_vreg_t apq_8064_imx132_vreg[] = {
	{"cam_vio", REG_VS, 0, 0, 0},
	{"cam_vana", REG_LDO, 2800000, 2850000, 85600},
	{"cam_vdig", REG_VS, 0, 0, 0},
};
static struct msm_camera_sensor_flash_data flash_imx132 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_csi_lane_params imx132_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_board_info_imx132 = {
	.mount_angle	= 270,  // zte-modify, 20121220 fuyipeng modify for angle
	.cam_vreg = apq_8064_imx132_vreg,
	.num_vreg = ARRAY_SIZE(apq_8064_imx132_vreg),
	.gpio_conf = &apq8064_front_cam_gpio_conf,
	.i2c_conf = &apq8064_front_cam_i2c_conf,
	.csi_lane_params = &imx132_csi_lane_params,
};

static struct msm_camera_sensor_info msm_camera_sensor_imx132_data = {
       .sensor_name = "imx132",
	.pdata  = &msm_camera_csi_device_data[1],
	.flash_data  = &flash_imx132,
	.sensor_platform_info = &sensor_board_info_imx132,
	.csi_if = 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
};
/*ZTEBSP yaotong 20121010 add for sony imx132 end*/

static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};

void __init apq8064_init_cam(void)
{
	msm_gpiomux_install(apq8064_cam_common_configs,
			ARRAY_SIZE(apq8064_cam_common_configs));

	if (machine_is_apq8064_cdp()) {
		sensor_board_info_imx074.mount_angle = 0;
		sensor_board_info_mt9m114.mount_angle = 0;
	} else if (machine_is_apq8064_liquid())
		sensor_board_info_imx074.mount_angle = 180;

	platform_device_register(&msm_camera_server);
	platform_device_register(&msm8960_device_i2c_mux_gsbi4);
	platform_device_register(&msm8960_device_csiphy0);
	platform_device_register(&msm8960_device_csiphy1);
	platform_device_register(&msm8960_device_csid0);
	platform_device_register(&msm8960_device_csid1);
	platform_device_register(&msm8960_device_ispif);
	platform_device_register(&msm8960_device_vfe);
	platform_device_register(&msm8960_device_vpe);
}

#ifdef CONFIG_I2C
static struct i2c_board_info apq8064_camera_i2c_boardinfo[] = {
	{
	I2C_BOARD_INFO("imx074", 0x1A),
	.platform_data = &msm_camera_sensor_imx074_data,
	},
	{
	I2C_BOARD_INFO("mt9m114", 0x48),
	.platform_data = &msm_camera_sensor_mt9m114_data,
	},
	{
	I2C_BOARD_INFO("ov2720", 0x6C>>1),
	.platform_data = &msm_camera_sensor_ov2720_data,
	},
	{
	I2C_BOARD_INFO("sc628a", 0x60),//zhangzhao 0x6e----->0x60
	},
	{
	I2C_BOARD_INFO("imx091", 0x34),
	.platform_data = &msm_camera_sensor_imx091_data,
	},
	{
	I2C_BOARD_INFO("s5k3l1yx", 0x20),
	.platform_data = &msm_camera_sensor_s5k3l1yx_data,
	},
	{/*zhangzhao add for p864a20 evb start*/
	I2C_BOARD_INFO("mt9d115", 0x7A),
	.platform_data = &msm_camera_sensor_mt9d115_data,
	},	
	{/*ZTEBSP yaotong 20121010 add for sony imx132*/
	I2C_BOARD_INFO("imx132", 0x6C),
	.platform_data = &msm_camera_sensor_imx132_data,
	},
{/*zhangzhao add for p864a20 evb start*/
	I2C_BOARD_INFO("ov9740", 0x30),
	.platform_data = &msm_camera_sensor_ov9740_data,
	},
	/* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, start */
	#ifdef CONFIG_FLASH_LM3642
	{
	I2C_BOARD_INFO("lm3642", (0xc6 >> 1)),
	},
	#endif
    /* [ECID:000000] ZTEBSP wangjianping, 20121031 added for camera flash LM3462, end */
};

struct msm_camera_board_info apq8064_camera_board_info = {
	.board_info = apq8064_camera_i2c_boardinfo,
	.num_i2c_board_info = ARRAY_SIZE(apq8064_camera_i2c_boardinfo),
};
#endif
#endif
