/* arch/arm/mach-msm/include/mach/board_lge.h
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2012, The Linux Foundation. All rights reserved.
 * Copyright (c) 2012, LGE Inc.
 * Author: Brian Swetland <swetland@google.com>
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

#ifndef __ASM_ARCH_MSM_BOARD_XIAOMI_H
#define __ASM_ARCH_MSM_BOARD_XIAOMI_H

#ifdef CONFIG_ANDROID_PERSISTENT_RAM
#define XIAOMI_PERSISTENT_RAM_SIZE	(SZ_1M)
#endif

#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define XIAOMI_RAM_CONSOLE_SIZE	(124*SZ_1K * 2)
#endif

void __init xiaomi_reserve(void);

#ifdef CONFIG_LCD_KCAL
struct kcal_data {
	int red;
	int green;
	int blue;
};

struct kcal_platform_data {
	int (*set_values) (int r, int g, int b);
	int (*get_values) (int *r, int *g, int *b);
	int (*refresh_display) (void);
};
#endif

#ifdef CONFIG_ANDROID_PERSISTENT_RAM
void __init xiaomi_add_persistent_ram(void);
#else
static inline void __init xiaomi_add_persistent_ram(void)
{
	/* empty */
}
#endif

#ifdef CONFIG_ANDROID_RAM_CONSOLE
void __init xiaomi_add_ramconsole_devices(void);
#else
static inline void __init xiaomi_add_ramconsole_devices(void)
{
	/* empty */
}
#endif

#endif // __ASM_ARCH_MSM_BOARD_XIAOMI_H
