/*
 * Copyright Motorola, Inc. 2009-2011
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


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/mm.h> 

#include <linux/ctype.h>
#include <linux/string.h>

#include <linux/lte/AMP_typedef.h>
#include <linux/lte/AMP_lte_log.h>
#include <linux/lte/AMP_lte_error.h>

#include "lte_data_driver_cmds.h"


// IKCHUM-2783: ignore unused variables caused by disabling AMP_LOG_Print
#pragma GCC diagnostic ignored "-Wunused-variable"


//******************************************************************************
// static functions
//******************************************************************************


/**
 * strtok was removed from the kernel
 *
 * @param char *s - source string
 * @param const char *delim - delimiters
 *
 * @return char* - new token or 0
 */
static char* __strtok(char *s, const char *delim)
{
    char *spanp;
    int c, sc;
    char *tok;
    static char *last;


    if (s == NULL && (s = last) == NULL)
        return (NULL);

cont:
    c = *s++;
    for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
        if (c == sc)
            goto cont;
    }

    if (c == 0) {
        last = NULL;
        return (NULL);
    }
    tok = s - 1;


    for (;;) {
        c = *s++;
        spanp = (char *)delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                last = s;
                return (tok);
            }
        } while (sc != 0);
    }
}


/**
 * Replaces characters specified in the "hits" array with white space
 *
 * @param char* string - source string
 * @param char* hits - hit string
 *
 * @return void
 */
static void replace_with_whitespace(char* string, char* hits)
{
    int len = 0;
    int hit = 0;
    char* s = string;
    char* h = hits;
    
    
    len = strlen(string);
    if (!len)
        return;
    
    // scan the string looking for hit characters to replace
    while (*s != 0x00)
    {
        hit = 0;
        h = hits;
        while (*h != 0x00)
        {
            if (*s == *h)
            {
                hit = 1;
                break;
            }
            h++;
        }
        
        // replace with a space 
        if (hit)
            *s = ' ';
        
        s++;
    }
}


/**
 * Converts all characters to lowercase
 *
 * @param char* string - source string
 *
 * @return void
 */
static void __strlwr(char* string)
{
    unsigned char* s = (unsigned char*) string;

    if (!s)
        return;

    while(*s != 0x00)
    {
        *s = tolower(*s);
        s++;
    }

}


/*****************************************************************************
 * @param[in,out] dest Where to store the parameter.  String pointer.
 * @param[in] src Pointer to the parameter entered.
 * @param[in] param1 Size of dest.
 * @param[in] param2 Unused.
 *
 * @brief Parses and stores a passed in string parameter.  Wrapper to provide
 *        a generic interface.
 *
 * @return None.
 *****************************************************************************/
static void stringParser(void *dest, char *src, long param1, long param2)
{
    char *destChar = dest;

    strncpy(destChar, src, param1 - 1);
    destChar[param1 - 1] = '\0';
}


/*****************************************************************************
 * @param[in,out] dest Where to store the parameter; long pointer.
 * @param[in] src Pointer to the parameter entered.
 * @param[in] param1 Minimum value of the parameter.
 * @param[in] param2 Maximum value of the parameter.
 *
 * @brief Parses and stores a passed in long parameter.  Wrapper to provide
 *        a generic interface.
 *
 * @return None.
 *****************************************************************************/
static void longParser(void *dest, char *src, long param1, long param2)
{
    int ival = 0;

    sscanf(src, "%d", &ival);
    COERCE_VAL(ival, param1, param2);
    *((long *)dest) = ival;
}


#define BYTES_PER_CHAR 2
/*****************************************************************************
 * @param[in,out] dest Where to store the parameter; unsigned char array.
 * @param[in] src Pointer to the parameter entered.
 * @param[in] param1 Unused
 * @param[in] param2 Unused.
 *
 * @brief Parses and stores a passed in MAC address.  Wrapper to provide
 *        a generic interface.  Sets to all zeros if MAC invalid.
 *
 * @return None.
 *****************************************************************************/
