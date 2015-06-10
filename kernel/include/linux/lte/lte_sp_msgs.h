/*
 * Copyright (C) 2009-2011 Motorola, Inc.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 * Revisions:
 * Motorola        4/15/09       Initial version
 */

#ifndef LTE_SP_MSGS_H
#define LTE_SP_MSGS_H

#include <linux/lte/host_nas_intfc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	eIPC_LTE_SP_INIT_STATUS_REQ=0xF0,
	eIPC_LTE_SP_INIT_STATUS_RSP=0xF1,
	// Message from SP to QA with default bearer parameters. Avoid parsing again at QA
	eIPC_LTE_SP_QA_REGISTER_CNF=0xF2,
	eIPC_LTE_SP_QA_DETACH_CNF=0xF3,
	// Message from QA to SP signaling creation of secondary PDN
	eIPC_LTE_QA_SP_PDN_CNF=0xF4,
	eIPC_LTE_QA_SP_PDN_REJ=0xF5,
	eIPC_LTE_QA_SP_PDN_REL=0xF6
} tLTE_SP_MSGS_E;

typedef struct SpInitStatusRsp
{
	UINT8 u8Status;
} tSP_INIT_STATUS_RSP_S;

typedef struct SpQaRegisterCnf
{
	UINT8  u8Apn[MAX_APN_NAME_LENGTH];
	UINT8  u8BearerId;
	tQOS_S tQos;
} tSP_QA_REGISTER_CNF_S;

#ifdef __cplusplus
}
#endif
#endif
