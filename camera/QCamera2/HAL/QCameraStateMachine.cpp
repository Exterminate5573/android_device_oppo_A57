/* Copyright (c) 2012-2017, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#define LOG_TAG "QCameraStateMachine"

// System dependencies
#include <utils/Errors.h>
#include <stdio.h>

// Camera dependencies
#include "QCamera2HWI.h"
#include "QCameraStateMachine.h"

extern "C" {
#include "mm_camera_dbg.h"
}

namespace qcamera {

/*===========================================================================
 * FUNCTION   : smEvtProcRoutine
 *
 * DESCRIPTION: Statemachine process thread routine to handle events
 *              in different state.
 *
 * PARAMETERS :
 *   @data    : ptr to QCameraStateMachine object
 *
 * RETURN     : none
 *==========================================================================*/
void *QCameraStateMachine::smEvtProcRoutine(void *data)
{
    int running = 1, ret;
    QCameraStateMachine *pme = (QCameraStateMachine *)data;

    LOGH("E");
    do {
        do {
            ret = cam_sem_wait(&pme->cmd_sem);
            if (ret != 0 && errno != EINVAL) {
                LOGE("cam_sem_wait error (%s)",
                            strerror(errno));
                return NULL;
            }
        } while (ret != 0);

        // we got notified about new cmd avail in cmd queue
        // first check API cmd queue
        qcamera_sm_cmd_t *node = (qcamera_sm_cmd_t *)pme->api_queue.dequeue();
        if (node == NULL) {
            // no API cmd, then check evt cmd queue
            node = (qcamera_sm_cmd_t *)pme->evt_queue.dequeue();
        }
        if (node != NULL) {
            switch (node->cmd) {
            case QCAMERA_SM_CMD_TYPE_API:
                pme->stateMachine(node->evt, node->evt_payload);
                // API is in a way sync call, so evt_payload is managed by HWI
                // no need to free payload for API
                break;
            case QCAMERA_SM_CMD_TYPE_EVT:
                pme->stateMachine(node->evt, node->evt_payload);

                // EVT is async call, so payload need to be free after use
                free(node->evt_payload);
                node->evt_payload = NULL;
                break;
            case QCAMERA_SM_CMD_TYPE_EXIT:
                running = 0;
                break;
            default:
                break;
            }
            free(node);
            node = NULL;
        }
    } while (running);
    LOGH("X");
    return NULL;
}

/*===========================================================================
 * FUNCTION   : QCameraStateMachine
 *
 * DESCRIPTION: constructor of QCameraStateMachine. Will start process thread
 *
 * PARAMETERS :
 *   @ctrl    : ptr to HWI object
 *
 * RETURN     : none
 *==========================================================================*/
QCameraStateMachine::QCameraStateMachine(QCamera2HardwareInterface *ctrl) :
    api_queue(),
    evt_queue()
{
    m_parent = ctrl;
    m_state = QCAMERA_SM_STATE_PREVIEW_STOPPED;
    cmd_pid = 0;
    cam_sem_init(&cmd_sem, 0);
    pthread_create(&cmd_pid,
                   NULL,
                   smEvtProcRoutine,
                   this);
    pthread_setname_np(cmd_pid, "CAM_stMachine");
    m_bDelayPreviewMsgs = false;
    m_DelayedMsgs = 0;
    m_RestoreZSL = TRUE;
    m_bPreviewCallbackNeeded = TRUE;
}

/*===========================================================================
 * FUNCTION   : ~QCameraStateMachine
 *
 * DESCRIPTION: desctructor of QCameraStateMachine. Will stop process thread.
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCameraStateMachine::~QCameraStateMachine()
{
    cam_sem_destroy(&cmd_sem);
}

/*===========================================================================
 * FUNCTION   : releaseThread
 *
 * DESCRIPTION: Sends an exit command and terminates the state machine thread
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
void QCameraStateMachine::releaseThread()
{
    if (cmd_pid != 0) {
        qcamera_sm_cmd_t *node =
            (qcamera_sm_cmd_t *)malloc(sizeof(qcamera_sm_cmd_t));
        if (NULL != node) {
            memset(node, 0, sizeof(qcamera_sm_cmd_t));
            node->cmd = QCAMERA_SM_CMD_TYPE_EXIT;

            if (api_queue.enqueue((void *)node)) {
                cam_sem_post(&cmd_sem);
            } else {
                free(node);
                node = NULL;
            }

            /* wait until cmd thread exits */
            if (pthread_join(cmd_pid, NULL) != 0) {
                LOGW("pthread dead already\n");
            }
        }
        cmd_pid = 0;
    }
}

