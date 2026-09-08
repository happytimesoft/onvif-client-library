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
#include "onvif_probe.h"
#include "onvif_event.h"
#include "onvif_api.h"
#include "config.h"
#include "linked_list.h"

/***************************************************************************************/
PPSN_CTX * m_dev_fl;        // device free list
PPSN_CTX * m_dev_ul;        // device used list

BOOL       m_bStateDetect;  // device state detect flag
pthread_t  m_hStateDetect;  // device state detect threand handler

#define MAX_DEV_NUMS     10

/***************************************************************************************/

ONVIF_DEVICE_EX * findDevice(ONVIF_DEVICE_EX * pdevice)
{
    ONVIF_DEVICE_EX * p_dev = (ONVIF_DEVICE_EX *) pps_lookup_start(m_dev_ul);
    while (p_dev)
    {
        if (strcmp(p_dev->onvif_device.binfo.XAddr.host, pdevice->onvif_device.binfo.XAddr.host) == 0 &&
            p_dev->onvif_device.binfo.XAddr.port == pdevice->onvif_device.binfo.XAddr.port)
        {
            break;
        }
        else if (p_dev->onvif_device.binfo.EndpointReference[0] != '\0' && 
            pdevice->onvif_device.binfo.EndpointReference[0] != '\0' &&
            !strcmp(p_dev->onvif_device.binfo.EndpointReference, pdevice->onvif_device.binfo.EndpointReference))
        {
            break;
        }
        
        p_dev = (ONVIF_DEVICE_EX *) pps_lookup_next(m_dev_ul, p_dev);
    }

    pps_lookup_end(m_dev_ul);

    return p_dev;  
}

ONVIF_DEVICE_EX * findDeviceByEndpointReference(char * p_EndpointReference)
{
    ONVIF_DEVICE_EX * p_dev = (ONVIF_DEVICE_EX *) pps_lookup_start(m_dev_ul);
    while (p_dev)
    {
        if (strcmp(p_dev->onvif_device.binfo.EndpointReference, p_EndpointReference) == 0)
        {
            break;
        }
        
        p_dev = (ONVIF_DEVICE_EX *) pps_lookup_next(m_dev_ul, p_dev);
    }

    pps_lookup_end(m_dev_ul);

    return p_dev;
}

ONVIF_DEVICE_EX * addDevice(ONVIF_DEVICE_EX * pdevice)
{
    ONVIF_DEVICE_EX * p_dev = (ONVIF_DEVICE_EX *) pps_fl_pop(m_dev_fl);
    if (p_dev)
    {
        memcpy(p_dev, pdevice, sizeof(ONVIF_DEVICE_EX));
        p_dev->p_user = 0;
        p_dev->onvif_device.events.init_term_time = 60;

        // set device login information
        onvif_SetAuthInfo(&p_dev->onvif_device, "admin", "admin");

        // set auth method
        onvif_SetAuthMethod(&p_dev->onvif_device, AuthMethod_UsernameToken);
        
        // set request timeout
        onvif_SetReqTimeout(&p_dev->onvif_device, 5000);
        
        pps_ul_add(m_dev_ul, p_dev);
    }

    return p_dev;
}

void onvifServiceCapabilitiesTest(ONVIF_DEVICE_EX * p_dev)
{
    uint32 i;
    BOOL ret;

    // device service capabilities
    tds_GetServiceCapabilities_RES tds;
    memset(&tds, 0, sizeof(tds));

    ret = onvif_tds_GetServiceCapabilities(&p_dev->onvif_device, NULL, &tds);

    printf("onvif_tds_GetServiceCapabilities return ret = %d\n", ret);

    // media service capabilities
    trt_GetServiceCapabilities_RES trt;
    memset(&trt, 0, sizeof(trt));

    ret = onvif_trt_GetServiceCapabilities(&p_dev->onvif_device, NULL, &trt);

    printf("onvif_trt_GetServiceCapabilities return ret = %d\n", ret);

    // ptz service capabilities
    ptz_GetServiceCapabilities_RES ptz;
    memset(&ptz, 0, sizeof(ptz));

    ret = onvif_ptz_GetServiceCapabilities(&p_dev->onvif_device, NULL, &ptz);

    printf("onvif_ptz_GetServiceCapabilities return ret = %d\n", ret);

    // event service capabilities
    tev_GetServiceCapabilities_RES tev;
    memset(&tev, 0, sizeof(tev));

    ret = onvif_tev_GetServiceCapabilities(&p_dev->onvif_device, NULL, &tev);

    printf("onvif_tev_GetServiceCapabilities return ret = %d\n", ret);

    // imaging service capabilities
    img_GetServiceCapabilities_RES img;
    memset(&img, 0, sizeof(img));

    ret = onvif_img_GetServiceCapabilities(&p_dev->onvif_device, NULL, &img);

    printf("onvif_img_GetServiceCapabilities return ret = %d\n", ret);

    // analytics service capabilities
    tan_GetServiceCapabilities_RES tan;
    memset(&tan, 0, sizeof(tan));

    ret = onvif_tan_GetServiceCapabilities(&p_dev->onvif_device, NULL, &tan);

    printf("onvif_tan_GetServiceCapabilities return ret = %d\n", ret);

    // media2 service capabilities
    tr2_GetServiceCapabilities_RES tr2;
    memset(&tr2, 0, sizeof(tr2));

    ret = onvif_tr2_GetServiceCapabilities(&p_dev->onvif_device, NULL, &tr2);

    printf("onvif_tr2_GetServiceCapabilities return ret = %d\n", ret);

#ifdef DEVICEIO_SUPPORT
    // deviceIO service capabilities
    tmd_GetServiceCapabilities_RES tmd;
    memset(&tmd, 0, sizeof(tmd));

    ret = onvif_tmd_GetServiceCapabilities(&p_dev->onvif_device, NULL, &tmd);

    printf("onvif_tmd_GetServiceCapabilities return ret = %d\n", ret);
#endif

#ifdef PROFILE_G_SUPPORT
    // recording service capabilities
    trc_GetServiceCapabilities_RES trc;
    memset(&trc, 0, sizeof(trc));

    ret = onvif_trc_GetServiceCapabilities(&p_dev->onvif_device, NULL, &trc);

    printf("onvif_trc_GetServiceCapabilities return ret = %d\n", ret);

    // replay service capabilities
    trp_GetServiceCapabilities_RES trp;
    memset(&trp, 0, sizeof(trp));

    ret = onvif_trp_GetServiceCapabilities(&p_dev->onvif_device, NULL, &trp);

    printf("onvif_trp_GetServiceCapabilities return ret = %d\n", ret);

    // search service capabilities
    tse_GetServiceCapabilities_RES tse;
    memset(&tse, 0, sizeof(tse));

    ret = onvif_tse_GetServiceCapabilities(&p_dev->onvif_device, NULL, &tse);

    printf("onvif_tse_GetServiceCapabilities return ret = %d\n", ret);
#endif

#ifdef PROFILE_C_SUPPORT
    // access control service capabilities
    tac_GetServiceCapabilities_RES tac;
    memset(&tac, 0, sizeof(tac));

    ret = onvif_tac_GetServiceCapabilities(&p_dev->onvif_device, NULL, &tac);

    printf("onvif_tac_GetServiceCapabilities return ret = %d\n", ret);

    // door control service capabilities
    tdc_GetServiceCapabilities_RES tdc;
    memset(&tdc, 0, sizeof(tdc));

    ret = onvif_tdc_GetServiceCapabilities(&p_dev->onvif_device, NULL, &tdc);

    printf("onvif_tdc_GetServiceCapabilities return ret = %d\n", ret);
#endif

#ifdef THERMAL_SUPPORT
    // thermal service capabilities
    tth_GetServiceCapabilities_RES tth;
    memset(&tth, 0, sizeof(tth));

    ret = onvif_tth_GetServiceCapabilities(&p_dev->onvif_device, NULL, &tth);

    printf("onvif_tth_GetServiceCapabilities return ret = %d\n", ret);
#endif

#ifdef CREDENTIAL_SUPPORT
    // crendential service capabilities
    tcr_GetServiceCapabilities_RES tcr;
    memset(&tcr, 0, sizeof(tcr));

    ret = onvif_tcr_GetServiceCapabilities(&p_dev->onvif_device, NULL, &tcr);

    printf("onvif_tcr_GetServiceCapabilities return ret = %d\n", ret);
#endif

#ifdef ACCESS_RULES
    // access rules service capabilities
    tar_GetServiceCapabilities_RES tar;
    memset(&tar, 0, sizeof(tar));

    ret = onvif_tar_GetServiceCapabilities(&p_dev->onvif_device, NULL, &tar);

    printf("onvif_tar_GetServiceCapabilities return ret = %d\n", ret);
#endif

#ifdef SCHEDULE_SUPPORT
    // schedule service capabilities
    tsc_GetServiceCapabilities_RES tsc;
    memset(&tsc, 0, sizeof(tsc));

    ret = onvif_tsc_GetServiceCapabilities(&p_dev->onvif_device, NULL, &tsc);

    printf("onvif_tsc_GetServiceCapabilities return ret = %d\n", ret);
#endif

#ifdef RECEIVER_SUPPORT
    // receiver service capabilities
    trv_GetServiceCapabilities_RES trv;
    memset(&trv, 0, sizeof(trv));

    ret = onvif_trv_GetServiceCapabilities(&p_dev->onvif_device, NULL, &trv);

    printf("onvif_trv_GetServiceCapabilities return ret = %d\n", ret);
#endif

#ifdef PROVISIONING_SUPPORT
    // provisioning service capabilities
    tpv_GetServiceCapabilities_RES tpv;
    memset(&tpv, 0, sizeof(tpv));

    ret = onvif_tpv_GetServiceCapabilities(&p_dev->onvif_device, NULL, &tpv);

    printf("onvif_tpv_GetServiceCapabilities return ret = %d\n", ret);
#endif

#ifdef SECURITY_SUPPORT
    // security service capabilities
    tas_GetServiceCapabilities_RES tas;
    memset(&tas, 0, sizeof(tas));

    ret = onvif_tas_GetServiceCapabilities(&p_dev->onvif_device, NULL, &tas);

    printf("onvif_tas_GetServiceCapabilities return ret = %d\n", ret);

    for (i = 0; i < tas.Capabilities.KeystoreCapabilities.sizeSignatureAlgorithms; i++)
    {
        onvif_free_AlgorithmIdentifier(&tas.Capabilities.KeystoreCapabilities.SignatureAlgorithms[i]);
    }
#endif
}

void onvifNetworkTest(ONVIF_DEVICE_EX * p_dev)
{
    BOOL ret;

    {
        tds_GetNetworkInterfaces_RES res;
        memset(&res, 0, sizeof(res));
        
        ret = onvif_tds_GetNetworkInterfaces(&p_dev->onvif_device, NULL, &res);

        printf("onvif_tds_GetNetworkInterfaces return ret = %d\n", ret);

        onvif_free_NetworkInterfaces(&res.NetworkInterfaces);
    }

    {
        tds_GetScopes_RES res;
        memset(&res, 0, sizeof(res));
        
        ret = onvif_tds_GetScopes(&p_dev->onvif_device, NULL, &res);

        printf("onvif_tds_GetScopes return ret = %d\n", ret);
    }

    {
        tds_GetNetworkDefaultGateway_RES res;
        memset(&res, 0, sizeof(res));
        
        ret = onvif_tds_GetNetworkDefaultGateway(&p_dev->onvif_device, NULL, &res);

        printf("onvif_tds_GetNetworkDefaultGateway return ret = %d\n", ret);
    }

    {
        tds_GetDNS_RES res;
        memset(&res, 0, sizeof(res));
        
        ret = onvif_tds_GetDNS(&p_dev->onvif_device, NULL, &res);

        printf("onvif_tds_GetDNS return ret = %d\n", ret);
    }

    {
        tds_GetZeroConfiguration_RES res;
        memset(&res, 0, sizeof(res));
        
        ret = onvif_tds_GetZeroConfiguration(&p_dev->onvif_device, NULL, &res);

        printf("onvif_tds_GetZeroConfiguration return ret = %d\n", ret);
    }
}

void onvifMediaTest(ONVIF_DEVICE_EX * p_dev)
{
    BOOL ret;

    while (1)
    {
        // GetAudioOutputs
        trt_GetAudioOutputs_RES res;
        memset(&res, 0, sizeof(res));        
        
        ret = onvif_trt_GetAudioOutputs(&p_dev->onvif_device, NULL, &res);

        printf("onvif_trt_GetAudioOutputs return ret = %d\n", ret);

        if (!ret || NULL == res.AudioOutputs)
        {
            break;
        }

        // GetAudioOutputConfigurations
        trt_GetAudioOutputConfigurations_RES res1;
        memset(&res1, 0, sizeof(res1));

        ret = onvif_trt_GetAudioOutputConfigurations(&p_dev->onvif_device, NULL, &res1);

        printf("onvif_trt_GetAudioOutputConfigurations return ret = %d\n", ret);

        if (ret && res1.Configurations)
        {
            // GetAudioOutputConfiguration
            trt_GetAudioOutputConfiguration_REQ req2;
            trt_GetAudioOutputConfiguration_RES res2;

            memset(&req2, 0, sizeof(req2));
            memset(&res2, 0, sizeof(res2));

            strcpy(req2.ConfigurationToken, res1.Configurations->Configuration.token);

            ret = onvif_trt_GetAudioOutputConfiguration(&p_dev->onvif_device, &req2, &res2);

            printf("onvif_trt_GetAudioOutputConfiguration return ret = %d\n", ret);

            // SetAudioOutputConfiguration
            trt_SetAudioOutputConfiguration_REQ req3;
            memset(&req3, 0, sizeof(req3));

            memcpy(&req3.Configuration, &res2.Configuration, sizeof(onvif_AudioOutputConfiguration));
            strcpy(req3.Configuration.Name, "test");

            ret = onvif_trt_SetAudioOutputConfiguration(&p_dev->onvif_device, &req3, NULL);

            printf("onvif_trt_SetAudioOutputConfiguration return ret = %d\n", ret);

            // AddAudioOutputConfiguration
            trt_AddAudioOutputConfiguration_REQ req4;
            memset(&req4, 0, sizeof(req4));

            strcpy(req4.ProfileToken, p_dev->onvif_device.curProfile->token);
            strcpy(req4.ConfigurationToken, res2.Configuration.token);
            
            ret = onvif_trt_AddAudioOutputConfiguration(&p_dev->onvif_device, &req4, NULL);

            printf("onvif_trt_AddAudioOutputConfiguration return ret = %d\n", ret);

            // RemoveAudioOutputConfiguration
            trt_RemoveAudioOutputConfiguration_REQ req5;
            memset(&req5, 0, sizeof(req5));

            strcpy(req5.ProfileToken, p_dev->onvif_device.curProfile->token);
            
            ret = onvif_trt_RemoveAudioOutputConfiguration(&p_dev->onvif_device, &req5, NULL);

            printf("onvif_trt_RemoveAudioOutputConfiguration return ret = %d\n", ret);
        
            onvif_free_AudioOutputConfigurations(&res1.Configurations);
        }
        
        // GetAudioDecoderConfigurations
        trt_GetAudioDecoderConfigurations_RES res6;
        memset(&res6, 0, sizeof(res6));

        ret = onvif_trt_GetAudioDecoderConfigurations(&p_dev->onvif_device, NULL, &res6);

        printf("onvif_trt_GetAudioDecoderConfigurations return ret = %d\n", ret);

        if (ret && res6.Configurations)
        {
            // GetAudioDecoderConfiguration
            trt_GetAudioDecoderConfiguration_REQ req7;
            trt_GetAudioDecoderConfiguration_RES res7;

            memset(&req7, 0, sizeof(req7));
            memset(&res7, 0, sizeof(res7));

            strcpy(req7.ConfigurationToken, res6.Configurations->Configuration.token);

            ret = onvif_trt_GetAudioDecoderConfiguration(&p_dev->onvif_device, &req7, &res7);

            printf("onvif_trt_GetAudioDecoderConfiguration return ret = %d\n", ret);

            // onvif_SetAudioDecoderConfiguration
            trt_SetAudioDecoderConfiguration_REQ req8;
            memset(&req8, 0, sizeof(req8));

            memcpy(&req8.Configuration, &res7.Configuration, sizeof(onvif_AudioDecoderConfiguration));
            strcpy(req8.Configuration.Name, "test");

            ret = onvif_trt_SetAudioDecoderConfiguration(&p_dev->onvif_device, &req8, NULL);

            printf("onvif_trt_SetAudioDecoderConfiguration return ret = %d\n", ret);

            // AddAudioDecoderConfiguration
            trt_AddAudioDecoderConfiguration_REQ req9;
            memset(&req9, 0, sizeof(req9));

            strcpy(req9.ProfileToken, p_dev->onvif_device.curProfile->token);
            strcpy(req9.ConfigurationToken, res7.Configuration.token);
            
            ret = onvif_trt_AddAudioDecoderConfiguration(&p_dev->onvif_device, &req9, NULL);

            printf("onvif_trt_AddAudioDecoderConfiguration return ret = %d\n", ret);

            // RemoveAudioDecoderConfiguration
            trt_RemoveAudioDecoderConfiguration_REQ req10;
            memset(&req10, 0, sizeof(req10));

            strcpy(req10.ProfileToken, p_dev->onvif_device.curProfile->token);
            
            ret = onvif_trt_RemoveAudioDecoderConfiguration(&p_dev->onvif_device, &req10, NULL);

            printf("onvif_trt_RemoveAudioDecoderConfiguration return ret = %d\n", ret);

            onvif_free_AudioDecoderConfigurations(&res6.Configurations);
        }
        
        onvif_free_AudioOutputs(&res.AudioOutputs);

        break;
    } 

    while (1)
    {
        // GetMetadataConfigurations
        trt_GetMetadataConfigurations_RES res;
        memset(&res, 0, sizeof(res));
        
        ret = onvif_trt_GetMetadataConfigurations(&p_dev->onvif_device, NULL, &res);

        printf("onvif_trt_GetMetadataConfigurations return ret = %d\n", ret);

        if (NULL == res.Configurations)
        {
            break;
        }
        
        // GetMetadataConfiguration
        trt_GetMetadataConfiguration_REQ req1;
        trt_GetMetadataConfiguration_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.ConfigurationToken, res.Configurations->Configuration.token);
        
        ret = onvif_trt_GetMetadataConfiguration(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_trt_GetMetadataConfiguration return ret = %d\n", ret);

        onvif_free_Configs(&res1.Configuration.AnalyticsEngineConfiguration.AnalyticsModule);

        // GetMetadataConfigurationOptions
        trt_GetMetadataConfigurationOptions_RES res2;

        memset(&res2, 0, sizeof(res2));

        ret = onvif_trt_GetMetadataConfigurationOptions(&p_dev->onvif_device, NULL, &res2);

        printf("onvif_trt_GetMetadataConfigurationOptions return ret = %d\n", ret);

        // SetMetadataConfiguration
        trt_SetMetadataConfiguration_REQ req3;

        memcpy(&req3.Configuration, &res.Configurations->Configuration, sizeof(onvif_MetadataConfiguration));
        req3.ForcePersistence = FALSE;
        req3.Configuration.PTZStatus.Status = !req3.Configuration.PTZStatus.Status;
        
        ret = onvif_trt_SetMetadataConfiguration(&p_dev->onvif_device, &req3, NULL);

        printf("onvif_trt_SetMetadataConfiguration return ret = %d\n", ret);

        // SetMetadataConfiguration
        req3.Configuration.PTZStatus.Status = !req3.Configuration.PTZStatus.Status;
        
        ret = onvif_trt_SetMetadataConfiguration(&p_dev->onvif_device, &req3, NULL);

        printf("onvif_trt_SetMetadataConfiguration return ret = %d\n", ret);

        if (p_dev->onvif_device.curProfile)
        {
            // AddMetadataConfiguration
            trt_AddMetadataConfiguration_REQ req4;

            strcpy(req4.ConfigurationToken, res.Configurations->Configuration.token);
            strcpy(req4.ProfileToken, p_dev->onvif_device.curProfile->token);

            ret = onvif_trt_AddMetadataConfiguration(&p_dev->onvif_device, &req4, NULL);

            printf("onvif_trt_AddMetadataConfiguration return ret = %d\n", ret);

            // RemoveMetadataConfiguration
            trt_RemoveMetadataConfiguration_REQ req5;

            strcpy(req5.ProfileToken, p_dev->onvif_device.curProfile->token);

            ret = onvif_trt_RemoveMetadataConfiguration(&p_dev->onvif_device, &req5, NULL);

            printf("onvif_trt_RemoveMetadataConfiguration return ret = %d\n", ret);
        }

        // free resource
        onvif_free_MetadataConfigurations(&res.Configurations);

        break;
    }

    while (1)
    {
        // GetVideoAnalyticsConfigurations
        trt_GetVideoAnalyticsConfigurations_RES res;
        memset(&res, 0, sizeof(res));
        
        ret = onvif_trt_GetVideoAnalyticsConfigurations(&p_dev->onvif_device, NULL, &res);

        printf("onvif_trt_GetVideoAnalyticsConfigurations return ret = %d\n", ret);

        if (NULL == res.Configurations)
        {
            break;
        }
        
        // GetVideoAnalyticsConfiguration
        trt_GetVideoAnalyticsConfiguration_REQ req1;
        trt_GetVideoAnalyticsConfiguration_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.ConfigurationToken, res.Configurations->Configuration.token);
        
        ret = onvif_trt_GetVideoAnalyticsConfiguration(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_trt_GetVideoAnalyticsConfiguration return ret = %d\n", ret);

        onvif_free_Configs(&res1.Configuration.AnalyticsEngineConfiguration.AnalyticsModule);
        onvif_free_Configs(&res1.Configuration.RuleEngineConfiguration.Rule);
        
        // SetVideoAnalyticsConfiguration
        trt_SetVideoAnalyticsConfiguration_REQ req2;

        memcpy(&req2.Configuration, &res.Configurations->Configuration, sizeof(onvif_VideoAnalyticsConfiguration));
        req2.ForcePersistence = FALSE;
        strcpy(req2.Configuration.Name, "video_analytics_test");
        
        ret = onvif_trt_SetVideoAnalyticsConfiguration(&p_dev->onvif_device, &req2, NULL);

        printf("onvif_trt_SetVideoAnalyticsConfiguration return ret = %d\n", ret);

        // SetVideoAnalyticsConfiguration
        memcpy(&req2.Configuration, &res.Configurations->Configuration, sizeof(onvif_VideoAnalyticsConfiguration));
        
        ret = onvif_trt_SetVideoAnalyticsConfiguration(&p_dev->onvif_device, &req2, NULL);

        printf("onvif_trt_SetVideoAnalyticsConfiguration return ret = %d\n", ret);

        if (p_dev->onvif_device.curProfile)
        {
            // AddMetadataConfiguration
            trt_AddVideoAnalyticsConfiguration_REQ req3;

            strcpy(req3.ConfigurationToken, res.Configurations->Configuration.token);
            strcpy(req3.ProfileToken, p_dev->onvif_device.curProfile->token);

            ret = onvif_trt_AddVideoAnalyticsConfiguration(&p_dev->onvif_device, &req3, NULL);

            printf("onvif_trt_AddVideoAnalyticsConfiguration return ret = %d\n", ret);

            // RemoveMetadataConfiguration
            trt_RemoveVideoAnalyticsConfiguration_REQ req4;

            strcpy(req4.ProfileToken, p_dev->onvif_device.curProfile->token);

            ret = onvif_trt_RemoveVideoAnalyticsConfiguration(&p_dev->onvif_device, &req4, NULL);

            printf("onvif_trt_RemoveVideoAnalyticsConfiguration return ret = %d\n", ret);
        }

        // free resource
        onvif_free_VideoAnalyticsConfigurations(&res.Configurations);

        break;
    }
}

