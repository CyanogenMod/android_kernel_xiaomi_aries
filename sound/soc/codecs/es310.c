/* sound/soc/codecs/es310.c
 *
 * Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
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
#include <linux/module.h>
#include <sound/es310.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

#define MODULE_NAME "vp_audience_es310"

#undef  ES310_DEBUG_PRINT
#define ES310_DEBUG_PRINT

#define ES310_I2C_CMD_RESET 0x80020000
#define ES310_I2C_CMD_SUSPEND 0x80100001
#define ES310_I2C_CMD_SYNC 0x80000000

#define PRESET_BASE 0x80310000
#define ES310_PRESET_HANDSET_INCALL_NB		    (PRESET_BASE)
#define ES310_PRESET_HEADSET_INCALL_NB 	           (PRESET_BASE + 1)
#define ES310_PRESET_HANDSFREE_REC_NB		    (PRESET_BASE + 2)
#define ES310_PRESET_HANDSFREE_INCALL_NB		    (PRESET_BASE + 3)
#define ES310_PRESET_HANDSET_INCALL_WB	           (PRESET_BASE + 4)
#define ES310_PRESET_HEADSET_INCALL_WB		    (PRESET_BASE + 5)
#define ES310_PRESET_AUDIOPATH_DISABLE               (PRESET_BASE + 6)
#define ES310_PRESET_HANDSFREE_INCALL_WB	    (PRESET_BASE + 7)
#define ES310_PRESET_HANDSET_VOIP_WB		           (PRESET_BASE + 8)
#define ES310_PRESET_HEADSET_VOIP_WB                   (PRESET_BASE + 9)
#define ES310_PRESET_HANDSFREE_REC_WB                 (PRESET_BASE + 10)
#define ES310_PRESET_HANDSFREE_VOIP_WB               (PRESET_BASE + 11)
#define ES310_PRESET_VOICE_RECOGNIZTION_WB       (PRESET_BASE + 12)
#define ES310_PRESET_HEADSET_REC_WB                     (PRESET_BASE + 13)

#define ES310_IOCTL_MAGIC ';'
#define ES310_SET_CONFIG _IOW(ES310_IOCTL_MAGIC, 2, unsigned int *)
#define ES310_SET_PARAM _IOW(ES310_IOCTL_MAGIC, 4, struct ES310_config_data *)
#define ES310_SYNC_CMD _IO(ES310_IOCTL_MAGIC, 9)
#define ES310_SLEEP_CMD _IO(ES310_IOCTL_MAGIC, 11)
#define ES310_RESET_CMD _IO(ES310_IOCTL_MAGIC, 12)
#define ES310_WAKEUP_CMD _IO(ES310_IOCTL_MAGIC, 13)
#define ES310_MDELAY _IOW(ES310_IOCTL_MAGIC, 14, unsigned int)
#define ES310_READ_FAIL_COUNT _IOR(ES310_IOCTL_MAGIC, 15, unsigned int *)
#define ES310_READ_SYNC_DONE _IOR(ES310_IOCTL_MAGIC, 16, bool *)
#define ES310_READ_DATA _IOR(ES310_IOCTL_MAGIC, 17, unsigned long *)
#define ES310_WRITE_MSG _IOW(ES310_IOCTL_MAGIC, 18, unsigned long)
#define ES310_SET_PRESET _IOW(ES310_IOCTL_MAGIC, 19, unsigned long)

static unsigned char ES310_IOCTL_PORTCONFIGS[28][4] = {
	{0x80, 0x0C, 0x0A, 0x00},
	{0x80, 0x0D, 0x00, 0x0F},
	{0x80, 0x0C, 0x0A, 0x02},
	{0x80, 0x0D, 0x00, 0x00},
	{0x80, 0x0C, 0x0A, 0x03},
	{0x80, 0x0D, 0x00, 0x01},
	{0x80, 0x0C, 0x0A, 0x04},
	{0x80, 0x0D, 0x00, 0x00},
	{0x80, 0x0C, 0x0A, 0x05},
	{0x80, 0x0D, 0x00, 0x01},
	{0x80, 0x0C, 0x0A, 0x06},
	{0x80, 0x0D, 0x00, 0x01},
	{0x80, 0x0C, 0x0A, 0x07},
	{0x80, 0x0D, 0x00, 0x01},
	{0x80, 0x0C, 0x0C, 0x00},
	{0x80, 0x0D, 0x00, 0x0F},
	{0x80, 0x0C, 0x0C, 0x02},
	{0x80, 0x0D, 0x00, 0x00},
	{0x80, 0x0C, 0x0C, 0x03},
	{0x80, 0x0D, 0x00, 0x01},
	{0x80, 0x0C, 0x0C, 0x04},
	{0x80, 0x0D, 0x00, 0x00},
	{0x80, 0x0C, 0x0C, 0x05},
	{0x80, 0x0D, 0x00, 0x01},
	{0x80, 0x0C, 0x0C, 0x06},
	{0x80, 0x0D, 0x00, 0x01},
	{0x80, 0x0C, 0x0C, 0x07},
	{0x80, 0x0D, 0x00, 0x01}
};

static char default_config_data[] = {
	1, 0x80, 0x26, 0x00, 0x43,
	2, 0x80, 0x26, 0x00, 0x49,
	3, 0x80, 0x26, 0x00, 0x43,
	4, 0x80, 0x26, 0x00, 0x49,
};

static int mic_switch_table[6];

#if defined (ES310_DEBUG_PRINT)
#define D(fmt, args...) printk(KERN_INFO "[%s:%5d] " fmt, __func__, __LINE__, ##args)
#else
#define D(fmt, args...) do {} while (0)
#endif

struct es310_config {
	struct i2c_client *client;
	struct es310_platform_data *pdata;
	struct clk *anc_mclk;
	int suspended;
	int current_config;
	bool synced;
	unsigned int read_fail_count;
	char *config_data;
	int config_data_length;
	int current_preset;
};

static struct es310_config *es310_data = NULL;

static int es310_i2c_read(char *buf, int len)
{
	int rc;

	struct i2c_msg msgs[] = {
		{
		 .addr = es310_data->client->addr,
		 .flags = I2C_M_RD,
		 .len = len,
		 .buf = buf,
		 },
	};

	rc = i2c_transfer(es310_data->client->adapter, msgs, 1);
	if (rc < 0) {
		pr_err("%s: transfer error %d\n", __func__, rc);
		return rc;
	}

	return 0;
}

static int es310_i2c_write(char *buf, int len)
{
	int rc;

	struct i2c_msg msgs[] = {
		{
		 .addr = es310_data->client->addr,
		 .flags = 0,
		 .len = len,
		 .buf = buf,
		 },
	};

	rc = i2c_transfer(es310_data->client->adapter, msgs, 1);
	if (rc < 0) {
		D("i2c_transfer error %d\n", rc);
		return rc;
	}

	return 0;
}

static void es310_i2c_reset(unsigned int cmd)
{
	int rc = 0;
	unsigned char msgbuf[4];

	msgbuf[0] = (cmd >> 24) & 0xff;
	msgbuf[1] = (cmd >> 16) & 0xff;
	msgbuf[2] = (cmd >> 8) & 0xff;
	msgbuf[3] = cmd & 0xff;

	rc = es310_i2c_write(msgbuf, 4);
	if (!rc) {
		D("reset failed\n");
		msleep(30);
	}
}

int es310_execute_cmd(unsigned int cmd)
{
	int rc = 0, retries = 3;
	unsigned char cmdbuf[4], readbuf[4];

	cmdbuf[0] = (cmd >> 24) & 0xff;
	cmdbuf[1] = (cmd >> 16) & 0xff;
	cmdbuf[2] = (cmd >> 8) & 0xff;
	cmdbuf[3] = cmd & 0xff;

	while (retries--) {
		mdelay(20);
		rc = es310_i2c_write(cmdbuf, 4);
		if (rc < 0) {
			D("error %d\n", rc);
			es310_i2c_reset(ES310_I2C_CMD_RESET);
			return rc;
		}

		if (cmd == ES310_I2C_CMD_SUSPEND)
			return rc;

		memset(readbuf, 0xaa, sizeof(readbuf));
		rc = es310_i2c_read(readbuf, 4);
		if (rc < 0) {
			D("ack-read error %d (%d retries)\n", rc, retries);
			continue;
		}

		if ((cmdbuf[0] == readbuf[0]) && (cmdbuf[1] == readbuf[1])
		    && (cmdbuf[2] == readbuf[2]) && (cmdbuf[3] == readbuf[3])) {
			rc = 0;
			break;
		} else if (cmdbuf[2] == 0xff && cmdbuf[3] == 0xff) {
			D("illegal cmd %08x, %x, %x, %x, %x\n", cmd, readbuf[0],
			  readbuf[1], readbuf[2], readbuf[3]);
			rc = -EINVAL;
			continue;
		} else if (cmdbuf[2] == 0x00 && cmdbuf[3] == 0x00) {
			D("not ready(%d retries), %x, %x, %x, %x\n", retries,
			  readbuf[0], readbuf[1], readbuf[2], readbuf[3]);
			rc = -EBUSY;
			continue;
		} else {
			rc = -EBUSY;
			continue;
		}
	}

	if (rc) {
		D("failed execute cmd %08x (%d)\n", cmd, rc);
		es310_i2c_reset(ES310_I2C_CMD_RESET);
	}
	return rc;
}

int es310_port_config(void)
{
	int i, rc = 0;

	for (i = 0; i < 28; i++) {
		rc = es310_i2c_write(ES310_IOCTL_PORTCONFIGS[i], 4);
		mdelay(20);
	}

	return rc;
}

int es310_sleep(void)
{
	int rc = 0;

	if (es310_data->suspended == 1)
		return rc;

	rc = es310_execute_cmd(ES310_I2C_CMD_SUSPEND);
	if (rc < 0) {
		D("suspend error\n");
		return -1;
	}
	msleep(30);

	es310_data->suspended = 1;
	es310_data->current_config = 0;

	clk_disable(es310_data->anc_mclk);
	clk_unprepare(es310_data->anc_mclk);

	return rc;
}

static int es310_wakeup(void)
{
	int rc = 0, retries = 3;

	if (!es310_data->suspended)
		return 0;

	rc = clk_prepare(es310_data->anc_mclk);
	if (rc) {
		D("clk_prepare failed %d\n", rc);
		goto error_out;
	}

	rc = clk_enable(es310_data->anc_mclk);
	if (rc) {
		D("clk_enable failed %d\n", rc);
		goto error_clk_unprepare;
	}
	msleep(5);

	rc = gpio_request(es310_data->pdata->gpio_es310_wakeup,
			  "es310_wakeup_pin");
	if (rc < 0) {
		D("request wakeup gpio failed\n");
		goto error_clk_disable;
	}

	rc = gpio_direction_output(es310_data->pdata->gpio_es310_wakeup, 1);
	if (rc < 0) {
		D("set wakeup gpio to HIGH failed\n");
		goto error_gpio_free;
	}
	msleep(1);

	rc = gpio_direction_output(es310_data->pdata->gpio_es310_wakeup, 0);
	if (rc < 0) {
		D("set wakeup gpio to LOW failed\n");
		goto error_gpio_free;
	}
	msleep(40);

	rc = gpio_direction_output(es310_data->pdata->gpio_es310_wakeup, 1);
	if (rc < 0) {
		D("set wakeup gpio to HIGH failed\n");
		goto error_gpio_free;
	}

	while (retries--) {
		rc = es310_execute_cmd(ES310_I2C_CMD_SYNC);
		if (!(rc < 0))
			break;
	}

	if (rc < 0 || retries == 0)
		D("es310 wakeup failed (%d)\n", rc);

	es310_data->suspended = 0;
	gpio_free(es310_data->pdata->gpio_es310_wakeup);

	return 0;

error_gpio_free:
	gpio_free(es310_data->pdata->gpio_es310_wakeup);
error_clk_disable:
	clk_disable(es310_data->anc_mclk);
error_clk_unprepare:
	clk_unprepare(es310_data->anc_mclk);
error_out:
	return rc;
}

static int es310_hardreset(void)
{
	int rc = 0;

	rc = es310_wakeup();
	if (rc < 0) {
		D("es310_wakeup failed\n");
		return rc;
	}

	rc = gpio_direction_output(es310_data->pdata->gpio_es310_reset, 0);
	if (rc < 0) {
		D("gpio_direction_output failed for es310_reset\n");
		return -1;
	}
	mdelay(1);

	if (gpio_is_valid(es310_data->pdata->gpio_es310_reset)) {
		gpio_set_value(es310_data->pdata->gpio_es310_reset, 1);
	} else
		D("es310_reset is not a valid gpio\n");
	mdelay(50);

	return 0;
}

int es310_build_cmds(char *cmds, int newid)
{
	int i, length, found = 0, index = 0;
	char *config;

	if (es310_data->config_data) {
		config = es310_data->config_data;
		length = es310_data->config_data_length;
	} else {
		config = default_config_data;
		length = sizeof(default_config_data);
	}

	for (i = 0; (i + 4) < length; i += 5) {
		if (config[i] == newid) {
			found = 1;
			cmds[index++] = config[i + 1];
			cmds[index++] = config[i + 2];
			cmds[index++] = config[i + 3];
			cmds[index++] = config[i + 4];
		}
	}
	if (!found) {
		D("no match found in config table, newid=%d\n", newid);
	}

	return index;
}

int es310_set_config(int newid, int config)
{
	int rc = 0, cmds_len = 0, pass, n = 128, remainder;
	unsigned char cmds[800] = { 0 }, buffer[128];
	unsigned char *cmds_ptr;

	D("newid:%d, current:%d\n", newid, es310_data->current_config);

	if (es310_data->suspended) {
		D("ES310 suspended, wakeup it up\n");
		rc = es310_wakeup();
		if (rc < 0) {
			return rc;
		}
	}

	es310_data->current_config = newid;
	cmds_len = es310_build_cmds(cmds, newid);
	if (cmds_len == 0) {
		D("No cmds found");
	}
	cmds_ptr = cmds;

	pass = cmds_len / n;
	remainder = cmds_len % n;
	D("ES310 set path, total cmd %d, pass %d, remainder %d", cmds_len, pass,
	  remainder);

	while (pass) {
		rc = es310_i2c_write(cmds_ptr, n);
		if (rc < 0) {
			D("ES310 CMD block write error!\n");
			es310_i2c_reset(ES310_I2C_CMD_RESET);
			return rc;
		}
		mdelay(20);

		cmds_ptr += n;
		pass--;
		memset(buffer, 0, sizeof(buffer));

		rc = es310_i2c_read(buffer, n);
		if (rc < 0) {
			D("CMD ACK block read error\n");
			es310_data->read_fail_count++;
			es310_i2c_reset(ES310_I2C_CMD_RESET);
			return rc;
		} else {
			if (*buffer != 0x80) {
				D("CMD ACK fail, ES310 may be died\n");
				es310_data->read_fail_count++;
				es310_i2c_reset(ES310_I2C_CMD_RESET);
				return -1;
			}
		}
	}

	if (remainder) {
		rc = es310_i2c_write(cmds_ptr, remainder);
		if (rc < 0) {
			D("ES310 CMD block write error!\n");
			es310_i2c_reset(ES310_I2C_CMD_RESET);
			return rc;
		}
		mdelay(20);

		memset(buffer, 0, sizeof(buffer));
		rc = es310_i2c_read(buffer, remainder);
		if (rc < 0) {
			D("CMD ACK block read error\n");
			es310_data->read_fail_count++;
			es310_i2c_reset(ES310_I2C_CMD_RESET);
			return rc;
		} else {
			if (*buffer != 0x80) {
				D("CMD ACK fail, ES310 may be died\n");
				es310_data->read_fail_count++;
				es310_i2c_reset(ES310_I2C_CMD_RESET);
				return -1;
			}
		}
	}

	D("ES310 set path(%d) ok\n", newid);
	return rc;
}

static int setup_mic_switch(int mode)
{
	int rc = 0;

	if (mode == 1) {
		rc = gpio_direction_output(es310_data->pdata->
					   gpio_es310_mic_switch, 1);
		if (rc < 0) {
			D("set switch gpio to HIGH failed\n");
		}
	} else if (mode == 2) {
		rc = gpio_direction_output(es310_data->pdata->
					   gpio_es310_mic_switch, 0);
		if (rc < 0) {
			D("set switch gpio to LOW failed\n");
		}
	}
	msleep(5);

	return rc;
}

int es310_set_preset(unsigned int preset_mode)
{
	int rc = 0;

	if (es310_data->suspended) {
		D("ES310 suspended, wakeup it up\n");
		rc = es310_wakeup();
		if (rc < 0) {
			return rc;
		}
	}

	D("Set preset mode: 0x%x", preset_mode);
	rc = es310_execute_cmd(preset_mode);
	if (rc == 0)
		es310_data->current_preset = preset_mode;
	else
		D("Set preset mode 0x%x failed\n", preset_mode);

	return rc;
}

static long es310_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc = 0;
	void __user *argp = (void __user *)arg;
	unsigned int id, preset;
	char buffer[4];
	struct ES310_config_data cfg;

	switch (cmd) {
	case ES310_RESET_CMD:
		rc = es310_hardreset();
		break;
	case ES310_SLEEP_CMD:
		rc = es310_sleep();
		break;
	case ES310_WAKEUP_CMD:
		rc = es310_wakeup();
		break;
	case ES310_SYNC_CMD:
		mdelay(120);

		rc = es310_execute_cmd(ES310_I2C_CMD_SYNC);
		if (rc < 0) {
			D("sync command error %d", rc);
		} else {
			rc = es310_port_config();
			if (rc) {
				D("error setting port config\n");
			}

			rc = es310_sleep();
			if (rc < 0) {
				D("sleep command fail");
			} else {
				es310_data->synced = 1;
			}
		}
		break;
	case ES310_READ_SYNC_DONE:
		if (copy_to_user
		    (argp, &es310_data->synced, sizeof(es310_data->synced)))
			return -EFAULT;
		break;
	case ES310_MDELAY:
		break;
	case ES310_READ_FAIL_COUNT:
		if (copy_to_user
		    (argp, &es310_data->read_fail_count,
		     sizeof(es310_data->read_fail_count)))
			return -EFAULT;
		break;

	case ES310_SET_CONFIG:
		if (copy_from_user(&id, argp, sizeof(unsigned int))) {
			D("copy from user failed.\n");
			return -EFAULT;
		}

		D("SET_CONFIG, id:%d, current_config:%d, suspended:%d\n", id,
		  es310_data->current_config, es310_data->suspended);

		if (id >= 5)
			return -EINVAL;

		if (id == 0) {
			D("start to es310_sleep, id:%d \n", id);
			rc = es310_sleep();
		} else {
			D("start to es310_set_config, id:%d \n", id);
			rc = es310_set_config(id, 0);
			if (!rc) {
				if (mic_switch_table[id] != 0) {
					rc = setup_mic_switch(mic_switch_table
							      [id]);
				}
			}
		}

		if (rc < 0) {
			D("ES310_SET_CONFIG (%d) error %d!\n", id, rc);
		}

		break;

	case ES310_SET_PARAM:
		es310_data->config_data_length = 0;
		cfg.data = 0;
		if (copy_from_user(&cfg, argp, sizeof(cfg))) {
			D("copy from user failed.\n");
			return -EFAULT;
		}

		if (cfg.len <= 0 || cfg.len > (sizeof(char) * 6144)) {
			D("invalid data length %d\n", cfg.len);
			return -EINVAL;
		}

		if (cfg.data == NULL) {
			D("invalid data\n");
			return -EINVAL;
		}

		if (es310_data->config_data == NULL) {
			es310_data->config_data = kmalloc(cfg.len, GFP_KERNEL);
		}

		if (!es310_data->config_data) {
			D("out of memory\n");
			return -ENOMEM;
		}

		if (copy_from_user(es310_data->config_data, cfg.data, cfg.len)) {
			D("copy data from user failed.\n");
			kfree(es310_data->config_data);
			es310_data->config_data = NULL;
			return -EFAULT;
		}

		es310_data->config_data_length = cfg.len;
		rc = 0;
		break;

	case ES310_READ_DATA:
		rc = es310_wakeup();
		if (rc < 0) {
			return rc;
		}

		rc = es310_i2c_read(buffer, 4);
		if (rc < 0) {
			D("es310_i2c_read error %d\n", rc);
			return rc;
		}

		if (copy_to_user(argp, &buffer, 4)) {
			return -EFAULT;
		}

		break;

	case ES310_WRITE_MSG:
		rc = es310_wakeup();
		if (rc < 0) {
			return rc;
		}

		if (copy_from_user(buffer, argp, sizeof(buffer))) {
			return -EFAULT;
		}

		rc = es310_i2c_write(buffer, 4);
		break;

	case ES310_SET_PRESET:
		if (copy_from_user(&preset, argp, sizeof(unsigned int))) {
			D("copy from user failed.\n");
			return -EFAULT;
		}

		D("current preset:0x%x, new preset:0x%x",
		  es310_data->current_preset, preset);
		rc = es310_set_preset(preset);
		break;

	default:
		D("invalid command\n");
		rc = -EINVAL;
		break;
	}

	return rc;
}

static int es310_open(struct inode *inode, struct file *file)
{
	return 0;
}

int es310_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations es310_fileops = {
	.open = es310_open,
	.unlocked_ioctl = es310_ioctl,
	.release = es310_release,
};

static struct miscdevice es310_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "audience_es310",
	.fops = &es310_fileops,
};

static int es310_i2c_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct es310_config *data;
	struct es310_platform_data *pdata;
	int err = 0;

	pdata = client->dev.platform_data;
	if (pdata == NULL) {
		D("platform data is null\n");
		return -ENODEV;
	}

	data = kzalloc(sizeof(struct es310_config), GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;

	es310_data = data;
	es310_data->pdata = pdata;
	es310_data->current_config = 0;
	es310_data->synced = false;
	es310_data->read_fail_count = 1;
	es310_data->current_preset = ES310_PRESET_AUDIOPATH_DISABLE;
	data->client = client;
	i2c_set_clientdata(client, data);

	if (pdata->power_on) {
		err = pdata->power_on(1);
		if (err) {
			D("power setup failed %d\n", err);
			return err;
		}
	}

	es310_data->anc_mclk = clk_get(NULL, "gp0_clk");
	if (IS_ERR(es310_data->anc_mclk)) {
		D("get anc_mclk failed (%ld)\n",
		  (signed long)es310_data->anc_mclk);
		return -EIO;
	} else {
		err = clk_set_rate(es310_data->anc_mclk, 12000000);
		if (err) {
			D("clk_set_rate failed %d\n", err);
			return err;
		}

		err = clk_prepare(es310_data->anc_mclk);
		if (err) {
			D("clk_prepare failed %d\n", err);
			return err;
		}

		err = clk_enable(es310_data->anc_mclk);
		if (err) {
			D("clk_enable failed %d\n", err);
			return err;
		}
	}

	err = gpio_request(pdata->gpio_es310_reset, "es310 GPIO reset");
	if (err < 0) {
		D("gpio request reset pin failed\n");
		goto error_clk_unprepare;
	}

	err =
	    gpio_request(pdata->gpio_es310_mic_switch, "voiceproc_mic_switch");
	if (err < 0) {
		D("request voiceproc mic switch gpio failed\n");
		goto error_gpio_free_reset;
	}

	err = es310_hardreset();
	if (err < 0) {
		D("es310_hardreset error %d", err);
		goto error_gpio_free_switch;
	}

	err = misc_register(&es310_device);
	if (err) {
		D("es310_device register failed\n");
		goto error_gpio_free_switch;
	}

	return 0;

error_gpio_free_switch:
	gpio_free(pdata->gpio_es310_mic_switch);
error_gpio_free_reset:
	gpio_free(pdata->gpio_es310_reset);
error_clk_unprepare:
	clk_disable_unprepare(es310_data->anc_mclk);

	return err;
}

static int es310_i2c_remove(struct i2c_client *client)
{
	struct es310_config *data = i2c_get_clientdata(client);

	if (data->pdata) {
		gpio_free(data->pdata->gpio_es310_mic_switch);
		gpio_free(data->pdata->gpio_es310_reset);
	}

	kfree(data);

	i2c_set_clientdata(client, NULL);
	return 0;
}

static struct i2c_device_id es310_i2c_idtable[] = {
	{"audience_es310", 0},
};

static struct i2c_driver es310_i2c_driver = {
	.probe = es310_i2c_probe,
	.remove = es310_i2c_remove,
	.id_table = es310_i2c_idtable,
	.driver = {
		   .name = MODULE_NAME,
		   },
};

static int __init es310_init(void)
{
	memset(mic_switch_table, 0, sizeof(mic_switch_table));
	mic_switch_table[1] = 1;
	mic_switch_table[2] = 2;
	mic_switch_table[3] = 1;
	mic_switch_table[4] = 1;
	return i2c_add_driver(&es310_i2c_driver);
}

static void __exit es310_exit(void)
{
	return i2c_del_driver(&es310_i2c_driver);
}

module_init(es310_init);
module_exit(es310_exit);
