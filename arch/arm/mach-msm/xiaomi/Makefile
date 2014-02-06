subdir-ccflags-$(CONFIG_ARCH_MSM) += -Iarch/arm/mach-msm

-include $(src)/Makefile.board

obj-$(CONFIG_MACH_XIAOMI) += devices_xiaomi.o

CFLAGS_devices_xiaomi.o += -Idrivers/staging/android
