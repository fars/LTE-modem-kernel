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
 *  @File:  lte_classifier.h
 *
 * Exported lte classifier "public" functionality to other kernel modules
 *
 */
#ifndef _LTE_CLASSIFIER_H_
#define _LTE_CLASSIFIER_H_


#include <linux/version.h>


void QA_SetLogLevel(short log_level);
long lte_qa_set_eps_mac_map(unsigned char eps_id, unsigned char* mac_addr);
long lte_qa_set_bridge_mode(unsigned long enable);
long lte_qa_get_bridge_mode(unsigned long* penabled);


#endif // _LTE_CLASSIFIER_H_