/*===========================================================================
 * FUNCTION   : applyDelayedMsgs
 *
 * DESCRIPTION: Enable if needed any delayed message types
 *
 * PARAMETERS : None
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraStateMachine::applyDelayedMsgs()
{
    int32_t rc = NO_ERROR;

    if (m_bDelayPreviewMsgs && m_DelayedMsgs) {
        rc = m_parent->enableMsgType(m_DelayedMsgs);
        m_bDelayPreviewMsgs = false;
        m_DelayedMsgs = 0;
    } else if (m_bDelayPreviewMsgs) {
        m_bDelayPreviewMsgs = false;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : procAPI
 *
 * DESCRIPTION: process incoming API request from framework layer.
 *
 * PARAMETERS :
 *   @evt          : event to be processed
 *   @api_payload  : API payload. Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraStateMachine::procAPI(qcamera_sm_evt_enum_t evt,
                                     void *api_payload)
{
    qcamera_sm_cmd_t *node =
        (qcamera_sm_cmd_t *)malloc(sizeof(qcamera_sm_cmd_t));
    if (NULL == node) {
        LOGE("No memory for qcamera_sm_cmd_t");
        return NO_MEMORY;
    }

    memset(node, 0, sizeof(qcamera_sm_cmd_t));
    node->cmd = QCAMERA_SM_CMD_TYPE_API;
    node->evt = evt;
    node->evt_payload = api_payload;
    if (api_queue.enqueue((void *)node)) {
        cam_sem_post(&cmd_sem);
        return NO_ERROR;
    } else {
        LOGE("API enqueue failed API = %d", evt);
        free(node);
        return UNKNOWN_ERROR;
    }
}

/*===========================================================================
 * FUNCTION   : procEvt
 *
 * DESCRIPTION: process incoming envent from mm-camera-interface and
 *              mm-jpeg-interface.
 *
 * PARAMETERS :
 *   @evt          : event to be processed
 *   @evt_payload  : event payload. Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraStateMachine::procEvt(qcamera_sm_evt_enum_t evt,
                                     void *evt_payload)
{
    qcamera_sm_cmd_t *node =
        (qcamera_sm_cmd_t *)malloc(sizeof(qcamera_sm_cmd_t));
    if (NULL == node) {
        LOGE("No memory for qcamera_sm_cmd_t");
        return NO_MEMORY;
    }

    memset(node, 0, sizeof(qcamera_sm_cmd_t));
    node->cmd = QCAMERA_SM_CMD_TYPE_EVT;
    node->evt = evt;
    node->evt_payload = evt_payload;
    if (evt_queue.enqueue((void *)node)) {
        cam_sem_post(&cmd_sem);
        return NO_ERROR;
    } else {
        LOGE("EVENT enqueue failed Event = %d", evt);
        free(node);
        return UNKNOWN_ERROR;
    }
}

/*===========================================================================
 * FUNCTION   : stateMachine
 *
 * DESCRIPTION: finite state machine entry function. Depends on state,
 *              incoming event will be handled differently.
 *
 * PARAMETERS :
 *   @evt      : event to be processed
 *   @payload  : event payload. Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraStateMachine::stateMachine(qcamera_sm_evt_enum_t evt, void *payload)
{
    int32_t rc = NO_ERROR;
    LOGL("m_state %d, event (%d)", m_state, evt);
    switch (m_state) {
    case QCAMERA_SM_STATE_PREVIEW_STOPPED:
        rc = procEvtPreviewStoppedState(evt, payload);
        break;
    case QCAMERA_SM_STATE_PREVIEW_READY:
        rc = procEvtPreviewReadyState(evt, payload);
        break;
    case QCAMERA_SM_STATE_PREVIEWING:
        rc = procEvtPreviewingState(evt, payload);
        break;
    case QCAMERA_SM_STATE_PREPARE_SNAPSHOT:
        rc = procEvtPrepareSnapshotState(evt, payload);
        break;
    case QCAMERA_SM_STATE_PIC_TAKING:
        rc = procEvtPicTakingState(evt, payload);
        break;
    case QCAMERA_SM_STATE_RECORDING:
        rc = procEvtRecordingState(evt, payload);
        break;
    case QCAMERA_SM_STATE_VIDEO_PIC_TAKING:
        rc = procEvtVideoPicTakingState(evt, payload);
        break;
    case QCAMERA_SM_STATE_PREVIEW_PIC_TAKING:
        rc = procEvtPreviewPicTakingState(evt, payload);
        break;
    default:
        break;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : procEvtPreviewStoppedState
 *
 * DESCRIPTION: finite state machine function to handle event in state of
 *              QCAMERA_SM_STATE_PREVIEW_STOPPED.
 *
 * PARAMETERS :
 *   @evt      : event to be processed
 *   @payload  : event payload. Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraStateMachine::procEvtPreviewStoppedState(qcamera_sm_evt_enum_t evt,
                                                        void *payload)
{
    int32_t rc = NO_ERROR;
    qcamera_api_result_t result;
    memset(&result, 0, sizeof(qcamera_api_result_t));

    LOGL("event (%d)", evt);
    switch (evt) {
    case QCAMERA_SM_EVT_SET_PREVIEW_WINDOW:
        {
            rc = m_parent->setPreviewWindow((struct preview_stream_ops *)payload);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_CALLBACKS:
        {
            qcamera_sm_evt_setcb_payload_t *setcbs =
                (qcamera_sm_evt_setcb_payload_t *)payload;
            rc = m_parent->setCallBacks(setcbs->notify_cb,
                                        setcbs->data_cb,
                                        setcbs->data_cb_timestamp,
                                        setcbs->get_memory,
                                        setcbs->user);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_ENABLE_MSG_TYPE:
        {
            rc = m_parent->waitDeferredWork(m_parent->mParamInitJob);
            if (NO_ERROR != rc) {
                LOGE("Param init deferred work failed");
            } else {
                rc = m_parent->enableMsgType(*((int32_t *)payload));
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_DISABLE_MSG_TYPE:
        {
            rc = m_parent->waitDeferredWork(m_parent->mParamInitJob);
            if (NO_ERROR != rc) {
                LOGE("Param init deferred work failed");
            } else {
                rc = m_parent->disableMsgType(*((int32_t *)payload));
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_MSG_TYPE_ENABLED:
        {
            int enabled = m_parent->msgTypeEnabled(*((int32_t *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = enabled;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS:
        {
            bool needRestart = false;

            rc = m_parent->waitDeferredWork(m_parent->mParamInitJob);
            if (NO_ERROR != rc) {
                LOGE("Param init deferred work failed");
            } else {
                rc = m_parent->updateParameters((char*)payload, needRestart);
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_STOP:
        {
            m_parent->m_memoryPool.clear();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_COMMIT:
        {
            rc = m_parent->commitParameterChanges();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_RESTART:
        {
            m_parent->setNeedRestart(false);
            result.status            = rc;
            result.request_api       = evt;
            result.result_type       = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_GET_PARAMS:
        {
            rc = m_parent->waitDeferredWork(m_parent->mParamInitJob);
            if (NO_ERROR != rc) {
                LOGE("Param init deferred work failed");
                char* nullParams = (char *)malloc(1);
                if (nullParams) {
                    memset(nullParams, 0, 1);
                }
                result.params = nullParams;
            } else {
                result.params = m_parent->getParameters();
            }
            rc = result.params ? NO_ERROR : UNKNOWN_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_PARAMS;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PUT_PARAMS:
        {
            rc = m_parent->putParameters((char*)payload);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PREPARE_PREVIEW:
        {
            rc = m_parent->waitDeferredWork(m_parent->mParamInitJob);
            if (NO_ERROR != rc) {
                LOGE("Param init deferred work failed");
            } else {
                rc = m_parent->preparePreview();
            }
            if (rc == NO_ERROR) {
                //prepare preview success, move to ready state
                m_state = QCAMERA_SM_STATE_PREVIEW_READY;
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_START_PREVIEW:
        {
            rc = m_parent->waitDeferredWork(m_parent->mParamInitJob);
            if (NO_ERROR != rc) {
                LOGE("Param init deferred work failed");
            } else if (m_parent->mPreviewWindow == NULL) {
                rc = m_parent->preparePreview();
                if(rc == NO_ERROR) {
                    // preview window is not set yet, move to previewReady state
                    m_state = QCAMERA_SM_STATE_PREVIEW_READY;
                } else {
                    LOGE("preparePreview failed");
                }
            } else {
                rc = m_parent->preparePreview();
                if (rc == NO_ERROR) {
                    applyDelayedMsgs();
                    rc = m_parent->startPreview();
                    if (rc != NO_ERROR) {
                        m_parent->unpreparePreview();
                    } else {
                        // start preview success, move to previewing state
                        m_state = QCAMERA_SM_STATE_PREVIEWING;
                    }
                }
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_START_NODISPLAY_PREVIEW:
        {
            rc = m_parent->waitDeferredWork(m_parent->mParamInitJob);
            if (NO_ERROR != rc) {
                LOGE("Param init deferred work failed");
            } else {
                rc = m_parent->preparePreview();
            }
            if (rc == NO_ERROR) {
                applyDelayedMsgs();
                rc = m_parent->startPreview();
                if (rc != NO_ERROR) {
                    m_parent->unpreparePreview();
                } else {
                    m_state = QCAMERA_SM_STATE_PREVIEWING;
                }
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
    break;
    case QCAMERA_SM_EVT_STOP_PREVIEW:
        {
            // no op needed here
            LOGW("already in preview stopped state, do nothing");
            result.status = NO_ERROR;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PREVIEW_ENABLED:
    case QCAMERA_SM_EVT_RECORDING_ENABLED:
        {
            result.status = NO_ERROR;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = 0;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_RELEASE:
        {
            rc = m_parent->release();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STORE_METADATA_IN_BUFS:
        {
            rc = m_parent->storeMetaDataInBuffers(*((int *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_DUMP:
        {
            rc = m_parent->waitDeferredWork(m_parent->mParamInitJob);
            if (NO_ERROR != rc) {
                LOGE("Param init deferred work failed");
            } else {
                rc = m_parent->dump(*((int *)payload));
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SEND_COMMAND:
        {
            rc = m_parent->waitDeferredWork(m_parent->mParamInitJob);
            if (NO_ERROR != rc) {
                LOGE("Param init deferred work failed");
            } else {
                qcamera_sm_evt_command_payload_t *cmd_payload =
                        (qcamera_sm_evt_command_payload_t *)payload;
                rc = m_parent->sendCommand(cmd_payload->cmd,
                        cmd_payload->arg1,
                        cmd_payload->arg2);
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_RELEASE_RECORIDNG_FRAME:
        {
            LOGW("Free video handle %d %d", evt, m_state);
            QCameraVideoMemory::closeNativeHandle((const void *)payload);
        }
    case QCAMERA_SM_EVT_PRE_START_RECORDING:
    case QCAMERA_SM_EVT_RESTART_STOP_PREVIEW:
    case QCAMERA_SM_EVT_RESTART_START_PREVIEW:
    case QCAMERA_SM_EVT_START_RECORDING:
    case QCAMERA_SM_EVT_STOP_RECORDING:
    case QCAMERA_SM_EVT_PREPARE_SNAPSHOT:
    case QCAMERA_SM_EVT_PRE_TAKE_PICTURE:
    case QCAMERA_SM_EVT_TAKE_PICTURE:
        {
            LOGE("Error!! cannot handle evt(%d) in state(%d)", evt, m_state);
            rc = INVALID_OPERATION;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_START_AUTO_FOCUS:
    case QCAMERA_SM_EVT_CANCEL_PICTURE:
        {
            // no op needed here
            LOGW("No ops for evt(%d) in state(%d)", evt, m_state);
            result.status = NO_ERROR;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STOP_AUTO_FOCUS:
        {
            rc = m_parent->waitDeferredWork(m_parent->mParamInitJob);
            if (NO_ERROR != rc) {
                LOGE("Param init deferred work failed");
            } else {
                rc = m_parent->cancelAutoFocus();
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_REG_FACE_IMAGE:
        {
            int32_t faceID = 0;
            qcamera_sm_evt_reg_face_payload_t *reg_payload =
                (qcamera_sm_evt_reg_face_payload_t *)payload;
            rc = m_parent->registerFaceImage(reg_payload->img_ptr,
                                             reg_payload->config,
                                             faceID);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_HANDLE;
            result.handle = faceID;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_THERMAL_NOTIFY:
        {
            rc = m_parent->waitDeferredWork(m_parent->mParamInitJob);
            if (NO_ERROR != rc) {
                LOGE("Param init deferred work failed");
            } else {
                rc = m_parent->updateThermalLevel(payload);
            }
        }
        break;
    case QCAMERA_SM_EVT_EVT_NOTIFY:
        {
            mm_camera_event_t *cam_evt = (mm_camera_event_t *)payload;
            switch (cam_evt->server_event_type) {
            case CAM_EVENT_TYPE_DAEMON_DIED:
                {
                    m_parent->sendEvtNotify(CAMERA_MSG_ERROR,
                                            CAMERA_ERROR_SERVER_DIED,
                                            0);
                }
                break;
            default:
                LOGE("Invalid internal event %d in state(%d)",
                             cam_evt->server_event_type, m_state);
                break;
            }
        }
        break;
    case QCAMERA_SM_EVT_SNAPSHOT_DONE:
        {
            // No ops, but need to notify
            LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalEvtResult(&result);
        }
       break;
    case QCAMERA_SM_EVT_EVT_INTERNAL:
       {
           qcamera_sm_internal_evt_payload_t *internal_evt =
               (qcamera_sm_internal_evt_payload_t *)payload;
           switch (internal_evt->evt_type) {
           case QCAMERA_INTERNAL_EVT_LED_MODE_OVERRIDE:
               rc = m_parent->waitDeferredWork(m_parent->mParamInitJob);
               if (NO_ERROR != rc) {
                   LOGE("Param init deferred work failed");
               } else {
                   rc = m_parent->mParameters.updateFlashMode(internal_evt->led_data);
               }
               break;
           default:
               LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
               break;
           }
       }
       break;
    case QCAMERA_SM_EVT_JPEG_EVT_NOTIFY:
    default:
        LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
        break;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : procEvtPreviewReadyState
 *
 * DESCRIPTION: finite state machine function to handle event in state of
 *              QCAMERA_SM_STATE_PREVIEW_READY.
 *
 * PARAMETERS :
 *   @evt      : event to be processed
 *   @payload  : event payload. Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraStateMachine::procEvtPreviewReadyState(qcamera_sm_evt_enum_t evt,
                                                      void *payload)
{
    int32_t rc = NO_ERROR;
    qcamera_api_result_t result;
    memset(&result, 0, sizeof(qcamera_api_result_t));

    LOGL("event (%d)", evt);
    switch (evt) {
    case QCAMERA_SM_EVT_SET_PREVIEW_WINDOW:
        {
            m_parent->setPreviewWindow((struct preview_stream_ops *)payload);
            if (m_parent->mPreviewWindow != NULL) {
                applyDelayedMsgs();
                rc = m_parent->startPreview();
                if (rc != NO_ERROR) {
                    m_parent->unpreparePreview();
                    m_state = QCAMERA_SM_STATE_PREVIEW_STOPPED;
                } else {
                    m_state = QCAMERA_SM_STATE_PREVIEWING;
                }
            }

            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_CALLBACKS:
        {
            qcamera_sm_evt_setcb_payload_t *setcbs =
                (qcamera_sm_evt_setcb_payload_t *)payload;
            rc = m_parent->setCallBacks(setcbs->notify_cb,
                                        setcbs->data_cb,
                                        setcbs->data_cb_timestamp,
                                        setcbs->get_memory,
                                        setcbs->user);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_ENABLE_MSG_TYPE:
        {
            rc = m_parent->enableMsgType(*((int32_t *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_DISABLE_MSG_TYPE:
        {
            rc = m_parent->disableMsgType(*((int32_t *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_MSG_TYPE_ENABLED:
        {
            int enabled = m_parent->msgTypeEnabled(*((int32_t *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = enabled;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS:
        {
            bool needRestart = false;
            rc = m_parent->updateParameters((char*)payload, needRestart);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_STOP:
        {
            LOGD("Stopping preview...");
            // need restart preview for parameters to take effect
            m_parent->unpreparePreview();
            // Clear memory pools
            m_parent->m_memoryPool.clear();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_COMMIT:
        {
            rc = m_parent->commitParameterChanges();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_RESTART:
        {
            // prepare preview again
            rc = m_parent->preparePreview();
            if (rc != NO_ERROR) {
                m_state = QCAMERA_SM_STATE_PREVIEW_STOPPED;
            }
            m_parent->setNeedRestart(false);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_GET_PARAMS:
        {
            result.params = m_parent->getParameters();
            rc = result.params ? NO_ERROR : UNKNOWN_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_PARAMS;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PUT_PARAMS:
        {
            rc = m_parent->putParameters((char*)payload);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PREPARE_PREVIEW:
        {
            // no ops here
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_START_NODISPLAY_PREVIEW:
        {
            rc = m_parent->startPreview();
            if (rc != NO_ERROR) {
                m_parent->unpreparePreview();
                m_state = QCAMERA_SM_STATE_PREVIEW_STOPPED;
            } else {
                m_state = QCAMERA_SM_STATE_PREVIEWING;
            }
            // no ops here
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_START_PREVIEW:
        {
            if (m_parent->mPreviewWindow != NULL) {
                rc = m_parent->startPreview();
                if (rc != NO_ERROR) {
                    m_parent->unpreparePreview();
                    m_state = QCAMERA_SM_STATE_PREVIEW_STOPPED;
                } else {
                    m_state = QCAMERA_SM_STATE_PREVIEWING;
                }
            }
            // no ops here
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STOP_PREVIEW:
        {
            m_parent->unpreparePreview();
            rc = 0;
            m_state = QCAMERA_SM_STATE_PREVIEW_STOPPED;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PREVIEW_ENABLED:
        {
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = 1;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_RECORDING_ENABLED:
        {
            rc = 0;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = 0;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STORE_METADATA_IN_BUFS:
        {
            rc = m_parent->storeMetaDataInBuffers(*((int *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_DUMP:
        {
            rc = m_parent->dump(*((int *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_START_AUTO_FOCUS:
        {
            rc = m_parent->autoFocus();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STOP_AUTO_FOCUS:
        {
            rc = m_parent->cancelAutoFocus();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SEND_COMMAND:
        {
            qcamera_sm_evt_command_payload_t *cmd_payload =
                (qcamera_sm_evt_command_payload_t *)payload;
            rc = m_parent->sendCommand(cmd_payload->cmd,
                                       cmd_payload->arg1,
                                       cmd_payload->arg2);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_REG_FACE_IMAGE:
        {
            int32_t faceID = 0;
            qcamera_sm_evt_reg_face_payload_t *reg_payload =
                (qcamera_sm_evt_reg_face_payload_t *)payload;
            rc = m_parent->registerFaceImage(reg_payload->img_ptr,
                                             reg_payload->config,
                                             faceID);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_HANDLE;
            result.handle = faceID;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_RELEASE_RECORIDNG_FRAME:
        {
            LOGW("Free video handle %d %d", evt, m_state);
            QCameraVideoMemory::closeNativeHandle((const void *)payload);
        }
    case QCAMERA_SM_EVT_PRE_START_RECORDING:
    case QCAMERA_SM_EVT_RESTART_STOP_PREVIEW:
    case QCAMERA_SM_EVT_RESTART_START_PREVIEW:
    case QCAMERA_SM_EVT_START_RECORDING:
    case QCAMERA_SM_EVT_STOP_RECORDING:
    case QCAMERA_SM_EVT_PREPARE_SNAPSHOT:
    case QCAMERA_SM_EVT_PRE_TAKE_PICTURE:
    case QCAMERA_SM_EVT_TAKE_PICTURE:
    case QCAMERA_SM_EVT_CANCEL_PICTURE:
    case QCAMERA_SM_EVT_RELEASE:
        {
            LOGE("Error!! cannot handle evt(%d) in state(%d)", evt, m_state);
            rc = INVALID_OPERATION;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_EVT_NOTIFY:
        {
            mm_camera_event_t *cam_evt = (mm_camera_event_t *)payload;
            switch (cam_evt->server_event_type) {
            case CAM_EVENT_TYPE_DAEMON_DIED:
                {
                    m_parent->sendEvtNotify(CAMERA_MSG_ERROR,
                                            CAMERA_ERROR_SERVER_DIED,
                                            0);
                }
                break;
            default:
                LOGE("Invalid internal event %d in state(%d)",
                             cam_evt->server_event_type, m_state);
                break;
            }
        }
        break;
    case QCAMERA_SM_EVT_SNAPSHOT_DONE:
        {
            // No ops, but need to notify
            LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalEvtResult(&result);
        }
       break;
    case QCAMERA_SM_EVT_EVT_INTERNAL:
       {
           qcamera_sm_internal_evt_payload_t *internal_evt =
                   (qcamera_sm_internal_evt_payload_t *)payload;
           switch (internal_evt->evt_type) {
           case QCAMERA_INTERNAL_EVT_LED_MODE_OVERRIDE:
               rc = m_parent->mParameters.updateFlashMode(internal_evt->led_data);
               break;
           default:
               LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
               break;
           }
       }
       break;
    case QCAMERA_SM_EVT_JPEG_EVT_NOTIFY:
    case QCAMERA_SM_EVT_THERMAL_NOTIFY:
    default:
        LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
        break;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : procEvtPreviewingState
 *
 * DESCRIPTION: finite state machine function to handle event in state of
 *              QCAMERA_SM_STATE_PREVIEWING.
 *
 * PARAMETERS :
 *   @evt      : event to be processed
 *   @payload  : event payload. Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraStateMachine::procEvtPreviewingState(qcamera_sm_evt_enum_t evt,
                                                    void *payload)
{
    int32_t rc = NO_ERROR;
    qcamera_api_result_t result;
    memset(&result, 0, sizeof(qcamera_api_result_t));

    LOGL("event (%d)", evt);
    switch (evt) {
    case QCAMERA_SM_EVT_SET_PREVIEW_WINDOW:
        {
            // Error setting preview window during previewing
            LOGE("Error!! cannot set preview window when preview is running");
            rc = INVALID_OPERATION;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_CALLBACKS:
        {
            qcamera_sm_evt_setcb_payload_t *setcbs =
                (qcamera_sm_evt_setcb_payload_t *)payload;
            rc = m_parent->setCallBacks(setcbs->notify_cb,
                                        setcbs->data_cb,
                                        setcbs->data_cb_timestamp,
                                        setcbs->get_memory,
                                        setcbs->user);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_ENABLE_MSG_TYPE:
        {
            int32_t enable_msgs = *((int32_t *)payload);
            if (m_bDelayPreviewMsgs &&
                    (enable_msgs & CAMERA_MSG_PREVIEW_FRAME)) {
                enable_msgs &= ~CAMERA_MSG_PREVIEW_FRAME;
                m_DelayedMsgs = CAMERA_MSG_PREVIEW_FRAME;
            }
            rc = m_parent->enableMsgType(enable_msgs);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_DISABLE_MSG_TYPE:
        {
            int32_t disable_msgs = *((int32_t *)payload);
            if (m_bDelayPreviewMsgs && m_DelayedMsgs) {
                m_DelayedMsgs &= ~disable_msgs;
                if (0 == m_DelayedMsgs) {
                    m_bDelayPreviewMsgs = false;
                }
            }
            rc = m_parent->disableMsgType(disable_msgs);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_MSG_TYPE_ENABLED:
        {
            int32_t msgs = *((int32_t *)payload);
            int enabled = m_parent->msgTypeEnabled(msgs);
            if (m_bDelayPreviewMsgs && m_DelayedMsgs) {
                enabled |= (msgs & m_DelayedMsgs);
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = enabled;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS:
        {
            bool needRestart = false;
            rc = m_parent->updateParameters((char*)payload, needRestart);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_STOP:
        {
            LOGD("Stopping preview...");
            // stop preview
            rc = m_parent->stopPreview();
            // Clear memory pools
            m_parent->m_memoryPool.clear();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_COMMIT:
        {
            rc = m_parent->commitParameterChanges();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_RESTART:
        {
            // start preview again
            rc = m_parent->preparePreview();
            if (rc == NO_ERROR) {
                applyDelayedMsgs();
                rc = m_parent->startPreview();
                if (rc != NO_ERROR) {
                    m_parent->unpreparePreview();
                }
                if (rc != NO_ERROR) {
                    m_state = QCAMERA_SM_STATE_PREVIEW_STOPPED;
                }
            }
            m_parent->setNeedRestart(false);
            result.status            = rc;
            result.request_api       = evt;
            result.result_type       = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_GET_PARAMS:
        {
            result.params = m_parent->getParameters();
            rc = result.params ? NO_ERROR : UNKNOWN_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_PARAMS;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PUT_PARAMS:
        {
            rc = m_parent->putParameters((char*)payload);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PREPARE_PREVIEW:
        {
            // no ops here
            LOGW("Already in preview ready state, no ops here");
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_START_PREVIEW:
    case QCAMERA_SM_EVT_START_NODISPLAY_PREVIEW:
        {
            // no ops here
            LOGW("Already in previewing, no ops here to start preview");
            applyDelayedMsgs();
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STOP_PREVIEW:
        {
            m_state = QCAMERA_SM_STATE_PREVIEW_STOPPED;
            rc = m_parent->stopPreview();
            applyDelayedMsgs();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PREVIEW_ENABLED:
        {
            applyDelayedMsgs();
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = 1;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_RECORDING_ENABLED:
        {
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = 0;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STORE_METADATA_IN_BUFS:
        {
            rc = m_parent->storeMetaDataInBuffers(*((int *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_DUMP:
        {
            rc = m_parent->dump(*((int *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_START_AUTO_FOCUS:
        {
            rc = m_parent->autoFocus();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STOP_AUTO_FOCUS:
        {
            rc = m_parent->cancelAutoFocus();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PRE_START_RECORDING:
        {
            rc = m_parent->preStartRecording();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_START_RECORDING:
        {
            rc = m_parent->startRecording();
            if (rc == NO_ERROR) {
                // move state to recording state
                m_state = QCAMERA_SM_STATE_RECORDING;
                applyDelayedMsgs();
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PREPARE_SNAPSHOT:
        {
            rc = m_parent->prepareHardwareForSnapshot(FALSE);
            if (rc == NO_ERROR) {
                // Do not signal API result in this case.
                // Need to wait for snapshot done in metadta.
                m_state = QCAMERA_SM_STATE_PREPARE_SNAPSHOT;
                applyDelayedMsgs();
            } else {
                // Do not change state in this case.
                LOGE("prepareHardwareForSnapshot failed %d",
                     rc);

                result.status = rc;
                result.request_api = evt;
                result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
                m_parent->signalAPIResult(&result);
            }
        }
        break;
    case QCAMERA_SM_EVT_PRE_TAKE_PICTURE:
        {
            rc = m_parent->preTakePicture();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_TAKE_PICTURE:
       {
           LOGL("QCAMERA_SM_EVT_TAKE_PICTURE ");
           if ( m_parent->mParameters.getRecordingHintValue() == true) {
                m_parent->stopPreview();
                m_parent->mParameters.updateRecordingHintValue(FALSE);
                // start preview again
                rc = m_parent->preparePreview();
                if (rc == NO_ERROR) {
                    rc = m_parent->startPreview();
                    if (rc != NO_ERROR) {
                        m_parent->unpreparePreview();
                    }
                }
           }
           if (m_parent->isZSLMode() || m_parent->isLongshotEnabled()) {
               bool restartPreview = m_parent->isPreviewRestartEnabled();
               if ((restartPreview) && (m_parent->mParameters.getManualCaptureMode()
                       >= CAM_MANUAL_CAPTURE_TYPE_3)) {
                   /* stop preview and disable ZSL now */
                   m_parent->stopPreview();
                   m_parent->mParameters.updateZSLModeValue(FALSE);
                   m_RestoreZSL = TRUE;
                   m_bDelayPreviewMsgs = true;
                   m_state = QCAMERA_SM_STATE_PIC_TAKING;
               } else {
                   m_state = QCAMERA_SM_STATE_PREVIEW_PIC_TAKING;
                   m_bDelayPreviewMsgs = true;
               }

               rc = m_parent->takePicture();
               if (rc != NO_ERROR) {
                   // move state to previewing state
                   m_parent->unconfigureAdvancedCapture();
                   m_state = QCAMERA_SM_STATE_PREVIEWING;
               }
               if (!(m_parent->isRetroPicture()) || (rc != NO_ERROR)) {
                   LOGD("signal API result, m_state = %d",
                          m_state);
                   result.status = rc;
                   result.request_api = evt;
                   result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
                   m_parent->signalAPIResult(&result);
               }
           } else {
               m_state = QCAMERA_SM_STATE_PIC_TAKING;
               rc = m_parent->takePicture();
               if (rc != NO_ERROR) {
                   int32_t temp_rc = NO_ERROR;
                   // move state to preview stopped state
                   m_parent->unconfigureAdvancedCapture();
                   m_parent->stopPreview();
                   // start preview again
                   temp_rc = m_parent->preparePreview();
                   if (temp_rc == NO_ERROR) {
                       temp_rc = m_parent->startPreview();
                       if (temp_rc != NO_ERROR) {
                           m_parent->unpreparePreview();
                           m_state = QCAMERA_SM_STATE_PREVIEW_STOPPED;
                       } else {
                           m_state = QCAMERA_SM_STATE_PREVIEWING;
                       }
                   } else {
                       m_state = QCAMERA_SM_STATE_PREVIEW_STOPPED;
                   }
               }
               result.status = rc;
               result.request_api = evt;
               result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
               m_parent->signalAPIResult(&result);
           }
        }
        break;
    case QCAMERA_SM_EVT_SEND_COMMAND:
        {
            qcamera_sm_evt_command_payload_t *cmd_payload =
                    (qcamera_sm_evt_command_payload_t *)payload;
            rc = m_parent->sendCommand(cmd_payload->cmd,
                    cmd_payload->arg1,
                    cmd_payload->arg2);
            m_bPreviewNeedsRestart =
                    (QCAMERA_SM_EVT_RESTART_PERVIEW == cmd_payload->arg1);
            m_bPreviewDelayedRestart =
                    (QCAMERA_SM_EVT_DELAYED_RESTART == cmd_payload->arg2);

#ifndef VANILLA_HAL
            if ((CAMERA_CMD_LONGSHOT_ON == cmd_payload->cmd) &&
                    (m_bPreviewNeedsRestart)) {
                m_parent->stopPreview();
                // Clear memory pools
                m_parent->m_memoryPool.clear();

                if (!m_bPreviewDelayedRestart) {
                    // start preview again
                    rc = m_parent->preparePreview();
                    if (rc == NO_ERROR) {
                        applyDelayedMsgs();
                        rc = m_parent->startPreview();
                        if (rc != NO_ERROR) {
                            m_parent->unpreparePreview();
                        }
                    }
                }
            }
#endif
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SEND_COMMAND_RESTART:
        {
#ifndef VANILLA_HAL
            qcamera_sm_evt_command_payload_t *cmd_payload =
                    (qcamera_sm_evt_command_payload_t *)payload;
            if ((CAMERA_CMD_LONGSHOT_ON == cmd_payload->cmd) &&
                    (m_bPreviewNeedsRestart) &&
                    (m_bPreviewDelayedRestart)) {
                // start preview again
                rc = m_parent->preparePreview();
                if (rc == NO_ERROR) {
                    rc = m_parent->startPreview();
                    if (rc != NO_ERROR) {
                        m_parent->unpreparePreview();
                    }
                }
            }
#endif
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_REG_FACE_IMAGE:
        {
            int32_t faceID = 0;
            qcamera_sm_evt_reg_face_payload_t *reg_payload =
                (qcamera_sm_evt_reg_face_payload_t *)payload;
            rc = m_parent->registerFaceImage(reg_payload->img_ptr,
                                             reg_payload->config,
                                             faceID);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_HANDLE;
            result.handle = faceID;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_RELEASE_RECORIDNG_FRAME:
        {
            LOGW("Free video handle %d %d", evt, m_state);
            QCameraVideoMemory::closeNativeHandle((const void *)payload);
        }
    case QCAMERA_SM_EVT_RELEASE:
        {
            LOGE("Error!! cannot handle evt(%d) in state(%d)", evt, m_state);
            rc = INVALID_OPERATION;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_CANCEL_PICTURE:
    case QCAMERA_SM_EVT_STOP_RECORDING:
        {
            // no op needed here
            LOGW("No ops for evt(%d) in state(%d)", evt, m_state);
            result.status = NO_ERROR;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);

        }
        break;
    case QCAMERA_SM_EVT_EVT_INTERNAL:
        {
            qcamera_sm_internal_evt_payload_t *internal_evt =
                (qcamera_sm_internal_evt_payload_t *)payload;
            switch (internal_evt->evt_type) {
            case QCAMERA_INTERNAL_EVT_FOCUS_UPDATE:
                rc = m_parent->processAutoFocusEvent(internal_evt->focus_data);
                break;
            case QCAMERA_INTERNAL_EVT_PREP_SNAPSHOT_DONE:
                break;
            case QCAMERA_INTERNAL_EVT_FACE_DETECT_RESULT:
                rc = m_parent->processFaceDetectionResult(&internal_evt->faces_data);
                break;
            case QCAMERA_INTERNAL_EVT_HISTOGRAM_STATS:
                rc = m_parent->processHistogramStats(internal_evt->stats_data);
                break;
            case QCAMERA_INTERNAL_EVT_CROP_INFO:
                rc = m_parent->processZoomEvent(internal_evt->crop_data);
                break;
            case QCAMERA_INTERNAL_EVT_ASD_UPDATE:
                rc = m_parent->processASDUpdate(internal_evt->asd_data);
                break;
            case QCAMERA_INTERNAL_EVT_LED_MODE_OVERRIDE:
                rc = m_parent->mParameters.updateFlashMode(internal_evt->led_data);
                break;
            case QCAMERA_INTERNAL_EVT_AWB_UPDATE:
                rc = m_parent->transAwbMetaToParams(internal_evt->awb_data);
                break;
            case QCAMERA_INTERNAL_EVT_AE_UPDATE:
                rc = m_parent->processAEInfo(internal_evt->ae_data);
                break;
            case QCAMERA_INTERNAL_EVT_FOCUS_POS_UPDATE:
                rc = m_parent->processFocusPositionInfo(internal_evt->focus_pos);
                break;
            case QCAMERA_INTERNAL_EVT_HDR_UPDATE:
                rc = m_parent->processHDRData(internal_evt->hdr_data);
                break;
            case QCAMERA_INTERNAL_EVT_RETRO_AEC_UNLOCK:
                rc = m_parent->processRetroAECUnlock();
                break;
            case QCAMERA_INTERNAL_EVT_ZSL_CAPTURE_DONE:
                rc = m_parent->processZSLCaptureDone();
                break;
            default:
                LOGE("Invalid internal event %d in state(%d)",
                             internal_evt->evt_type, m_state);
                break;
            }
        }
        break;
    case QCAMERA_SM_EVT_EVT_NOTIFY:
        {
            mm_camera_event_t *cam_evt = (mm_camera_event_t *)payload;
            switch (cam_evt->server_event_type) {
            case CAM_EVENT_TYPE_DAEMON_DIED:
                {
                    m_parent->sendEvtNotify(CAMERA_MSG_ERROR,
                                            CAMERA_ERROR_SERVER_DIED,
                                            0);
                }
                break;
            default:
                LOGW("no handling for server evt (%d) at this state",
                       cam_evt->server_event_type);
                break;
            }
        }
        break;
    case QCAMERA_SM_EVT_THERMAL_NOTIFY:
        {
            rc = m_parent->updateThermalLevel(payload);
        }
        break;
    case QCAMERA_SM_EVT_SNAPSHOT_DONE:
        {
            // No ops, but need to notify
            LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalEvtResult(&result);
        }
       break;
    case QCAMERA_SM_EVT_RESTART_STOP_PREVIEW:
        {
            m_parent->stopPreview();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
       break;
    case QCAMERA_SM_EVT_RESTART_START_PREVIEW:
        {
            rc = m_parent->preparePreview();
            if (rc == NO_ERROR) {
                rc = m_parent->startPreview();
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
       break;
    case QCAMERA_SM_EVT_JPEG_EVT_NOTIFY:
    default:
        LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
        break;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : procEvtPrepareSnapshotState
 *
 * DESCRIPTION: finite state machine function to handle event in state of
 *              QCAMERA_SM_STATE_PREPARE_SNAPSHOT.
 *
 * PARAMETERS :
 *   @evt      : event to be processed
 *   @payload  : event payload. Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraStateMachine::procEvtPrepareSnapshotState(qcamera_sm_evt_enum_t evt,
                                                    void *payload)
{
    int32_t rc = NO_ERROR;
    qcamera_api_result_t result;
    memset(&result, 0, sizeof(qcamera_api_result_t));

    LOGL("event (%d)", evt);
    switch (evt) {
    case QCAMERA_SM_EVT_SET_PREVIEW_WINDOW:
    case QCAMERA_SM_EVT_SET_CALLBACKS:
    case QCAMERA_SM_EVT_ENABLE_MSG_TYPE:
    case QCAMERA_SM_EVT_DISABLE_MSG_TYPE:
    case QCAMERA_SM_EVT_MSG_TYPE_ENABLED:
    case QCAMERA_SM_EVT_SET_PARAMS:
    case QCAMERA_SM_EVT_SET_PARAMS_STOP:
    case QCAMERA_SM_EVT_SET_PARAMS_COMMIT:
    case QCAMERA_SM_EVT_SET_PARAMS_RESTART:
    case QCAMERA_SM_EVT_GET_PARAMS:
    case QCAMERA_SM_EVT_PUT_PARAMS:
    case QCAMERA_SM_EVT_PREPARE_PREVIEW:
    case QCAMERA_SM_EVT_START_PREVIEW:
    case QCAMERA_SM_EVT_START_NODISPLAY_PREVIEW:
    case QCAMERA_SM_EVT_STOP_PREVIEW:
    case QCAMERA_SM_EVT_PREVIEW_ENABLED:
    case QCAMERA_SM_EVT_RECORDING_ENABLED:
    case QCAMERA_SM_EVT_STORE_METADATA_IN_BUFS:
    case QCAMERA_SM_EVT_DUMP:
    case QCAMERA_SM_EVT_START_AUTO_FOCUS:
    case QCAMERA_SM_EVT_STOP_AUTO_FOCUS:
    case QCAMERA_SM_EVT_PRE_START_RECORDING:
    case QCAMERA_SM_EVT_RESTART_STOP_PREVIEW:
    case QCAMERA_SM_EVT_RESTART_START_PREVIEW:
    case QCAMERA_SM_EVT_START_RECORDING:
    case QCAMERA_SM_EVT_PRE_TAKE_PICTURE:
    case QCAMERA_SM_EVT_TAKE_PICTURE:
    case QCAMERA_SM_EVT_PREPARE_SNAPSHOT:
    case QCAMERA_SM_EVT_SEND_COMMAND:
    case QCAMERA_SM_EVT_CANCEL_PICTURE:
    case QCAMERA_SM_EVT_STOP_RECORDING:
    case QCAMERA_SM_EVT_RELEASE_RECORIDNG_FRAME:
    case QCAMERA_SM_EVT_RELEASE:
        {
            LOGE("Error!! cannot handle evt(%d) in state(%d)", evt, m_state);
            rc = INVALID_OPERATION;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_EVT_INTERNAL:
        {
            qcamera_sm_internal_evt_payload_t *internal_evt =
                (qcamera_sm_internal_evt_payload_t *)payload;
            switch (internal_evt->evt_type) {
            case QCAMERA_INTERNAL_EVT_FOCUS_UPDATE:
                rc = m_parent->processAutoFocusEvent(internal_evt->focus_data);
                break;
            case QCAMERA_INTERNAL_EVT_PREP_SNAPSHOT_DONE:
                m_parent->processPrepSnapshotDoneEvent(internal_evt->prep_snapshot_state);
                m_state = QCAMERA_SM_STATE_PREVIEWING;

                result.status = NO_ERROR;
                result.request_api = QCAMERA_SM_EVT_PREPARE_SNAPSHOT;
                result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
                m_parent->signalAPIResult(&result);
                break;
            case QCAMERA_INTERNAL_EVT_FACE_DETECT_RESULT:
                rc = m_parent->processFaceDetectionResult(&internal_evt->faces_data);
                break;
            case QCAMERA_INTERNAL_EVT_HISTOGRAM_STATS:
                rc = m_parent->processHistogramStats(internal_evt->stats_data);
                break;
            case QCAMERA_INTERNAL_EVT_CROP_INFO:
                rc = m_parent->processZoomEvent(internal_evt->crop_data);
                break;
            case QCAMERA_INTERNAL_EVT_ASD_UPDATE:
                rc = m_parent->processASDUpdate(internal_evt->asd_data);
                break;
            case QCAMERA_INTERNAL_EVT_LED_MODE_OVERRIDE:
                LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
                break;
            case QCAMERA_INTERNAL_EVT_AWB_UPDATE:
                rc = m_parent->transAwbMetaToParams(internal_evt->awb_data);
                break;
            case QCAMERA_INTERNAL_EVT_AE_UPDATE:
                rc = m_parent->processAEInfo(internal_evt->ae_data);
                break;
            case QCAMERA_INTERNAL_EVT_FOCUS_POS_UPDATE:
                rc = m_parent->processFocusPositionInfo(internal_evt->focus_pos);
                break;
            case QCAMERA_INTERNAL_EVT_HDR_UPDATE:
                rc = m_parent->processHDRData(internal_evt->hdr_data);
                break;
            case QCAMERA_INTERNAL_EVT_RETRO_AEC_UNLOCK:
                rc = m_parent->processRetroAECUnlock();
                break;
            case QCAMERA_INTERNAL_EVT_ZSL_CAPTURE_DONE:
                rc = m_parent->processZSLCaptureDone();
                break;
            default:
                LOGE("Invalid internal event %d in state(%d)",
                             internal_evt->evt_type, m_state);
                break;
            }
        }
        break;
    case QCAMERA_SM_EVT_EVT_NOTIFY:
        {
            mm_camera_event_t *cam_evt = (mm_camera_event_t *)payload;
            switch (cam_evt->server_event_type) {
            case CAM_EVENT_TYPE_DAEMON_DIED:
                {
                    // Send internal events to stop indefinite wait on prepare
                    // snapshot done event.
                    result.status = rc;
                    result.request_api = QCAMERA_SM_EVT_PREPARE_SNAPSHOT;
                    result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
                    m_parent->signalAPIResult(&result);

                    result.status = rc;
                    result.request_api = QCAMERA_SM_EVT_TAKE_PICTURE;
                    result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
                    m_parent->signalAPIResult(&result);

                    m_parent->sendEvtNotify(CAMERA_MSG_ERROR,
                                            CAMERA_ERROR_SERVER_DIED,
                                            0);
                }
                break;
            default:
                LOGE("Invalid internal event %d in state(%d)",
                             cam_evt->server_event_type, m_state);
                break;
            }
        }
        break;
    case QCAMERA_SM_EVT_SNAPSHOT_DONE:
        {
            // No ops, but need to notify
            LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalEvtResult(&result);
        }
       break;
    case QCAMERA_SM_EVT_THERMAL_NOTIFY:
        {
            rc = m_parent->updateThermalLevel(payload);
        }
        break;
    case QCAMERA_SM_EVT_JPEG_EVT_NOTIFY:
    default:
        LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
        break;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : procEvtPicTakingState
 *
 * DESCRIPTION: finite state machine function to handle event in state of
 *              QCAMERA_SM_STATE_PIC_TAKING.
 *
 * PARAMETERS :
 *   @evt      : event to be processed
 *   @payload  : event payload. Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraStateMachine::procEvtPicTakingState(qcamera_sm_evt_enum_t evt,
                                                   void *payload)
{
    int32_t rc = NO_ERROR;
    qcamera_api_result_t result;
    memset(&result, 0, sizeof(qcamera_api_result_t));

    LOGL("event (%d)", evt);
    switch (evt) {
    case QCAMERA_SM_EVT_SET_PREVIEW_WINDOW:
        {
            // Error setting preview window during previewing
            LOGE("Error!! cannot set preview window when preview is running");
            rc = INVALID_OPERATION;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_CALLBACKS:
        {
            qcamera_sm_evt_setcb_payload_t *setcbs =
                (qcamera_sm_evt_setcb_payload_t *)payload;
            rc = m_parent->setCallBacks(setcbs->notify_cb,
                                        setcbs->data_cb,
                                        setcbs->data_cb_timestamp,
                                        setcbs->get_memory,
                                        setcbs->user);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_ENABLE_MSG_TYPE:
        {
            rc = m_parent->enableMsgType(*((int32_t *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_DISABLE_MSG_TYPE:
        {
            rc = m_parent->disableMsgType(*((int32_t *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_MSG_TYPE_ENABLED:
        {
            int enabled = m_parent->msgTypeEnabled(*((int32_t *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = enabled;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS:
        {
            bool needRestart = false;
            rc = m_parent->updateParameters((char*)payload, needRestart);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_STOP:
        {
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_COMMIT:
        {
            rc = m_parent->commitParameterChanges();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_RESTART:
        {
            m_parent->setNeedRestart(false);
            result.status           =  rc;
            result.request_api      =  evt;
            result.result_type      =  QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_GET_PARAMS:
        {
            result.params = m_parent->getParameters();
            rc = result.params ? NO_ERROR : UNKNOWN_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_PARAMS;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PUT_PARAMS:
        {
            rc = m_parent->putParameters((char*)payload);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STOP_PREVIEW:
        {
            // cancel picture first
            rc = m_parent->cancelPicture();
            m_state = QCAMERA_SM_STATE_PREVIEW_STOPPED;

            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PREVIEW_ENABLED:
        {
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = 0;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_RECORDING_ENABLED:
        {
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = 0;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STORE_METADATA_IN_BUFS:
        {
            rc = m_parent->storeMetaDataInBuffers(*((int *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_DUMP:
        {
            rc = m_parent->dump(*((int *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_START_AUTO_FOCUS:
        {
            rc = m_parent->autoFocus();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STOP_AUTO_FOCUS:
        {
            rc = m_parent->cancelAutoFocus();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SEND_COMMAND:
        {
            qcamera_sm_evt_command_payload_t *cmd_payload =
                (qcamera_sm_evt_command_payload_t *)payload;
            rc = m_parent->sendCommand(cmd_payload->cmd,
                                       cmd_payload->arg1,
                                       cmd_payload->arg2);
#ifndef VANILLA_HAL
            if ( CAMERA_CMD_LONGSHOT_OFF == cmd_payload->cmd ) {
                // move state to previewing state
                m_state = QCAMERA_SM_STATE_PREVIEWING;
            }
#endif
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_CANCEL_PICTURE:
        {
            rc = m_parent->cancelPicture();
            m_state = QCAMERA_SM_STATE_PREVIEW_STOPPED;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_REG_FACE_IMAGE:
        {
            int32_t faceID = 0;
            qcamera_sm_evt_reg_face_payload_t *reg_payload =
                (qcamera_sm_evt_reg_face_payload_t *)payload;
            rc = m_parent->registerFaceImage(reg_payload->img_ptr,
                                             reg_payload->config,
                                             faceID);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_HANDLE;
            result.handle = faceID;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PRE_TAKE_PICTURE:
        {
           if ( m_parent->isLongshotEnabled() ) {
               // no ops here, need to singal NO_ERROR
               rc = NO_ERROR;
            } else {
                LOGE("Error!! cannot handle evt(%d) in state(%d)", evt, m_state);
                rc = INVALID_OPERATION;
            }

            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_TAKE_PICTURE:
        {
           if ( m_parent->isLongshotEnabled() ) {
               rc = m_parent->longShot();
            } else {
                LOGE("Error!! cannot handle evt(%d) in state(%d)", evt, m_state);
                rc = INVALID_OPERATION;
            }

            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PREPARE_SNAPSHOT:
    case QCAMERA_SM_EVT_PRE_START_RECORDING:
    case QCAMERA_SM_EVT_RESTART_STOP_PREVIEW:
    case QCAMERA_SM_EVT_RESTART_START_PREVIEW:
    case QCAMERA_SM_EVT_START_RECORDING:
    case QCAMERA_SM_EVT_STOP_RECORDING:
    case QCAMERA_SM_EVT_RELEASE_RECORIDNG_FRAME:
    case QCAMERA_SM_EVT_PREPARE_PREVIEW:
    case QCAMERA_SM_EVT_START_PREVIEW:
    case QCAMERA_SM_EVT_START_NODISPLAY_PREVIEW:
    case QCAMERA_SM_EVT_RELEASE:
        {
            LOGE("Error!! cannot handle evt(%d) in state(%d)", evt, m_state);
            rc = INVALID_OPERATION;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_EVT_INTERNAL:
        {
            qcamera_sm_internal_evt_payload_t *internal_evt =
                (qcamera_sm_internal_evt_payload_t *)payload;
            switch (internal_evt->evt_type) {
            case QCAMERA_INTERNAL_EVT_FOCUS_UPDATE:
                rc = m_parent->processAutoFocusEvent(internal_evt->focus_data);
                break;
            case QCAMERA_INTERNAL_EVT_PREP_SNAPSHOT_DONE:
                break;
            case QCAMERA_INTERNAL_EVT_FACE_DETECT_RESULT:
                break;
            case QCAMERA_INTERNAL_EVT_HISTOGRAM_STATS:
                break;
            case QCAMERA_INTERNAL_EVT_CROP_INFO:
                rc = m_parent->processZoomEvent(internal_evt->crop_data);
                break;
            case QCAMERA_INTERNAL_EVT_ASD_UPDATE:
                rc = m_parent->processASDUpdate(internal_evt->asd_data);
                break;
            case QCAMERA_INTERNAL_EVT_LED_MODE_OVERRIDE:
                LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
                break;
            case QCAMERA_INTERNAL_EVT_AWB_UPDATE:
                rc = m_parent->transAwbMetaToParams(internal_evt->awb_data);
                break;
            case QCAMERA_INTERNAL_EVT_AE_UPDATE:
                rc = m_parent->processAEInfo(internal_evt->ae_data);
                break;
            case QCAMERA_INTERNAL_EVT_FOCUS_POS_UPDATE:
                rc = m_parent->processFocusPositionInfo(internal_evt->focus_pos);
                break;
            case QCAMERA_INTERNAL_EVT_HDR_UPDATE:
                rc = m_parent->processHDRData(internal_evt->hdr_data);
                break;
            case QCAMERA_INTERNAL_EVT_RETRO_AEC_UNLOCK:
                rc = m_parent->processRetroAECUnlock();
                break;
            case QCAMERA_INTERNAL_EVT_ZSL_CAPTURE_DONE:
                rc = m_parent->processZSLCaptureDone();
                break;
            default:
                break;
            }
        }
        break;
    case QCAMERA_SM_EVT_EVT_NOTIFY:
        {
            mm_camera_event_t *cam_evt = (mm_camera_event_t *)payload;
            switch (cam_evt->server_event_type) {
            case CAM_EVENT_TYPE_DAEMON_DIED:
                {
                    // Send internal events to stop indefinite wait on prepare
                    // snapshot done event.
                    result.status = rc;
                    result.request_api = QCAMERA_SM_EVT_PREPARE_SNAPSHOT;
                    result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
                    m_parent->signalAPIResult(&result);

                    result.status = rc;
                    result.request_api = QCAMERA_SM_EVT_TAKE_PICTURE;
                    result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
                    m_parent->signalAPIResult(&result);

                    m_parent->sendEvtNotify(CAMERA_MSG_ERROR,
                                            CAMERA_ERROR_SERVER_DIED,
                                            0);
                }
                break;
            case CAM_EVENT_TYPE_CAC_DONE:
                if (m_parent->isCACEnabled() || m_parent->mParameters.isOEMFeatEnabled()) {
                    LOGD("[LONG_SHOT_DBG] : Received CAC Done");
                    if (m_parent->isLongshotEnabled()
                            && !m_parent->isCaptureShutterEnabled()) {
                        // play shutter sound for longshot
                        // after CAC stage is done
                        m_parent->playShutter();
                    }
                    m_parent->mCACDoneReceived = TRUE;
                }
                break;
            default:
                LOGH("no handling for server evt (%d) at this state",
                       cam_evt->server_event_type);
                break;
            }
        }
        break;
    case QCAMERA_SM_EVT_JPEG_EVT_NOTIFY:
        {
            qcamera_jpeg_evt_payload_t *jpeg_job =
                (qcamera_jpeg_evt_payload_t *)payload;
            rc = m_parent->processJpegNotify(jpeg_job);
        }
        break;
    case QCAMERA_SM_EVT_STOP_CAPTURE_CHANNEL:
        {
            bool restartPreview = m_parent->isPreviewRestartEnabled();
            rc = m_parent->stopCaptureChannel(restartPreview);

            if (restartPreview && (NO_ERROR == rc)) {
                m_parent->unconfigureAdvancedCapture();
                rc = m_parent->preparePreview();
                if (NO_ERROR == rc) {
                    m_parent->m_bPreviewStarted = true;
                    applyDelayedMsgs();
                    rc = m_parent->startPreview();
                }
            }

            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SNAPSHOT_DONE:
        {
            rc = m_parent->cancelPicture();

            bool restartPreview = m_parent->isPreviewRestartEnabled();
            if (restartPreview) {
                if (m_parent->mParameters.getManualCaptureMode()
                        >= CAM_MANUAL_CAPTURE_TYPE_3) {
                    m_parent->mParameters.updateZSLModeValue(m_RestoreZSL);
                    m_RestoreZSL = FALSE;
                    rc = m_parent->preparePreview();
                    if (NO_ERROR == rc) {
                        m_parent->m_bPreviewStarted = true;
                        applyDelayedMsgs();
                        rc = m_parent->startPreview();
                    }
                }
                m_state = QCAMERA_SM_STATE_PREVIEWING;
            } else {
                m_state = QCAMERA_SM_STATE_PREVIEW_STOPPED;
            }

            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalEvtResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_THERMAL_NOTIFY:
        {
            rc = m_parent->updateThermalLevel(payload);
        }
        break;
    default:
        LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
        break;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : procEvtRecordingState
 *
 * DESCRIPTION: finite state machine function to handle event in state of
 *              QCAMERA_SM_STATE_RECORDING.
 *
 * PARAMETERS :
 *   @evt      : event to be processed
 *   @payload  : event payload. Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraStateMachine::procEvtRecordingState(qcamera_sm_evt_enum_t evt,
                                                   void *payload)
{
    int32_t rc = NO_ERROR;
    qcamera_api_result_t result;
    memset(&result, 0, sizeof(qcamera_api_result_t));

    LOGL("event (%d)", evt);
    switch (evt) {
    case QCAMERA_SM_EVT_PREPARE_PREVIEW:
    case QCAMERA_SM_EVT_START_PREVIEW:
    case QCAMERA_SM_EVT_SET_PREVIEW_WINDOW:
        {
            // WA: CTS test VideoSnapshot will try to
            //     start preview during video recording.
            LOGH("CTS video restart op");
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_CALLBACKS:
        {
            qcamera_sm_evt_setcb_payload_t *setcbs =
                (qcamera_sm_evt_setcb_payload_t *)payload;
            rc = m_parent->setCallBacks(setcbs->notify_cb,
                                        setcbs->data_cb,
                                        setcbs->data_cb_timestamp,
                                        setcbs->get_memory,
                                        setcbs->user);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_ENABLE_MSG_TYPE:
        {
            rc = m_parent->enableMsgType(*((int32_t *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_DISABLE_MSG_TYPE:
        {
            rc = m_parent->disableMsgType(*((int32_t *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_MSG_TYPE_ENABLED:
        {
            int enabled = m_parent->msgTypeEnabled(*((int32_t *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = enabled;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS:
        {
            bool needRestart = false;
            rc = m_parent->updateParameters((char*)payload, needRestart);
            if (rc == NO_ERROR) {
                if (needRestart) {
                    // cannot set parameters that requires restart during recording
                    LOGE("Error!! cannot set parameters that requires restart during recording");
                    rc = BAD_VALUE;
                }
            }
            if (rc != NO_ERROR) {
                m_parent->setNeedRestart(false);
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_COMMIT:
        {
            rc = m_parent->commitParameterChanges();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_STOP:
    case QCAMERA_SM_EVT_SET_PARAMS_RESTART:
        {
            ALOGE("%s: Error!! cannot handle evt(%d) in state(%d)", __func__, evt, m_state);
            rc = INVALID_OPERATION;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_GET_PARAMS:
        {
            result.params = m_parent->getParameters();
            rc = result.params ? NO_ERROR : UNKNOWN_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_PARAMS;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PUT_PARAMS:
        {
            rc = m_parent->putParameters((char*)payload);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PREVIEW_ENABLED:
        {
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = 0;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_RECORDING_ENABLED:
        {
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = 1;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STORE_METADATA_IN_BUFS:
        {
            rc = m_parent->storeMetaDataInBuffers(*((int *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_DUMP:
        {
            rc = m_parent->dump(*((int *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_START_AUTO_FOCUS:
        {
            rc = m_parent->autoFocus();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STOP_AUTO_FOCUS:
        {
            rc = m_parent->cancelAutoFocus();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SEND_COMMAND:
        {
            qcamera_sm_evt_command_payload_t *cmd_payload =
                (qcamera_sm_evt_command_payload_t *)payload;
            rc = m_parent->sendCommand(cmd_payload->cmd,
                                       cmd_payload->arg1,
                                       cmd_payload->arg2);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PRE_TAKE_PICTURE:
        {
            // No ops here, send NO_ERROR.
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_TAKE_PICTURE:
        {
            m_state = QCAMERA_SM_STATE_VIDEO_PIC_TAKING;
            rc = m_parent->takeLiveSnapshot();
            if (rc != NO_ERROR) {
                m_parent->unconfigureAdvancedCapture();
                m_state = QCAMERA_SM_STATE_RECORDING;
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PRE_START_RECORDING:
    case QCAMERA_SM_EVT_RESTART_STOP_PREVIEW:
    case QCAMERA_SM_EVT_RESTART_START_PREVIEW:
    case QCAMERA_SM_EVT_START_RECORDING:
        {
            // no ops here
            LOGW("already in recording state, no ops for start_recording");
            rc = 0;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STOP_RECORDING:
        {
            rc = m_parent->stopRecording();
            m_state = QCAMERA_SM_STATE_PREVIEWING;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STOP_PREVIEW:
        {
            rc = m_parent->stopRecording();
            m_state = QCAMERA_SM_STATE_PREVIEWING;

            rc = m_parent->stopPreview();
            m_state = QCAMERA_SM_STATE_PREVIEW_STOPPED;

            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_RELEASE_RECORIDNG_FRAME:
        {
            rc = m_parent->releaseRecordingFrame((const void *)payload);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_REG_FACE_IMAGE:
        {
            int32_t faceID = 0;
            qcamera_sm_evt_reg_face_payload_t *reg_payload =
                (qcamera_sm_evt_reg_face_payload_t *)payload;
            rc = m_parent->registerFaceImage(reg_payload->img_ptr,
                                             reg_payload->config,
                                             faceID);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_HANDLE;
            result.handle = faceID;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PREPARE_SNAPSHOT:
        {
            //In Video snapshot, prepare hardware is a no-op.
            result.status = NO_ERROR;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_CANCEL_PICTURE:
    case QCAMERA_SM_EVT_START_NODISPLAY_PREVIEW:
    case QCAMERA_SM_EVT_RELEASE:
        {
            LOGE("Error!! cannot handle evt(%d) in state(%d)", evt, m_state);
            rc = INVALID_OPERATION;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_EVT_INTERNAL:
        {
            qcamera_sm_internal_evt_payload_t *internal_evt =
                (qcamera_sm_internal_evt_payload_t *)payload;
            switch (internal_evt->evt_type) {
            case QCAMERA_INTERNAL_EVT_FOCUS_UPDATE:
                rc = m_parent->processAutoFocusEvent(internal_evt->focus_data);
                break;
            case QCAMERA_INTERNAL_EVT_PREP_SNAPSHOT_DONE:
                break;
            case QCAMERA_INTERNAL_EVT_FACE_DETECT_RESULT:
                rc = m_parent->processFaceDetectionResult(&internal_evt->faces_data);
                break;
            case QCAMERA_INTERNAL_EVT_HISTOGRAM_STATS:
                rc = m_parent->processHistogramStats(internal_evt->stats_data);
                break;
            case QCAMERA_INTERNAL_EVT_CROP_INFO:
                rc = m_parent->processZoomEvent(internal_evt->crop_data);
                break;
            case QCAMERA_INTERNAL_EVT_ASD_UPDATE:
                rc = m_parent->processASDUpdate(internal_evt->asd_data);
                break;
            case QCAMERA_INTERNAL_EVT_LED_MODE_OVERRIDE:
                LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
                break;
            case QCAMERA_INTERNAL_EVT_AWB_UPDATE:
                rc = m_parent->transAwbMetaToParams(internal_evt->awb_data);
                break;
            case QCAMERA_INTERNAL_EVT_AE_UPDATE:
                rc = m_parent->processAEInfo(internal_evt->ae_data);
                break;
            case QCAMERA_INTERNAL_EVT_FOCUS_POS_UPDATE:
                rc = m_parent->processFocusPositionInfo(internal_evt->focus_pos);
                break;
            case QCAMERA_INTERNAL_EVT_HDR_UPDATE:
                rc = m_parent->processHDRData(internal_evt->hdr_data);
                break;
            case QCAMERA_INTERNAL_EVT_RETRO_AEC_UNLOCK:
                rc = m_parent->processRetroAECUnlock();
                break;
            case QCAMERA_INTERNAL_EVT_ZSL_CAPTURE_DONE:
                rc = m_parent->processZSLCaptureDone();
                break;
            default:
                break;
            }
        }
        break;
    case QCAMERA_SM_EVT_EVT_NOTIFY:
        {
            mm_camera_event_t *cam_evt = (mm_camera_event_t *)payload;
            switch (cam_evt->server_event_type) {
            case CAM_EVENT_TYPE_DAEMON_DIED:
                {
                    m_parent->sendEvtNotify(CAMERA_MSG_ERROR,
                                            CAMERA_ERROR_SERVER_DIED,
                                            0);
                }
                break;
            default:
                LOGE("Invalid internal event %d in state(%d)",
                             cam_evt->server_event_type, m_state);
                break;
            }
        }
        break;
    case QCAMERA_SM_EVT_THERMAL_NOTIFY:
        {
            rc = m_parent->updateThermalLevel(payload);
        }
        break;
    case QCAMERA_SM_EVT_SNAPSHOT_DONE:
        {
            // No ops, but need to notify
            LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalEvtResult(&result);
        }
       break;
    case QCAMERA_SM_EVT_JPEG_EVT_NOTIFY:
    default:
        LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
        break;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : procEvtVideoPicTakingState
 *
 * DESCRIPTION: finite state machine function to handle event in state of
 *              QCAMERA_SM_STATE_VIDEO_PIC_TAKING.
 *
 * PARAMETERS :
 *   @evt      : event to be processed
 *   @payload  : event payload. Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraStateMachine::procEvtVideoPicTakingState(qcamera_sm_evt_enum_t evt,
                                                        void *payload)
{
    int32_t rc = NO_ERROR;
    qcamera_api_result_t result;
    memset(&result, 0, sizeof(qcamera_api_result_t));

    LOGL("event (%d)", evt);
    switch (evt) {
    case QCAMERA_SM_EVT_SET_PREVIEW_WINDOW:
        {
            // Error setting preview window during previewing
            LOGE("Error!! cannot set preview window when preview is running");
            rc = INVALID_OPERATION;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_CALLBACKS:
        {
            qcamera_sm_evt_setcb_payload_t *setcbs =
                (qcamera_sm_evt_setcb_payload_t *)payload;
            rc = m_parent->setCallBacks(setcbs->notify_cb,
                                        setcbs->data_cb,
                                        setcbs->data_cb_timestamp,
                                        setcbs->get_memory,
                                        setcbs->user);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_ENABLE_MSG_TYPE:
        {
            rc = m_parent->enableMsgType(*((int32_t *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_DISABLE_MSG_TYPE:
        {
            rc = m_parent->disableMsgType(*((int32_t *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_MSG_TYPE_ENABLED:
        {
            int enabled = m_parent->msgTypeEnabled(*((int32_t *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = enabled;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS:
        {
            bool needRestart = false;
            rc = m_parent->updateParameters((char*)payload, needRestart);
            if (rc == NO_ERROR) {
                if (needRestart) {
                    // cannot set parameters that requires restart during recording
                    LOGE("Error!! cannot set parameters that requires restart during recording");
                    rc = BAD_VALUE;
                }
            }
            if (rc != NO_ERROR) {
                m_parent->setNeedRestart(false);
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_COMMIT:
        {
            rc = m_parent->commitParameterChanges();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_STOP:
    case QCAMERA_SM_EVT_SET_PARAMS_RESTART:
        {
            ALOGE("%s: Error!! cannot handle evt(%d) in state(%d)", __func__, evt, m_state);
            rc = INVALID_OPERATION;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_GET_PARAMS:
        {
            result.params = m_parent->getParameters();
            rc = result.params ? NO_ERROR : UNKNOWN_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_PARAMS;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PUT_PARAMS:
        {
            rc = m_parent->putParameters((char*)payload);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PREVIEW_ENABLED:
        {
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = 1;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_RECORDING_ENABLED:
        {
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = 1;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STORE_METADATA_IN_BUFS:
        {
            rc = m_parent->storeMetaDataInBuffers(*((int *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_DUMP:
        {
            rc = m_parent->dump(*((int *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_START_AUTO_FOCUS:
        {
            rc = m_parent->autoFocus();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STOP_AUTO_FOCUS:
        {
            rc = m_parent->cancelAutoFocus();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SEND_COMMAND:
        {
            qcamera_sm_evt_command_payload_t *cmd_payload =
                (qcamera_sm_evt_command_payload_t *)payload;
            rc = m_parent->sendCommand(cmd_payload->cmd,
                                       cmd_payload->arg1,
                                       cmd_payload->arg2);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STOP_RECORDING:
        {
            rc = m_parent->cancelLiveSnapshot();
            m_state = QCAMERA_SM_STATE_RECORDING;

            rc = m_parent->stopRecording();
            m_state = QCAMERA_SM_STATE_PREVIEWING;

            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_RELEASE_RECORIDNG_FRAME:
        {
            rc = m_parent->releaseRecordingFrame((const void *)payload);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_CANCEL_PICTURE:
        {
            rc = m_parent->cancelLiveSnapshot();
            m_state = QCAMERA_SM_STATE_RECORDING;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_REG_FACE_IMAGE:
        {
            int32_t faceID = 0;
            qcamera_sm_evt_reg_face_payload_t *reg_payload =
                (qcamera_sm_evt_reg_face_payload_t *)payload;
            rc = m_parent->registerFaceImage(reg_payload->img_ptr,
                                             reg_payload->config,
                                             faceID);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_HANDLE;
            result.handle = faceID;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STOP_PREVIEW:
        {
            rc = m_parent->cancelLiveSnapshot();
            m_state = QCAMERA_SM_STATE_RECORDING;

            rc = m_parent->stopRecording();
            m_state = QCAMERA_SM_STATE_PREVIEWING;

            rc = m_parent->stopPreview();
            m_state = QCAMERA_SM_STATE_PREVIEW_STOPPED;

            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PRE_START_RECORDING:
    case QCAMERA_SM_EVT_RESTART_STOP_PREVIEW:
    case QCAMERA_SM_EVT_RESTART_START_PREVIEW:
    case QCAMERA_SM_EVT_START_RECORDING:
    case QCAMERA_SM_EVT_PREPARE_PREVIEW:
    case QCAMERA_SM_EVT_START_PREVIEW:
    case QCAMERA_SM_EVT_START_NODISPLAY_PREVIEW:
    case QCAMERA_SM_EVT_PREPARE_SNAPSHOT:
    case QCAMERA_SM_EVT_PRE_TAKE_PICTURE:
    case QCAMERA_SM_EVT_TAKE_PICTURE:
    case QCAMERA_SM_EVT_RELEASE:
        {
            LOGE("Error!! cannot handle evt(%d) in state(%d)", evt, m_state);
            rc = INVALID_OPERATION;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_EVT_INTERNAL:
        {
            qcamera_sm_internal_evt_payload_t *internal_evt =
                (qcamera_sm_internal_evt_payload_t *)payload;
            switch (internal_evt->evt_type) {
            case QCAMERA_INTERNAL_EVT_FOCUS_UPDATE:
                rc = m_parent->processAutoFocusEvent(internal_evt->focus_data);
                break;
            case QCAMERA_INTERNAL_EVT_PREP_SNAPSHOT_DONE:
                break;
            case QCAMERA_INTERNAL_EVT_FACE_DETECT_RESULT:
                rc = m_parent->processFaceDetectionResult(&internal_evt->faces_data);
                break;
            case QCAMERA_INTERNAL_EVT_HISTOGRAM_STATS:
                rc = m_parent->processHistogramStats(internal_evt->stats_data);
                break;
            case QCAMERA_INTERNAL_EVT_CROP_INFO:
                rc = m_parent->processZoomEvent(internal_evt->crop_data);
                break;
            case QCAMERA_INTERNAL_EVT_ASD_UPDATE:
                rc = m_parent->processASDUpdate(internal_evt->asd_data);
                break;
            case QCAMERA_INTERNAL_EVT_LED_MODE_OVERRIDE:
                LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
                break;
            case QCAMERA_INTERNAL_EVT_AWB_UPDATE:
                rc = m_parent->transAwbMetaToParams(internal_evt->awb_data);
                break;
            case QCAMERA_INTERNAL_EVT_AE_UPDATE:
                rc = m_parent->processAEInfo(internal_evt->ae_data);
                break;
            case QCAMERA_INTERNAL_EVT_FOCUS_POS_UPDATE:
                rc = m_parent->processFocusPositionInfo(internal_evt->focus_pos);
                break;
            case QCAMERA_INTERNAL_EVT_HDR_UPDATE:
                rc = m_parent->processHDRData(internal_evt->hdr_data);
                break;
            case QCAMERA_INTERNAL_EVT_RETRO_AEC_UNLOCK:
                rc = m_parent->processRetroAECUnlock();
                break;
            case QCAMERA_INTERNAL_EVT_ZSL_CAPTURE_DONE:
                rc = m_parent->processZSLCaptureDone();
                break;
            default:
                break;
            }
        }
        break;
    case QCAMERA_SM_EVT_EVT_NOTIFY:
        {
            mm_camera_event_t *cam_evt = (mm_camera_event_t *)payload;
            switch (cam_evt->server_event_type) {
            case CAM_EVENT_TYPE_DAEMON_DIED:
                {
                    m_parent->sendEvtNotify(CAMERA_MSG_ERROR,
                                            CAMERA_ERROR_SERVER_DIED,
                                            0);
                }
                break;
            default:
                LOGE("Invalid internal event %d in state(%d)",
                             cam_evt->server_event_type, m_state);
                break;
            }
        }
        break;
    case QCAMERA_SM_EVT_JPEG_EVT_NOTIFY:
        {
            qcamera_jpeg_evt_payload_t *jpeg_job =
                (qcamera_jpeg_evt_payload_t *)payload;
            rc = m_parent->processJpegNotify(jpeg_job);
        }
        break;
    case QCAMERA_SM_EVT_SNAPSHOT_DONE:
        {
            rc = m_parent->cancelLiveSnapshot();
            m_state = QCAMERA_SM_STATE_RECORDING;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalEvtResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_THERMAL_NOTIFY:
        {
            rc = m_parent->updateThermalLevel(payload);
        }
        break;
    default:
        LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
        break;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : procEvtPreviewPicTakingState
 *
 * DESCRIPTION: finite state machine function to handle event in state of
 *              QCAMERA_SM_STATE_PREVIEW_PIC_TAKING.
 *
 * PARAMETERS :
 *   @evt      : event to be processed
 *   @payload  : event payload. Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraStateMachine::procEvtPreviewPicTakingState(qcamera_sm_evt_enum_t evt,
                                                          void *payload)
{
    int32_t rc = NO_ERROR;
    qcamera_api_result_t result;
    memset(&result, 0, sizeof(qcamera_api_result_t));

    LOGL("event (%d)", evt);
    switch (evt) {
    case QCAMERA_SM_EVT_SET_CALLBACKS:
        {
            qcamera_sm_evt_setcb_payload_t *setcbs =
                (qcamera_sm_evt_setcb_payload_t *)payload;
            rc = m_parent->setCallBacks(setcbs->notify_cb,
                                        setcbs->data_cb,
                                        setcbs->data_cb_timestamp,
                                        setcbs->get_memory,
                                        setcbs->user);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_ENABLE_MSG_TYPE:
        {
            rc = m_parent->enableMsgType(*((int32_t *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_DISABLE_MSG_TYPE:
        {
            rc = m_parent->disableMsgType(*((int32_t *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_MSG_TYPE_ENABLED:
        {
            int enabled = m_parent->msgTypeEnabled(*((int32_t *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = enabled;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS:
        {
            bool needRestart = false;
            rc = m_parent->updateParameters((char*)payload, needRestart);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_STOP:
        {
            // need restart preview for parameters to take effect
            LOGD("Stopping preview...");
            // stop preview
            rc = m_parent->stopPreview();
            // Clear memory pools
            m_parent->m_memoryPool.clear();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_COMMIT:
        {
            // commit parameter changes to server
            rc = m_parent->commitParameterChanges();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SET_PARAMS_RESTART:
        {
            // start preview again
            rc = m_parent->preparePreview();
            if (rc == NO_ERROR) {
                applyDelayedMsgs();
                rc = m_parent->startPreview();
                if (rc != NO_ERROR) {
                    m_parent->unpreparePreview();
                }
            }
            if (rc != NO_ERROR) {
                m_state = QCAMERA_SM_STATE_PIC_TAKING;
            }
            m_parent->setNeedRestart(false);
            result.status           =  rc;
            result.request_api      =  evt;
            result.result_type      =  QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_GET_PARAMS:
        {
            result.params = m_parent->getParameters();
            rc = result.params ? NO_ERROR : UNKNOWN_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_PARAMS;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PUT_PARAMS:
        {
            rc = m_parent->putParameters((char*)payload);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PREVIEW_ENABLED:
        {
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = 1;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_RECORDING_ENABLED:
        {
            rc = NO_ERROR;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_ENABLE_FLAG;
            result.enabled = 0;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STORE_METADATA_IN_BUFS:
        {
            rc = m_parent->storeMetaDataInBuffers(*((int *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_DUMP:
        {
            rc = m_parent->dump(*((int *)payload));
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_START_AUTO_FOCUS:
        {
            rc = m_parent->autoFocus();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STOP_AUTO_FOCUS:
        {
            rc = m_parent->cancelAutoFocus();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_SEND_COMMAND:
        {
            qcamera_sm_evt_command_payload_t *cmd_payload =
                (qcamera_sm_evt_command_payload_t *)payload;
            rc = m_parent->sendCommand(cmd_payload->cmd,
                                       cmd_payload->arg1,
                                       cmd_payload->arg2);
#ifndef VANILLA_HAL
            if ( CAMERA_CMD_LONGSHOT_OFF == cmd_payload->cmd ) {
                // move state to previewing state
                m_state = QCAMERA_SM_STATE_PREVIEWING;
            }
#endif
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_RELEASE_RECORIDNG_FRAME:
        {
            rc = m_parent->releaseRecordingFrame((const void *)payload);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_CANCEL_PICTURE:
        {
            if (m_parent->isZSLMode() || m_parent->isLongshotEnabled()) {
                rc = m_parent->cancelPicture();
            } else {
                rc = m_parent->cancelLiveSnapshot();
            }
            m_state = QCAMERA_SM_STATE_PREVIEWING;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_STOP_PREVIEW:
        {
            if (m_parent->isZSLMode()) {
                // cancel picture first
                rc = m_parent->cancelPicture();
                m_parent->stopChannel(QCAMERA_CH_TYPE_ZSL);
            } else if (m_parent->isLongshotEnabled()) {
                // just cancel picture
                rc = m_parent->cancelPicture();
            } else {
                rc = m_parent->cancelLiveSnapshot();
                m_parent->stopChannel(QCAMERA_CH_TYPE_PREVIEW);
            }
            // unprepare preview
            m_parent->unpreparePreview();
            m_state = QCAMERA_SM_STATE_PREVIEW_STOPPED;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PRE_START_RECORDING:
        {
            if (m_parent->isZSLMode()) {
                LOGE("Error!! cannot handle evt(%d) in state(%d) in ZSL mode", evt, m_state);
                rc = INVALID_OPERATION;
            } else if (m_parent->isLongshotEnabled()) {
                LOGE("Error!! cannot handle evt(%d) in state(%d) in Longshot mode", evt, m_state);
                rc = INVALID_OPERATION;
            } else {
                rc = m_parent->preStartRecording();
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_START_RECORDING:
        {
            if (m_parent->isZSLMode()) {
                LOGE("Error!! cannot handle evt(%d) in state(%d) in ZSL mode",
                       evt, m_state);
                rc = INVALID_OPERATION;
            } else if (m_parent->isLongshotEnabled()) {
                LOGE("Error!! cannot handle evt(%d) in state(%d) in Longshot mode",
                       evt, m_state);
                rc = INVALID_OPERATION;
            } else {
                rc = m_parent->startRecording();
                if (rc == NO_ERROR) {
                    m_state = QCAMERA_SM_STATE_VIDEO_PIC_TAKING;
                }
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_REG_FACE_IMAGE:
        {
            int32_t faceID = 0;
            qcamera_sm_evt_reg_face_payload_t *reg_payload =
                (qcamera_sm_evt_reg_face_payload_t *)payload;
            rc = m_parent->registerFaceImage(reg_payload->img_ptr,
                                             reg_payload->config,
                                             faceID);
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_HANDLE;
            result.handle = faceID;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_PRE_TAKE_PICTURE:
        {
           if ( m_parent->isLongshotEnabled() ) {
               // no ops here, need to singal NO_ERROR
               rc = NO_ERROR;
            } else {
                LOGE("Error!! cannot handle evt(%d) in state(%d)", evt, m_state);
                rc = INVALID_OPERATION;
            }

            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_TAKE_PICTURE:
        {
            if ( m_parent->isLongshotEnabled() ) {
               rc = m_parent->longShot();
            } else {
                LOGE("Error!! cannot handle evt(%d) in state(%d)", evt, m_state);
                rc = INVALID_OPERATION;
            }

            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;

    case QCAMERA_SM_EVT_PREPARE_SNAPSHOT:
        {
          LOGD("Prepare Snapshot");
          if (m_parent->isRetroPicture()) {
              LOGD("Prepare Snapshot in Retro Mode");
              rc = m_parent->prepareHardwareForSnapshot(FALSE);
              if (rc != NO_ERROR) {
                  LOGE("prepareHardwareForSnapshot failed %d",
                       rc);
                  result.status = rc;
                  result.request_api = evt;
                  result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
                  m_parent->signalAPIResult(&result);
              }
          }
          else {
              LOGE("Error!! cannot handle evt(%d) in state(%d)",
                 evt, m_state);
              rc = INVALID_OPERATION;
              result.status = rc;
              result.request_api = evt;
              result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
              m_parent->signalAPIResult(&result);
          }
        }
        break;
    case QCAMERA_SM_EVT_STOP_RECORDING:
    case QCAMERA_SM_EVT_PREPARE_PREVIEW:
    case QCAMERA_SM_EVT_START_PREVIEW:
    case QCAMERA_SM_EVT_START_NODISPLAY_PREVIEW:
    case QCAMERA_SM_EVT_SET_PREVIEW_WINDOW:
    case QCAMERA_SM_EVT_RELEASE:
        {
            LOGE("Error!! cannot handle evt(%d) in state(%d)", evt, m_state);
            rc = INVALID_OPERATION;
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_EVT_INTERNAL:
        {
            qcamera_sm_internal_evt_payload_t *internal_evt =
                (qcamera_sm_internal_evt_payload_t *)payload;
            switch (internal_evt->evt_type) {
            case QCAMERA_INTERNAL_EVT_FOCUS_UPDATE:
                rc = m_parent->processAutoFocusEvent(internal_evt->focus_data);
                break;
            case QCAMERA_INTERNAL_EVT_PREP_SNAPSHOT_DONE:
                LOGD("Received QCAMERA_INTERNAL_EVT_PREP_SNAPSHOT_DONE event");
                if (m_parent->isRetroPicture()) {
                    m_parent->processPrepSnapshotDoneEvent(internal_evt->prep_snapshot_state);
                    LOGD("Retro picture");
                    result.status = NO_ERROR;
                    result.request_api = QCAMERA_SM_EVT_PREPARE_SNAPSHOT;
                    result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
                    m_parent->signalAPIResult(&result);
                }
                else {
                    LOGE("Invalid Case for  "
                            "QCAMERA_INTERNAL_EVT_READY_FOR_SNAPSHOT event");
                }
                break;
            case QCAMERA_INTERNAL_EVT_FACE_DETECT_RESULT:
                rc = m_parent->processFaceDetectionResult(&internal_evt->faces_data);
                break;
            case QCAMERA_INTERNAL_EVT_READY_FOR_SNAPSHOT:
                // This is valid only in Retro picture Mode
                if (m_parent->isRetroPicture()) {
                    LOGD("Received QCAMERA_INTERNAL_EVT_READY_FOR_SNAPSHOT event");
                    result.status = NO_ERROR;
                    result.request_api = QCAMERA_SM_EVT_TAKE_PICTURE;
                    result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
                    m_parent->signalAPIResult(&result);
                }
                else {
                    LOGD("Wrong Case for QCAMERA_INTERNAL_EVT_READY_FOR_SNAPSHOT event");
                }
                break;
            case QCAMERA_INTERNAL_EVT_HISTOGRAM_STATS:
                rc = m_parent->processHistogramStats(internal_evt->stats_data);
                break;
            case QCAMERA_INTERNAL_EVT_CROP_INFO:
                rc = m_parent->processZoomEvent(internal_evt->crop_data);
                break;
            case QCAMERA_INTERNAL_EVT_ASD_UPDATE:
                rc = m_parent->processASDUpdate(internal_evt->asd_data);
                break;
            case QCAMERA_INTERNAL_EVT_LED_MODE_OVERRIDE:
                LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
                break;
            case QCAMERA_INTERNAL_EVT_AWB_UPDATE:
                rc = m_parent->transAwbMetaToParams(internal_evt->awb_data);
                break;
            case QCAMERA_INTERNAL_EVT_AE_UPDATE:
                rc = m_parent->processAEInfo(internal_evt->ae_data);
                break;
            case QCAMERA_INTERNAL_EVT_FOCUS_POS_UPDATE:
                rc = m_parent->processFocusPositionInfo(internal_evt->focus_pos);
                break;
            case QCAMERA_INTERNAL_EVT_HDR_UPDATE:
                rc = m_parent->processHDRData(internal_evt->hdr_data);
                break;
            case QCAMERA_INTERNAL_EVT_RETRO_AEC_UNLOCK:
                rc = m_parent->processRetroAECUnlock();
                break;
            case QCAMERA_INTERNAL_EVT_ZSL_CAPTURE_DONE:
                rc = m_parent->processZSLCaptureDone();
                break;
            default:
                break;
            }
        }
        break;
    case QCAMERA_SM_EVT_EVT_NOTIFY:
        {
            mm_camera_event_t *cam_evt = (mm_camera_event_t *)payload;
            switch (cam_evt->server_event_type) {
            case CAM_EVENT_TYPE_DAEMON_DIED:
                {
                    // Send internal events to stop indefinite wait on prepare
                    // snapshot done event.
                    result.status = rc;
                    result.request_api = QCAMERA_SM_EVT_PREPARE_SNAPSHOT;
                    result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
                    m_parent->signalAPIResult(&result);

                    result.status = rc;
                    result.request_api = QCAMERA_SM_EVT_TAKE_PICTURE;
                    result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
                    m_parent->signalAPIResult(&result);

                    m_parent->sendEvtNotify(CAMERA_MSG_ERROR,
                                            CAMERA_ERROR_SERVER_DIED,
                                            0);
                }
                break;
            case CAM_EVENT_TYPE_CAC_DONE:
                if (m_parent->isCACEnabled() || m_parent->mParameters.isOEMFeatEnabled()) {
                    LOGD("[LONG_SHOT_DBG] : Received CAC Done");
                    if ((m_parent->isLongshotEnabled())
                            && (!m_parent->isCaptureShutterEnabled())) {
                        // play shutter sound for longshot
                        // after CAC stage is done
                        m_parent->playShutter();
                    }
                    m_parent->mCACDoneReceived = TRUE;
                }
                break;
            default:
                LOGE("Invalid internal event %d in state(%d)",
                             cam_evt->server_event_type, m_state);
                break;
            }
        }
        break;
    case QCAMERA_SM_EVT_JPEG_EVT_NOTIFY:
        {
            LOGL("Calling Process Jpeg Notify");
            qcamera_jpeg_evt_payload_t *jpeg_job =
                (qcamera_jpeg_evt_payload_t *)payload;
            rc = m_parent->processJpegNotify(jpeg_job);
        }
        break;
    case QCAMERA_SM_EVT_SNAPSHOT_DONE:
        {
            LOGL("Snapshot Done");
            if (m_parent->isZSLMode() || m_parent->isLongshotEnabled()) {
                rc = m_parent->cancelPicture();
            } else {
                rc = m_parent->cancelLiveSnapshot();
            }
            m_state = QCAMERA_SM_STATE_PREVIEWING;
            if (m_parent->isRetroPicture()){
                result.status = rc;
                result.request_api = evt;
                result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
                LOGL("\n Signalling for JPEG snapshot done!!");
                m_parent->signalAPIResult(&result);

            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalEvtResult(&result);
        }
        break;
    case QCAMERA_SM_EVT_THERMAL_NOTIFY:
        {
            rc = m_parent->updateThermalLevel(payload);
        }
        break;
    case QCAMERA_SM_EVT_RESTART_STOP_PREVIEW:
        {
            m_parent->stopPreview();
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
       break;
    case QCAMERA_SM_EVT_RESTART_START_PREVIEW:
        {
            rc = m_parent->preparePreview();
            if (rc == NO_ERROR) {
                rc = m_parent->startPreview();
            }
            result.status = rc;
            result.request_api = evt;
            result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
            m_parent->signalAPIResult(&result);
        }
       break;
    default:
        LOGW("Cannot handle evt(%d) in state(%d)", evt, m_state);
        break;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : isRecording
 *
 * DESCRIPTION: check if recording is in process.
 *
 * PARAMETERS : None
 *
 * RETURN     : true -- recording
 *              false -- not in recording mode
 *==========================================================================*/
bool QCameraStateMachine::isRecording()
{
    switch (m_state) {
    case QCAMERA_SM_STATE_RECORDING:
    case QCAMERA_SM_STATE_VIDEO_PIC_TAKING:
        return true;
    default:
        return false;
    }
}

/*===========================================================================
 * FUNCTION   : isPreviewRunning
 *
 * DESCRIPTION: check if preview is in process.
 *
 * PARAMETERS : None
 *
 * RETURN     : true -- preview running
 *              false -- preview stopped
 *==========================================================================*/
bool QCameraStateMachine::isPreviewRunning()
{
    switch (m_state) {
    case QCAMERA_SM_STATE_PREVIEWING:
    case QCAMERA_SM_STATE_RECORDING:
    case QCAMERA_SM_STATE_VIDEO_PIC_TAKING:
    case QCAMERA_SM_STATE_PREVIEW_PIC_TAKING:
    case QCAMERA_SM_STATE_PREPARE_SNAPSHOT:
    case QCAMERA_SM_STATE_PREVIEW_READY:
        return true;
    default:
        return false;
    }
}

/*===========================================================================
 * FUNCTION   : isPreviewReady
 *
 * DESCRIPTION: check if preview is in ready state.
 *
 * PARAMETERS : None
 *
 * RETURN     : true -- preview is in ready state
 *              false -- preview is stopped
 *==========================================================================*/
bool QCameraStateMachine::isPreviewReady()
{
    switch (m_state) {
    case QCAMERA_SM_STATE_PREVIEW_READY:
        return true;
    default:
        return false;
    }
}

/*===========================================================================
 * FUNCTION   : isCaptureRunning
 *
 * DESCRIPTION: check if image capture is in process.
 *
 * PARAMETERS : None
 *
 * RETURN     : true -- capture running
 *              false -- capture stopped
 *==========================================================================*/
bool QCameraStateMachine::isCaptureRunning()
{
    switch (m_state) {
    case QCAMERA_SM_STATE_PIC_TAKING:
    case QCAMERA_SM_STATE_VIDEO_PIC_TAKING:
    case QCAMERA_SM_STATE_PREVIEW_PIC_TAKING:
        return true;
    default:
        return false;
    }
}
/*===========================================================================
 * FUNCTION   : isNonZSLCaptureRunning
 *
 * DESCRIPTION: check if image capture is in process in non ZSL mode.
 *
 * PARAMETERS : None
 *
 * RETURN     : true -- capture running in non ZSL mode
 *              false -- Either in not capture mode or captur is not in non ZSL mode
 *==========================================================================*/
bool QCameraStateMachine::isNonZSLCaptureRunning()
{
    switch (m_state) {
    case QCAMERA_SM_STATE_PIC_TAKING:
        return true;
    default:
        return false;
    }
}

/*===========================================================================
 * FUNCTION   : dump
 *
 * DESCRIPTION: Composes a string based on current configuration
 *
 * PARAMETERS : none
 *
 * RETURN     : Formatted string
 *==========================================================================*/
String8 QCameraStateMachine::dump()
{
    String8 str("\n");
    char s[128];

    snprintf(s, 128, "Is Preview Running: %d\n", isPreviewRunning());
    str += s;

    snprintf(s, 128, "Is Capture Running: %d\n", isCaptureRunning());
    str += s;

    snprintf(s, 128, "Is Non ZSL Capture Running: %d\n",
        isNonZSLCaptureRunning());
    str += s;

    snprintf(s, 128, "Current State: %d \n", m_state);
    str += s;

    switch(m_state){
        case QCAMERA_SM_STATE_PREVIEW_STOPPED:
        snprintf(s, 128, " QCAMERA_SM_STATE_PREVIEW_STOPPED \n");
        break;

        case QCAMERA_SM_STATE_PREVIEW_READY:
        snprintf(s, 128, " QCAMERA_SM_STATE_PREVIEW_READY \n");
        break;

        case QCAMERA_SM_STATE_PREVIEWING:
        snprintf(s, 128, " QCAMERA_SM_STATE_PREVIEWING \n");
        break;

        case QCAMERA_SM_STATE_PREPARE_SNAPSHOT:
        snprintf(s, 128, " QCAMERA_SM_STATE_PREPARE_SNAPSHOT \n");
        break;

        case QCAMERA_SM_STATE_PIC_TAKING:
        snprintf(s, 128, " QCAMERA_SM_STATE_PIC_TAKING \n");
        break;

        case QCAMERA_SM_STATE_RECORDING:
        snprintf(s, 128, " QCAMERA_SM_STATE_RECORDING \n");
        break;

        case QCAMERA_SM_STATE_VIDEO_PIC_TAKING:
        snprintf(s, 128, " QCAMERA_SM_STATE_VIDEO_PIC_TAKING \n");
        break;

        case QCAMERA_SM_STATE_PREVIEW_PIC_TAKING:
        snprintf(s, 128, " QCAMERA_SM_STATE_PREVIEW_PIC_TAKING \n");
        break;
    }
    str += s;

    return str;
}

}; // namespace qcamera