static void macParser(void *dest, char *src, long param1, long param2)
{
    unsigned long macIdx;
    char macByte[3];
    unsigned char *macPtr = dest;

    if (strnlen(src, ETH_ALEN * BYTES_PER_CHAR + 1) != ETH_ALEN * BYTES_PER_CHAR)
    {
        memset(dest, 0, ETH_ALEN);
    }
    else
    {
        macByte[2] = '\0';
        for (macIdx = 0; macIdx < ETH_ALEN; macIdx++)
        {
            /* Copy over one byte of the MAC address */
            memcpy(macByte, &(src[macIdx * BYTES_PER_CHAR]), BYTES_PER_CHAR);
            macPtr[macIdx] = (unsigned char)simple_strtoul(macByte, NULL, 16);
        }
    }
}

// ***************************************************************************
// Supported command parameters
// ***************************************************************************

static PARSING_INFO net_new_params[] = {
    {stringParser, offsetof(lte_dd_cmd_net_new_t, ifname),
        NETDRV_MAX_DEV_NAME, 0},
    {longParser, offsetof(lte_dd_cmd_net_new_t, eps_id),
        LTE_DD_MIN_EPS, LTE_DD_MAX_EPS},
    {longParser, offsetof(lte_dd_cmd_net_new_t, priority),
        0, 255}};
static PARSING_INFO net_delete_params[] = {
    {stringParser, offsetof(lte_dd_cmd_net_delete_t, ifname),
        NETDRV_MAX_DEV_NAME, 0}};
static PARSING_INFO net_eps_params[] = {
    {stringParser, offsetof(lte_dd_cmd_net_eps_t, ifname),
        NETDRV_MAX_DEV_NAME, 0},
    {longParser, offsetof(lte_dd_cmd_net_eps_t, eps_id),
        LTE_DD_MIN_EPS, LTE_DD_MAX_EPS}};
static PARSING_INFO net_eps_mac_map_add_dl_params[] = {
    {longParser, offsetof(lte_dd_cmd_net_eps_mac_map_add_dl_t, eps_id),
        LTE_DD_MIN_EPS, LTE_DD_MAX_EPS},
    {macParser, offsetof(lte_dd_cmd_net_eps_mac_map_add_dl_t, mac_addr),
        0, 0}};
static PARSING_INFO net_eps_mac_map_rm_dl_params[] = {
    {longParser, offsetof(lte_dd_cmd_net_eps_mac_map_rm_dl_t, eps_id),
        LTE_DD_MIN_EPS, LTE_DD_MAX_EPS}};
static PARSING_INFO net_bridge_mode_dl_params[] = {
    {longParser, offsetof(lte_dd_cmd_net_bridge_mode_dl_t, enable),
        0, 1}};
static PARSING_INFO net_eps_mac_map_add_ul_params[] = {
    {longParser, offsetof(lte_dd_cmd_net_eps_mac_map_add_ul_t, eps_id),
        LTE_DD_MIN_EPS, LTE_DD_MAX_EPS},
    {macParser, offsetof(lte_dd_cmd_net_eps_mac_map_add_ul_t, mac_addr),
        0, 0}};
static PARSING_INFO net_eps_mac_map_rm_ul_params[] = {
    {longParser, offsetof(lte_dd_cmd_net_eps_mac_map_rm_ul_t, eps_id),
        LTE_DD_MIN_EPS, LTE_DD_MAX_EPS}};
static PARSING_INFO net_bridge_mode_ul_params[] = {
    {longParser, offsetof(lte_dd_cmd_net_bridge_mode_ul_t, enable),
        0, 1}};
static PARSING_INFO net_priority_params[] = {
    {stringParser, offsetof(lte_dd_cmd_net_priority_t, ifname),
        NETDRV_MAX_DEV_NAME, 0},
    {longParser, offsetof(lte_dd_cmd_net_priority_t, priority),
        0, 255}};
