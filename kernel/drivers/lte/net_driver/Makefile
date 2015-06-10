#
# Makefile for kernel network device driver
#

obj-y := lte_drv.o
EXTRA_CFLAGS := $(HOST_PLATFORM_CFLAGS) -Idrivers/lte/include -Idrivers/lte/common/ -Iinclude/
lte_drv-objs := net_driver_main.o net_driver_config.o ../common/time_stamp_log.o


