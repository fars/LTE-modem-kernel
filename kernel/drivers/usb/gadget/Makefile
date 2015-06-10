#
# USB peripheral controller drivers
#
ifeq ($(CONFIG_USB_GADGET_DEBUG),y)
	EXTRA_CFLAGS		+= -DDEBUG
endif

obj-$(CONFIG_USB_DUMMY_HCD)	+= dummy_hcd.o
obj-$(CONFIG_USB_NET2280)	+= net2280.o
obj-$(CONFIG_USB_AMD5536UDC)	+= amd5536udc.o
obj-$(CONFIG_USB_PXA25X)	+= pxa25x_udc.o
obj-$(CONFIG_USB_PXA27X)	+= pxa27x_udc.o
obj-$(CONFIG_USB_IMX)		+= imx_udc.o
obj-$(CONFIG_USB_GOKU)		+= goku_udc.o
obj-$(CONFIG_USB_OMAP)		+= omap_udc.o
obj-$(CONFIG_USB_LH7A40X)	+= lh7a40x_udc.o
obj-$(CONFIG_USB_S3C2410)	+= s3c2410_udc.o
obj-$(CONFIG_USB_AT91)		+= at91_udc.o
obj-$(CONFIG_USB_ATMEL_USBA)	+= atmel_usba_udc.o
obj-$(CONFIG_USB_FSL_USB2)	+= fsl_usb2_udc.o
obj-$(CONFIG_USB_M66592)	+= m66592-udc.o
obj-$(CONFIG_USB_FSL_QE)	+= fsl_qe_udc.o
obj-$(CONFIG_USB_CI13XXX)	+= ci13xxx_udc.o
obj-$(CONFIG_MOT_FEAT_ADB_ENDEV)+= adb_enable.o
#
# USB gadget drivers
#
g_zero-objs			:= zero.o
g_ether-objs			:= ether.o
g_serial-objs			:= serial.o
g_midi-objs			:= gmidi.o
gadgetfs-objs			:= inode.o
g_file_storage-objs		:= file_storage.o
g_printer-objs			:= printer.o
g_cdc-objs			:= cdc2.o
g_android-objs              := mot_android.o
g_mot_android-objs              := mot_android.o

ifeq ($(CONFIG_USB_GADGET_MASS_STORAGE),y)
     g_mot_android-objs += f_mass_storage.o
     g_android-objs += f_mass_storage.o
endif

ifeq ($(CONFIG_USB_GADGET_ADB),y)
     g_mot_android-objs += f_adb.o
     g_android-objs += f_adb.o
endif

ifeq ($(CONFIG_USB_GADGET_MTP),y)
     g_mot_android-objs += f_mtp.o
     g_android-objs += f_mtp.o
endif

ifeq ($(CONFIG_USB_GADGET_NETWORK_INTERFACE),y)
     g_mot_android-objs += f_usbnet.o
     g_android-objs += f_usbnet.o
endif

ifeq ($(CONFIG_USB_GADGET_SINGLE_ACM),y)
     g_mot_android-objs += f_acm.o u_serial.o
     g_android-objs += f_acm.o u_serial.o
else
ifeq ($(CONFIG_USB_GADGET_DUAL_ACM),y)
     g_mot_android-objs += f_acm.o u_serial.o
     g_android-objs += f_acm.o u_serial.o
endif
endif

ifeq ($(CONFIG_USB_GADGET_SINGLE_ECM),y)
     g_mot_android-objs += f_ecm.o u_ether.o
     g_android-objs += f_ecm.o u_ether.o
else
ifeq ($(CONFIG_USB_GADGET_DUAL_ECM),y)
     g_mot_android-objs += f_ecm.o u_ether.o
     g_android-objs += f_ecm.o u_ether.o
endif
ifeq ($(CONFIG_USB_GADGET_FIVE_ECM),y)
     g_mot_android-objs += f_ecm.o u_ether.o
     g_android-objs += f_ecm.o u_ether.o
endif
endif


obj-$(CONFIG_USB_ZERO)		+= g_zero.o
obj-$(CONFIG_USB_ETH)		+= g_ether.o
obj-$(CONFIG_USB_GADGETFS)	+= gadgetfs.o
obj-$(CONFIG_USB_FILE_STORAGE)	+= g_file_storage.o
obj-$(CONFIG_USB_G_SERIAL)	+= g_serial.o
obj-$(CONFIG_USB_G_PRINTER)	+= g_printer.o
obj-$(CONFIG_USB_MIDI_GADGET)	+= g_midi.o
obj-$(CONFIG_USB_CDC_COMPOSITE) += g_cdc.o
obj-$(CONFIG_USB_MOT_ANDROID)  += g_mot_android.o
obj-$(CONFIG_USB_ANDROID)	+= g_android.o

