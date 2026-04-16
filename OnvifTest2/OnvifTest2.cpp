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

#include "sys_inc.h"
#include "http.h"
#include "http_parse.h"
#include "onvif.h"
#include "onvif_event.h"
#include "onvif_api.h"

/***************************************************************************************/

ONVIF_DEVICE g_device;
ONVIF_DEVICE g_device2;

#define MAX_DEV_NUMS     10

/***************************************************************************************/

int getDeviceIndex(ONVIF_DEVICE * p_device)
{
    if (p_device == &g_device)
    {
        return 0;
    }
    else if (p_device == &g_device2)
    {
        return 1;
    }

    return -1;
}

ONVIF_DEVICE * getDeviceByIndex(int index)
{
    if (0 == index)
    {
        return &g_device;
    }
    else if (1 == index)
    {
        return &g_device2;
    }

    return NULL;
}

/**
 * onvif event notify callback 
 */
void eventNotifyCallback(Notify_REQ * p_req, void * p_data)
{
    NotificationMessageList * p_notify = p_req->notify;  
    NotificationMessageList * p_tmp = p_notify;

    printf("receive event : \r\n");
    printf("\tposturl : %s\r\n", p_req->PostUrl);

    while (p_tmp)
    {
        printf("\tTopic : %s\r\n", p_tmp->NotificationMessage.Topic);

        p_tmp = p_tmp->next;
    }

    int index = -1;
    ONVIF_DEVICE * p_dev = NULL;
    
    sscanf(p_req->PostUrl, "/subscription%d", &index);

    p_dev = getDeviceByIndex(index);
    if (NULL == p_dev)
    {
        return;
    }
    
    onvif_device_add_NotificationMessages(p_dev, p_notify);

    p_dev->events.notify_nums += onvif_get_NotificationMessages_nums(p_notify);

    // max save 100 event notify
    if (p_dev->events.notify_nums > 100)
    {
        p_dev->events.notify_nums -= onvif_device_free_NotificationMessages(p_dev, p_dev->events.notify_nums - 100);
    }
}

/**
 * onvif event subscribe disconnect callback
 */
void subscribeDisconnectCallback(ONVIF_DEVICE * p_dev, void * p_data)
{    
    printf("\r\nsubscribeDisconnectCallback, %s\r\n", p_dev->binfo.XAddr.host);

    BOOL ret = FALSE;
    
    ret = Subscribe(p_dev, getDeviceIndex(p_dev));

    printf("Subscribe, ret = %d\r\n", ret);
}

void errorHandler(ONVIF_DEVICE * p_device)
{
    if (p_device->authFailed) // Authentication failed 
    {
        printf("Authentication failed\r\n");
    }

    switch (p_device->errCode)
    {
    case ONVIF_ERR_ConnFailure: // Connection failed
        printf("connect failure\r\n");
        break;
        
    case ONVIF_ERR_MallocFailure: // Failed to allocate memory
        printf("memory malloc failure\r\n");
        break;
        
    case ONVIF_ERR_NotSupportHttps: // The device requires an HTTPS connection, but the onvif client library does not support it (the HTTPS compilation macro is not enabled)
        printf("not support https\r\n");
        break;
        
    case ONVIF_ERR_RecvTimeout: // Message receiving timeout
        printf("message receive timeout\r\n");
        break;
        
    case ONVIF_ERR_InvalidContentType: // Device response message content is invalid
        printf("invalid content type\r\n");
        break;
        
    case ONVIF_ERR_NullContent: // Device response message has no content
        printf("null content\r\n");
        break; 
        
    case ONVIF_ERR_ParseFailed: // Parsing the message failed
        printf("message parse failed\r\n");
        break;
        
    case ONVIF_ERR_HandleFailed: // Message handling failed
        printf("message handle failed\r\n");
        break;
        
    case ONVIF_ERR_HttpResponseError: // The device responded with an error message
        printf("code=%s\r\n", p_device->fault.Code);
        printf("subcode=%s\r\n", p_device->fault.Subcode);
        printf("reason=%s\r\n", p_device->fault.Reason);
        break;

    default:
        break;
    }
}

