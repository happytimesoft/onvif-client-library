/***************************************************************************************
 *
 *  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
 *
 *  By downloading, copying, installing or using the software you agree to this license.
 *  If you do not agree to this license, do not download, install, 
 *  copy or use the software.
 *
 *  Copyright (C) 2014-2026, Happytimesoft Corporation, all rights reserved.
 *
 *  Redistribution and use in binary forms, with or without modification, are permitted.
 *
 *  Unless required by applicable law or agreed to in writing, software distributed 
 *  under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 *  CONDITIONS OF ANY KIND, either express or implied. See the License for the specific
 *  language governing permissions and limitations under the License.
 *
****************************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include "sys_inc.h"
#include "onvif.h"


typedef struct
{
    ONVIF_DEVICE    onvif_device;
    
    int         flags;                      // FLAG_MANUAL mean added manual, other auto discovery device
    int         state;                      // 0 -- offline; 1 -- online

    void      * p_user;                     // user data
    
    pthread_t   thread_handler;             // get information thread handler
    BOOL        need_update;                // if update device information

    int         snapshot_len;               // devcie snapshot buffer length
    uint8     * snapshot;                   // device snapshot buffer
} ONVIF_DEVICE_EX;


#endif