void onvifUserTest(ONVIF_DEVICE_EX * p_dev)
{
    BOOL ret;
    char username[32], oldname[32], oldpass[32];

    // CreateUsers
    tds_CreateUsers_REQ req;
    memset(&req, 0, sizeof(req));

    snprintf(username, sizeof(username), "testuser%d", rand()%1000);

    strcpy(req.User.Username, username);
    req.User.PasswordFlag = 1;
    strcpy(req.User.Password, "testpass");
    req.User.UserLevel = UserLevel_Administrator;
    
    ret = onvif_tds_CreateUsers(&p_dev->onvif_device, &req, NULL);
    
    printf("onvif_tds_CreateUsers return ret = %d\n", ret);

    if (!ret)
    {
        return;
    }

    strcpy(oldname, p_dev->onvif_device.username);
    strcpy(oldpass, p_dev->onvif_device.password);

    onvif_SetAuthInfo(&p_dev->onvif_device, username, "testpass");

    // GetNetworkProtocols
    tds_GetNetworkProtocols_REQ req1;
    tds_GetNetworkProtocols_RES res1;

    memset(&req1, 0, sizeof(req1));
    memset(&res1, 0, sizeof(res1));

    ret = onvif_tds_GetNetworkProtocols(&p_dev->onvif_device, &req1, &res1);
    
    printf("onvif_tds_GetNetworkProtocols return ret = %d\n", ret);

    // DeleteUsers
    tds_DeleteUsers_REQ req2;
    memset(&req2, 0, sizeof(req2));

    strcpy(req2.Username, username);

    ret = onvif_tds_DeleteUsers(&p_dev->onvif_device, &req2, NULL);
    
    printf("onvif_tds_DeleteUsers return ret = %d\n", ret);

    onvif_SetAuthInfo(&p_dev->onvif_device, oldname, oldpass);
}

void onvifGeoLocationTest(ONVIF_DEVICE_EX * p_dev)
{
    BOOL ret;

    // GetGeoLocation
    tds_GetGeoLocation_RES res;

    memset(&res, 0, sizeof(res));
    
    ret = onvif_tds_GetGeoLocation(&p_dev->onvif_device, NULL, &res);

    printf("onvif_tds_GetGeoLocation return ret = %d\n", ret);

    if (NULL == res.Location)
    {
        LocationEntityList * p_item = onvif_add_LocationEntity(&res.Location);

        p_item->Location.EntityFlag = 1;
        strcpy(p_item->Location.Entity, "VideoSource");
        p_item->Location.TokenFlag = 1;
        strcpy(p_item->Location.Token, "VideoSourceToken");
        p_item->Location.GeoLocationFlag = 1;
        p_item->Location.GeoLocation.lonFlag = 1;
        p_item->Location.GeoLocation.lon = 100.0;
        p_item->Location.GeoLocation.latFlag = 1;
        p_item->Location.GeoLocation.lat = 100.0;
        p_item->Location.GeoLocation.elevationFlag = 1;
        p_item->Location.GeoLocation.elevation = 100.0;
    }

    // SetGeoLocation
    tds_SetGeoLocation_REQ req1;

    req1.Location = res.Location;
    
    ret = onvif_tds_SetGeoLocation(&p_dev->onvif_device, &req1, NULL);

    printf("onvif_tds_SetGeoLocation return ret = %d\n", ret);

    // DeleteGeoLocation
    tds_DeleteGeoLocation_REQ req2;

    req2.Location = res.Location;
    
    ret = onvif_tds_DeleteGeoLocation(&p_dev->onvif_device, &req2, NULL);

    printf("onvif_tds_DeleteGeoLocation return ret = %d\n", ret);

    // SetGeoLocation
    tds_SetGeoLocation_REQ req3;

    req3.Location = res.Location;
    
    ret = onvif_tds_SetGeoLocation(&p_dev->onvif_device, &req3, NULL);

    printf("onvif_tds_SetGeoLocation return ret = %d\n", ret);

    onvif_free_LocationEntities(&res.Location);
}

void onvifImageTest(ONVIF_DEVICE_EX * p_dev)
{
    BOOL ret;

    while (1)
    {
        // GetVideoSources
        trt_GetVideoSources_RES res;
        memset(&res, 0, sizeof(res));        
        
        ret = onvif_trt_GetVideoSources(&p_dev->onvif_device, NULL, &res);

        printf("onvif_trt_GetVideoSources return ret = %d\n", ret);

        if (!ret || NULL == res.VideoSources)
        {
            break;
        }

        // GetMoveOptions
        img_GetMoveOptions_REQ req1;
        img_GetMoveOptions_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.VideoSourceToken, res.VideoSources->VideoSource.token);

        ret = onvif_img_GetMoveOptions(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_img_GetMoveOptions return ret = %d\n", ret);

        if (ret && res1.MoveOptions.ContinuousFlag)
        {
            // Move
            img_Move_REQ req2;
            memset(&req2, 0, sizeof(req2));

            strcpy(req2.VideoSourceToken, res.VideoSources->VideoSource.token);
            req2.Focus.ContinuousFlag = 1;
            req2.Focus.Continuous.Speed = res1.MoveOptions.Continuous.Speed.Min;

            ret = onvif_img_Move(&p_dev->onvif_device, &req2, NULL);

            printf("onvif_img_Move return ret = %d\n", ret);

            // Stop
            img_Stop_REQ req3;
            memset(&req3, 0, sizeof(req3));

            strcpy(req3.VideoSourceToken, res.VideoSources->VideoSource.token);

            ret = onvif_img_Stop(&p_dev->onvif_device, &req3, NULL);

            printf("onvif_img_Stop return ret = %d\n", ret);
        }

        if (p_dev->onvif_device.Capabilities.image.Presets)
        {
            // GetPresets
            img_GetPresets_REQ req4;
            img_GetPresets_RES res4;
            
            memset(&req4, 0, sizeof(req4));
            memset(&res4, 0, sizeof(res4));

            strcpy(req4.VideoSourceToken, res.VideoSources->VideoSource.token);

            ret = onvif_img_GetPresets(&p_dev->onvif_device, &req4, &res4);

            printf("onvif_img_GetPresets return ret = %d\n", ret);

            // GetCurrentPreset
            img_GetCurrentPreset_REQ req5;
            img_GetCurrentPreset_RES res5;
            
            memset(&req5, 0, sizeof(req5));
            memset(&res5, 0, sizeof(res5));

            strcpy(req5.VideoSourceToken, res.VideoSources->VideoSource.token);

            ret = onvif_img_GetCurrentPreset(&p_dev->onvif_device, &req5, &res5);

            printf("onvif_img_GetCurrentPreset return ret = %d\n", ret);

            if (res4.Preset && p_dev->onvif_device.Capabilities.image.AdaptablePreset)
            {
                // SetCurrentPreset
                img_SetCurrentPreset_REQ req6;
                memset(&req6, 0, sizeof(req6));

                strcpy(req6.VideoSourceToken, res.VideoSources->VideoSource.token);
                strcpy(req6.PresetToken, res4.Preset->Preset.token);

                ret = onvif_img_SetCurrentPreset(&p_dev->onvif_device, &req6, NULL);

                printf("onvif_img_SetCurrentPreset return ret = %d\n", ret);

                // SetCurrentPreset, resume
                strcpy(req6.PresetToken, res5.Preset.token);

                ret = onvif_img_SetCurrentPreset(&p_dev->onvif_device, &req6, NULL);

                printf("onvif_img_SetCurrentPreset return ret = %d\n", ret);
            }

            onvif_free_ImagingPresets(&res4.Preset);
        }

        onvif_free_VideoSources(&res.VideoSources);

        break;
    }
}

#ifdef DEVICEIO_SUPPORT

void onvifDeviceIOTest(ONVIF_DEVICE_EX * p_dev)
{
    BOOL ret;

    // GetRelayoutputs
    tmd_GetRelayOutputs_RES res;

    memset(&res, 0, sizeof(res));

    ret = onvif_tmd_GetRelayOutputs(&p_dev->onvif_device, NULL, &res);

    printf("onvif_tmd_GetRelayOutputs return ret = %d\n", ret);

    if (NULL == res.RelayOutputs)
    {
        return;
    }

    // SetRelayOutputSettings
    tmd_SetRelayOutputSettings_REQ req1;

    memset(&req1, 0, sizeof(req1));

    memcpy(&req1.RelayOutput, &res.RelayOutputs->RelayOutput, sizeof(onvif_RelayOutput));

    req1.RelayOutput.Properties.IdleState = RelayIdleState_open;
    req1.RelayOutput.Properties.DelayTime = 20;
    
    ret = onvif_tmd_SetRelayOutputSettings(&p_dev->onvif_device, &req1, NULL);

    printf("onvif_tmd_SetRelayOutputSettings return ret = %d\n", ret);

    // SetRelayOutputSettings
    tmd_SetRelayOutputSettings_REQ req2;

    memset(&req2, 0, sizeof(req2));

    memcpy(&req2.RelayOutput, &res.RelayOutputs->RelayOutput, sizeof(onvif_RelayOutput));

    ret = onvif_tmd_SetRelayOutputSettings(&p_dev->onvif_device, &req2, NULL);

    printf("onvif_tmd_SetRelayOutputSettings return ret = %d\n", ret);

    // SetRelayoutputState
    tmd_SetRelayOutputState_REQ req3;

    memset(&req3, 0, sizeof(req3));

    strcpy(req3.RelayOutputToken, res.RelayOutputs->RelayOutput.token);
    req3.LogicalState = RelayLogicalState_inactive;

    ret = onvif_tmd_SetRelayOutputState(&p_dev->onvif_device, &req3, NULL);

    printf("onvif_tmd_SetRelayOutputState return ret = %d\n", ret);

    // SetRelayoutputState
    tmd_SetRelayOutputState_REQ req4;

    memset(&req4, 0, sizeof(req4));

    strcpy(req4.RelayOutputToken, res.RelayOutputs->RelayOutput.token);
    req4.LogicalState = RelayLogicalState_active;

    ret = onvif_tmd_SetRelayOutputState(&p_dev->onvif_device, &req4, NULL);

    printf("onvif_tmd_SetRelayOutputState return ret = %d\n", ret);

    // GetRelayOutputOptions
    tmd_GetRelayOutputOptions_REQ req5;
    tmd_GetRelayOutputOptions_RES res5;

    memset(&req5, 0, sizeof(req5));
    memset(&res5, 0, sizeof(res5));

    req5.RelayOutputTokenFlag = 1;
    strcpy(req5.RelayOutputToken, res.RelayOutputs->RelayOutput.token);

    ret = onvif_tmd_GetRelayOutputOptions(&p_dev->onvif_device, &req5, &res5);

    printf("onvif_tmd_GetRelayOutputOptions return ret = %d\n", ret);

    onvif_free_RelayOutputOptions(&res5.RelayOutputOptions);

    onvif_free_RelayOutputs(&res.RelayOutputs);

    // GetDigitalInputs
    tmd_GetDigitalInputs_RES res6;

    memset(&res6, 0, sizeof(res6));
    
    ret = onvif_tmd_GetDigitalInputs(&p_dev->onvif_device, NULL, &res6);

    printf("onvif_tmd_GetDigitalInputs return ret = %d\n", ret);

    if (NULL == res6.DigitalInputs)
    {
        return;
    }

    // GetDigitalInputConfigurationOptions
    tmd_GetDigitalInputConfigurationOptions_REQ req7;
    tmd_GetDigitalInputConfigurationOptions_RES res7;

    memset(&req7, 0, sizeof(req7));
    memset(&res7, 0, sizeof(res7));

    req7.TokenFlag = 1;
    strcpy(req7.Token, res6.DigitalInputs->DigitalInput.token);
    
    ret = onvif_tmd_GetDigitalInputConfigurationOptions(&p_dev->onvif_device, &req7, &res7);

    printf("onvif_tmd_GetDigitalInputConfigurationOptions return ret = %d\n", ret);

    // SetDigitalInputConfigurations
    tmd_SetDigitalInputConfigurations_REQ req8;

    memset(&req8, 0, sizeof(req8));

    req8.DigitalInputs = res6.DigitalInputs;
    req8.DigitalInputs->DigitalInput.IdleStateFlag = 1;
    req8.DigitalInputs->DigitalInput.IdleState = DigitalIdleState_open;
    
    ret = onvif_tmd_SetDigitalInputConfigurations(&p_dev->onvif_device, &req8, NULL);

    printf("onvif_tmd_SetDigitalInputConfigurations return ret = %d\n", ret);

    // SetDigitalInputConfigurations
    tmd_SetDigitalInputConfigurations_REQ req9;

    memset(&req9, 0, sizeof(req9));

    req9.DigitalInputs = res6.DigitalInputs;
    
    ret = onvif_tmd_SetDigitalInputConfigurations(&p_dev->onvif_device, &req9, NULL);

    printf("onvif_tmd_SetDigitalInputConfigurations return ret = %d\n", ret);

    onvif_free_DigitalInputs(&res6.DigitalInputs);
}

#endif // end of DEVICEIO_SUPPORT