int main(int argc, char* argv[])
{
    network_init();

    // open log file
    log_init("onviftest2.log");
    log_set_level(HT_LOG_DBG);

    // init sys buffer
    sys_buf_init(10 * MAX_DEV_NUMS);

    // init http message buffer
    http_msg_buf_init(10 * MAX_DEV_NUMS);

    // init event handler
    //  bind the http server to 0.0.0.0:30100 and https server to 0.0.0.0:30400
    // onvif_event_init(1, NULL, 30100, 1, NULL, 30400, "cert.pem", "key.pem", 1, MAX_DEV_NUMS);
    //  bind the http server to 0.0.0.0:30100
    onvif_event_init(1, NULL, 30100, 0, NULL, 0, NULL, NULL, 1, MAX_DEV_NUMS);
    
    // set event callback
    onvif_set_event_notify_cb(eventNotifyCallback, 0);
    
    // set event subscribe disconnect callback
    onvif_set_subscribe_disconnect_cb(subscribeDisconnectCallback, 0);

    // init g_device 
    memset(&g_device, 0, sizeof(g_device));

    // init device
    onvif_initDevice(&g_device, "192.168.1.3", 8000, 0);
    // set device login information
    onvif_SetAuthInfo(&g_device, "admin", "admin");
    // set auth method
    onvif_SetAuthMethod(&g_device, AuthMethod_UsernameToken);
    // set request timeout
    onvif_SetReqTimeout(&g_device, 5000);
        
    if (!GetSystemDateAndTime(&g_device))
    {
        errorHandler(&g_device);
        printf("%s, GetSystemDateAndTime failed\r\n", g_device.binfo.XAddr.host);
    }  
    
    if (!GetCapabilities(&g_device))
    {
        errorHandler(&g_device);
        printf("%s, GetCapabilities failed\r\n", g_device.binfo.XAddr.host);
    }

    if (!GetServices(&g_device))
    {
        errorHandler(&g_device);
        printf("%s, GetServices failed\r\n", g_device.binfo.XAddr.host);
    }

    if (!GetDeviceInformation(&g_device))
    {
        errorHandler(&g_device);
        printf("%s, GetDeviceInformation failed\r\n", g_device.binfo.XAddr.host);
    }

    if (g_device.Capabilities.events.support == 1)
    {
        if (Subscribe(&g_device, getDeviceIndex(&g_device)))
        {
            printf("Subscribe successful!\r\n");
        }
        else
        {
            errorHandler(&g_device);
            printf("Subscribe failed!\r\n");
        }    
    }

    // init device
    onvif_initDevice(&g_device2, "192.168.1.4", 8000, 0);
    // set device login information
    onvif_SetAuthInfo(&g_device2, "admin", "admin");
    // set auth method
    onvif_SetAuthMethod(&g_device2, AuthMethod_UsernameToken);
    // set request timeout
    onvif_SetReqTimeout(&g_device2, 5000);
        
    if (!GetSystemDateAndTime(&g_device2))
    {
        errorHandler(&g_device2);
        printf("%s, GetSystemDateAndTime failed\r\n", g_device2.binfo.XAddr.host);
    }  
    
    if (!GetCapabilities(&g_device2))
    {
        errorHandler(&g_device2);
        printf("%s, GetCapabilities failed\r\n", g_device2.binfo.XAddr.host);
    }

    if (!GetServices(&g_device2))
    {
        errorHandler(&g_device2);
        printf("%s, GetServices failed\r\n", g_device2.binfo.XAddr.host);
    }

    if (!GetDeviceInformation(&g_device2))
    {
        errorHandler(&g_device2);
        printf("%s, GetDeviceInformation failed\r\n", g_device2.binfo.XAddr.host);
    }

    if (g_device2.Capabilities.events.support == 1)
    {
        if (Subscribe(&g_device2, getDeviceIndex(&g_device2)))
        {
            printf("Subscribe successful!\r\n");
        }
        else
        {
            errorHandler(&g_device2);
            printf("Subscribe failed!\r\n");
        }
    }

    for (;;)
    {
        if (getchar() == 'q')
        {
            break;
        }
        
        sleep(1);
    }

    onvif_free_device(&g_device); 
    onvif_free_device(&g_device2);

    onvif_event_deinit();
    
    http_msg_buf_deinit();
    sys_buf_deinit();

    log_close();

    return 0;
}



