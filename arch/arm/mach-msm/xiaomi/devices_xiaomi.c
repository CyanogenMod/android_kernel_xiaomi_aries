/* Copyright (c) 2012, LGE Inc.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/memory.h>
#include <linux/persistent_ram.h>

#include <asm/setup.h>
#include <asm/sizes.h>
#include <asm/system_info.h>
#include <asm/memory.h>

#include <mach/board_xiaomi.h>

#include <ram_console.h>

#ifdef CONFIG_LCD_KCAL
extern int kcal_set_values(int kcal_r, int kcal_g, int kcal_b);
static int __init display_kcal_setup(char *kcal)
{
	char vaild_k = 0;
	int kcal_r = 0;
	int kcal_g = 0;
	int kcal_b = 0;

	sscanf(kcal, "%d|%d|%d|%c", &kcal_r, &kcal_g, &kcal_b, &vaild_k );
	pr_info("kcal is %d|%d|%d|%c\n", kcal_r, kcal_g, kcal_b, vaild_k);

	if (vaild_k != 'K') {
		pr_info("kcal not calibrated yet : %d\n", vaild_k);
		kcal_r = kcal_g = kcal_b = 255;
		pr_info("set to default : %d\n", kcal_r);
	}

	kcal_set_values(kcal_r, kcal_g, kcal_b);
	return 1;
}
__setup("lge.kcal=", display_kcal_setup);
#endif

#ifdef CONFIG_ANDROID_PERSISTENT_RAM
static struct persistent_ram_descriptor pram_descs[] = {
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	{
		.name = "ram_console",
		.size = XIAOMI_RAM_CONSOLE_SIZE,
	},
#endif
};

static struct persistent_ram xiaomi_persistent_ram = {
	.size = XIAOMI_PERSISTENT_RAM_SIZE,
	.num_descs = ARRAY_SIZE(pram_descs),
	.descs = pram_descs,
};

void __init xiaomi_add_persistent_ram(void)
{
	struct persistent_ram *pram = &xiaomi_persistent_ram;
	struct membank* bank = &meminfo.bank[0];

	pram->start = bank->start + bank->size - XIAOMI_PERSISTENT_RAM_SIZE;

	persistent_ram_early_init(pram);
}
#endif

void __init xiaomi_reserve(void)
{
	xiaomi_add_persistent_ram();
}

#ifdef CONFIG_ANDROID_RAM_CONSOLE
static struct platform_device ram_console_device = {
	.name = "ram_console",
	.id = -1
};

void __init xiaomi_add_ramconsole_devices(void)
{
	platform_device_register(&ram_console_device);
}
#endif /* CONFIG_ANDROID_RAM_CONSOLE */