static PARSING_INFO net_log_params[] = {
    {longParser, offsetof(lte_dd_cmd_net_log_t, log_level),
        AMP_LOG_LEVEL_ERROR, AMP_LOG_LEVEL_DEBUG3}};
static PARSING_INFO net_perf_test_params[] = {
    {longParser, offsetof(lte_dd_cmd_net_perf_test_t, duration),
        1, (5 * 60)},
    {longParser, offsetof(lte_dd_cmd_net_perf_test_t, packet_size),
        64, NETDRV_MTU},
    {longParser, offsetof(lte_dd_cmd_net_perf_test_t, packet_delay),
        0, 0x7FFFFFFF}};
static PARSING_INFO dd_log_params[] = {
    {longParser, offsetof(lte_dd_cmd_dd_log_t, log_level),
        AMP_LOG_LEVEL_ERROR, AMP_LOG_LEVEL_DEBUG3}};
static PARSING_INFO dd_ul_packet_profile_params[] = {
    {longParser, offsetof(lte_dd_cmd_dd_ul_packet_profile_t, enable),
        0, 1},
    {longParser, offsetof(lte_dd_cmd_dd_ul_packet_profile_t, reset),
        0, 1}};
static PARSING_INFO dd_dl_packet_profile_params[] = {
    {longParser, offsetof(lte_dd_cmd_dd_dl_packet_profile_t, enable),
        0, 1},
    {longParser, offsetof(lte_dd_cmd_dd_dl_packet_profile_t, reset),
        0, 1}};
static PARSING_INFO dd_multipacket_params[] = {
    {longParser, offsetof(lte_dd_cmd_dd_multipacket_t, enable),
        0, 1}};
static PARSING_INFO dd_lpbp_mode_params[] = {
    {longParser, offsetof(lte_dd_cmd_dd_lpbp_mode_t, enable),
        0, 1}};
static PARSING_INFO dd_disable_ul_params[] = {
    {longParser, offsetof(lte_dd_cmd_dd_disable_ul_t, enable),
        0, 1}};
static PARSING_INFO dd_qos_log_params[] = {
    {longParser, offsetof(lte_dd_cmd_qos_log_t, log_level),
        AMP_LOG_LEVEL_ERROR, AMP_LOG_LEVEL_DEBUG3}};
static PARSING_INFO dd_qos_mode_params[] = {
    {longParser, offsetof(lte_dd_cmd_qos_mode_t, mode),
        0, 1}};
static PARSING_INFO hif_cmd_params[] = {
    {longParser, offsetof(lte_dd_cmd_hif_cmd_t, cmd),
        -2147483647, 2147483647}};

// ***************************************************************************
// Supported command data
// ***************************************************************************

