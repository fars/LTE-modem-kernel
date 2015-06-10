#
# Makefile for kernel QA Module
#

obj-y += lte_classifier.o
EXTRA_CFLAGS := -DLTE93 $(HOST_PLATFORM_CFLAGS) -Idrivers/lte/include -Idrivers/lte/common/  -Idrivers/lte/legacy_include/ -Idrivers/lte/legacy_include/linux/wimax/ -Idrivers/lte/lte_classifier/ -Idrivers/lte/net_driver/ -Iinclude/
lte_classifier-objs := lte_qos_agent.o ../common/ptr_fifo.o ../common/time_calc.o 