void onvifMedia2Test(ONVIF_DEVICE_EX * p_dev)
{
    BOOL ret;
    
    while (1)
    {
        // GetAudioEncoderConfigurationOptions
        tr2_GetAudioEncoderConfigurationOptions_REQ req;
        tr2_GetAudioEncoderConfigurationOptions_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetAudioEncoderConfigurationOptions(&p_dev->onvif_device, &req, &res);
        if (ret)
        {
            onvif_free_AudioEncoder2ConfigurationOptions(&res.Options);
        }
        
        printf("onvif_tr2_GetAudioEncoderConfigurationOptions return ret = %d\n", ret);

        break;
    }

    while (1)
    {
        // GetAudioEncoderConfigurations
        tr2_GetAudioEncoderConfigurations_REQ req;
        tr2_GetAudioEncoderConfigurations_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetAudioEncoderConfigurations(&p_dev->onvif_device, &req, &res);

        printf("onvif_tr2_GetAudioEncoderConfigurations return ret = %d\n", ret);
        
        if (!ret || NULL == res.Configurations)
        {
            break;
        }

        // SetAudioEncoderConfiguration
        tr2_SetAudioEncoderConfiguration_REQ req1;
        tr2_SetAudioEncoderConfiguration_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        memcpy(&req1.Configuration, &res.Configurations->Configuration, sizeof(onvif_AudioEncoder2Configuration));
    
        ret = onvif_tr2_SetAudioEncoderConfiguration(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tr2_SetAudioEncoderConfiguration return ret = %d\n", ret);

        onvif_free_AudioEncoder2Configurations(&res.Configurations);

        break;
    }
    
    while (1)
    {
        // GetAudioSourceConfigurationOptions
        tr2_GetAudioSourceConfigurationOptions_REQ req;
        tr2_GetAudioSourceConfigurationOptions_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetAudioSourceConfigurationOptions(&p_dev->onvif_device, &req, &res);

        printf("onvif_tr2_GetAudioSourceConfigurationOptions return ret = %d\n", ret);

        break;
    }

    while (1)
    {
        // GetAudioSourceConfigurations
        tr2_GetAudioSourceConfigurations_REQ req;
        tr2_GetAudioSourceConfigurations_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetAudioSourceConfigurations(&p_dev->onvif_device, &req, &res);

        printf("onvif_tr2_GetAudioSourceConfigurations return ret = %d\n", ret);

        if (!ret || NULL == res.Configurations)
        {
            break;
        }

        // SetAudioSourceConfiguration
        tr2_SetAudioSourceConfiguration_REQ req1;
        tr2_SetAudioSourceConfiguration_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        memcpy(&req1.Configuration, &res.Configurations->Configuration, sizeof(onvif_AudioSourceConfiguration));
    
        ret = onvif_tr2_SetAudioSourceConfiguration(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tr2_SetAudioSourceConfiguration return ret = %d\n", ret);

        onvif_free_AudioSourceConfigurations(&res.Configurations);
        
        break;
    }

    while (1)
    {
        // GetVideoEncoderConfigurationOptions
        tr2_GetVideoEncoderConfigurationOptions_REQ req;
        tr2_GetVideoEncoderConfigurationOptions_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetVideoEncoderConfigurationOptions(&p_dev->onvif_device, &req, &res);
        if (ret)
        {
            onvif_free_VideoEncoder2ConfigurationOptions(&res.Options);
        }

        printf("onvif_tr2_GetVideoEncoderConfigurationOptions return ret = %d\n", ret);

        break;
    }

    while (1)
    {
        // GetVideoEncoderConfigurations
        tr2_GetVideoEncoderConfigurations_REQ req;
        tr2_GetVideoEncoderConfigurations_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetVideoEncoderConfigurations(&p_dev->onvif_device, &req, &res);
        
        printf("onvif_tr2_GetVideoEncoderConfigurations return ret = %d\n", ret);

        if (!ret || NULL == res.Configurations)
        {
            break;
        }

        // SetVideoEncoderConfiguration
        tr2_SetVideoEncoderConfiguration_REQ req1;
        tr2_SetVideoEncoderConfiguration_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        memcpy(&req1.Configuration, &res.Configurations->Configuration, sizeof(onvif_VideoEncoder2Configuration));
    
        ret = onvif_tr2_SetVideoEncoderConfiguration(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tr2_SetVideoEncoderConfiguration return ret = %d\n", ret);

        onvif_free_VideoEncoder2Configurations(&res.Configurations);

        break;
    }

    while (1)
    {
        // GetVideoSourceConfigurationOptions
        tr2_GetVideoSourceConfigurationOptions_REQ req;
        tr2_GetVideoSourceConfigurationOptions_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetVideoSourceConfigurationOptions(&p_dev->onvif_device, &req, &res);

        printf("onvif_tr2_GetVideoSourceConfigurationOptions return ret = %d\n", ret);

        break;
    }

    while (1)
    {
        // GetVideoSourceConfigurations
        tr2_GetVideoSourceConfigurations_REQ req;
        tr2_GetVideoSourceConfigurations_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetVideoSourceConfigurations(&p_dev->onvif_device, &req, &res);

        printf("onvif_tr2_GetVideoSourceConfigurations return ret = %d\n", ret);

        if (!ret || NULL == res.Configurations)
        {
            break;
        }        

        // SetVideoSourceConfiguration
        tr2_SetVideoSourceConfiguration_REQ req1;
        tr2_SetVideoSourceConfiguration_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        memcpy(&req1.Configuration, &res.Configurations->Configuration, sizeof(onvif_VideoSourceConfiguration));
    
        ret = onvif_tr2_SetVideoSourceConfiguration(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tr2_SetVideoSourceConfiguration return ret = %d\n", ret);

        // GetVideoEncoderInstances
        tr2_GetVideoEncoderInstances_REQ req2;
        tr2_GetVideoEncoderInstances_RES res2;

        memset(&req2, 0, sizeof(req2));
        memset(&res2, 0, sizeof(res2));

        strcpy(req2.ConfigurationToken, res.Configurations->Configuration.token);
    
        ret = onvif_tr2_GetVideoEncoderInstances(&p_dev->onvif_device, &req2, &res2);

        printf("onvif_tr2_GetVideoEncoderInstances return ret = %d\n", ret);

        onvif_free_VideoSourceConfigurations(&res.Configurations);

        break;
    }

    {
        // GetMetadataConfigurationOptions
        tr2_GetMetadataConfigurationOptions_REQ req;
        tr2_GetMetadataConfigurationOptions_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetMetadataConfigurationOptions(&p_dev->onvif_device, &req, &res);

        printf("onvif_tr2_GetMetadataConfigurationOptions return ret = %d\n", ret);
    }

    {
        // GetMetadataConfigurations
        tr2_GetMetadataConfigurations_REQ req;
        tr2_GetMetadataConfigurations_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetMetadataConfigurations(&p_dev->onvif_device, &req, &res);
        if (ret)
        {
            onvif_free_MetadataConfigurations(&res.Configurations);
        }

        printf("onvif_tr2_GetMetadataConfigurations return ret = %d\n", ret);
    }

    while (1)
    {
        // GetProfiles
        tr2_GetProfiles_REQ req;
        tr2_GetProfiles_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetProfiles(&p_dev->onvif_device, &req, &res);

        printf("onvif_tr2_GetProfiles return ret = %d\n", ret);
        
        if (!ret || NULL == res.Profiles)
        {
            break;
        }

        // GetStreamUri
        tr2_GetStreamUri_REQ req1;
        tr2_GetStreamUri_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.Protocol, "RtspUnicast");
        strcpy(req1.ProfileToken, res.Profiles->MediaProfile.token);
    
        ret = onvif_tr2_GetStreamUri(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tr2_GetStreamUri return ret = %d\n", ret);

        if (ret)
        {
            printf("onvif_tr2_GetStreamUri, profile=%s, uri=%s\n", req1.ProfileToken, res1.Uri);
        }
        
        // SetSynchronizationPoint
        tr2_SetSynchronizationPoint_REQ req2;
        tr2_SetSynchronizationPoint_RES res2;

        memset(&req2, 0, sizeof(req2));
        memset(&res2, 0, sizeof(res2));

        strcpy(req2.ProfileToken, res.Profiles->MediaProfile.token);
    
        ret = onvif_tr2_SetSynchronizationPoint(&p_dev->onvif_device, &req2, &res2);

        printf("onvif_tr2_SetSynchronizationPoint return ret = %d\n", ret);

        // AddConfiguration
        tr2_AddConfiguration_REQ req3;
        tr2_AddConfiguration_RES res3;

        memset(&req3, 0, sizeof(req3));
        memset(&res3, 0, sizeof(res3));

        req3.sizeConfiguration = 1;
        strcpy(req3.Configuration[0].Type, "All");
        strcpy(req3.ProfileToken, res.Profiles->MediaProfile.token);
    
        ret = onvif_tr2_AddConfiguration(&p_dev->onvif_device, &req3, &res3);
        
        printf("onvif_tr2_AddConfiguration return ret = %d\n", ret);

        // free profiles
        onvif_free_MediaProfiles(&res.Profiles);
        
        break;
    }

    while (1)
    {
        // CreateProfile
        tr2_CreateProfile_REQ req;
        tr2_CreateProfile_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));

        strcpy(req.Name, "test");
    
        ret = onvif_tr2_CreateProfile(&p_dev->onvif_device, &req, &res);
        
        printf("onvif_tr2_CreateProfile return ret = %d\n", ret);
        
        if (ret)
        {
            printf("onvif_tr2_CreateProfile token = %s\n", res.Token);
        }
        else
        {
            break;
        }

        // DeleteProfile
        tr2_DeleteProfile_REQ req1;
        tr2_DeleteProfile_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.Token, res.Token);
    
        ret = onvif_tr2_DeleteProfile(&p_dev->onvif_device, &req1, &res1);
        
        printf("onvif_tr2_DeleteProfile return ret = %d\n", ret);

        
        break;        
    }

    while (1)
    {
        // GetAudioOutputConfigurations
        tr2_GetAudioOutputConfigurations_REQ req;
        tr2_GetAudioOutputConfigurations_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetAudioOutputConfigurations(&p_dev->onvif_device, &req, &res);

        printf("onvif_tr2_GetAudioOutputConfigurations return ret = %d\n", ret);

        if (!ret || NULL == res.Configurations)
        {
            break;
        }

        // SetAudioOutputConfiguration
        tr2_SetAudioOutputConfiguration_REQ req1;

        memset(&req1, 0, sizeof(req1));

        memcpy(&req1.Configuration, &res.Configurations->Configuration, sizeof(onvif_AudioOutputConfiguration));
    
        ret = onvif_tr2_SetAudioOutputConfiguration(&p_dev->onvif_device, &req1, NULL);

        printf("onvif_tr2_SetAudioOutputConfiguration return ret = %d\n", ret);

        onvif_free_AudioOutputConfigurations(&res.Configurations);
        
        break;
    }

    while (1)
    {
        // GetAudioOutputConfigurationOptions
        tr2_GetAudioOutputConfigurationOptions_REQ req;
        tr2_GetAudioOutputConfigurationOptions_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetAudioOutputConfigurationOptions(&p_dev->onvif_device, &req, &res);

        printf("onvif_tr2_GetAudioOutputConfigurationOptions return ret = %d\n", ret);
        
        break;
    }
    
    while (1)
    {
        // GetAudioDecoderConfigurations
        tr2_GetAudioDecoderConfigurations_REQ req;
        tr2_GetAudioDecoderConfigurations_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetAudioDecoderConfigurations(&p_dev->onvif_device, &req, &res);

        printf("onvif_tr2_GetAudioDecoderConfigurations return ret = %d\n", ret);

        if (!ret || NULL == res.Configurations)
        {
            break;
        }

        // SetAudioDecoderConfiguration
        tr2_SetAudioDecoderConfiguration_REQ req1;

        memset(&req1, 0, sizeof(req1));

        memcpy(&req1.Configuration, &res.Configurations->Configuration, sizeof(onvif_AudioDecoderConfiguration));
    
        ret = onvif_tr2_SetAudioDecoderConfiguration(&p_dev->onvif_device, &req1, NULL);

        printf("onvif_tr2_SetAudioDecoderConfiguration return ret = %d\n", ret);

        onvif_free_AudioDecoderConfigurations(&res.Configurations);
        
        break;
    }

    while (1)
    {
        // GetAudioDecoderConfigurationOptions
        tr2_GetAudioDecoderConfigurationOptions_REQ req;
        tr2_GetAudioDecoderConfigurationOptions_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetAudioDecoderConfigurationOptions(&p_dev->onvif_device, &req, &res);

        printf("onvif_tr2_GetAudioDecoderConfigurationOptions return ret = %d\n", ret);

        onvif_free_AudioEncoder2ConfigurationOptions(&res.Options);
        
        break;
    }

    while (1)
    {
        // GetProfiles
        tr2_GetProfiles_REQ req;
        tr2_GetProfiles_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetProfiles(&p_dev->onvif_device, &req, &res);

        printf("onvif_tr2_GetProfiles return ret = %d\n", ret);
        
        if (!ret || NULL == res.Profiles)
        {
            break;
        }

        // GetSnapshotUri
        tr2_GetSnapshotUri_REQ req1;
        tr2_GetSnapshotUri_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.ProfileToken, res.Profiles->MediaProfile.token);
        
        ret = onvif_tr2_GetSnapshotUri(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tr2_GetSnapshotUri return ret = %d\n", ret);

        // StartMulticastStreaming
        tr2_StartMulticastStreaming_REQ req2;
        tr2_StartMulticastStreaming_RES res2;

        memset(&req2, 0, sizeof(req2));
        memset(&res2, 0, sizeof(res2));

        strcpy(req2.ProfileToken, res.Profiles->MediaProfile.token);
        
        ret = onvif_tr2_StartMulticastStreaming(&p_dev->onvif_device, &req2, &res2);

        printf("onvif_tr2_StartMulticastStreaming return ret = %d\n", ret);


        // StopMulticastStreaming
        tr2_StopMulticastStreaming_REQ req3;
        tr2_StopMulticastStreaming_RES res3;

        memset(&req3, 0, sizeof(req3));
        memset(&res3, 0, sizeof(res3));

        strcpy(req3.ProfileToken, res.Profiles->MediaProfile.token);
        
        ret = onvif_tr2_StopMulticastStreaming(&p_dev->onvif_device, &req3, &res3);

        printf("onvif_tr2_StopMulticastStreaming return ret = %d\n", ret);

        
        onvif_free_MediaProfiles(&res.Profiles);
        
        break;
    }

    while (1)
    {
        // GetVideoSourceConfigurations
        tr2_GetVideoSourceConfigurations_REQ req;
        tr2_GetVideoSourceConfigurations_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetVideoSourceConfigurations(&p_dev->onvif_device, &req, &res);

        printf("onvif_tr2_GetVideoSourceConfigurations return ret = %d\n", ret);
        
        if (!ret || NULL == res.Configurations)
        {
            break;
        }
        
        // GetVideoSourceModes
        tr2_GetVideoSourceModes_REQ req1;
        tr2_GetVideoSourceModes_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.VideoSourceToken, res.Configurations->Configuration.SourceToken);
    
        ret = onvif_tr2_GetVideoSourceModes(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tr2_GetVideoSourceModes return ret = %d\n", ret);
        
        if (!ret || NULL == res1.VideoSourceModes)
        {
            onvif_free_VideoSourceConfigurations(&res.Configurations);
            break;
        }

        // SetVideoSourceMode
        tr2_SetVideoSourceMode_REQ req2;
        tr2_SetVideoSourceMode_RES res2;

        memset(&req2, 0, sizeof(req2));
        memset(&res2, 0, sizeof(res2));

        strcpy(req2.VideoSourceToken, res.Configurations->Configuration.SourceToken);
        strcpy(req2.VideoSourceModeToken, res1.VideoSourceModes->VideoSourceMode.token);
        
        ret = onvif_tr2_SetVideoSourceMode(&p_dev->onvif_device, &req2, &res2);

        printf("onvif_tr2_SetVideoSourceMode return ret = %d\n", ret);

                
        onvif_free_VideoSourceModes(&res1.VideoSourceModes);
        onvif_free_VideoSourceConfigurations(&res.Configurations);
        
        break;
    }

}

void onvifMedia2OSDTest(ONVIF_DEVICE_EX * p_dev)
{
    BOOL ret;
    
    while (1)
    {
        // GetVideoSourceConfigurations
        tr2_GetVideoSourceConfigurations_REQ req;
        tr2_GetVideoSourceConfigurations_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetVideoSourceConfigurations(&p_dev->onvif_device, &req, &res);

        printf("onvif_tr2_GetVideoSourceConfigurations return ret = %d\n", ret);
        
        if (!ret || NULL == res.Configurations)
        {
            break;
        }
        
        // GetOSDOptions
        tr2_GetOSDOptions_REQ req1;
        tr2_GetOSDOptions_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.ConfigurationToken, res.Configurations->Configuration.token);
    
        ret = onvif_tr2_GetOSDOptions(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tr2_GetOSDOptions return ret = %d\n", ret);

        onvif_free_VideoSourceConfigurations(&res.Configurations);

        break;
    }
    
    while (1)
    {
        // GetVideoSourceConfigurations
        tr2_GetVideoSourceConfigurations_REQ req;
        tr2_GetVideoSourceConfigurations_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
    
        ret = onvif_tr2_GetVideoSourceConfigurations(&p_dev->onvif_device, &req, &res);

        printf("onvif_tr2_GetVideoSourceConfigurations return ret = %d\n", ret);
        
        if (!ret || NULL == res.Configurations)
        {
            break;
        }
        
        // CreateOSD
        tr2_CreateOSD_REQ req1;
        tr2_CreateOSD_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        req1.OSD.Type = OSDType_Text;
        strcpy(req1.OSD.VideoSourceConfigurationToken, res.Configurations->Configuration.token);
        req1.OSD.Position.Type = OSDPosType_UpperRight;
        req1.OSD.TextStringFlag = 1;        
        req1.OSD.TextString.Type = OSDTextType_Plain;
        req1.OSD.TextString.PlainTextFlag = 1;
        strcpy(req1.OSD.TextString.PlainText, "test");
    
        ret = onvif_tr2_CreateOSD(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tr2_CreateOSD return ret = %d\n", ret);
        
        // GetOSDs
        tr2_GetOSDs_REQ req2;
        tr2_GetOSDs_RES res2;

        memset(&req2, 0, sizeof(req2));
        memset(&res2, 0, sizeof(res2));
    
        ret = onvif_tr2_GetOSDs(&p_dev->onvif_device, &req2, &res2);

        printf("onvif_tr2_GetOSDs return ret = %d\n", ret);
        
        if (!ret || NULL == res2.OSDs)
        {
            onvif_free_VideoSourceConfigurations(&res.Configurations);
            break;
        }
        
        // SetOSD
        tr2_SetOSD_REQ req3;
        tr2_SetOSD_RES res3;

        memset(&req3, 0, sizeof(req3));
        memset(&res3, 0, sizeof(res3));

        memcpy(&req3.OSD, &res2.OSDs->OSD, sizeof(onvif_OSDConfiguration));
    
        ret = onvif_tr2_SetOSD(&p_dev->onvif_device, &req3, &res3);

        printf("onvif_tr2_SetOSD return ret = %d\n", ret);

        // DeleteOSD
        tr2_DeleteOSD_REQ req4;

        memset(&req4, 0, sizeof(req4));

        strcpy(req4.OSDToken, res1.OSDToken);
    
        ret = onvif_tr2_DeleteOSD(&p_dev->onvif_device, &req4, NULL);

        printf("onvif_tr2_DeleteOSD return ret = %d\n", ret);
        
        
        onvif_free_OSDConfigurations(&res2.OSDs);
        onvif_free_VideoSourceConfigurations(&res.Configurations);
        
        break;
    }
}

void onvifMedia2MaskTest(ONVIF_DEVICE_EX * p_dev)
{
    BOOL ret;
    
    // GetVideoSourceConfigurations
    tr2_GetVideoSourceConfigurations_REQ req;
    tr2_GetVideoSourceConfigurations_RES res;

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    ret = onvif_tr2_GetVideoSourceConfigurations(&p_dev->onvif_device, &req, &res);

    printf("onvif_tr2_GetVideoSourceConfigurations return ret = %d\n", ret);
    
    if (!ret || NULL == res.Configurations)
    {
        return;
    }

    // CreateMask
    tr2_CreateMask_REQ req1;
    tr2_CreateMask_RES res1;
    
    memset(&req1, 0, sizeof(req1));
    memset(&res1, 0, sizeof(res1));

    strcpy(req1.Mask.ConfigurationToken, res.Configurations->Configuration.token);
    req1.Mask.Polygon.sizePoint = 5;
    req1.Mask.Polygon.Point[0].x = 0;
    req1.Mask.Polygon.Point[0].y = 0;
    req1.Mask.Polygon.Point[1].x = 400;
    req1.Mask.Polygon.Point[1].y = 0;
    req1.Mask.Polygon.Point[2].x = 400;
    req1.Mask.Polygon.Point[2].y = 400;
    req1.Mask.Polygon.Point[3].x = 0;
    req1.Mask.Polygon.Point[3].y = 400;
    req1.Mask.Polygon.Point[4].x = 0;
    req1.Mask.Polygon.Point[4].y = 0;

    strcpy(req1.Mask.Type, "Pixelated");
    req1.Mask.Enabled = TRUE;

    ret = onvif_tr2_CreateMask(&p_dev->onvif_device, &req1, &res1);

    printf("onvif_tr2_CreateMask return ret = %d\n", ret);

    // GetMasks
    tr2_GetMasks_REQ req2;
    tr2_GetMasks_RES res2;

    memset(&req2, 0, sizeof(req2));
    memset(&res2, 0, sizeof(res2));

    ret = onvif_tr2_GetMasks(&p_dev->onvif_device, &req2, &res2);

    printf("onvif_tr2_GetMasks return ret = %d\n", ret);

    // GetMaskOptions
    tr2_GetMaskOptions_REQ req3;
    tr2_GetMaskOptions_RES res3;

    memset(&req3, 0, sizeof(req3));
    memset(&res3, 0, sizeof(res3));

    strcpy(req3.ConfigurationToken, res.Configurations->Configuration.token);

    ret = onvif_tr2_GetMaskOptions(&p_dev->onvif_device, &req3, &res3);

    printf("onvif_tr2_GetMaskOptions return ret = %d\n", ret);

    // SetMask
    tr2_SetMask_REQ req4;
    tr2_SetMask_RES res4;

    memset(&req4, 0, sizeof(req4));
    memset(&res4, 0, sizeof(res4));

    memcpy(&req4.Mask, &req1.Mask, sizeof(onvif_Mask));
    req4.Mask.Enabled = FALSE;
    strcpy(req4.Mask.token, res1.Token);
    
    ret = onvif_tr2_SetMask(&p_dev->onvif_device, &req4, &res4);
    
    printf("onvif_tr2_SetMask return ret = %d\n", ret);

    // DeleteMask
    tr2_DeleteMask_REQ req5;
    tr2_DeleteMask_RES res5;

    memset(&req5, 0, sizeof(req5));
    memset(&res5, 0, sizeof(res5));

    strcpy(req5.Token, res1.Token);

    ret = onvif_tr2_DeleteMask(&p_dev->onvif_device, &req5, &res5);
    
    printf("onvif_tr2_DeleteMask return ret = %d\n", ret);


    // free resources
    onvif_free_Masks(&res2.Masks);
    onvif_free_VideoSourceConfigurations(&res.Configurations);
}

void onvifAnalyticsTest(ONVIF_DEVICE_EX * p_dev)
{
    BOOL ret;

    // GetVideoAnalyticsConfigurations

    trt_GetVideoAnalyticsConfigurations_RES res;

    memset(&res, 0, sizeof(res));
    
    ret = onvif_trt_GetVideoAnalyticsConfigurations(&p_dev->onvif_device, NULL, &res);

    printf("onvif_trt_GetVideoAnalyticsConfigurations return ret = %d\n", ret);

    if (NULL == res.Configurations)
    {
        return;
    }
        
    while (1)
    {
        // GetSupportedAnalyticsModules

        tan_GetSupportedAnalyticsModules_REQ req1;
        tan_GetSupportedAnalyticsModules_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.ConfigurationToken, res.Configurations->Configuration.token);

        ret = onvif_tan_GetSupportedAnalyticsModules(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tan_GetSupportedAnalyticsModules return ret = %d\n", ret);
        
        // GetAnalyticsModules

        tan_GetAnalyticsModules_REQ req2;
        tan_GetAnalyticsModules_RES res2;

        memset(&req2, 0, sizeof(req2));
        memset(&res2, 0, sizeof(res2));

        strcpy(req2.ConfigurationToken, res.Configurations->Configuration.token);

        ret = onvif_tan_GetAnalyticsModules(&p_dev->onvif_device, &req2, &res2);

        printf("onvif_tan_GetAnalyticsModules return ret = %d\n", ret);

        onvif_free_Configs(&res2.AnalyticsModule);

        // GetAnalyticsModuleOptions

        tan_GetAnalyticsModuleOptions_REQ req3;
        tan_GetAnalyticsModuleOptions_RES res3;
        
        memset(&req3, 0, sizeof(req3));
        memset(&res3, 0, sizeof(res3));

        strcpy(req3.ConfigurationToken, res.Configurations->Configuration.token);
        
        ret = onvif_tan_GetAnalyticsModuleOptions(&p_dev->onvif_device, &req3, &res3);

        printf("onvif_tan_GetAnalyticsModuleOptions return ret = %d\n", ret);

        onvif_free_AnalyticsModuleConfigOptions(&res3.Options);

        onvif_free_ConfigDescriptions(&res1.SupportedAnalyticsModules.AnalyticsModuleDescription);
        
        break;
    }

    while (1)
    {
        // GetSupportedRules

        tan_GetSupportedRules_REQ req1;
        tan_GetSupportedRules_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.ConfigurationToken, res.Configurations->Configuration.token);
        
        ret = onvif_tan_GetSupportedRules(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tan_GetSupportedRules return ret = %d\n", ret);
            
        // GetRules
        
        tan_GetRules_REQ req2;
        tan_GetRules_RES res2;

        memset(&req2, 0, sizeof(req2));
        memset(&res2, 0, sizeof(res2));

        strcpy(req2.ConfigurationToken, res.Configurations->Configuration.token);

        ret = onvif_tan_GetRules(&p_dev->onvif_device, &req2, &res2);

        printf("onvif_tan_GetRules return ret = %d\n", ret);

        onvif_free_Configs(&res2.Rule);

        // GetRuleOptions

        tan_GetRuleOptions_REQ req3;
        tan_GetRuleOptions_RES res3;
        
        memset(&req3, 0, sizeof(req3));
        memset(&res3, 0, sizeof(res3));

        strcpy(req3.ConfigurationToken, res.Configurations->Configuration.token);
        
        ret = onvif_tan_GetRuleOptions(&p_dev->onvif_device, &req3, &res3);

        printf("onvif_tan_GetRuleOptions return ret = %d\n", ret);

        onvif_free_ConfigOptions(&res3.RuleOptions);

        onvif_free_ConfigDescriptions(&res1.SupportedRules.RuleDescription);
        
        break;
    }

    onvif_free_VideoAnalyticsConfigurations(&res.Configurations);

    // GetSupportedMetadata

    tan_GetSupportedMetadata_REQ req4;
    tan_GetSupportedMetadata_RES res4;

    memset(&req4, 0, sizeof(req4));
    memset(&res4, 0, sizeof(res4));
    
    ret = onvif_tan_GetSupportedMetadata(&p_dev->onvif_device, &req4, &res4);

    printf("onvif_tan_GetSupportedMetadata return ret = %d\n", ret);

    onvif_free_MetadataInfos(&res4.AnalyticsModule);
}

#ifdef PROFILE_C_SUPPORT

void onvifAccessControlTest(ONVIF_DEVICE_EX * p_dev)
{
    BOOL ret;

    while (1)
    {
        // GetAccessPointInfoList
        tac_GetAccessPointInfoList_RES res;

        memset(&res, 0, sizeof(res));
        
        ret = onvif_tac_GetAccessPointInfoList(&p_dev->onvif_device, NULL, &res);

        printf("onvif_tac_GetAccessPointInfoList return ret = %d\n", ret);

        if (!ret || !res.AccessPointInfo)
        {
            break;
        }

        // GetAccessPointInfo
        tac_GetAccessPointInfo_REQ req1;
        tac_GetAccessPointInfo_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.token[0], res.AccessPointInfo->AccessPointInfo.token);
        
        ret = onvif_tac_GetAccessPointInfo(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tac_GetAccessPointInfo return ret = %d\n", ret);

        if (ret)
        {
            onvif_free_AccessPoints(&res1.AccessPointInfo);
        }

        onvif_free_AccessPoints(&res.AccessPointInfo);

        break;
    }

    while (1)
    {
        // GetAreaInfoList
        tac_GetAreaInfoList_RES res;

        memset(&res, 0, sizeof(res));
        
        ret = onvif_tac_GetAreaInfoList(&p_dev->onvif_device, NULL, &res);

        printf("onvif_tac_GetAreaInfoList return ret = %d\n", ret);

        if (!ret || !res.AreaInfo)
        {
            break;
        }

        // GetAreaInfo
        tac_GetAreaInfo_REQ req1;
        tac_GetAreaInfo_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.token[0], res.AreaInfo->AreaInfo.token);
        
        ret = onvif_tac_GetAreaInfo(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tac_GetAreaInfo return ret = %d\n", ret);

        if (ret)
        {
            onvif_free_Areas(&res1.AreaInfo);
        }

        onvif_free_Areas(&res.AreaInfo);

        break;
    }

    while (1)
    {
        // GetAccessPointInfoList
        tac_GetAccessPointInfoList_RES res;

        memset(&res, 0, sizeof(res));
        
        ret = onvif_tac_GetAccessPointInfoList(&p_dev->onvif_device, NULL, &res);

        printf("onvif_tac_GetAccessPointInfoList return ret = %d\n", ret);

        if (!ret || !res.AccessPointInfo)
        {
            break;
        }

        // GetAccessPointState
        tac_GetAccessPointState_REQ req1;
        tac_GetAccessPointState_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.Token, res.AccessPointInfo->AccessPointInfo.token);
        
        ret = onvif_tac_GetAccessPointState(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tac_GetAccessPointState return ret = %d\n", ret);

        // EnableAccessPoint
        tac_EnableAccessPoint_REQ req2;

        memset(&req2, 0, sizeof(req2));

        strcpy(req2.Token, res.AccessPointInfo->AccessPointInfo.token);
        
        ret = onvif_tac_EnableAccessPoint(&p_dev->onvif_device, &req2, NULL);

        printf("onvif_tac_EnableAccessPoint return ret = %d\n", ret);

        // GetAccessPointState
        memset(&res1, 0, sizeof(res1));
        
        ret = onvif_tac_GetAccessPointState(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tac_GetAccessPointState return ret = %d\n", ret);

        // DisableAccessPoint
        tac_DisableAccessPoint_REQ req3;

        memset(&req3, 0, sizeof(req3));

        strcpy(req3.Token, res.AccessPointInfo->AccessPointInfo.token);
        
        ret = onvif_tac_DisableAccessPoint(&p_dev->onvif_device, &req3, NULL);

        printf("onvif_tac_DisableAccessPoint return ret = %d\n", ret);

        // GetAccessPointState
        memset(&res1, 0, sizeof(res1));
        
        ret = onvif_tac_GetAccessPointState(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tac_GetAccessPointState return ret = %d\n", ret);
        
        onvif_free_AccessPoints(&res.AccessPointInfo);

        break;
    }
}

void onvifDoorControlTest(ONVIF_DEVICE_EX * p_dev)
{    
    BOOL ret;

    while (1)
    {
        // GetDoorList
        tdc_GetDoorList_RES res;

        memset(&res, 0, sizeof(res));
        
        ret = onvif_tdc_GetDoorList(&p_dev->onvif_device, NULL, &res);

        printf("onvif_tdc_GetDoorList return ret = %d\n", ret);

        if (!ret || !res.Door)
        {
            break;
        }

        // GetDoors
        tdc_GetDoors_REQ req1;
        tdc_GetDoors_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.token[0], res.Door->Door.DoorInfo.token);
        
        ret = onvif_tdc_GetDoors(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tdc_GetDoors return ret = %d\n", ret);

        if (ret)
        {
            onvif_free_Doors(&res1.Door);
        }

        onvif_free_Doors(&res.Door);

        break;
    }

    while (p_dev->onvif_device.Capabilities.doorcontrol.DoorManagementSupported)
    {
        // CreateDoor
        tdc_CreateDoor_REQ req;
        tdc_CreateDoor_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));

        strcpy(req.Door.DoorInfo.Name, "testname");
        req.Door.DoorInfo.DescriptionFlag = 1;
        strcpy(req.Door.DoorInfo.Description, "testdesc");
        req.Door.DoorInfo.Capabilities.Access = 1;
        req.Door.DoorInfo.Capabilities.Lock = 1;
        req.Door.DoorInfo.Capabilities.Unlock = 1;
        req.Door.DoorInfo.Capabilities.Block = 1;
        req.Door.DoorInfo.Capabilities.DoubleLock = 1;
        
        ret = onvif_tdc_CreateDoor(&p_dev->onvif_device, &req, &res);

        printf("onvif_tdc_CreateDoor return ret = %d\n", ret);

        // SetDoor
        tdc_SetDoor_REQ req2;
        memset(&req2, 0, sizeof(req2));

        memcpy(&req2.Door, &req.Door, sizeof(onvif_Door));
        strcpy(req2.Door.DoorInfo.token, res.Token);
        strcpy(req2.Door.DoorInfo.Name, "testname1");

        ret = onvif_tdc_SetDoor(&p_dev->onvif_device, &req2, NULL);

        printf("onvif_tdc_SetDoor return ret = %d\n", ret);

        // ModifyDoor
        tdc_ModifyDoor_REQ req3;
        memset(&req3, 0, sizeof(req3));

        memcpy(&req3.Door, &req.Door, sizeof(onvif_Door));
        strcpy(req3.Door.DoorInfo.token, res.Token);
        strcpy(req3.Door.DoorInfo.Name, "testname2");

        ret = onvif_tdc_ModifyDoor(&p_dev->onvif_device, &req3, NULL);

        printf("onvif_tdc_ModifyDoor return ret = %d\n", ret);

        // DeleteDoor
        tdc_DeleteDoor_REQ req4;
        memset(&req4, 0, sizeof(req4));

        strcpy(req4.Token, res.Token);

        ret = onvif_tdc_DeleteDoor(&p_dev->onvif_device, &req4, NULL);

        printf("onvif_tdc_DeleteDoor return ret = %d\n", ret);
        
        break;
    }
    
    while (1)
    {
        // GetDoorInfoList
        tdc_GetDoorInfoList_RES res;

        memset(&res, 0, sizeof(res));
        
        ret = onvif_tdc_GetDoorInfoList(&p_dev->onvif_device, NULL, &res);

        printf("onvif_tdc_GetDoorInfoList return ret = %d\n", ret);

        if (!ret || !res.DoorInfo)
        {
            break;
        }

        // GetDoorInfo
        tdc_GetDoorInfo_REQ req1;
        tdc_GetDoorInfo_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.token[0], res.DoorInfo->DoorInfo.token);
        
        ret = onvif_tdc_GetDoorInfo(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tdc_GetDoorInfo return ret = %d\n", ret);

        if (ret)
        {
            onvif_free_DoorInfos(&res1.DoorInfo);
        }

        onvif_free_DoorInfos(&res.DoorInfo);

        break;
    }

    while (1)
    {
        // GetDoorInfoList
        tdc_GetDoorInfoList_RES res;

        memset(&res, 0, sizeof(res));
        
        ret = onvif_tdc_GetDoorInfoList(&p_dev->onvif_device, NULL, &res);

        printf("onvif_tdc_GetDoorInfoList return ret = %d\n", ret);

        if (!ret || !res.DoorInfo)
        {
            break;
        }

        // GetDoorState
        tdc_GetDoorState_REQ req1;
        tdc_GetDoorState_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.Token, res.DoorInfo->DoorInfo.token);
        
        ret = onvif_tdc_GetDoorState(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tdc_GetDoorState return ret = %d\n", ret);

        onvif_free_DoorInfos(&res.DoorInfo);

        break;
    }

    while (1)
    {
        // GetDoorInfoList
        tdc_GetDoorInfoList_RES res;

        memset(&res, 0, sizeof(res));
        
        ret = onvif_tdc_GetDoorInfoList(&p_dev->onvif_device, NULL, &res);

        printf("onvif_tdc_GetDoorInfoList return ret = %d\n", ret);

        if (!ret || !res.DoorInfo)
        {
            break;
        }

        if (res.DoorInfo->DoorInfo.Capabilities.Access)
        {
            // AccessDoor
            tdc_AccessDoor_REQ req;

            memset(&req, 0, sizeof(req));
            
            strcpy(req.Token, res.DoorInfo->DoorInfo.token);
            
            ret = onvif_tdc_AccessDoor(&p_dev->onvif_device, &req, NULL);

            printf("onvif_tdc_AccessDoor return ret = %d\n", ret);
        }

        if (res.DoorInfo->DoorInfo.Capabilities.Lock)
        {
            // LockDoor
            tdc_LockDoor_REQ req;

            memset(&req, 0, sizeof(req));
            
            strcpy(req.Token, res.DoorInfo->DoorInfo.token);
            
            ret = onvif_tdc_LockDoor(&p_dev->onvif_device, &req, NULL);

            printf("onvif_tdc_LockDoor return ret = %d\n", ret);
        }

        if (res.DoorInfo->DoorInfo.Capabilities.Unlock)
        {
            // UnlockDoor
            tdc_UnlockDoor_REQ req;

            memset(&req, 0, sizeof(req));
            
            strcpy(req.Token, res.DoorInfo->DoorInfo.token);
            
            ret = onvif_tdc_UnlockDoor(&p_dev->onvif_device, &req, NULL);

            printf("onvif_tdc_UnlockDoor return ret = %d\n", ret);
        }

        if (res.DoorInfo->DoorInfo.Capabilities.DoubleLock)
        {
            // DoubleLockDoor
            tdc_DoubleLockDoor_REQ req;

            memset(&req, 0, sizeof(req));
            
            strcpy(req.Token, res.DoorInfo->DoorInfo.token);
            
            ret = onvif_tdc_DoubleLockDoor(&p_dev->onvif_device, &req, NULL);

            printf("onvif_tdc_DoubleLockDoor return ret = %d\n", ret);
        }

        if (res.DoorInfo->DoorInfo.Capabilities.Block)
        {
            // BlockDoor
            tdc_BlockDoor_REQ req;

            memset(&req, 0, sizeof(req));
            
            strcpy(req.Token, res.DoorInfo->DoorInfo.token);
            
            ret = onvif_tdc_BlockDoor(&p_dev->onvif_device, &req, NULL);

            printf("onvif_tdc_BlockDoor return ret = %d\n", ret);
        }

        if (res.DoorInfo->DoorInfo.Capabilities.LockDown)
        {
            // LockDownDoor
            tdc_LockDownDoor_REQ req;

            memset(&req, 0, sizeof(req));
            
            strcpy(req.Token, res.DoorInfo->DoorInfo.token);
            
            ret = onvif_tdc_LockDownDoor(&p_dev->onvif_device, &req, NULL);

            printf("onvif_tdc_LockDownDoor return ret = %d\n", ret);
        }

        if (res.DoorInfo->DoorInfo.Capabilities.LockDown)
        {
            // LockDownReleaseDoor
            tdc_LockDownReleaseDoor_REQ req;

            memset(&req, 0, sizeof(req));
            
            strcpy(req.Token, res.DoorInfo->DoorInfo.token);
            
            ret = onvif_tdc_LockDownReleaseDoor(&p_dev->onvif_device, &req, NULL);

            printf("onvif_tdc_LockDownReleaseDoor return ret = %d\n", ret);
        }

        if (res.DoorInfo->DoorInfo.Capabilities.LockOpen)
        {
            // LockOpenDoor
            tdc_LockOpenDoor_REQ req;

            memset(&req, 0, sizeof(req));
            
            strcpy(req.Token, res.DoorInfo->DoorInfo.token);
            
            ret = onvif_tdc_LockOpenDoor(&p_dev->onvif_device, &req, NULL);

            printf("onvif_tdc_LockOpenDoor return ret = %d\n", ret);
        }

        if (res.DoorInfo->DoorInfo.Capabilities.LockOpen)
        {
            // LockOpenReleaseDoor
            tdc_LockOpenReleaseDoor_REQ req;

            memset(&req, 0, sizeof(req));
            
            strcpy(req.Token, res.DoorInfo->DoorInfo.token);
            
            ret = onvif_tdc_LockOpenReleaseDoor(&p_dev->onvif_device, &req, NULL);

            printf("onvif_tdc_LockOpenReleaseDoor return ret = %d\n", ret);
        }

        onvif_free_DoorInfos(&res.DoorInfo);

        break;
    }   
}

#endif // end of PROFILE_C_SUPPORT

#ifdef PROFILE_G_SUPPORT

void onvifRecordingTest(ONVIF_DEVICE_EX * p_dev)
{
    BOOL ret;

    while (p_dev->onvif_device.Capabilities.recording.DynamicRecordings)
    {
        // CreateRecording
        trc_CreateRecording_REQ req;
        trc_CreateRecording_RES res;

        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));

        strcpy(req.RecordingConfiguration.Source.SourceId, "test");
        strcpy(req.RecordingConfiguration.Source.Name, "test");
        strcpy(req.RecordingConfiguration.Source.Location, "test");
        strcpy(req.RecordingConfiguration.Source.Description, "test");
        strcpy(req.RecordingConfiguration.Source.Address, "test");
        strcpy(req.RecordingConfiguration.Content, "test");
        
        ret = onvif_trc_CreateRecording(&p_dev->onvif_device, &req, &res);

        printf("onvif_trc_CreateRecording return ret = %d\n", ret);

        if (!ret)
        {
            break;
        }

        // GetRecordings
        trc_GetRecordings_RES res1;
        memset(&res1, 0, sizeof(res1));
        
        ret = onvif_trc_GetRecordings(&p_dev->onvif_device, NULL, &res1);

        printf("onvif_trc_GetRecordings return ret = %d\n", ret);

        if (!ret || NULL == res1.Recordings)
        {
            break;
        }

        // SetRecordingConfiguration
        trc_SetRecordingConfiguration_REQ req2;
        memset(&req2, 0, sizeof(req2));

        strcpy(req2.RecordingToken, res.RecordingToken);
        memcpy(&req2.RecordingConfiguration, &res1.Recordings->Recording.Configuration, sizeof(onvif_RecordingConfiguration));
        
        ret = onvif_trc_SetRecordingConfiguration(&p_dev->onvif_device, &req2, NULL);

        printf("onvif_trc_SetRecordingConfiguration return ret = %d\n", ret);

        // GetRecordingConfiguration
        trc_GetRecordingConfiguration_REQ req3;
        trc_GetRecordingConfiguration_RES res3;

        memset(&req3, 0, sizeof(req3));
        memset(&res3, 0, sizeof(res3));

        strcpy(req3.RecordingToken, res.RecordingToken);
        
        ret = onvif_trc_GetRecordingConfiguration(&p_dev->onvif_device, &req3, &res3);

        printf("onvif_trc_GetRecordingConfiguration return ret = %d\n", ret);

        // GetRecordingOptions
        trc_GetRecordingOptions_REQ req4;
        trc_GetRecordingOptions_RES res4;
        
        memset(&req4, 0, sizeof(req4));
        memset(&res4, 0, sizeof(res4));

        strcpy(req4.RecordingToken, res.RecordingToken);
        
        ret = onvif_trc_GetRecordingOptions(&p_dev->onvif_device, &req4, &res4);

        printf("onvif_trc_GetRecordingOptions return ret = %d\n", ret);

        // CreateTrack
        trc_CreateTrack_REQ req5;
        trc_CreateTrack_RES res5;

        memset(&req5, 0, sizeof(req5));
        memset(&res5, 0, sizeof(res5));

        strcpy(req5.RecordingToken, res.RecordingToken);
        req5.TrackConfiguration.TrackType = TrackType_Video;
        strcpy(req5.TrackConfiguration.Description, "trackdesc");
        
        ret = onvif_trc_CreateTrack(&p_dev->onvif_device, &req5, &res5);

        printf("onvif_trc_CreateTrack return ret = %d\n", ret);

        if (ret)
        {
            // GetTrackConfiguration
            trc_GetTrackConfiguration_REQ req6;
            trc_GetTrackConfiguration_RES res6;

            memset(&req6, 0, sizeof(req6));
            memset(&res6, 0, sizeof(res6));

            strcpy(req6.RecordingToken, res.RecordingToken);
            strcpy(req6.TrackToken, res5.TrackToken);
            
            ret = onvif_trc_GetTrackConfiguration(&p_dev->onvif_device, &req6, &res6);

            printf("onvif_trc_GetTrackConfiguration return ret = %d\n", ret);

            // SetTrackConfiguration
            trc_SetTrackConfiguration_REQ req7;

            memset(&req7, 0, sizeof(req7));

            strcpy(req7.RecordingToken, res.RecordingToken);
            strcpy(req7.TrackToken, res5.TrackToken);
            memcpy(&req7.TrackConfiguration, &res6.TrackConfiguration, sizeof(onvif_TrackConfiguration));
            
            ret = onvif_trc_SetTrackConfiguration(&p_dev->onvif_device, &req7, NULL);

            printf("onvif_trc_SetTrackConfiguration return ret = %d\n", ret);

            // DeleteTrack
            trc_DeleteTrack_REQ req8;

            memset(&req8, 0, sizeof(req8));

            strcpy(req8.RecordingToken, res.RecordingToken);
            strcpy(req8.TrackToken, res5.TrackToken);
            
            ret = onvif_trc_DeleteTrack(&p_dev->onvif_device, &req8, NULL);

            printf("onvif_trc_DeleteTrack return ret = %d\n", ret);
        }

        // CreateRecordingJob
        trc_CreateRecordingJob_REQ req9;
        trc_CreateRecordingJob_RES res9;

        memset(&req9, 0, sizeof(req9));
        memset(&res9, 0, sizeof(res9));

        strcpy(req9.JobConfiguration.RecordingToken, res.RecordingToken);
        strcpy(req9.JobConfiguration.Mode, "Idle");
        req9.JobConfiguration.Priority = 1;
        
        ret = onvif_trc_CreateRecordingJob(&p_dev->onvif_device, &req9, &res9);

        printf("onvif_trc_CreateRecordingJob return ret = %d\n", ret);

        // GetRecordingJobs
        trc_GetRecordingJobs_RES res10;

        memset(&res10, 0, sizeof(res10));
        
        ret = onvif_trc_GetRecordingJobs(&p_dev->onvif_device, NULL, &res10);

        printf("onvif_trc_GetRecordingJobs return ret = %d\n", ret);

        if (ret && res10.RecordingJobs)
        {
            onvif_free_RecordingJobs(&res10.RecordingJobs);
        }

        // GetRecordingJobConfiguration
        trc_GetRecordingJobConfiguration_REQ req11;
        trc_GetRecordingJobConfiguration_RES res11;

        memset(&req11, 0, sizeof(req11));
        memset(&res11, 0, sizeof(res11));

        strcpy(req11.JobToken, res9.JobToken);       
        
        ret = onvif_trc_GetRecordingJobConfiguration(&p_dev->onvif_device, &req11, &res11);

        printf("onvif_trc_GetRecordingJobConfiguration return ret = %d\n", ret);

        // SetRecordingJobConfiguration
        trc_SetRecordingJobConfiguration_REQ req12;
        trc_SetRecordingJobConfiguration_RES res12;

        memset(&req12, 0, sizeof(req12));
        memset(&res12, 0, sizeof(res12));

        strcpy(req12.JobToken, res9.JobToken);
        memcpy(&req12.JobConfiguration, &res11.JobConfiguration, sizeof(onvif_RecordingJobConfiguration));        
        
        ret = onvif_trc_SetRecordingJobConfiguration(&p_dev->onvif_device, &req12, &res12);

        printf("onvif_trc_SetRecordingJobConfiguration return ret = %d\n", ret);

        // SetRecordingJobMode
        trc_SetRecordingJobMode_REQ req13;

        memset(&req13, 0, sizeof(req13));

        strcpy(req13.JobToken, res9.JobToken);
        strcpy(req13.Mode, "Active");
        
        ret = onvif_trc_SetRecordingJobMode(&p_dev->onvif_device, &req13, NULL);

        printf("onvif_trc_SetRecordingJobMode return ret = %d\n", ret);

        // GetRecordingJobState
        trc_GetRecordingJobState_REQ req14;
        trc_GetRecordingJobState_RES res14;

        memset(&req14, 0, sizeof(req14));
        memset(&res14, 0, sizeof(res14));

        strcpy(req14.JobToken, res9.JobToken);
        
        ret = onvif_trc_GetRecordingJobState(&p_dev->onvif_device, &req14, &res14);

        printf("onvif_trc_GetRecordingJobState return ret = %d\n", ret);

        // SetRecordingJobMode
        trc_SetRecordingJobMode_REQ req15;

        memset(&req15, 0, sizeof(req15));

        strcpy(req15.JobToken, res9.JobToken);
        strcpy(req15.Mode, "Idle");
        
        ret = onvif_trc_SetRecordingJobMode(&p_dev->onvif_device, &req15, NULL);

        printf("onvif_trc_SetRecordingJobMode return ret = %d\n", ret);

        // DeleteRecordingJob
        trc_DeleteRecordingJob_REQ req16;

        memset(&req16, 0, sizeof(req16));

        strcpy(req16.JobToken, res9.JobToken);
        
        ret = onvif_trc_DeleteRecordingJob(&p_dev->onvif_device, &req16, NULL);

        printf("onvif_trc_DeleteRecordingJob return ret = %d\n", ret);

        // DeleteRecording
        trc_DeleteRecording_REQ req17;

        memset(&req17, 0, sizeof(req17));

        strcpy(req17.RecordingToken, res.RecordingToken);
        
        ret = onvif_trc_DeleteRecording(&p_dev->onvif_device, &req17, NULL);

        printf("onvif_trc_DeleteRecording return ret = %d\n", ret);
        
        onvif_free_Recordings(&res1.Recordings);

        break;
    }
}

