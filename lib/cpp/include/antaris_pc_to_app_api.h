/*
 * Copyright 2022 Antaris, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __ANTARIS_PC_TO_APP_API_H__
#define __ANTARIS_PC_TO_APP_API_H__

#include "antaris_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void * PCApiServerContext;
typedef void * PCToAppClientContext;

typedef enum {
    e_app2PC_invalid = 0,
    e_app2PC_register,
    e_app2PC_getCurrentLocation,
    e_app2PC_stageFileDownload,
    e_app2PC_sequenceDone,
    e_app2PC_payloadPowerControl,
    e_app2PC_healthCheckResponse,
    e_app2PC_shutdownResponse,
    e_app2PC_PayloadMetricsResponse,
    e_app2PC_sdkVersionInfo,
    e_app2PC_GnssEphStartReq,
    e_app2PC_GnssEphStopReq,
    e_app2PC_GetEpsVoltageStartReq,
    e_app2PC_GetEpsVoltageStopReq,
    e_app2PC_StartSesThermMgmntReq,
    e_app2PC_StopSesThermMgmntReq,
    e_app2PC_SesTempReq,
    e_app2PC_PaSatOsMsg,
    e_app2PC_PstoEsFcmOperation,
    e_app2PC_RespSatOsPaMsg,
    e_app2PC_MaxRequest
} AppToPCCallbackId_e;

typedef struct {
    UINT16 major;
    UINT16 minor;
    UINT16 patch;
} AntarisAppSdkVersion_t;

typedef union {
    ReqRegisterParams               register_request;
    ReqGetCurrentLocationParams     get_location;
    ReqStageFileDownloadParams      stage_file_download;
    CmdSequenceDoneParams           sequence_done;
    ReqPayloadPowerControlParams    payload_power_ctrl;
    RespHealthCheckParams           health_check_response;
    RespShutdownParams              shutdown_response;
    PayloadMetricsResponse          payload_metrics_respose;
    ReqGnssEphStartDataReq          gnss_eph_start;
    ReqGnssEphStopDataReq           gnss_eph_stop;
    GnssEphData                     gnss_eph_data;
    ReqGetEpsVoltageStartReq        get_eps_voltage_start;
    ReqGetEpsVoltageStopReq         get_eps_voltage_stop;
    GetEpsVoltage                   get_eps_voltage;
    StartSesThermMgmntReq           start_ses_therm_mgmnt;
    StopSesThermMgmntReq            stop_ses_therm_mgmnt;
    SesTempReq                      ses_temp_req;
    PaSatOsMsg                      pa_satos_msg;
    AntarisAppSdkVersion_t          sdk_version;
    HostToPeerFcmOperation          pstoes_fcm_operation;
    RespSatOsPaMsg                  resp_satos_pa_msg;
} AppToPCCallbackParams_t;

typedef    UINT16 SHORT_APP_ID_t;
#define AN_PS_APP_ID_INVALID       (SHORT_APP_ID_t)(-1)

typedef struct {
    SHORT_APP_ID_t      appId;
    char                auth_key[AUTH_KEY_LEN + 1];
}cookie_t;

typedef void (*PCAppCallbackFn_t)(PCApiServerContext, cookie_t cookie ,  AppToPCCallbackId_e, AppToPCCallbackParams_t *, AntarisReturnCode *);

PCApiServerContext an_pc_pa_create_server(UINT16 port, PCAppCallbackFn_t callback_fn, UINT32 ssl_flag);
void an_pc_pa_delete_server(PCApiServerContext ctx);

typedef enum {
    e_PC2App_invalid = 0,
    e_PC2App_startSequence,
    e_PC2App_shutdownApp,
    e_PC2App_responseRegister,
    e_PC2App_responseGetCurrentLocation,
    e_PC2App_responseStageFileDownload,
    e_PC2App_responsePayloadPowerControl,
    e_PC2App_processHealthCheck,
    e_PC2App_ReqPayloadMetrics,
    e_PC2App_GnssEphData,
    e_PC2App_responseGnssEphStartReq,
    e_PC2App_responseGnssEphStopReq,
    e_PC2App_GetEpsVoltage,
    e_PC2App_respGetEpsVoltageStartReq,
    e_PC2App_respGetEpsVoltageStopReq,
    e_PC2App_respStartSesThermMgmntReq,
    e_PC2App_respStopSesThermMgmntReq,
    e_PC2App_respSesTempReq,
    e_PC2App_SesThrmlNtf,
    e_PC2App_respPaSatOsMsg,
    e_PC2App_NtfRemoteAcPowerStatus,
    e_PC2App_PstoEsFcmOperationNotify,
    e_PC2App_SatOsPaMsg,
    e_PC2App_MaxRequest,
} PCToAppApiId_e;

typedef union {
    StartSequenceParams                 start_sequence;
    ShutdownParams                      shutdown;
    RespRegisterParams                  resp_register;
    RespGetCurrentLocationParams        resp_location;
    RespStageFileDownloadParams         resp_stage_file_download;
    RespPayloadPowerControlParams       resp_payload_power_ctrl;
    HealthCheckParams                   health_check;
    ReqPayloadMetricsParams             payload_stats;
    GnssEphData                         gnss_eph_data;
    RespGnssEphStartDataReq             gnss_eph_start;
    RespGnssEphStopDataReq              gnss_eph_stop;
    GetEpsVoltage                       get_eps_voltage;
    RespGetEpsVoltageStartReq           get_eps_voltage_start;
    RespGetEpsVoltageStopReq            get_eps_voltage_stop;
    RespStartSesThermMgmntReq           start_ses_therm_mgmnt;
    RespStopSesThermMgmntReq            stop_ses_therm_mgmnt;
    RespSesTempReqParams                ses_temp;
    SesThermalStatusNtf                 ses_thermal_ntf;
    RespPaSatOsMsg                      pa_satos_msg_response;
    NtfRemoteAcPwrStatus                remote_app_status;
    HostToPeerFcmOperationNotify        pstoes_fcm_operation_notify;
    SatOsPaMsg                          satos_pa_msg;
} PCToAppApiParams_t;

PCToAppClientContext an_pc_pa_create_client(INT8 *peer_ip_str, UINT16 port, INT8 *client_ssl_addr, UINT32 ssl_flag);
void an_pc_pa_delete_client(PCToAppClientContext ctx);
AntarisReturnCode an_pc_pa_invoke_api(PCToAppClientContext ctx, PCToAppApiId_e api_id, PCToAppApiParams_t *api_params);

#ifdef __cplusplus
}
#endif


#endif // __ANTARIS_PC_TO_APP_API_H__