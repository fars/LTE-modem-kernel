/*
 * Copyright Motorola, Inc. 2009
 *
 * This Program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTIBILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at
 * your option) any later version.  You should have
 * received a copy of the GNU General Public License
 * along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave,
 *  Cambridge, MA 02139, USA
 */
/**
 *  @File:  lte_data_driver.h
 *
 * Exported LTE data driver "public" functionality to other kernel modules
 *
 * Last Changed Revision: $Rev$
 * Last Changed Date: $LastChangedDate$
 */
#ifndef _LTE_DATA_DRIVER_H_
#define _LTE_DATA_DRIVER_H_

#define HIF_HEADER_SIGNATURE            0xCEADBEAF

#if 0
// these defines are BP centric
#define USB_HIF_DATAOUT_ENDPOINT	1
#define USB_HIF_DATAIN_ENDPOINT		2
#define USB_HIF_NAS_ENDPOINT		3
#define USB_HIF_CTRLIN_ENDPOINT		4
#define USB_HIF_MONITOR_ENDPOINT	5
#endif

// these defines are AP centric
#define HIF_ENDPOINT_DATA_IN            1
#define HIF_ENDPOINT_DATA_OUT           2
#define HIF_ENDPOINT_MGMT_IN            3
#define HIF_ENDPOINT_MGMT_OUT           4

#define INIT_HIF_HEADER_DRV(phdr,ep,ln)\
{\
    phdr->signature = HIF_HEADER_SIGNATURE;\
    phdr->hif.end_point = ep;\
    phdr->hif.length = ln;\
}

#pragma pack(1)
// raw hif header
typedef struct hif_header
{
    unsigned short end_point;
    unsigned short length;
    
} hif_header_t;

// hif header passed between user space and kernel space, more robust
typedef struct hif_header_drv
{

    unsigned long signature;
    hif_header_t hif;
    
} hif_header_drv_t;

typedef struct
{

    size_t len;
    char* data;
    
} mgmt_packet_t;
#pragma pack()


#endif // _LTE_DATA_DRIVER_H_