void onvifSearchTest(ONVIF_DEVICE_EX * p_dev)
{
    BOOL ret;

    tse_GetRecordingSummary_RES res;
    memset(&res, 0, sizeof(res));
    
    ret = onvif_tse_GetRecordingSummary(&p_dev->onvif_device, NULL, &res);

    printf("onvif_tse_GetRecordingSummary return ret = %d\n", ret);

    while (1)
    {
        tse_FindRecordings_REQ req1;
        tse_FindRecordings_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        req1.KeepAliveTime = 10;
        req1.MaxMatchesFlag = 1;
        req1.MaxMatches = 1;
        
        ret = onvif_tse_FindRecordings(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tse_FindRecordings return ret = %d\n", ret);

        if (ret)
        {
            sleep(3);

            tse_GetRecordingSearchResults_REQ req2;
            tse_GetRecordingSearchResults_RES res2;

            memset(&req2, 0, sizeof(req2));
            memset(&res2, 0, sizeof(res2));

            strcpy(req2.SearchToken, res1.SearchToken);
            
            ret = onvif_tse_GetRecordingSearchResults(&p_dev->onvif_device, &req2, &res2);

            printf("onvif_tse_GetRecordingSearchResults return ret = %d\n", ret);

            onvif_free_RecordingInformations(&res2.ResultList.RecordInformation);

            tse_EndSearch_REQ req3;
            tse_EndSearch_RES res3;

            memset(&req3, 0, sizeof(req3));
            memset(&res3, 0, sizeof(res3));

            strcpy(req3.SearchToken, res1.SearchToken);
            
            ret = onvif_tse_EndSearch(&p_dev->onvif_device, &req3, &res3);

            printf("onvif_tse_EndSearch return ret = %d\n", ret);
        }

        break;
    }

    while (1)
    {
        tse_FindEvents_REQ req1;
        tse_FindEvents_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        req1.KeepAliveTime = 10;
        req1.MaxMatchesFlag = 1;
        req1.MaxMatches = 1;
        
        ret = onvif_tse_FindEvents(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tse_FindEvents return ret = %d\n", ret);

        if (ret)
        {
            sleep(3);

            tse_GetEventSearchResults_REQ req2;
            tse_GetEventSearchResults_RES res2;

            memset(&req2, 0, sizeof(req2));
            memset(&res2, 0, sizeof(res2));

            strcpy(req2.SearchToken, res1.SearchToken);
            
            ret = onvif_tse_GetEventSearchResults(&p_dev->onvif_device, &req2, &res2);

            printf("onvif_tse_GetEventSearchResults return ret = %d\n", ret);

            onvif_free_FindEventResults(&res2.ResultList.Result);

            tse_EndSearch_REQ req3;
            tse_EndSearch_RES res3;

            memset(&req3, 0, sizeof(req3));
            memset(&res3, 0, sizeof(res3));

            strcpy(req3.SearchToken, res1.SearchToken);
            
            ret = onvif_tse_EndSearch(&p_dev->onvif_device, &req3, &res3);

            printf("onvif_tse_EndSearch return ret = %d\n", ret);
        }

        break;
    }

    while (1)
    {
        tse_FindMetadata_REQ req1;
        tse_FindMetadata_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        req1.KeepAliveTime = 10;
        req1.MaxMatchesFlag = 1;
        req1.MaxMatches = 1;
        
        ret = onvif_tse_FindMetadata(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tse_FindMetadata return ret = %d\n", ret);

        if (ret)
        {
            sleep(3);

            tse_GetMetadataSearchResults_REQ req2;
            tse_GetMetadataSearchResults_RES res2;

            memset(&req2, 0, sizeof(req2));
            memset(&res2, 0, sizeof(res2));

            strcpy(req2.SearchToken, res1.SearchToken);
            
            ret = onvif_tse_GetMetadataSearchResults(&p_dev->onvif_device, &req2, &res2);

            printf("onvif_tse_GetMetadataSearchResults return ret = %d\n", ret);

            onvif_free_FindMetadataResults(&res2.ResultList.Result);

            tse_EndSearch_REQ req3;
            tse_EndSearch_RES res3;

            memset(&req3, 0, sizeof(req3));
            memset(&res3, 0, sizeof(res3));

            strcpy(req3.SearchToken, res1.SearchToken);
            
            ret = onvif_tse_EndSearch(&p_dev->onvif_device, &req3, &res3);

            printf("onvif_tse_EndSearch return ret = %d\n", ret);
        }

        break;
    }

    while (1)
    {
        tse_FindPTZPosition_REQ req1;
        tse_FindPTZPosition_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        req1.KeepAliveTime = 10;
        req1.MaxMatchesFlag = 1;
        req1.MaxMatches = 1;
        
        ret = onvif_tse_FindPTZPosition(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tse_FindPTZPosition return ret = %d\n", ret);

        if (ret)
        {
            sleep(3);

            tse_GetPTZPositionSearchResults_REQ req2;
            tse_GetPTZPositionSearchResults_RES res2;

            memset(&req2, 0, sizeof(req2));
            memset(&res2, 0, sizeof(res2));

            strcpy(req2.SearchToken, res1.SearchToken);
            
            ret = onvif_tse_GetPTZPositionSearchResults(&p_dev->onvif_device, &req2, &res2);

            printf("onvif_tse_GetPTZPositionSearchResults return ret = %d\n", ret);

            onvif_free_FindPTZPositionResults(&res2.ResultList.Result);

            tse_EndSearch_REQ req3;
            tse_EndSearch_RES res3;

            memset(&req3, 0, sizeof(req3));
            memset(&res3, 0, sizeof(res3));

            strcpy(req3.SearchToken, res1.SearchToken);
            
            ret = onvif_tse_EndSearch(&p_dev->onvif_device, &req3, &res3);

            printf("onvif_tse_EndSearch return ret = %d\n", ret);
        }

        break;
    }
}

void onvifReplayTest(ONVIF_DEVICE_EX * p_dev)
{
    BOOL ret;

    // GetReplayConfiguration
    trp_GetReplayConfiguration_RES res;
    memset(&res, 0, sizeof(res));

    ret = onvif_trp_GetReplayConfiguration(&p_dev->onvif_device, NULL, &res);

    printf("onvif_trp_GetReplayConfiguration return ret = %d\n", ret);

    // SetReplayConfiguration
    trp_SetReplayConfiguration_REQ req2;
    memset(&req2, 0, sizeof(req2));

    req2.Configuration.SessionTimeout = 120;
    
    ret = onvif_trp_SetReplayConfiguration(&p_dev->onvif_device, &req2, NULL);

    printf("onvif_trp_SetReplayConfiguration return ret = %d\n", ret);

    // GetRecordings
    trc_GetRecordings_RES res3;
    memset(&res3, 0, sizeof(res3));

    ret = onvif_trc_GetRecordings(&p_dev->onvif_device, NULL, &res3);

    printf("onvif_trc_GetRecordings return ret = %d\n", ret);

    if (res3.Recordings)
    {
        trp_GetReplayUri_REQ req4;
        trp_GetReplayUri_RES res4;

        memset(&req4, 0, sizeof(req4));
        memset(&res4, 0, sizeof(res4));

        strcpy(req4.RecordingToken, res3.Recordings->Recording.RecordingToken);
        req4.StreamSetup.Stream = StreamType_RTP_Unicast;
        req4.StreamSetup.Transport.Protocol = TransportProtocol_UDP;
        
        ret = onvif_trp_GetReplayUri(&p_dev->onvif_device, &req4, &res4);

        printf("onvif_trp_GetReplayUri return ret = %d\n", ret);
        if (ret)
        {
            printf("replay uri : %s\n", res4.Uri);
        }

        onvif_free_Recordings(&res3.Recordings);
    }

    // SetReplayConfiguration
    trp_SetReplayConfiguration_REQ req5;
    memset(&req5, 0, sizeof(req5));

    req5.Configuration.SessionTimeout = res.Configuration.SessionTimeout;
    
    ret = onvif_trp_SetReplayConfiguration(&p_dev->onvif_device, &req5, NULL);

    printf("onvif_trp_SetReplayConfiguration return ret = %d\n", ret);
    
}

#endif // end of PROFILE_G_SUPPORT

#ifdef THERMAL_SUPPORT

void onvifThermalTest(ONVIF_DEVICE_EX * p_dev)
{    
    BOOL ret;
    
    // GetConfigurations
    tth_GetConfigurations_RES res;
    memset(&res, 0, sizeof(res));
    
    ret = onvif_tth_GetConfigurations(&p_dev->onvif_device, NULL, &res);

    printf("onvif_tth_GetConfigurations return ret = %d\n", ret);

    if (res.Configurations)
    {
        // GetConfiguration
        tth_GetConfiguration_REQ req1;
        tth_GetConfiguration_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.VideoSourceToken, res.Configurations->token);

        ret = onvif_tth_GetConfiguration(&p_dev->onvif_device, &req1, &res1);

        printf("onvif_tth_GetConfiguration return ret = %d\n", ret);

        // GetConfigurationOptions
        tth_GetConfigurationOptions_REQ req2;
        tth_GetConfigurationOptions_RES res2;

        memset(&req2, 0, sizeof(req2));
        memset(&res2, 0, sizeof(res2));

        strcpy(req2.VideoSourceToken, res.Configurations->token);

        ret = onvif_tth_GetConfigurationOptions(&p_dev->onvif_device, &req2, &res2);

        printf("onvif_tth_GetConfigurationOptions return ret = %d\n", ret);

        if (res2.Options.ColorPalette)
        {
            onvif_free_ColorPalettes(&res2.Options.ColorPalette);
        }

        if (res2.Options.NUCTable)
        {
            onvif_free_NUCTables(&res2.Options.NUCTable);
        }

        // SetConfiguration
        tth_SetConfiguration_REQ req3;
        memset(&req3, 0, sizeof(req3));

        strcpy(req3.VideoSourceToken, res.Configurations->token);
        memcpy(&req3.Configuration, &res.Configurations->Configuration, sizeof(onvif_ThermalConfiguration));

        ret = onvif_tth_SetConfiguration(&p_dev->onvif_device, &req3, NULL);

        printf("onvif_tth_SetConfiguration return ret = %d\n", ret);

        // GetRadiometryConfiguration
        tth_GetRadiometryConfiguration_REQ req4;
        tth_GetRadiometryConfiguration_RES res4;

        memset(&req4, 0, sizeof(req4));
        memset(&res4, 0, sizeof(res4));

        strcpy(req4.VideoSourceToken, res.Configurations->token);

        ret = onvif_tth_GetRadiometryConfiguration(&p_dev->onvif_device, &req4, &res4);

        printf("onvif_tth_GetRadiometryConfiguration return ret = %d\n", ret);

        if (ret)
        {
            // onvif_tth_SetRadiometryConfiguration
            tth_SetRadiometryConfiguration_REQ req5;
            memset(&req5, 0, sizeof(req5));

            strcpy(req5.VideoSourceToken, res.Configurations->token);
            memcpy(&req5.Configuration, &res4.Configuration, sizeof(onvif_RadiometryConfiguration));

            ret = onvif_tth_SetRadiometryConfiguration(&p_dev->onvif_device, &req5, NULL);

            printf("onvif_tth_SetRadiometryConfiguration return ret = %d\n", ret);
        }

        // GetRadiometryConfigurationOptions
        tth_GetRadiometryConfigurationOptions_REQ req6;
        tth_GetRadiometryConfigurationOptions_RES res6;

        memset(&req6, 0, sizeof(req6));
        memset(&res6, 0, sizeof(res6));

        strcpy(req6.VideoSourceToken, res.Configurations->token);

        ret = onvif_tth_GetRadiometryConfigurationOptions(&p_dev->onvif_device, &req6, &res6);

        printf("onvif_tth_GetRadiometryConfigurationOptions return ret = %d\n", ret);
        

        onvif_free_ThermalConfigurations(&res.Configurations);
    }
    
}

#endif // end of THERMAL_SUPPORT

#ifdef CREDENTIAL_SUPPORT

void onvifCredentialTest(ONVIF_DEVICE_EX * p_device)
{
    uint32 i;
    BOOL ret;

    // GetCredentialList
    tcr_GetCredentialList_REQ req;
    tcr_GetCredentialList_RES res;

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    ret = onvif_tcr_GetCredentialList(&p_device->onvif_device, &req, &res);

    printf("onvif_tcr_GetCredentialList return ret = %d\n", ret);

    if (!ret)
    {
        return;
    }

    // GetCredentials
    tcr_GetCredentials_REQ req1;
    tcr_GetCredentials_RES res1;

    memset(&req1, 0, sizeof(req1));
    memset(&res1, 0, sizeof(res1));

    for (i = 0; i < res.sizeCredential; i++)
    {
        req1.sizeToken++;
        strcpy(req1.Token[i], res.Credential[i].token);
    }

    ret = onvif_tcr_GetCredentials(&p_device->onvif_device, &req1, &res1);

    printf("onvif_tcr_GetCredentials return ret = %d\n", ret);

    // GetCredentialInfo
    tcr_GetCredentialInfo_REQ req2;
    tcr_GetCredentialInfo_RES res2;

    memset(&req2, 0, sizeof(req2));
    memset(&res2, 0, sizeof(res2));

    for (i = 0; i < res.sizeCredential; i++)
    {
        req2.sizeToken++;
        strcpy(req2.Token[i], res.Credential[i].token);
    }

    ret = onvif_tcr_GetCredentialInfo(&p_device->onvif_device, &req2, &res2);

    printf("onvif_tcr_GetCredentialInfo return ret = %d\n", ret);

    // GetCredentialInfoList
    tcr_GetCredentialInfoList_REQ req3;
    tcr_GetCredentialInfoList_RES res3;

    memset(&req3, 0, sizeof(req3));
    memset(&res3, 0, sizeof(res3));

    ret = onvif_tcr_GetCredentialInfoList(&p_device->onvif_device, &req3, &res3);

    printf("onvif_tcr_GetCredentialInfoList return ret = %d\n", ret);

    if (res.sizeCredential > 0)
    {
        // GetCredentialState
        tcr_GetCredentialState_REQ req4;
        tcr_GetCredentialState_RES res4;

        memset(&req4, 0, sizeof(req4));
        memset(&res4, 0, sizeof(res4));

        strcpy(req4.Token, res.Credential[0].token);
        
        ret = onvif_tcr_GetCredentialState(&p_device->onvif_device, &req4, &res4);

        printf("onvif_tcr_GetCredentialState return ret = %d\n", ret);

        // EnableCredential
        tcr_EnableCredential_REQ req5;

        memset(&req5, 0, sizeof(req5));

        strcpy(req5.Token, res.Credential[0].token);
        
        ret = onvif_tcr_EnableCredential(&p_device->onvif_device, &req5, NULL);

        printf("onvif_tcr_EnableCredential return ret = %d\n", ret);

        // DisableCredential
        tcr_DisableCredential_REQ req6;

        memset(&req6, 0, sizeof(req6));

        strcpy(req6.Token, res.Credential[0].token);
        
        ret = onvif_tcr_DisableCredential(&p_device->onvif_device, &req6, NULL);

        printf("onvif_tcr_DisableCredential return ret = %d\n", ret);

        // ResetAntipassbackViolation
        tcr_ResetAntipassbackViolation_REQ req7;

        memset(&req7, 0, sizeof(req7));

        strcpy(req7.CredentialToken, res.Credential[0].token);
        
        ret = onvif_tcr_ResetAntipassbackViolation(&p_device->onvif_device, &req7, NULL);

        printf("onvif_tcr_ResetAntipassbackViolation return ret = %d\n", ret);

        // CreateCredential
        tcr_CreateCredential_REQ req8;
        tcr_CreateCredential_RES res8;

        memset(&req8, 0, sizeof(req8));
        memset(&res8, 0, sizeof(res8));

        memcpy(&req8.Credential, &res.Credential[0], sizeof(onvif_Credential));
        memcpy(&req8.State, &res4.State, sizeof(onvif_CredentialState));

        strcpy(req8.Credential.token, "");
        strcpy(req8.Credential.Description, "test");
        
        ret = onvif_tcr_CreateCredential(&p_device->onvif_device, &req8, &res8);

        printf("onvif_tcr_CreateCredential return ret = %d\n", ret);

        // ModifyCredential
        tcr_ModifyCredential_REQ req9;
        memset(&req9, 0, sizeof(req9));

        strcpy(req9.Credential.token, res8.Token);
        strcpy(req9.Credential.Description, "test11");

        ret = onvif_tcr_ModifyCredential(&p_device->onvif_device, &req9, NULL);

        printf("onvif_tcr_ModifyCredential return ret = %d\n", ret);

        // DeleteCredential
        tcr_DeleteCredential_REQ req10;
        memset(&req10, 0, sizeof(req10));

        strcpy(req10.Token, res8.Token);

        ret = onvif_tcr_DeleteCredential(&p_device->onvif_device, &req10, NULL);

        printf("onvif_tcr_DeleteCredential return ret = %d\n", ret);

        // GetSupportedFormatTypes
        tcr_GetSupportedFormatTypes_REQ req11;
        tcr_GetSupportedFormatTypes_RES res11;

        memset(&req11, 0, sizeof(req11));
        memset(&res11, 0, sizeof(res11));

        strcpy(req11.CredentialIdentifierTypeName, "pt:Card");
        
        ret = onvif_tcr_GetSupportedFormatTypes(&p_device->onvif_device, &req11, &res11);

        printf("onvif_tcr_GetSupportedFormatTypes return ret = %d\n", ret);

        // GetCredentialIdentifiers
        tcr_GetCredentialIdentifiers_REQ req12;
        tcr_GetCredentialIdentifiers_RES res12;

        memset(&req12, 0, sizeof(req12));
        memset(&res12, 0, sizeof(res12));

        strcpy(req12.CredentialToken, res.Credential[0].token);
        
        ret = onvif_tcr_GetCredentialIdentifiers(&p_device->onvif_device, &req12, &res12);

        printf("onvif_tcr_GetCredentialIdentifiers return ret = %d\n", ret);

        // GetCredentialAccessProfiles
        tcr_GetCredentialAccessProfiles_REQ req13;
        tcr_GetCredentialAccessProfiles_RES res13;

        memset(&req13, 0, sizeof(req13));
        memset(&res13, 0, sizeof(res13));

        strcpy(req13.CredentialToken, res.Credential[0].token);
        
        ret = onvif_tcr_GetCredentialAccessProfiles(&p_device->onvif_device, &req13, &res13);

        printf("onvif_tcr_GetCredentialAccessProfiles return ret = %d\n", ret);
    }
}

#endif // end of CREDENTIAL_SUPPORT

#ifdef ACCESS_RULES

void onvifAccessRulesTest(ONVIF_DEVICE_EX * p_device)
{
    uint32 i;
    BOOL ret;

    // GetAccessProfileList
    tar_GetAccessProfileList_REQ req;
    tar_GetAccessProfileList_RES res;

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    ret = onvif_tar_GetAccessProfileList(&p_device->onvif_device, &req, &res);

    printf("onvif_tar_GetAccessProfileList return ret = %d\n", ret);

    if (!ret)
    {
        return;
    }

    // GetAccessProfiles
    tar_GetAccessProfiles_REQ req1;
    tar_GetAccessProfiles_RES res1;

    memset(&req1, 0, sizeof(req1));
    memset(&res1, 0, sizeof(res1));

    for (i = 0; i < res.sizeAccessProfile; i++)
    {
        req1.sizeToken++;
        strcpy(req1.Token[i], res.AccessProfile[i].token);
    }

    ret = onvif_tar_GetAccessProfiles(&p_device->onvif_device, &req1, &res1);

    printf("onvif_tar_GetAccessProfiles return ret = %d\n", ret);

    // GetAccessProfileInfoList
    tar_GetAccessProfileInfoList_REQ req2;
    tar_GetAccessProfileInfoList_RES res2;

    memset(&req2, 0, sizeof(req2));
    memset(&res2, 0, sizeof(res2));

    ret = onvif_tar_GetAccessProfileInfoList(&p_device->onvif_device, &req2, &res2);

    printf("onvif_tar_GetAccessProfileInfoList return ret = %d\n", ret);

    // GetAccessProfileInfo
    tar_GetAccessProfileInfo_REQ req3;
    tar_GetAccessProfileInfo_RES res3;

    memset(&req3, 0, sizeof(req3));
    memset(&res3, 0, sizeof(res3));

    for (i = 0; i < res.sizeAccessProfile; i++)
    {
        req3.sizeToken++;
        strcpy(req3.Token[i], res.AccessProfile[i].token);
    }
    
    ret = onvif_tar_GetAccessProfileInfo(&p_device->onvif_device, &req3, &res3);

    printf("onvif_tar_GetAccessProfileInfo return ret = %d\n", ret);

    if (res.sizeAccessProfile > 0)
    {
        // CreateAccessProfile
        tar_CreateAccessProfile_REQ req4;
        tar_CreateAccessProfile_RES res4;

        memset(&req4, 0, sizeof(req4));
        memset(&res4, 0, sizeof(res4));

        memcpy(&req4.AccessProfile, &res.AccessProfile[0], sizeof(onvif_AccessProfile));
        strcpy(req4.AccessProfile.token, "");
        strcpy(req4.AccessProfile.Name, "test");
        strcpy(req4.AccessProfile.Description, "test");
        
        ret = onvif_tar_CreateAccessProfile(&p_device->onvif_device, &req4, &res4);

        printf("onvif_tar_CreateAccessProfile return ret = %d\n", ret);

        // ModifyAccessProfile
        tar_ModifyAccessProfile_REQ req5;

        memset(&req5, 0, sizeof(req5));

        memcpy(&req5.AccessProfile, &res.AccessProfile[0], sizeof(onvif_AccessProfile));
        strcpy(req5.AccessProfile.token, res4.Token);
        strcpy(req5.AccessProfile.Name, "test11");
        strcpy(req5.AccessProfile.Description, "test11");

        ret = onvif_tar_ModifyAccessProfile(&p_device->onvif_device, &req5, NULL);

        printf("onvif_tar_ModifyAccessProfile return ret = %d\n", ret);

        // DeleteAccessProfile
        tar_DeleteAccessProfile_REQ req6;

        memset(&req6, 0, sizeof(req6));

        strcpy(req6.Token, res4.Token);

        ret = onvif_tar_DeleteAccessProfile(&p_device->onvif_device, &req6, NULL);

        printf("onvif_tar_DeleteAccessProfile return ret = %d\n", ret);
    }
}

#endif // end of ACCESS_RULES

#ifdef SCHEDULE_SUPPORT

void onvifScheduleTest(ONVIF_DEVICE_EX * p_device)
{
    uint32 i;
    BOOL ret;

    // GetScheduleList
    tsc_GetScheduleList_REQ req;
    tsc_GetScheduleList_RES res;

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    ret = onvif_tsc_GetScheduleList(&p_device->onvif_device, &req, &res);

    printf("onvif_tsc_GetScheduleList return ret = %d\n", ret);

    if (!ret)
    {
        return;
    }

    // GetSchedules
    tsc_GetSchedules_REQ req1;
    tsc_GetSchedules_RES res1;

    memset(&req1, 0, sizeof(req1));
    memset(&res1, 0, sizeof(res1));

    for (i = 0; i < res.sizeSchedule; i++)
    {
        req1.sizeToken++;
        strcpy(req1.Token[i], res.Schedule[i].token);
    }

    ret = onvif_tsc_GetSchedules(&p_device->onvif_device, &req1, &res1);

    printf("onvif_tsc_GetSchedules return ret = %d\n", ret);

    // GetScheduleInfoList
    tsc_GetScheduleInfoList_REQ req2;
    tsc_GetScheduleInfoList_RES res2;

    memset(&req2, 0, sizeof(req2));
    memset(&res2, 0, sizeof(res2));

    ret = onvif_tsc_GetScheduleInfoList(&p_device->onvif_device, &req2, &res2);

    printf("onvif_tsc_GetScheduleInfoList return ret = %d\n", ret);

    // GetScheduleInfo
    tsc_GetScheduleInfo_REQ req3;
    tsc_GetScheduleInfo_RES res3;

    memset(&req3, 0, sizeof(req3));
    memset(&res3, 0, sizeof(res3));

    for (i = 0; i < res.sizeSchedule; i++)
    {
        req1.sizeToken++;
        strcpy(req3.Token[i], res.Schedule[i].token);
    }
    
    ret = onvif_tsc_GetScheduleInfo(&p_device->onvif_device, &req3, &res3);

    printf("onvif_tsc_GetScheduleInfo return ret = %d\n", ret);

    // CreateSchedule
    tsc_CreateSchedule_REQ req4;
    tsc_CreateSchedule_RES res4;

    memset(&req4, 0, sizeof(req4));
    memset(&res4, 0, sizeof(res4));

    strcpy(req4.Schedule.token, "");
    strcpy(req4.Schedule.Name, "test");
    req4.Schedule.DescriptionFlag = 1;
    strcpy(req4.Schedule.Description, "test");
    strcpy(req4.Schedule.Standard, "BEGIN:VCALENDAR\r\n"
        "BEGIN:VEVENT\r\n"
        "SUMMARY:Access 24*7\r\n"
        "DTSTART:19700101T000000\r\n"
        "DTEND:19700102T000000\r\n"
        "RRULE:FREQ=WEEKLY;BYDAY=MO,TU,WE,TH,FR,SA,SU\r\n"
        "END:VEVENT\r\n"
        "END:VCALENDAR");
    
    ret = onvif_tsc_CreateSchedule(&p_device->onvif_device, &req4, &res4);

    printf("onvif_tsc_CreateSchedule return ret = %d\n", ret);

    // ModifySchedule
    tsc_ModifySchedule_REQ req5;

    memset(&req5, 0, sizeof(req5));

    strcpy(req5.Schedule.token, res4.Token);
    strcpy(req5.Schedule.Name, "test11");
    req5.Schedule.DescriptionFlag = 1;
    strcpy(req5.Schedule.Description, "test11");
    strcpy(req4.Schedule.Standard, "BEGIN:VCALENDAR\r\n"
        "BEGIN:VEVENT\r\n"
        "SUMMARY:Access 24*7\r\n"
        "DTSTART:19700101T000000\r\n"
        "DTEND:19700102T000000\r\n"
        "RRULE:FREQ=WEEKLY;BYDAY=MO,TU,WE,TH,FR,SA,SU\r\n"
        "END:VEVENT\r\n"
        "END:VCALENDAR");

    ret = onvif_tsc_ModifySchedule(&p_device->onvif_device, &req5, NULL);

    printf("onvif_tsc_ModifySchedule return ret = %d\n", ret);

    // GetScheduleState
    tsc_GetScheduleState_REQ req6;
    tsc_GetScheduleState_RES res6;

    memset(&req6, 0, sizeof(req6));
    memset(&res6, 0, sizeof(res6));

    strcpy(req6.Token, res4.Token);

    ret = onvif_tsc_GetScheduleState(&p_device->onvif_device, &req6, &res6);

    printf("onvif_tsc_GetScheduleState return ret = %d\n", ret);

    // DeleteSchedule
    tsc_DeleteSchedule_REQ req7;

    memset(&req7, 0, sizeof(req7));

    strcpy(req7.Token, res4.Token);

    ret = onvif_tsc_DeleteSchedule(&p_device->onvif_device, &req7, NULL);

    printf("onvif_tsc_DeleteSchedule return ret = %d\n", ret);
}

void onvifSpecialDayGroupTest(ONVIF_DEVICE_EX * p_device)
{
    uint32 i;
    BOOL ret;

    // GetSpecialDayGroupList
    tsc_GetSpecialDayGroupList_REQ req;
    tsc_GetSpecialDayGroupList_RES res;

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    ret = onvif_tsc_GetSpecialDayGroupList(&p_device->onvif_device, &req, &res);

    printf("onvif_tsc_GetSpecialDayGroupList return ret = %d\n", ret);

    if (!ret)
    {
        return;
    }

    // GetSpecialDayGroups
    tsc_GetSpecialDayGroups_REQ req1;
    tsc_GetSpecialDayGroups_RES res1;

    memset(&req1, 0, sizeof(req1));
    memset(&res1, 0, sizeof(res1));

    for (i = 0; i < res.sizeSpecialDayGroup; i++)
    {
        req1.sizeToken++;
        strcpy(req1.Token[i], res.SpecialDayGroup[i].token);
    }

    ret = onvif_tsc_GetSpecialDayGroups(&p_device->onvif_device, &req1, &res1);

    printf("onvif_tsc_GetSpecialDayGroups return ret = %d\n", ret);

    // GetSpecialDayGroupInfoList
    tsc_GetSpecialDayGroupInfoList_REQ req2;
    tsc_GetSpecialDayGroupInfoList_RES res2;

    memset(&req2, 0, sizeof(req2));
    memset(&res2, 0, sizeof(res2));

    ret = onvif_tsc_GetSpecialDayGroupInfoList(&p_device->onvif_device, &req2, &res2);

    printf("onvif_tsc_GetSpecialDayGroupInfoList return ret = %d\n", ret);

    // GetSpecialDayGroupInfo
    tsc_GetSpecialDayGroupInfo_REQ req3;
    tsc_GetSpecialDayGroupInfo_RES res3;

    memset(&req3, 0, sizeof(req3));
    memset(&res3, 0, sizeof(res3));

    for (i = 0; i < res.sizeSpecialDayGroup; i++)
    {
        req1.sizeToken++;
        strcpy(req3.Token[i], res.SpecialDayGroup[i].token);
    }
    
    ret = onvif_tsc_GetSpecialDayGroupInfo(&p_device->onvif_device, &req3, &res3);

    printf("onvif_tsc_GetSpecialDayGroupInfo return ret = %d\n", ret);

    // CreateSpecialDayGroup
    tsc_CreateSpecialDayGroup_REQ req4;
    tsc_CreateSpecialDayGroup_RES res4;

    memset(&req4, 0, sizeof(req4));
    memset(&res4, 0, sizeof(res4));

    strcpy(req4.SpecialDayGroup.token, "");
    strcpy(req4.SpecialDayGroup.Name, "test");
    req4.SpecialDayGroup.DescriptionFlag = 1;
    strcpy(req4.SpecialDayGroup.Description, "test");
    req4.SpecialDayGroup.DaysFlag = 1;
    strcpy(req4.SpecialDayGroup.Days, "BEGIN:VCALENDAR\r\n"
        "PRODID:VERSION:2.0\r\n"
        "BEGIN:VEVENT\r\n"
        "SUMMARY:Christmas day\r\n"
        "DTSTART:20141225T000000\r\n"
        "DTEND:20141226T000000\r\n"
        "UID:Holiday@ONVIF.com\r\n"
        "END:VEVENT\r\n"
        "END:VCALENDAR\r\n");
    
    ret = onvif_tsc_CreateSpecialDayGroup(&p_device->onvif_device, &req4, &res4);

    printf("onvif_tsc_CreateSpecialDayGroup return ret = %d\n", ret);

    // ModifySpecialDayGroup
    tsc_ModifySpecialDayGroup_REQ req5;

    memset(&req5, 0, sizeof(req5));

    strcpy(req5.SpecialDayGroup.token, res4.Token);
    strcpy(req5.SpecialDayGroup.Name, "test11");
    req5.SpecialDayGroup.DescriptionFlag = 1;
    strcpy(req5.SpecialDayGroup.Description, "test11");
    req5.SpecialDayGroup.DaysFlag = 1;
    strcpy(req4.SpecialDayGroup.Days, "BEGIN:VCALENDAR\r\n"
        "BEGIN:VEVENT\r\n"
        "SUMMARY:Access 24*7\r\n"
        "DTSTART:19700101T000000\r\n"
        "DTEND:19700102T000000\r\n"
        "RRULE:FREQ=WEEKLY;BYDAY=MO,TU,WE,TH,FR,SA,SU\r\n"
        "END:VEVENT\r\n"
        "END:VCALENDAR");

    ret = onvif_tsc_ModifySpecialDayGroup(&p_device->onvif_device, &req5, NULL);

    printf("onvif_tsc_ModifySpecialDayGroup return ret = %d\n", ret);

    // DeleteSpecialDayGroup
    tsc_DeleteSpecialDayGroup_REQ req6;

    memset(&req6, 0, sizeof(req6));

    strcpy(req6.Token, res4.Token);

    ret = onvif_tsc_DeleteSpecialDayGroup(&p_device->onvif_device, &req6, NULL);

    printf("onvif_tsc_DeleteSpecialDayGroup return ret = %d\n", ret);
}

#endif // end of SCHEDULE_SUPPORT

#ifdef RECEIVER_SUPPORT

void onvifReceiverTest(ONVIF_DEVICE_EX * p_device)
{
    BOOL ret;

    // GetReceivers
    trv_GetReceivers_RES res;

    memset(&res, 0, sizeof(res));

    ret = onvif_trv_GetReceivers(&p_device->onvif_device, NULL, &res);

    printf("onvif_trv_GetReceivers return ret = %d\n", ret);

    onvif_free_Receivers(&res.Receivers);

    // CreateReceiver
    trv_CreateReceiver_REQ req1;
    trv_CreateReceiver_RES res1;

    memset(&req1, 0, sizeof(req1));
    memset(&res1, 0, sizeof(res1));

    req1.Configuration.Mode = ReceiverMode_NeverConnect;
    strcpy(req1.Configuration.MediaUri, "rtsp://192.168.0.1/test.264");
    req1.Configuration.StreamSetup.Stream = StreamType_RTP_Unicast;
    req1.Configuration.StreamSetup.Transport.Protocol = TransportProtocol_RTSP;

    ret = onvif_trv_CreateReceiver(&p_device->onvif_device, &req1, &res1);

    printf("onvif_trv_CreateReceiver return ret = %d\n", ret);

    if (!ret)
    {
        return;
    }

    // GetReceiver
    trv_GetReceiver_REQ req2;
    trv_GetReceiver_RES res2;

    memset(&req2, 0, sizeof(req2));
    memset(&res2, 0, sizeof(res2));

    strcpy(req2.ReceiverToken, res1.Receiver.Token);

    ret = onvif_trv_GetReceiver(&p_device->onvif_device, &req2, &res2);

    printf("onvif_trv_GetReceiver return ret = %d\n", ret);

    // ConfigureReceiver
    trv_ConfigureReceiver_REQ req3;

    memset(&req3, 0, sizeof(req3));

    strcpy(req3.ReceiverToken, res1.Receiver.Token);
    memcpy(&req3.Configuration, &res1.Receiver.Configuration, sizeof(onvif_ReceiverConfiguration));
    strcpy(req3.Configuration.MediaUri, "rtsp://192.168.0.2/test.264");

    ret = onvif_trv_ConfigureReceiver(&p_device->onvif_device, &req3, NULL);

    printf("onvif_trv_ConfigureReceiver return ret = %d\n", ret);

    // SetReceiverMode
    trv_SetReceiverMode_REQ req4;

    memset(&req4, 0, sizeof(req4));

    strcpy(req4.ReceiverToken, res1.Receiver.Token);
    req4.Mode = ReceiverMode_AlwaysConnect;

    ret = onvif_trv_SetReceiverMode(&p_device->onvif_device, &req4, NULL);

    printf("onvif_trv_SetReceiverMode return ret = %d\n", ret);

    // GetReceiverState
    trv_GetReceiverState_REQ req5;
    trv_GetReceiverState_RES res5;

    memset(&req5, 0, sizeof(req5));
    memset(&res5, 0, sizeof(res5));

    strcpy(req5.ReceiverToken, res1.Receiver.Token);

    ret = onvif_trv_GetReceiverState(&p_device->onvif_device, &req5, &res5);

    printf("onvif_trv_GetReceiverState return ret = %d\n", ret);

    // DeleteReceiver
    trv_DeleteReceiver_REQ req6;

    memset(&req6, 0, sizeof(req6));

    strcpy(req6.ReceiverToken, res1.Receiver.Token);

    ret = onvif_trv_DeleteReceiver(&p_device->onvif_device, &req6, NULL);

    printf("onvif_trv_DeleteReceiver return ret = %d\n", ret);
    
}

#endif // end of RECEIVER_SUPPORT

#ifdef IPFILTER_SUPPORT

void onvifIPAddressFilterTest(ONVIF_DEVICE_EX * p_device)
{        
    BOOL ret;

    // GetIPAddressFilter
    tds_GetIPAddressFilter_RES res;

    memset(&res, 0, sizeof(res));

    ret = onvif_tds_GetIPAddressFilter(&p_device->onvif_device, NULL, &res);

    printf("onvif_tds_GetIPAddressFilter return ret = %d\n", ret);

    // SetIPAddressFilter
    tds_SetIPAddressFilter_REQ req1;

    memset(&req1, 0, sizeof(req1));

    memcpy(&req1.IPAddressFilter, &res.IPAddressFilter, sizeof(onvif_IPAddressFilter));

    strcpy(req1.IPAddressFilter.IPv4Address[0].Address, "192.168.10.1");
    req1.IPAddressFilter.IPv4Address[0].PrefixLength = 24;

    ret = onvif_tds_SetIPAddressFilter(&p_device->onvif_device, &req1, NULL);

    printf("onvif_tds_SetIPAddressFilter return ret = %d\n", ret);

    // AddIPAddressFilter
    tds_AddIPAddressFilter_REQ req2;

    memset(&req2, 0, sizeof(req2));

    strcpy(req2.IPAddressFilter.IPv4Address[0].Address, "192.168.10.2");
    req2.IPAddressFilter.IPv4Address[0].PrefixLength = 24;

    ret = onvif_tds_AddIPAddressFilter(&p_device->onvif_device, &req2, NULL);

    printf("onvif_tds_AddIPAddressFilter return ret = %d\n", ret);

    // RemoveIPAddressFilter
    tds_RemoveIPAddressFilter_REQ req3;

    memset(&req3, 0, sizeof(req3));

    memcpy(&req3.IPAddressFilter, &req2.IPAddressFilter, sizeof(onvif_IPAddressFilter));

    ret = onvif_tds_RemoveIPAddressFilter(&p_device->onvif_device, &req3, NULL);

    printf("onvif_tds_RemoveIPAddressFilter return ret = %d\n", ret);

    // SetIPAddressFilter 
    tds_SetIPAddressFilter_REQ req4;

    memset(&req4, 0, sizeof(req4));

    memcpy(&req1.IPAddressFilter, &res.IPAddressFilter, sizeof(onvif_IPAddressFilter));
    
    ret = onvif_tds_SetIPAddressFilter(&p_device->onvif_device, &req4, NULL);

    printf("onvif_tds_SetIPAddressFilter return ret = %d\n", ret);

    // GetIPAddressFilter
    tds_GetIPAddressFilter_RES res5;

    memset(&res5, 0, sizeof(res5));

    ret = onvif_tds_GetIPAddressFilter(&p_device->onvif_device, NULL, &res5);

    printf("onvif_tds_GetIPAddressFilter return ret = %d\n", ret);

}

#endif // end of IPFILTER_SUPPORT

#ifdef PROVISIONING_SUPPORT

void onvifProvisioningTest(ONVIF_DEVICE_EX * p_device)
{
    BOOL ret;
    
    // GetVideoSources
    trt_GetVideoSources_RES res;
    memset(&res, 0, sizeof(res));

    ret = onvif_trt_GetVideoSources(&p_device->onvif_device, NULL, &res);

    printf("onvif_trt_GetVideoSources return ret = %d\n", ret);

    if (res.VideoSources)
    {
        // GetUsage
        tpv_GetUsage_REQ req1;
        tpv_GetUsage_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.VideoSource, res.VideoSources->VideoSource.token);
        
        ret = onvif_tpv_GetUsage(&p_device->onvif_device, &req1, &res1);

        printf("onvif_tpv_GetUsage return ret = %d\n", ret);

        // PanMove
        tpv_PanMove_REQ req2;
        memset(&req2, 0, sizeof(req2));

        strcpy(req2.VideoSource, res.VideoSources->VideoSource.token);
        req2.Direction = PanDirection_Left;
        
        ret = onvif_tpv_PanMove(&p_device->onvif_device, &req2, NULL);
        
        printf("onvif_tpv_PanMove return ret = %d\n", ret);

        // Stop
        tpv_Stop_REQ req3;
        memset(&req3, 0, sizeof(req3));

        strcpy(req3.VideoSource, res.VideoSources->VideoSource.token);
        
        ret = onvif_tpv_Stop(&p_device->onvif_device, &req3, NULL);
        
        printf("onvif_tpv_Stop return ret = %d\n", ret);

        // TiltMove
        tpv_TiltMove_REQ req4;
        memset(&req4, 0, sizeof(req4));

        strcpy(req4.VideoSource, res.VideoSources->VideoSource.token);
        req4.Direction = TiltDirection_Up;
        
        ret = onvif_tpv_TiltMove(&p_device->onvif_device, &req4, NULL);
        
        printf("onvif_tpv_TiltMove return ret = %d\n", ret);

        // Stop
        ret = onvif_tpv_Stop(&p_device->onvif_device, &req3, NULL);
        
        printf("onvif_tpv_Stop return ret = %d\n", ret);

        // ZoomMove
        tpv_ZoomMove_REQ req5;
        memset(&req5, 0, sizeof(req5));

        strcpy(req5.VideoSource, res.VideoSources->VideoSource.token);
        req5.Direction = ZoomDirection_Wide;
        
        ret = onvif_tpv_ZoomMove(&p_device->onvif_device, &req5, NULL);
        
        printf("onvif_tpv_ZoomMove return ret = %d\n", ret);

        // Stop
        ret = onvif_tpv_Stop(&p_device->onvif_device, &req3, NULL);
        
        printf("onvif_tpv_Stop return ret = %d\n", ret);

        // RollMove
        tpv_RollMove_REQ req6;
        memset(&req6, 0, sizeof(req6));

        strcpy(req6.VideoSource, res.VideoSources->VideoSource.token);
        req6.Direction = RollDirection_Clockwise;
        
        ret = onvif_tpv_RollMove(&p_device->onvif_device, &req6, NULL);
        
        printf("onvif_tpv_RollMove return ret = %d\n", ret);

        // Stop
        ret = onvif_tpv_Stop(&p_device->onvif_device, &req3, NULL);
        
        printf("onvif_tpv_Stop return ret = %d\n", ret);

        // FocusMove
        tpv_FocusMove_REQ req7;
        memset(&req7, 0, sizeof(req7));

        strcpy(req7.VideoSource, res.VideoSources->VideoSource.token);
        req7.Direction = FocusDirection_Near;
        
        ret = onvif_tpv_FocusMove(&p_device->onvif_device, &req7, NULL);
        
        printf("onvif_tpv_FocusMove return ret = %d\n", ret);

        // Stop
        ret = onvif_tpv_Stop(&p_device->onvif_device, &req3, NULL);
        
        printf("onvif_tpv_Stop return ret = %d\n", ret);
    }

    onvif_free_VideoSources(&res.VideoSources);
}

#endif // end of PROVISIONING_SUPPORT

#ifdef SECURITY_SUPPORT

void onvifSecurityTest(ONVIF_DEVICE_EX * p_device)
{
    BOOL ret;
    uint32 i;

    tas_UploadPassphrase_REQ req;
    tas_UploadPassphrase_RES res;

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    strcpy(req.Passphrase, "test");

    ret = onvif_tas_UploadPassphrase(&p_device->onvif_device, &req, &res);

    printf("onvif_tas_UploadPassphrase return ret = %d\n", ret);
    
    tas_GetAllPassphrases_RES res1;

    memset(&res1, 0, sizeof(res1));
    
    ret = onvif_tas_GetAllPassphrases(&p_device->onvif_device, NULL, &res1);

    printf("onvif_tas_GetAllPassphrases return ret = %d\n", ret);

    tas_DeletePassphrase_REQ req2;

    memset(&req2, 0, sizeof(req2));

    strcpy(req2.PassphraseID, res.PassphraseID);
    
    ret = onvif_tas_DeletePassphrase(&p_device->onvif_device, &req2, NULL);

    printf("onvif_tas_DeletePassphrase return ret = %d\n", ret);

    tas_CreateRSAKeyPair_REQ req3;
    tas_CreateRSAKeyPair_RES res3;

    memset(&req3, 0, sizeof(req3));
    memset(&res3, 0, sizeof(res3));

    req3.KeyLength = 2048;

    ret = onvif_tas_CreateRSAKeyPair(&p_device->onvif_device, &req3, &res3);

    printf("onvif_tas_CreateRSAKeyPair return ret = %d\n", ret);

    if (res3.EstimatedCreationTime > 0)
    {
        sleep(res3.EstimatedCreationTime);
    }

    tas_GetKeyStatus_REQ req4;
    tas_GetKeyStatus_RES res4;

    memset(&req4, 0, sizeof(req4));
    memset(&res4, 0, sizeof(res4));

    strcpy(req4.KeyID, res3.KeyID);

    ret = onvif_tas_GetKeyStatus(&p_device->onvif_device, &req4, &res4);

    printf("onvif_tas_GetKeyStatus return ret = %d\n", ret);

    tas_GetAllKeys_RES res5;

    memset(&res5, 0, sizeof(res5));
    
    ret = onvif_tas_GetAllKeys(&p_device->onvif_device, NULL, &res5);

    printf("onvif_tas_GetAllKeys return ret = %d\n", ret);

    tas_CreateSelfSignedCertificate_REQ req6;
    tas_CreateSelfSignedCertificate_RES res6;

    memset(&req6, 0, sizeof(req6));
    memset(&res6, 0, sizeof(res6));

    req6.Subject.sizeCountry = 1;
    strcpy(req6.Subject.Country[0], "CN");
    req6.Subject.sizeOrganization = 1;
    strcpy(req6.Subject.Organization[0], "HT");
    strcpy(req6.KeyID, res3.KeyID);
    strcpy(req6.SignatureAlgorithm.algorithm, "1.2.840.113549.1.1.5");

    ret = onvif_tas_CreateSelfSignedCertificate(&p_device->onvif_device, &req6, &res6);

    printf("onvif_tas_CreateSelfSignedCertificate return ret = %d\n", ret);

    tas_GetAllCertificates_RES res7;

    memset(&res7, 0, sizeof(res7));
    
    ret = onvif_tas_GetAllCertificates(&p_device->onvif_device, NULL, &res7);

    printf("onvif_tas_GetAllCertificates return ret = %d\n", ret);

    for (i = 0; i < res7.sizeCertificate; i++)
    {
        onvif_free_X509Certificate(&res7.Certificate[i]);
    }

    tas_GetCertificate_REQ req8;
    tas_GetCertificate_RES res8;

    memset(&req8, 0, sizeof(req8));
    memset(&res8, 0, sizeof(res8));

    strcpy(req8.CertificateID, res6.CertificateID);
    
    ret = onvif_tas_GetCertificate(&p_device->onvif_device, &req8, &res8);

    printf("onvif_tas_GetCertificate return ret = %d\n", ret);

    onvif_free_X509Certificate(&res8.Certificate);

    tas_DeleteCertificate_REQ req9;

    memset(&req9, 0, sizeof(req9));

    strcpy(req9.CertificateID, res6.CertificateID);

    ret = onvif_tas_DeleteCertificate(&p_device->onvif_device, &req9, NULL);
    
    printf("onvif_tas_GetCertificate return ret = %d\n", ret);
    
    tas_DeleteKey_REQ req10;

    memset(&req10, 0, sizeof(req10));

    strcpy(req10.KeyID, res3.KeyID);

    ret = onvif_tas_DeleteKey(&p_device->onvif_device, &req10, NULL);

    printf("onvif_tas_DeleteKey return ret = %d\n", ret);
}

#endif // end of SECURITY_SUPPORT

void onvifSystemMaintainTest(ONVIF_DEVICE_EX * p_device)
{
    BOOL ret;

    ret = SystemBackup(&p_device->onvif_device, "onvifsystem.backup");

    printf("SystemBackup return ret = %d\n", ret);

    if (ret)
    {
        //ret = SystemRestore(&p_device->onvif_device, "onvifsystem.backup");
        //printf("SystemRestore return ret = %d\n", ret);
    }

    tds_GetSystemLog_REQ req;
    tds_GetSystemLog_RES res;

    req.LogType = SystemLogType_System;

    memset(&res, 0, sizeof(res));
    
    ret = onvif_tds_GetSystemLog(&p_device->onvif_device, &req, &res);

    printf("onvif_tds_GetSystemLog return ret = %d\n", ret);

    if (res.String)
    {
        FreeBuff(res.String);
        res.String = NULL;
    }

    req.LogType = SystemLogType_Access;
    
    ret = onvif_tds_GetSystemLog(&p_device->onvif_device, &req, &res);

    printf("onvif_tds_GetSystemLog return ret = %d\n", ret);

    if (res.String)
    {
        FreeBuff(res.String);
        res.String = NULL;
    }
}

BOOL parseEventTopic1(XMLN * p_node, LKLIST * p_list)
{
    BOOL ret = FALSE;
    XMLN * p_child = p_node;
    
    while (p_child)
    {
        //if (p_child->f_attrib && 
        //    p_child->f_attrib->name && 
        //    p_child->f_attrib->data &&
        //    soap_strcmp(p_child->f_attrib->name, "topic") == 0 &&
        //    strcasecmp(p_child->f_attrib->data, "true") == 0)
        if (soap_strcmp(p_child->name, "MessageDescription") != 0)
        {
            ret = TRUE;
            
            hlist_add_at_back(p_list, p_child);
            
            p_child = p_child->f_child;
        }
        else 
        {
            break;
        }
    }

    return ret;
}

void printEventTopic(LKLIST * p_list)
{
    char topic[512] = {'\0'};
    
    LKNODE * p_node = hlist_lookup_start(p_list);
    while (p_node)
    {
        XMLN * p_xmln = (XMLN *)p_node->data;
        
        if (topic[0] != '\0')
        {
            strcat(topic, "/");
            strcat(topic, p_xmln->name);
        }
        else
        {
            strcpy(topic, p_xmln->name);
        }
        
        p_node = hlist_lookup_next(p_list, p_node);
    }
    hlist_lookup_end(p_list);

    printf("topic : %s\r\n", topic);
}

void parseEventTopic(char * xml)
{
    LKLIST * p_list = hlist_create(FALSE);
    
    XMLN * p_root = xml_parse(xml, (int)strlen(xml));
    if (p_root)
    {
        XMLN * p_child = p_root->f_child;
        while (p_child)
        {
            if (parseEventTopic1(p_child, p_list))
            {
                printEventTopic(p_list);
            }

            LKNODE * p_node = hlist_get_from_back(p_list);
            if (p_node)
            {
                p_child = (XMLN *)p_node->data;
                
                hlist_remove_from_back(p_list);
                
                if (p_child)
                {
                    p_child = p_child->next;
                }

                while (NULL == p_child)
                {
                    p_node = hlist_get_from_back(p_list);
                    if (p_node)
                    {
                        p_child = (XMLN *)p_node->data;
                        
                        hlist_remove_from_back(p_list);
                        
                        if (p_child)
                        {
                            p_child = p_child->next;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                p_child = p_child->next;
            }
        }
    }

    xml_node_del(p_root);

    hlist_destroy(p_list);
}

void onvifEventTest(ONVIF_DEVICE_EX * p_device)
{
    BOOL ret;
    
    tev_GetEventProperties_RES res;
    memset(&res, 0, sizeof(res));

    ret = onvif_tev_GetEventProperties(&p_device->onvif_device, NULL, &res);

    printf("onvif_tev_GetEventProperties return ret = %d\r\n", ret);

    if (ret)
    {
        parseEventTopic(res.TopicSet);
    }

    // printf("TopicSet = %s\r\n", res.TopicSet);
    
    FreeBuff(res.TopicSet);

    if (p_device->onvif_device.Capabilities.events.WSPullPointSupport == 1)
    {
        ret = CreatePullPointSubscription(&p_device->onvif_device);

        printf("CreatePullPointSubscription return ret = %d\r\n", ret);

        tev_PullMessages_RES res2;
        memset(&res2, 0, sizeof(res2));
        
        ret = PullMessages(&p_device->onvif_device, 60, 1, &res2);

        printf("PullMessages return ret = %d\r\n", ret);

        onvif_free_NotificationMessages(&res2.NotifyMessages);
    }
}

void onvifPTZTest(ONVIF_DEVICE_EX * p_device)
{
    BOOL ret;

    if (p_device->onvif_device.curProfile->ptz_cfg)
    {
        // GetConfigurationOptions
        ptz_GetConfigurationOptions_REQ req;
        ptz_GetConfigurationOptions_RES res;

        memset(&res, 0, sizeof(res));
        strcpy(req.ConfigurationToken, p_device->onvif_device.curProfile->ptz_cfg->Configuration.token);
        
        ret = onvif_ptz_GetConfigurationOptions(&p_device->onvif_device, &req, &res);

        printf("onvif_ptz_GetConfigurationOptions return ret = %d\n", ret);
    }    

    if (p_device->onvif_device.ptz_node)
    {
        // GetNode
        ptz_GetNode_REQ req1;
        ptz_GetNode_RES res1;

        memset(&req1, 0, sizeof(req1));
        memset(&res1, 0, sizeof(res1));

        strcpy(req1.NodeToken, p_device->onvif_device.ptz_node->PTZNode.token);

        ret = onvif_ptz_GetNode(&p_device->onvif_device, &req1, &res1);

        printf("onvif_ptz_GetNode return ret = %d\n", ret);

        if (p_device->onvif_device.ptz_node->PTZNode.GeoMove && 
            p_device->onvif_device.profiles)
        {
            // GeoMove
            ptz_GeoMove_REQ req2;
            memset(&req2, 0, sizeof(req2));

            strcpy(req2.ProfileToken, p_device->onvif_device.profiles->token);
            req2.Target.lonFlag = 1;
            req2.Target.lon = 20.2;
            req2.Target.latFlag = 1;
            req2.Target.lat = 84.3;
            req2.Target.elevationFlag = 1;
            req2.Target.elevation = 100.0;

            ret = onvif_ptz_GeoMove(&p_device->onvif_device, &req2, NULL);

            printf("onvif_ptz_GeoMove return ret = %d\n", ret);
        }
    }
}

void onvifPTZTest2(ONVIF_DEVICE_EX * p_device)
{
    BOOL ret;

    // CreatePresetTour
    ptz_CreatePresetTour_REQ req;
    ptz_CreatePresetTour_RES res;

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    strcpy(req.ProfileToken, p_device->onvif_device.curProfile->token);

    ret = onvif_ptz_CreatePresetTour(&p_device->onvif_device, &req, &res);

    printf("onvif_ptz_CreatePresetTour return ret = %d\n", ret);

    if (!ret)
    {
        return;
    }
    
    // GetPresetTours
    ptz_GetPresetTours_REQ req1;
    ptz_GetPresetTours_RES res1;

    memset(&req1, 0, sizeof(req1));
    memset(&res1, 0, sizeof(res1));

    strcpy(req1.ProfileToken, p_device->onvif_device.curProfile->token);

    ret = onvif_ptz_GetPresetTours(&p_device->onvif_device, &req1, &res1);

    printf("onvif_ptz_GetPresetTours return ret = %d\n", ret);

    onvif_free_PresetTours(&res1.PresetTour);

    // GetPresetTour
    ptz_GetPresetTour_REQ req2;
    ptz_GetPresetTour_RES res2;

    memset(&req2, 0, sizeof(req2));
    memset(&res2, 0, sizeof(res2));

    strcpy(req2.ProfileToken, p_device->onvif_device.curProfile->token);
    strcpy(req2.PresetTourToken, res.PresetTourToken);

    ret = onvif_ptz_GetPresetTour(&p_device->onvif_device, &req2, &res2);

    printf("onvif_ptz_GetPresetTour return ret = %d\n", ret);

    // ModifyPresetTour
    ptz_ModifyPresetTour_REQ req3;

    memset(&req3, 0, sizeof(req3));

    strcpy(req3.ProfileToken, p_device->onvif_device.curProfile->token);
    memcpy(&req3.PresetTour, &res2.PresetTour, sizeof(onvif_PresetTour));

    strcpy(req3.PresetTour.Name, "test");

    ret = onvif_ptz_ModifyPresetTour(&p_device->onvif_device, &req3, NULL);

    printf("onvif_ptz_ModifyPresetTour return ret = %d\n", ret);

    // OperatePresetTour
    ptz_OperatePresetTour_REQ req4;

    memset(&req4, 0, sizeof(req4));

    strcpy(req4.ProfileToken, p_device->onvif_device.curProfile->token);
    strcpy(req4.PresetTourToken, res.PresetTourToken);

    req4.Operation = PTZPresetTourOperation_Start;

    ret = onvif_ptz_OperatePresetTour(&p_device->onvif_device, &req4, NULL);

    printf("onvif_ptz_OperatePresetTour return ret = %d\n", ret);

    // GetPresetTourOptions
    ptz_GetPresetTourOptions_REQ req5;
    ptz_GetPresetTourOptions_RES res5;

    memset(&req5, 0, sizeof(req5));
    memset(&res5, 0, sizeof(res5));

    strcpy(req5.ProfileToken, p_device->onvif_device.curProfile->token);
    strcpy(req5.PresetTourToken, res.PresetTourToken);

    ret = onvif_ptz_GetPresetTourOptions(&p_device->onvif_device, &req5, &res5);

    printf("onvif_ptz_GetPresetTourOptions return ret = %d\n", ret);

    // GetPresetTour
    ptz_GetPresetTour_REQ req6;
    ptz_GetPresetTour_RES res6;

    memset(&req6, 0, sizeof(req6));
    memset(&res6, 0, sizeof(res6));

    strcpy(req6.ProfileToken, p_device->onvif_device.curProfile->token);
    strcpy(req6.PresetTourToken, res.PresetTourToken);

    ret = onvif_ptz_GetPresetTour(&p_device->onvif_device, &req6, &res6);

    printf("onvif_ptz_GetPresetTour return ret = %d\n", ret);

    // RemovePresetTour
    ptz_RemovePresetTour_REQ req7;

    memset(&req7, 0, sizeof(req7));

    strcpy(req7.ProfileToken, p_device->onvif_device.curProfile->token);
    strcpy(req7.PresetTourToken, res.PresetTourToken);

    ret = onvif_ptz_RemovePresetTour(&p_device->onvif_device, &req7, NULL);

    printf("onvif_ptz_RemovePresetTour return ret = %d\n", ret);
}

BOOL mediaProfile2Profile(ONVIF_DEVICE_EX * p_device)
{
    onvif_free_profiles(&p_device->onvif_device.profiles);

    MediaProfileList * p_mediaProfile = p_device->onvif_device.media_profiles;
    while (p_mediaProfile)
    {
        ONVIF_PROFILE * p_profile = onvif_add_profile(&p_device->onvif_device.profiles);
        if (p_profile)
        {
            p_profile->fixed = p_mediaProfile->MediaProfile.fixed;
            strcpy(p_profile->name, p_mediaProfile->MediaProfile.Name);
            strcpy(p_profile->token, p_mediaProfile->MediaProfile.token);
            strcpy(p_profile->stream_uri, p_mediaProfile->MediaProfile.stream_uri);
        }

        p_mediaProfile = p_mediaProfile->next;
    }

    return TRUE;
}

BOOL getDevInfo1(ONVIF_DEVICE_EX * p_device)
{
    if (!GetProfiles(&p_device->onvif_device))
    {
        return FALSE;
    }
    
    if (!GetStreamUris(&p_device->onvif_device, TransportProtocol_RTSP))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL getDevInfo2(ONVIF_DEVICE_EX * p_device)
{
    if (!tr2_GetProfiles(&p_device->onvif_device))
    {
        return FALSE;
    }
    
    if (!tr2_GetStreamUris(&p_device->onvif_device, "RTSP"))
    {
        return FALSE;
    }

    mediaProfile2Profile(p_device);

    return TRUE;
}

void * getDevInfoThread(void * argv)
{
    char profileToken[ONVIF_TOKEN_LEN];
    ONVIF_PROFILE   * p_profile = NULL;
    ONVIF_DEVICE_EX * p_device = (ONVIF_DEVICE_EX *) argv;

#define AUTH_FAILED_EXIT    if (p_device->onvif_device.authFailed) goto FAILED;
#define CONN_FAILED_EXIT    if (p_device->onvif_device.errCode == ONVIF_ERR_ConnFailure) goto FAILED;

    if (!GetSystemDateAndTime(&p_device->onvif_device))
    {
        CONN_FAILED_EXIT;
        AUTH_FAILED_EXIT;
    }

    if (!GetEndpointReference(&p_device->onvif_device))
    {
        CONN_FAILED_EXIT;
        AUTH_FAILED_EXIT;
    }
    
    if (!GetCapabilities(&p_device->onvif_device))
    {
        CONN_FAILED_EXIT;
        AUTH_FAILED_EXIT;
    }

    if (!GetServices(&p_device->onvif_device))
    {
        CONN_FAILED_EXIT;
        AUTH_FAILED_EXIT;
    }

    if (!GetDeviceInformation(&p_device->onvif_device))
    {
        CONN_FAILED_EXIT;
        AUTH_FAILED_EXIT;
    }

    if (!GetVideoSources(&p_device->onvif_device))
    {
        CONN_FAILED_EXIT;
        AUTH_FAILED_EXIT;
    }
    
    if (!GetImagingSettings(&p_device->onvif_device))
    {
        CONN_FAILED_EXIT;
        AUTH_FAILED_EXIT;
    }
    
    if (!GetVideoSourceConfigurations(&p_device->onvif_device))
    {
        CONN_FAILED_EXIT;
        AUTH_FAILED_EXIT;
    }

    if (!GetVideoEncoderConfigurations(&p_device->onvif_device))
    {
        CONN_FAILED_EXIT;
        AUTH_FAILED_EXIT;
    }
    
    if (GetAudioSources(&p_device->onvif_device))
    {
        if (!GetAudioSourceConfigurations(&p_device->onvif_device))
        {
            CONN_FAILED_EXIT;
            AUTH_FAILED_EXIT;
        }
    
        if (!GetAudioEncoderConfigurations(&p_device->onvif_device))
        {
            CONN_FAILED_EXIT;
            AUTH_FAILED_EXIT;
        }
    }

    if (p_device->onvif_device.Capabilities.ptz.support)
    {
        if (!GetNodes(&p_device->onvif_device))
        {
            CONN_FAILED_EXIT;
            AUTH_FAILED_EXIT;
        }

        if (!GetConfigurations(&p_device->onvif_device))
        {
            CONN_FAILED_EXIT;
            AUTH_FAILED_EXIT;
        }
    }
    
    /* save currrent profile token */
    
    if (p_device->onvif_device.curProfile)
    {
        strcpy(profileToken, p_device->onvif_device.curProfile->token);
    }
    else
    {
        memset(profileToken, 0, sizeof(profileToken));
    }

    if (p_device->onvif_device.Capabilities.media2.support)
    {
        if (!getDevInfo2(p_device))
        {
            getDevInfo1(p_device);
        }
    }
    else
    {
        getDevInfo1(p_device);
    }

    /* resume current profile */    
    if (profileToken[0] != '\0')
    {
        p_profile = onvif_find_profile(p_device->onvif_device.profiles, profileToken);
    }

    if (NULL == p_profile)
    {
        p_profile = p_device->onvif_device.profiles;
    }

    p_device->onvif_device.curProfile = p_profile;

    if (p_device->onvif_device.curProfile == NULL)
    {
        p_device->thread_handler = 0;
        return NULL;
    }

    if (p_profile)
    {
        int len;
        uint8 * buff;
        
        if (GetSnapshot(&p_device->onvif_device, p_profile->token, &buff, &len))
        {
            if (p_device->snapshot)
            {
                FreeBuff(p_device->snapshot);
                p_device->snapshot = NULL;
                p_device->snapshot_len = 0;
            }

            p_device->snapshot = buff;
            p_device->snapshot_len = len;

#if 0
            // save the snapshot to file, for test
            char name[256] = {'\0'};
            snprintf(name, sizeof(name), "%d.jpg", len);
            FILE * file = fopen(name, "wb");
            fwrite(p_buff, sizeof(char), len, file);
            fclose(file);
#endif
        }
    }

    onvifServiceCapabilitiesTest(p_device);
    
    if (p_device->onvif_device.Capabilities.events.support == 1)
    {
        onvifEventTest(p_device);
    }
    
    if (p_device->onvif_device.Capabilities.events.support == 1 && 
        p_device->onvif_device.events.subscribe == FALSE)
    {
        Subscribe(&p_device->onvif_device, pps_get_index(m_dev_ul, p_device));
    }

    if (p_device->onvif_device.Capabilities.ptz.support)
    {
        onvifPTZTest(p_device);

        onvifPTZTest2(p_device);
    }

    onvifNetworkTest(p_device);

    onvifMediaTest(p_device);
    
    onvifUserTest(p_device);

    onvifGeoLocationTest(p_device);

    if (p_device->onvif_device.Capabilities.image.support)
    {
        onvifImageTest(p_device);
    }

#ifdef DEVICEIO_SUPPORT
    if (p_device->onvif_device.Capabilities.deviceIO.support)
    {
        onvifDeviceIOTest(p_device);
    }
#endif

    if (p_device->onvif_device.Capabilities.media2.support)
    {
        onvifMedia2Test(p_device);

        if (p_device->onvif_device.Capabilities.media2.OSD)
        {    
            onvifMedia2OSDTest(p_device);
        }
        
        if (p_device->onvif_device.Capabilities.media2.Mask)
        {    
            onvifMedia2MaskTest(p_device);
        }
    }

    if (p_device->onvif_device.Capabilities.analytics.support)
    {
        onvifAnalyticsTest(p_device);
    }

#ifdef PROFILE_C_SUPPORT
    if (p_device->onvif_device.Capabilities.accesscontrol.support)
    {
        onvifAccessControlTest(p_device);
    }

    if (p_device->onvif_device.Capabilities.doorcontrol.support)
    {
        onvifDoorControlTest(p_device);
    }
#endif

#ifdef PROFILE_G_SUPPORT
    if (p_device->onvif_device.Capabilities.recording.support)
    {
        onvifRecordingTest(p_device);
    }

    if (p_device->onvif_device.Capabilities.search.support)
    {
        onvifSearchTest(p_device);
    }

    if (p_device->onvif_device.Capabilities.replay.support)
    {
        onvifReplayTest(p_device);
    }
#endif    

#ifdef THERMAL_SUPPORT
    if (p_device->onvif_device.Capabilities.thermal.support)
    {
        onvifThermalTest(p_device);
    }
#endif

#ifdef CREDENTIAL_SUPPORT
    if (p_device->onvif_device.Capabilities.credential.support)
    {
        onvifCredentialTest(p_device);
    }
#endif

#ifdef ACCESS_RULES
    if (p_device->onvif_device.Capabilities.accessrules.support)
    {
        onvifAccessRulesTest(p_device);
    }
#endif    

#ifdef SCHEDULE_SUPPORT
    if (p_device->onvif_device.Capabilities.schedule.support)
    {
        onvifScheduleTest(p_device);
        onvifSpecialDayGroupTest(p_device);
    }
#endif    

#ifdef RECEIVER_SUPPORT
    if (p_device->onvif_device.Capabilities.receiver.support)
    {
        onvifReceiverTest(p_device);
    }
#endif    

#ifdef IPFILTER_SUPPORT
    if (p_device->onvif_device.Capabilities.device.IPFilter)
    {
        onvifIPAddressFilterTest(p_device);
    }
#endif

#ifdef PROVISIONING_SUPPORT
    if (p_device->onvif_device.Capabilities.provisioning.support)
    {
        onvifProvisioningTest(p_device);
    }
#endif

#ifdef SECURITY_SUPPORT
    if (p_device->onvif_device.Capabilities.security.support)
    {
        onvifSecurityTest(p_device);
    }
#endif

    onvifSystemMaintainTest(p_device);

FAILED:

    p_device->thread_handler = 0;
    
    return NULL;
}

ONVIF_DEVICE_EX * findDeviceByNotify(Notify_REQ * p_notify)
{
    int    index = -1;
    ONVIF_DEVICE_EX * p_dev = NULL;

    sscanf(p_notify->PostUrl, "/subscription%d", &index);
    if (index >= 0)
    {
        p_dev = (ONVIF_DEVICE_EX *) pps_get_node_by_index(m_dev_ul, index);
    }
    
    return p_dev;
}

void updateDevice(ONVIF_DEVICE_EX * p_device)
{
    if (NULL == p_device)
    {
        return;
    }

    // The current thread has not ended
    if (p_device->thread_handler)
    {
        return;
    }

    BOOL need_update = FALSE;
    
    if (p_device->need_update)
    {
        need_update = TRUE;
    }
    
    if (!p_device->onvif_device.authFailed)
    {
        if (NULL == p_device->onvif_device.curProfile)
        {
            need_update = TRUE;
        }
        else if (p_device->onvif_device.curProfile->stream_uri[0] == '\0')
        {
            need_update = TRUE;
        }
    }
    
    if (need_update)
    {
        p_device->need_update = 0;
        p_device->thread_handler = sys_os_create_thread((void *)getDevInfoThread, p_device);
        if (p_device->thread_handler)
        {
            sys_os_detach_thread(p_device->thread_handler);
        }
    }
}

/**
 * devices state detect threand
 */
void stateDetectThread(void * argv)
{
    int count = 0;
    
    while (m_bStateDetect)
    {
        if (count++ < 30)
        {
            sleep(1);
            continue;
        }
        else
        {
            count = 0;
        }

        int deviceNums = pps_node_count(m_dev_ul);

        for (int i = 0; i < deviceNums; i++)
        {
            if (!m_bStateDetect)
            {
                break;
            }
            
            ONVIF_DEVICE_EX * p_device = (ONVIF_DEVICE_EX *) pps_get_node_by_index(m_dev_ul, i);
            if (p_device)
            {
                int state = p_device->state;

                GetSystemDateAndTime(&p_device->onvif_device);
                
                if (p_device->onvif_device.errCode == ONVIF_ERR_ConnFailure)
                {
                    p_device->state = 0;
                }
                else
                {
                    p_device->state = 1;
                }

                if (state != p_device->state)
                {
                    if (p_device->state)
                    {
                        updateDevice(p_device);
                    }
                }
            }
        }
    }
}

/**
 * start devices state detect
 */
void startStateDetect()
{
    m_bStateDetect = TRUE;
    m_hStateDetect = sys_os_create_thread((void *)stateDetectThread, NULL);
}

/**
 * stop devices state detect
 */
void stopStateDetect()
{
    m_bStateDetect = FALSE;
    sys_os_wait_thread(&m_hStateDetect);
}

/**
 * onvif device probed callback 
 */
void probeCallback(DEVICE_BINFO * p_res, int msgtype, void * p_data)
{
    ONVIF_DEVICE_EX * p_dev = NULL;    
    ONVIF_DEVICE_EX device;
    memset(&device, 0, sizeof(ONVIF_DEVICE_EX));

    if (msgtype == PROBE_MSGTYPE_MATCH || msgtype == PROBE_MSGTYPE_HELLO)
    {
        memcpy(&device.onvif_device.binfo, p_res, sizeof(DEVICE_BINFO));
        device.state = 1;

        p_dev = findDevice(&device);
        if (p_dev == NULL)
        {
            printf("Found device. ip : %s, port : %d\n", p_res->XAddr.host, p_res->XAddr.port);

            p_dev = addDevice(&device);
            if (p_dev)
            {
                updateDevice(p_dev);
            }
        }
        else 
        {
            updateDevice(p_dev);
        }
    }
    else if (msgtype == PROBE_MSGTYPE_BYE)
    {
        p_dev = findDeviceByEndpointReference(p_res->EndpointReference);
        if (p_dev)
        {
            p_dev->state = 0;
        }
    }
}

/**
 * onvif event notify callback 
 */
void eventNotifyCallback(Notify_REQ * p_req, void * p_data)
{
    ONVIF_DEVICE_EX * p_dev;
    NotificationMessageList * p_notify = p_req->notify;

    p_dev = findDeviceByNotify(p_req);
    if (p_dev)
    {
        onvif_free_NotificationMessages(&p_req->notify);
        return;
    }
    
    onvif_device_add_NotificationMessages(&p_dev->onvif_device, p_notify);

    p_dev->onvif_device.events.notify_nums += onvif_get_NotificationMessages_nums(p_notify);

    // max save 100 event notify
    if (p_dev->onvif_device.events.notify_nums > 100)
    {
        int nums = onvif_device_free_NotificationMessages(&p_dev->onvif_device, 
                        p_dev->onvif_device.events.notify_nums - 100);
        p_dev->onvif_device.events.notify_nums -= nums;
    }
}

/**
 * onvif event subscribe disconnect callback
 */
void subscribeDisconnectCallback(ONVIF_DEVICE * p_dev, void * p_data)
{
    printf("\r\nsubscribeDisconnectCallback, %s\r\n", p_dev->binfo.XAddr.host);
}

/**
 * free device memory 
 */
void freeDevice(ONVIF_DEVICE_EX * p_dev)
{
    if (NULL == p_dev)
    {
        return;
    }

    while (p_dev->thread_handler)
    {
        usleep(10*1000);
    }
    
    onvif_free_device(&p_dev->onvif_device);
    
    if (p_dev->snapshot)
    {
        FreeBuff(p_dev->snapshot);
        p_dev->snapshot = NULL;
        p_dev->snapshot_len = 0;
    }
}

void clearDevices()
{
    ONVIF_DEVICE_EX * next_dev;
    ONVIF_DEVICE_EX * dev = (ONVIF_DEVICE_EX *) pps_lookup_start(m_dev_ul);
    while (dev)
    {
        next_dev = (ONVIF_DEVICE_EX *) pps_lookup_next(m_dev_ul, dev);

        freeDevice(dev);
        
        dev = next_dev;
    }

    pps_lookup_end(m_dev_ul);    
}

void ptzLeftUpper(ONVIF_DEVICE_EX * p_dev)
{
    if (NULL == p_dev || 
        !p_dev->onvif_device.Capabilities.ptz.support || 
        NULL == p_dev->onvif_device.curProfile)
    {
        return;
    }
    
    ptz_ContinuousMove_REQ req;
    memset(&req, 0, sizeof(req));

    req.Velocity.PanTiltFlag = 1;
    req.Velocity.PanTilt.x = -0.5;
    req.Velocity.PanTilt.y = 0.5;
    strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

    onvif_ptz_ContinuousMove(&p_dev->onvif_device, &req, NULL);
}

void ptzUpper(ONVIF_DEVICE_EX * p_dev)
{
    if (NULL == p_dev || 
        !p_dev->onvif_device.Capabilities.ptz.support || 
        NULL == p_dev->onvif_device.curProfile)
    {
        return;
    }

    ptz_ContinuousMove_REQ req;
    memset(&req, 0, sizeof(req));

    req.Velocity.PanTiltFlag = 1;
    req.Velocity.PanTilt.x = 0;
    req.Velocity.PanTilt.y = 0.5;
    strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

    onvif_ptz_ContinuousMove(&p_dev->onvif_device, &req, NULL);
}

void ptzRightUpper(ONVIF_DEVICE_EX * p_dev)
{
    if (NULL == p_dev || 
        !p_dev->onvif_device.Capabilities.ptz.support || 
        NULL == p_dev->onvif_device.curProfile)
    {
        return;
    }

    ptz_ContinuousMove_REQ req;
    memset(&req, 0, sizeof(req));

    req.Velocity.PanTiltFlag = 1;
    req.Velocity.PanTilt.x = 0.5;
    req.Velocity.PanTilt.y = 0.5;
    strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

    onvif_ptz_ContinuousMove(&p_dev->onvif_device, &req, NULL);
}

void ptzPTZLeft(ONVIF_DEVICE_EX * p_dev)
{
    if (NULL == p_dev || 
        !p_dev->onvif_device.Capabilities.ptz.support || 
        NULL == p_dev->onvif_device.curProfile)
    {
        return;
    }
    
    ptz_ContinuousMove_REQ req;
    memset(&req, 0, sizeof(req));

    req.Velocity.PanTiltFlag = 1;
    req.Velocity.PanTilt.x = -0.5;
    req.Velocity.PanTilt.y = 0;
    strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

    onvif_ptz_ContinuousMove(&p_dev->onvif_device, &req, NULL);
}

void ptzRight(ONVIF_DEVICE_EX * p_dev)
{
    if (NULL == p_dev || 
        !p_dev->onvif_device.Capabilities.ptz.support || 
        NULL == p_dev->onvif_device.curProfile)
    {
        return;
    }
    
    ptz_ContinuousMove_REQ req;
    memset(&req, 0, sizeof(req));

    req.Velocity.PanTiltFlag = 1;
    req.Velocity.PanTilt.x = 0.5;
    req.Velocity.PanTilt.y = 0;
    strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

    onvif_ptz_ContinuousMove(&p_dev->onvif_device, &req, NULL);
}

void ptzLeftDown(ONVIF_DEVICE_EX * p_dev)
{
    if (NULL == p_dev || 
        !p_dev->onvif_device.Capabilities.ptz.support || 
        NULL == p_dev->onvif_device.curProfile)
    {
        return;
    }

    ptz_ContinuousMove_REQ req;
    memset(&req, 0, sizeof(req));

    req.Velocity.PanTiltFlag = 1;
    req.Velocity.PanTilt.x = -0.5;
    req.Velocity.PanTilt.y = -0.5;
    strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

    onvif_ptz_ContinuousMove(&p_dev->onvif_device, &req, NULL);
}

void ptzDown(ONVIF_DEVICE_EX * p_dev)
{
    if (NULL == p_dev || 
        !p_dev->onvif_device.Capabilities.ptz.support || 
        NULL == p_dev->onvif_device.curProfile)
    {
        return;
    }
    
    ptz_ContinuousMove_REQ req;
    memset(&req, 0, sizeof(req));

    req.Velocity.PanTiltFlag = 1;
    req.Velocity.PanTilt.x = 0;
    req.Velocity.PanTilt.y = -0.5;
    strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

    onvif_ptz_ContinuousMove(&p_dev->onvif_device, &req, NULL);
}

void ptzRightDown(ONVIF_DEVICE_EX * p_dev)
{
    if (NULL == p_dev || 
        !p_dev->onvif_device.Capabilities.ptz.support || 
        NULL == p_dev->onvif_device.curProfile)
    {
        return;
    }
    
    ptz_ContinuousMove_REQ req;
    memset(&req, 0, sizeof(req));

    req.Velocity.PanTiltFlag = 1;
    req.Velocity.PanTilt.x = 0.5;
    req.Velocity.PanTilt.y = -0.5;
    strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

    onvif_ptz_ContinuousMove(&p_dev->onvif_device, &req, NULL);
}

void ptzZoomIn(ONVIF_DEVICE_EX * p_dev)
{
    if (NULL == p_dev || 
        !p_dev->onvif_device.Capabilities.ptz.support || 
        NULL == p_dev->onvif_device.curProfile)
    {
        return;
    }

    ptz_ContinuousMove_REQ req;
    memset(&req, 0, sizeof(req));

    req.Velocity.ZoomFlag = 1;
    req.Velocity.Zoom.x = -0.5;
    strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

    onvif_ptz_ContinuousMove(&p_dev->onvif_device, &req, NULL);
}

void ptzZoomOut(ONVIF_DEVICE_EX * p_dev)
{
    if (NULL == p_dev || 
        !p_dev->onvif_device.Capabilities.ptz.support || 
        NULL == p_dev->onvif_device.curProfile)
    {
        return;
    }

    ptz_ContinuousMove_REQ req;
    memset(&req, 0, sizeof(req));

    req.Velocity.ZoomFlag = 1;
    req.Velocity.Zoom.x = 0.5;
    strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

    onvif_ptz_ContinuousMove(&p_dev->onvif_device, &req, NULL);
}

void ptzHome(ONVIF_DEVICE_EX * p_dev)
{
    if (NULL == p_dev || 
        !p_dev->onvif_device.Capabilities.ptz.support || 
        NULL == p_dev->onvif_device.curProfile)
    {
        return;
    }

    ptz_GotoHomePosition_REQ req;
    memset(&req, 0, sizeof(req));

    strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);

    onvif_ptz_GotoHomePosition(&p_dev->onvif_device, &req, NULL);
}

void ptzStop(ONVIF_DEVICE_EX * p_dev)
{
    if (NULL == p_dev || 
        !p_dev->onvif_device.Capabilities.ptz.support || 
        NULL == p_dev->onvif_device.curProfile)
    {
        return;
    }
    
    ptz_Stop_REQ req;
    memset(&req, 0, sizeof(req));

    req.ZoomFlag = 1;
    req.Zoom = TRUE;
    req.PanTiltFlag = 1;
    req.PanTilt = TRUE;
    strcpy(req.ProfileToken, p_dev->onvif_device.curProfile->token);
    
    onvif_ptz_Stop(&p_dev->onvif_device, &req, NULL);
}


int main(int argc, char* argv[])
{
    network_init();

    // open log file
    log_init("onviftest.log");
    log_set_level(HT_LOG_DBG);

    // init sys buffer
    sys_buf_init(10 * MAX_DEV_NUMS);

    // init http message buffer
    http_msg_buf_init(10 * MAX_DEV_NUMS);

    // max support 100 devices
    m_dev_fl = pps_fl_init(100, sizeof(ONVIF_DEVICE_EX), TRUE);
    m_dev_ul = pps_ul_init(m_dev_fl, TRUE);

    // init event handler
    //  bind the http server to 0.0.0.0:30100 and https server to 0.0.0.0:30400
    // onvif_event_init(1, NULL, 30100, 1, NULL, 30400, "cert.pem", "key.pem", 1, MAX_DEV_NUMS);
    //  bind the http server to 0.0.0.0:30100
    onvif_event_init(1, NULL, 30100, 0, NULL, 0, NULL, NULL, 1, MAX_DEV_NUMS);
    
    // set event callback
    onvif_set_event_notify_cb(eventNotifyCallback, 0);
    
    // set event subscribe disconnect callback
    onvif_set_subscribe_disconnect_cb(subscribeDisconnectCallback, 0);

    // set probe callback
    set_probe_cb(probeCallback, 0);
    
    // start probe thread
    start_probe(NULL, 30);

    // start device state detect
    startStateDetect();
    
    for (;;)
    {
        if (getchar() == 'q')
        {
            break;
        }
        
        sleep(1);
    }    

    stopStateDetect();
    
    stop_probe();

    onvif_event_deinit();
    
    clearDevices();

    pps_ul_free(m_dev_ul);
    pps_fl_free(m_dev_fl);

    http_msg_buf_deinit();
    sys_buf_deinit();
    
    log_close();

    return 0;
}