static COMMAND_INFO commandList[] = {
    {CMD_STR_NET_NEW, lte_dd_cmd_net_new,
     sizeof(net_new_params)/sizeof(PARSING_INFO), net_new_params},
    {CMD_STR_NET_DELETE, lte_dd_cmd_net_delete,
     sizeof(net_delete_params)/sizeof(PARSING_INFO), net_delete_params},
    {CMD_STR_NET_EPS, lte_dd_cmd_net_eps,
     sizeof(net_eps_params)/sizeof(PARSING_INFO), net_eps_params},
    {CMD_STR_NET_EPS_MAC_MAP_ADD_DL, lte_dd_cmd_net_eps_mac_map_add_dl,
     sizeof(net_eps_mac_map_add_dl_params)/sizeof(PARSING_INFO), net_eps_mac_map_add_dl_params},
    {CMD_STR_NET_EPS_MAC_MAP_RM_DL, lte_dd_cmd_net_eps_mac_map_rm_dl,
     sizeof(net_eps_mac_map_rm_dl_params)/sizeof(PARSING_INFO), net_eps_mac_map_rm_dl_params},
    {CMD_STR_NET_BRIDGE_MODE_DL, lte_dd_cmd_net_bridge_mode_dl,
     sizeof(net_bridge_mode_dl_params)/sizeof(PARSING_INFO), net_bridge_mode_dl_params},
    {CMD_STR_NET_EPS_MAC_MAP_ADD_UL, lte_dd_cmd_net_eps_mac_map_add_ul,
     sizeof(net_eps_mac_map_add_ul_params)/sizeof(PARSING_INFO), net_eps_mac_map_add_ul_params},
    {CMD_STR_NET_EPS_MAC_MAP_RM_UL, lte_dd_cmd_net_eps_mac_map_rm_ul,
     sizeof(net_eps_mac_map_rm_ul_params)/sizeof(PARSING_INFO), net_eps_mac_map_rm_ul_params},
    {CMD_STR_NET_BRIDGE_MODE_UL, lte_dd_cmd_net_bridge_mode_ul,
     sizeof(net_bridge_mode_ul_params)/sizeof(PARSING_INFO), net_bridge_mode_ul_params},
    {CMD_STR_NET_PRIORITY, lte_dd_cmd_net_priority,
     sizeof(net_priority_params)/sizeof(PARSING_INFO), net_priority_params}, 
    {CMD_STR_NET_LOG, lte_dd_cmd_net_log,
     sizeof(net_log_params)/sizeof(PARSING_INFO), net_log_params},
    {CMD_STR_NET_PERF_TEST, lte_dd_cmd_net_perf_test,
     sizeof(net_perf_test_params)/sizeof(PARSING_INFO), net_perf_test_params},
    {CMD_STR_DD_LOG, lte_dd_cmd_dd_log,
     sizeof(dd_log_params)/sizeof(PARSING_INFO), dd_log_params},
    {CMD_STR_DD_UL_PACKET_PROFILE, lte_dd_cmd_dd_ul_packet_profile,
     sizeof(dd_ul_packet_profile_params)/sizeof(PARSING_INFO),
                                                dd_ul_packet_profile_params},
    {CMD_STR_DD_DL_PACKET_PROFILE, lte_dd_cmd_dd_dl_packet_profile,
     sizeof(dd_dl_packet_profile_params)/sizeof(PARSING_INFO),
                                                dd_dl_packet_profile_params},
    {CMD_STR_DD_MULTIPACKET, lte_dd_cmd_dd_multipacket,
     sizeof(dd_multipacket_params)/sizeof(PARSING_INFO), dd_multipacket_params},
    {CMD_STR_DD_LPBP_MODE, lte_dd_cmd_dd_lpbp_mode,
     sizeof(dd_lpbp_mode_params)/sizeof(PARSING_INFO), dd_lpbp_mode_params},
    {CMD_STR_DD_DISABLE_UL, lte_dd_cmd_dd_disable_ul,
     sizeof(dd_disable_ul_params)/sizeof(PARSING_INFO), dd_disable_ul_params},
    {CMD_STR_QOS_LOG, lte_dd_cmd_qos_log,
     sizeof(dd_qos_log_params)/sizeof(PARSING_INFO), dd_qos_log_params},
    {CMD_STR_QOS_MODE, lte_dd_cmd_qos_mode,
     sizeof(dd_qos_mode_params)/sizeof(PARSING_INFO), dd_qos_mode_params},
    {CMD_STR_HIF_CMD, lte_dd_cmd_hif_cmd,
     sizeof(hif_cmd_params)/sizeof(PARSING_INFO), hif_cmd_params}};

//******************************************************************************
// exported functions
//******************************************************************************


/**
 * Parses a comman string and generates a list of command structures
 *
 * @param char* cmd_str - command string
 * @param lte_dd_cmd_t cmd_array - array of command structures
 * @param unsigned long* populated - array elements populated
 *
 *  Commands:
 *
 *      net_new:name:epsid:priority
 *      net_delete:name
 *      net_eps:name:epsid
 *      net_priority:name:priority
 *      net_log:level
 *      dd_log:level
 *
 * @return long, error code
 */
