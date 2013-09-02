#ifndef __LINUX_es310_H
#define __LINUX_es310_H

struct es310_platform_data {
	uint32_t gpio_es310_reset;
	uint32_t gpio_es310_clk;
	uint32_t gpio_es310_wakeup;
	uint32_t gpio_es310_mic_switch;
	int (*power_on) (int on);
};

struct ES310_config_data {
	unsigned int len;
	unsigned int unknown;
	unsigned char *data;
};

#endif