long lte_dd_parse_dd_cmds(char* cmd_str,
                          lte_dd_cmd_t cmd_array[MAX_DD_CMD_ARRAY],
                          unsigned long* populated)
{
    char* func_name = "lte_dd_parse_dd_cmds";
    int len = 0;
    int buflen = 0;
    int done = 0;
    int num_scan = 0;
    lte_dd_cmd_t* pcmd = NULL;
    char* token = 0;
    char* temp_str = NULL;
    char seps[] = " ,\t\n";
    char commandStr[128];
    char param[3][128] = {{0}, {0}, {0}};
    long commandIdx, commandTokenIdx;
    COMMAND_INFO *cmdInfo;
    PARSING_INFO *parsingInfo;

    // verify pointers
    if (!cmd_str || !cmd_array || !populated)
        return -1;
    
    *populated = 0;
    memset(cmd_array, 0x00, sizeof(cmd_array));
    pcmd = &(cmd_array[0]);
    
    // verify length
    len = strlen(cmd_str);
    if (!len)
        return -2;
    
    // alloc a temporary buffer since strtok is destructive
    buflen = len + 8;    
    temp_str = (char *) kmalloc(buflen, GFP_KERNEL);
    if (!temp_str)
        return -3;
    
    memset(temp_str, 0x00, buflen);
    strcpy(temp_str, cmd_str); 
    __strlwr(temp_str);
    token = __strtok(temp_str, seps);
    while(token != NULL && !done)
    {
        replace_with_whitespace(token, ":;");
        num_scan = sscanf(token, "%127s%127s%127s%127s", commandStr,
                        param[0], param[1], param[2]);
        if (num_scan <= 0)
        {
            AMP_LOG_Error(
                "%s - ERROR: error scanning command token %s\n",
                func_name, token);
            goto NEXT_TOKEN;     
        }   

        /* Search for a matching command */
        commandStr[127] = param[0][127] = param[1][127] = param[2][127] = '\0';
        for (commandIdx = 0;
             commandIdx < sizeof(commandList)/sizeof(COMMAND_INFO);
             commandIdx++)
        {
            cmdInfo = &commandList[commandIdx];
            if (strcmp(commandStr, cmdInfo->cmdString) == 0)
            {
                /* Include the command as a paramter */
                if (num_scan < (cmdInfo->numCmdParams + 1))
                {
                    AMP_LOG_Error(
                        "%s: ERROR: invalid number of arguments(%d) for '%s', "
                        "expected %ld; skipping '%s'\n",
                        func_name, num_scan, commandStr,
                        cmdInfo->numCmdParams + 1, token);
                    break;
                }
                else
                {
                    pcmd->cmd_id = cmdInfo->commandId;
                    for (commandTokenIdx = 0;
                         commandTokenIdx < cmdInfo->numCmdParams;
                         commandTokenIdx++)
                    {
                        parsingInfo = &cmdInfo->cmdParams[commandTokenIdx];
                        (parsingInfo->parsingFunc)(
                                         (unsigned char *)(&pcmd->cmd_data) +
                                         parsingInfo->paramDestOffset,
                                         param[commandTokenIdx],
                                         parsingInfo->funcParam1,
                                         parsingInfo->funcParam2);
                    }
                    *populated += 1;
                    pcmd++;

                    // if the array is full, then stop
                    if (MAX_DD_CMD_ARRAY == *populated)
                    {
                        AMP_LOG_Error(
                        "%s: ERROR: Array full, all commands may not have been "
                        "processed\n", func_name);
                        done = 1;
                    }
                }
                break;
            }
        }

        if (sizeof(commandList)/sizeof(COMMAND_INFO) == commandIdx)
        {
            AMP_LOG_Error("%s: ERROR: command %s is not a valid command,"
                     "skipping command\n", func_name, commandStr);
        }

NEXT_TOKEN:

        // next token
        token = __strtok(NULL, seps);

    }

    // free the temp string
    if (temp_str)
    {
        kfree(temp_str);
        temp_str = NULL;
    }

    return 0;
}
