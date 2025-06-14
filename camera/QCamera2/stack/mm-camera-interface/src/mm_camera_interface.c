/* Copyright (c) 2012-2016, The Linux Foundation. All rights reserved.
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

// To remove
#include <cutils/properties.h>

// System dependencies
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <linux/media.h>
#include <media/msm_cam_sensor-legacy-m.h>
#include <dlfcn.h>

#define IOCTL_H <SYSTEM_HEADER_PREFIX/ioctl.h>
#include IOCTL_H

#define EXTRA_ENTRY 8

// Camera dependencies
#include "mm_camera_dbg.h"
#include "mm_camera_interface.h"
#include "mm_camera.h"

int32_t (*mm_camera_shim_module_init)(mm_camera_shim_ops_t *shim_ops);

static pthread_mutex_t g_intf_lock = PTHREAD_MUTEX_INITIALIZER;

static mm_camera_ctrl_t g_cam_ctrl;

static pthread_mutex_t g_handler_lock = PTHREAD_MUTEX_INITIALIZER;
static uint16_t g_handler_history_count = 0; /* history count for handler */

// 16th (starting from 0) bit tells its a BACK or FRONT camera
#define CAM_SENSOR_FACING_MASK (1U<<16)
// 24th (starting from 0) bit tells its a MAIN or AUX camera
#define CAM_SENSOR_TYPE_MASK (1U<<24)
// 25th (starting from 0) bit tells its YUV sensor or not
#define CAM_SENSOR_FORMAT_MASK (1U<<25)

/*===========================================================================
 * FUNCTION   : mm_camera_util_generate_handler
 *
 * DESCRIPTION: utility function to generate handler for camera/channel/stream
 *
 * PARAMETERS :
 *   @index: index of the object to have handler
 *
 * RETURN     : uint32_t type of handle that uniquely identify the object
 *==========================================================================*/
uint32_t mm_camera_util_generate_handler(uint8_t index)
{
    uint32_t handler = 0;
    pthread_mutex_lock(&g_handler_lock);
    g_handler_history_count++;
    if (0 == g_handler_history_count) {
        g_handler_history_count++;
    }
    handler = g_handler_history_count;
    handler = (handler<<8) | index;
    pthread_mutex_unlock(&g_handler_lock);
    return handler;
}

/*===========================================================================
 * FUNCTION   : mm_camera_util_get_index_by_handler
 *
 * DESCRIPTION: utility function to get index from handle
 *
 * PARAMETERS :
 *   @handler: object handle
 *
 * RETURN     : uint8_t type of index derived from handle
 *==========================================================================*/
uint8_t mm_camera_util_get_index_by_handler(uint32_t handler)
{
    return (handler&0x000000ff);
}

/*===========================================================================
 * FUNCTION   : mm_camera_util_get_dev_name
 *
 * DESCRIPTION: utility function to get device name from camera handle
 *
 * PARAMETERS :
 *   @cam_handle: camera handle
 *
 * RETURN     : char ptr to the device name stored in global variable
 * NOTE       : caller should not free the char ptr
 *==========================================================================*/
const char *mm_camera_util_get_dev_name(uint32_t cam_handle)
{
    char *dev_name = NULL;
    uint8_t cam_idx = mm_camera_util_get_index_by_handler(cam_handle);
    if(cam_idx < MM_CAMERA_MAX_NUM_SENSORS) {
        dev_name = g_cam_ctrl.video_dev_name[cam_idx];
    }
    return dev_name;
}

/*===========================================================================
 * FUNCTION   : mm_camera_util_get_camera_by_handler
 *
 * DESCRIPTION: utility function to get camera object from camera handle
 *
 * PARAMETERS :
 *   @cam_handle: camera handle
 *
 * RETURN     : ptr to the camera object stored in global variable
 * NOTE       : caller should not free the camera object ptr
 *==========================================================================*/
mm_camera_obj_t* mm_camera_util_get_camera_by_handler(uint32_t cam_handle)
{
    mm_camera_obj_t *cam_obj = NULL;
    uint8_t cam_idx = mm_camera_util_get_index_by_handler(cam_handle);

    if (cam_idx < MM_CAMERA_MAX_NUM_SENSORS &&
        (NULL != g_cam_ctrl.cam_obj[cam_idx]) &&
        (cam_handle == g_cam_ctrl.cam_obj[cam_idx]->my_hdl)) {
        cam_obj = g_cam_ctrl.cam_obj[cam_idx];
    }
    return cam_obj;
}

/*===========================================================================
 * FUNCTION   : mm_camera_util_get_camera_by_session_id
 *
 * DESCRIPTION: utility function to get camera object from camera sessionID
 *
 * PARAMETERS :
 *   @session_id: sessionid for which cam obj mapped
 *
 * RETURN     : ptr to the camera object stored in global variable
 * NOTE       : caller should not free the camera object ptr
 *==========================================================================*/
mm_camera_obj_t* mm_camera_util_get_camera_by_session_id(uint32_t session_id)
{
   int cam_idx = 0;
   mm_camera_obj_t *cam_obj = NULL;
   for (cam_idx = 0; cam_idx < MM_CAMERA_MAX_NUM_SENSORS; cam_idx++) {
        if ((NULL != g_cam_ctrl.cam_obj[cam_idx]) &&
                (session_id == (uint32_t)g_cam_ctrl.cam_obj[cam_idx]->sessionid)) {
            LOGD("session id:%d match idx:%d\n", session_id, cam_idx);
            cam_obj = g_cam_ctrl.cam_obj[cam_idx];
        }
    }
    return cam_obj;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_query_capability
 *
 * DESCRIPTION: query camera capability
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_query_capability(uint32_t camera_handle)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    LOGD("E: camera_handler = %d ", camera_handle);

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_query_capability(my_obj);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X rc = %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_set_parms
 *
 * DESCRIPTION: set parameters per camera
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @parms        : ptr to a param struct to be set to server
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 * NOTE       : Assume the parms struct buf is already mapped to server via
 *              domain socket. Corresponding fields of parameters to be set
 *              are already filled in by upper layer caller.
 *==========================================================================*/
static int32_t mm_camera_intf_set_parms(uint32_t camera_handle,
                                        parm_buffer_t *parms)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_set_parms(my_obj, parms);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_get_parms
 *
 * DESCRIPTION: get parameters per camera
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @parms        : ptr to a param struct to be get from server
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 * NOTE       : Assume the parms struct buf is already mapped to server via
 *              domain socket. Parameters to be get from server are already
 *              filled in by upper layer caller. After this call, corresponding
 *              fields of requested parameters will be filled in by server with
 *              detailed information.
 *==========================================================================*/
static int32_t mm_camera_intf_get_parms(uint32_t camera_handle,
                                        parm_buffer_t *parms)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_get_parms(my_obj, parms);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_do_auto_focus
 *
 * DESCRIPTION: performing auto focus
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 * NOTE       : if this call success, we will always assume there will
 *              be an auto_focus event following up.
 *==========================================================================*/
static int32_t mm_camera_intf_do_auto_focus(uint32_t camera_handle)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_do_auto_focus(my_obj);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_cancel_auto_focus
 *
 * DESCRIPTION: cancel auto focus
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_cancel_auto_focus(uint32_t camera_handle)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_cancel_auto_focus(my_obj);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_prepare_snapshot
 *
 * DESCRIPTION: prepare hardware for snapshot
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @do_af_flag   : flag indicating if AF is needed
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_prepare_snapshot(uint32_t camera_handle,
                                               int32_t do_af_flag)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_prepare_snapshot(my_obj, do_af_flag);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_flush
 *
 * DESCRIPTION: flush the current camera state and buffers
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_flush(uint32_t camera_handle)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_flush(my_obj);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_close
 *
 * DESCRIPTION: close a camera by its handle
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_close(uint32_t camera_handle)
{
    int32_t rc = -1;
    uint8_t cam_idx = camera_handle & 0x00ff;
    mm_camera_obj_t * my_obj = NULL;

    LOGD("E: camera_handler = %d ", camera_handle);

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if (my_obj){
        my_obj->ref_count--;

        if(my_obj->ref_count > 0) {
            /* still have reference to obj, return here */
            LOGD("ref_count=%d\n", my_obj->ref_count);
            pthread_mutex_unlock(&g_intf_lock);
            rc = 0;
        } else {
            /* need close camera here as no other reference
             * first empty g_cam_ctrl's referent to cam_obj */
            g_cam_ctrl.cam_obj[cam_idx] = NULL;

            pthread_mutex_lock(&my_obj->cam_lock);
            pthread_mutex_unlock(&g_intf_lock);
            rc = mm_camera_close(my_obj);
            pthread_mutex_destroy(&my_obj->cam_lock);
            free(my_obj);
        }
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_add_channel
 *
 * DESCRIPTION: add a channel
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @attr         : bundle attribute of the channel if needed
 *   @channel_cb   : callback function for bundle data notify
 *   @userdata     : user data ptr
 *
 * RETURN     : uint32_t type of channel handle
 *              0  -- invalid channel handle, meaning the op failed
 *              >0 -- successfully added a channel with a valid handle
 * NOTE       : if no bundle data notify is needed, meaning each stream in the
 *              channel will have its own stream data notify callback, then
 *              attr, channel_cb, and userdata can be NULL. In this case,
 *              no matching logic will be performed in channel for the bundling.
 *==========================================================================*/
static uint32_t mm_camera_intf_add_channel(uint32_t camera_handle,
                                           mm_camera_channel_attr_t *attr,
                                           mm_camera_buf_notify_t channel_cb,
                                           void *userdata)
{
    uint32_t ch_id = 0;
    mm_camera_obj_t * my_obj = NULL;

    LOGD("E camera_handler = %d", camera_handle);
    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        ch_id = mm_camera_add_channel(my_obj, attr, channel_cb, userdata);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X ch_id = %d", ch_id);
    return ch_id;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_del_channel
 *
 * DESCRIPTION: delete a channel by its handle
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 * NOTE       : all streams in the channel should be stopped already before
 *              this channel can be deleted.
 *==========================================================================*/
static int32_t mm_camera_intf_del_channel(uint32_t camera_handle,
                                          uint32_t ch_id)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    LOGD("E ch_id = %d", ch_id);
    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_del_channel(my_obj, ch_id);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X");
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_get_bundle_info
 *
 * DESCRIPTION: query bundle info of the channel
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *   @bundle_info  : bundle info to be filled in
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 * NOTE       : all streams in the channel should be stopped already before
 *              this channel can be deleted.
 *==========================================================================*/
static int32_t mm_camera_intf_get_bundle_info(uint32_t camera_handle,
                                              uint32_t ch_id,
                                              cam_bundle_config_t *bundle_info)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    LOGD("E ch_id = %d", ch_id);
    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_get_bundle_info(my_obj, ch_id, bundle_info);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X");
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_register_event_notify
 *
 * DESCRIPTION: register for event notify
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @evt_cb       : callback for event notify
 *   @user_data    : user data ptr
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_register_event_notify(uint32_t camera_handle,
                                                    mm_camera_event_notify_t evt_cb,
                                                    void * user_data)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    LOGD("E ");
    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_register_event_notify(my_obj, evt_cb, user_data);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("E rc = %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_qbuf
 *
 * DESCRIPTION: enqueue buffer back to kernel
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *   @buf          : buf ptr to be enqueued
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_qbuf(uint32_t camera_handle,
                                    uint32_t ch_id,
                                    mm_camera_buf_def_t *buf)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_qbuf(my_obj, ch_id, buf);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X evt_type = %d",rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_qbuf
 *
 * DESCRIPTION: enqueue buffer back to kernel
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *   @buf          : buf ptr to be enqueued
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_cancel_buf(uint32_t camera_handle, uint32_t ch_id, uint32_t stream_id,
                     uint32_t buf_idx)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_cancel_buf(my_obj, ch_id, stream_id, buf_idx);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X evt_type = %d",rc);
    return rc;
}


/*===========================================================================
 * FUNCTION   : mm_camera_intf_get_queued_buf_count
 *
 * DESCRIPTION: returns the queued buffer count
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *   @stream_id : stream id
 *
 * RETURN     : int32_t - queued buffer count
 *
 *==========================================================================*/
static int32_t mm_camera_intf_get_queued_buf_count(uint32_t camera_handle,
        uint32_t ch_id, uint32_t stream_id)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_get_queued_buf_count(my_obj, ch_id, stream_id);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X queued buffer count = %d",rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_link_stream
 *
 * DESCRIPTION: link a stream into a new channel
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *   @stream_id    : stream id
 *   @linked_ch_id : channel in which the stream will be linked
 *
 * RETURN     : int32_t type of stream handle
 *              0  -- invalid stream handle, meaning the op failed
 *              >0 -- successfully linked a stream with a valid handle
 *==========================================================================*/
static int32_t mm_camera_intf_link_stream(uint32_t camera_handle,
        uint32_t ch_id,
        uint32_t stream_id,
        uint32_t linked_ch_id)
{
    uint32_t id = 0;
    mm_camera_obj_t * my_obj = NULL;

    LOGD("E handle = %u ch_id = %u",
          camera_handle, ch_id);

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        id = mm_camera_link_stream(my_obj, ch_id, stream_id, linked_ch_id);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }

    LOGD("X stream_id = %u", stream_id);
    return (int32_t)id;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_add_stream
 *
 * DESCRIPTION: add a stream into a channel
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *
 * RETURN     : uint32_t type of stream handle
 *              0  -- invalid stream handle, meaning the op failed
 *              >0 -- successfully added a stream with a valid handle
 *==========================================================================*/
static uint32_t mm_camera_intf_add_stream(uint32_t camera_handle,
                                          uint32_t ch_id)
{
    uint32_t stream_id = 0;
    mm_camera_obj_t * my_obj = NULL;

    LOGD("E handle = %d ch_id = %d",
          camera_handle, ch_id);

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        stream_id = mm_camera_add_stream(my_obj, ch_id);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X stream_id = %d", stream_id);
    return stream_id;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_del_stream
 *
 * DESCRIPTION: delete a stream by its handle
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *   @stream_id    : stream handle
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 * NOTE       : stream should be stopped already before it can be deleted.
 *==========================================================================*/
static int32_t mm_camera_intf_del_stream(uint32_t camera_handle,
                                         uint32_t ch_id,
                                         uint32_t stream_id)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    LOGD("E handle = %d ch_id = %d stream_id = %d",
          camera_handle, ch_id, stream_id);

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_del_stream(my_obj, ch_id, stream_id);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X rc = %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_config_stream
 *
 * DESCRIPTION: configure a stream
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *   @stream_id    : stream handle
 *   @config       : stream configuration
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_config_stream(uint32_t camera_handle,
                                            uint32_t ch_id,
                                            uint32_t stream_id,
                                            mm_camera_stream_config_t *config)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    LOGD("E handle = %d, ch_id = %d,stream_id = %d",
          camera_handle, ch_id, stream_id);

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    LOGD("mm_camera_intf_config_stream stream_id = %d",stream_id);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_config_stream(my_obj, ch_id, stream_id, config);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X rc = %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_start_channel
 *
 * DESCRIPTION: start a channel, which will start all streams in the channel
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_start_channel(uint32_t camera_handle,
                                            uint32_t ch_id)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_start_channel(my_obj, ch_id);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X rc = %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_stop_channel
 *
 * DESCRIPTION: stop a channel, which will stop all streams in the channel
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_stop_channel(uint32_t camera_handle,
                                           uint32_t ch_id)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_stop_channel(my_obj, ch_id);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X rc = %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_request_super_buf
 *
 * DESCRIPTION: for burst mode in bundle, reuqest certain amount of matched
 *              frames from superbuf queue
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id             : channel handle
 *   @buf                : request buffer info
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_request_super_buf(uint32_t camera_handle,
        uint32_t ch_id, mm_camera_req_buf_t *buf)
{
    int32_t rc = -1;
    LOGD("E camera_handler = %d,ch_id = %d",
          camera_handle, ch_id);
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj && buf) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_request_super_buf (my_obj, ch_id, buf);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X rc = %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_cancel_super_buf_request
 *
 * DESCRIPTION: for burst mode in bundle, cancel the reuqest for certain amount
 *              of matched frames from superbuf queue
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_cancel_super_buf_request(uint32_t camera_handle,
                                                       uint32_t ch_id)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    LOGD("E camera_handler = %d,ch_id = %d",
          camera_handle, ch_id);
    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_cancel_super_buf_request(my_obj, ch_id);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X rc = %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_flush_super_buf_queue
 *
 * DESCRIPTION: flush out all frames in the superbuf queue
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *   @frame_idx    : frame index
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_flush_super_buf_queue(uint32_t camera_handle,
                                                    uint32_t ch_id, uint32_t frame_idx)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    LOGD("E camera_handler = %d,ch_id = %d",
          camera_handle, ch_id);
    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_flush_super_buf_queue(my_obj, ch_id, frame_idx);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X rc = %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_start_zsl_snapshot
 *
 * DESCRIPTION: Starts zsl snapshot
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_start_zsl_snapshot(uint32_t camera_handle,
        uint32_t ch_id)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    LOGD("E camera_handler = %d,ch_id = %d",
          camera_handle, ch_id);
    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_start_zsl_snapshot_ch(my_obj, ch_id);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X rc = %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_stop_zsl_snapshot
 *
 * DESCRIPTION: Stops zsl snapshot
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_stop_zsl_snapshot(uint32_t camera_handle,
        uint32_t ch_id)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    LOGD("E camera_handler = %d,ch_id = %d",
          camera_handle, ch_id);
    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_stop_zsl_snapshot_ch(my_obj, ch_id);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X rc = %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_configure_notify_mode
 *
 * DESCRIPTION: Configures channel notification mode
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *   @notify_mode  : notification mode
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_configure_notify_mode(uint32_t camera_handle,
                                                    uint32_t ch_id,
                                                    mm_camera_super_buf_notify_mode_t notify_mode)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    LOGD("E camera_handler = %d,ch_id = %d",
          camera_handle, ch_id);
    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_config_channel_notify(my_obj, ch_id, notify_mode);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X rc = %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_map_buf
 *
 * DESCRIPTION: mapping camera buffer via domain socket to server
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @buf_type     : type of buffer to be mapped. could be following values:
 *                   CAM_MAPPING_BUF_TYPE_CAPABILITY
 *                   CAM_MAPPING_BUF_TYPE_SETPARM_BUF
 *                   CAM_MAPPING_BUF_TYPE_GETPARM_BUF
 *   @fd           : file descriptor of the buffer
 *   @size         : size of the buffer
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_map_buf(uint32_t camera_handle,
    uint8_t buf_type, int fd, size_t size, void *buffer)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_map_buf(my_obj, buf_type, fd, size, buffer);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_map_bufs
 *
 * DESCRIPTION: mapping camera buffer via domain socket to server
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @buf_type     : type of buffer to be mapped. could be following values:
 *                   CAM_MAPPING_BUF_TYPE_CAPABILITY
 *                   CAM_MAPPING_BUF_TYPE_SETPARM_BUF
 *                   CAM_MAPPING_BUF_TYPE_GETPARM_BUF
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_map_bufs(uint32_t camera_handle,
        const cam_buf_map_type_list *buf_map_list)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_map_bufs(my_obj, buf_map_list);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_unmap_buf
 *
 * DESCRIPTION: unmapping camera buffer via domain socket to server
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @buf_type     : type of buffer to be unmapped. could be following values:
 *                   CAM_MAPPING_BUF_TYPE_CAPABILITY
 *                   CAM_MAPPING_BUF_TYPE_SETPARM_BUF
 *                   CAM_MAPPING_BUF_TYPE_GETPARM_BUF
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_unmap_buf(uint32_t camera_handle,
                                        uint8_t buf_type)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_unmap_buf(my_obj, buf_type);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_set_stream_parms
 *
 * DESCRIPTION: set parameters per stream
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *   @s_id         : stream handle
 *   @parms        : ptr to a param struct to be set to server
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 * NOTE       : Assume the parms struct buf is already mapped to server via
 *              domain socket. Corresponding fields of parameters to be set
 *              are already filled in by upper layer caller.
 *==========================================================================*/
static int32_t mm_camera_intf_set_stream_parms(uint32_t camera_handle,
                                               uint32_t ch_id,
                                               uint32_t s_id,
                                               cam_stream_parm_buffer_t *parms)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    LOGD("E camera_handle = %d,ch_id = %d,s_id = %d",
          camera_handle, ch_id, s_id);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_set_stream_parms(my_obj, ch_id, s_id, parms);
    }else{
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X rc = %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_get_stream_parms
 *
 * DESCRIPTION: get parameters per stream
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *   @s_id         : stream handle
 *   @parms        : ptr to a param struct to be get from server
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 * NOTE       : Assume the parms struct buf is already mapped to server via
 *              domain socket. Parameters to be get from server are already
 *              filled in by upper layer caller. After this call, corresponding
 *              fields of requested parameters will be filled in by server with
 *              detailed information.
 *==========================================================================*/
static int32_t mm_camera_intf_get_stream_parms(uint32_t camera_handle,
                                               uint32_t ch_id,
                                               uint32_t s_id,
                                               cam_stream_parm_buffer_t *parms)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    LOGD("E camera_handle = %d,ch_id = %d,s_id = %d",
          camera_handle, ch_id, s_id);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_get_stream_parms(my_obj, ch_id, s_id, parms);
    }else{
        pthread_mutex_unlock(&g_intf_lock);
    }

    LOGD("X rc = %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_map_stream_buf
 *
 * DESCRIPTION: mapping stream buffer via domain socket to server
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *   @s_id         : stream handle
 *   @buf_type     : type of buffer to be mapped. could be following values:
 *                   CAM_MAPPING_BUF_TYPE_STREAM_BUF
 *                   CAM_MAPPING_BUF_TYPE_STREAM_INFO
 *                   CAM_MAPPING_BUF_TYPE_OFFLINE_INPUT_BUF
 *   @buf_idx      : index of buffer within the stream buffers, only valid if
 *                   buf_type is CAM_MAPPING_BUF_TYPE_STREAM_BUF or
 *                   CAM_MAPPING_BUF_TYPE_OFFLINE_INPUT_BUF
 *   @plane_idx    : plane index. If all planes share the same fd,
 *                   plane_idx = -1; otherwise, plean_idx is the
 *                   index to plane (0..num_of_planes)
 *   @fd           : file descriptor of the buffer
 *   @size         : size of the buffer
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_map_stream_buf(uint32_t camera_handle,
        uint32_t ch_id, uint32_t stream_id, uint8_t buf_type,
        uint32_t buf_idx, int32_t plane_idx, int fd,
        size_t size, void *buffer)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    LOGD("E camera_handle = %d, ch_id = %d, s_id = %d, buf_idx = %d, plane_idx = %d",
          camera_handle, ch_id, stream_id, buf_idx, plane_idx);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_map_stream_buf(my_obj, ch_id, stream_id,
                                      buf_type, buf_idx, plane_idx,
                                      fd, size, buffer);
    }else{
        pthread_mutex_unlock(&g_intf_lock);
    }

    LOGD("X rc = %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_map_stream_bufs
 *
 * DESCRIPTION: mapping stream buffers via domain socket to server
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *   @buf_map_list : list of buffers to be mapped
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_map_stream_bufs(uint32_t camera_handle,
                                              uint32_t ch_id,
                                              const cam_buf_map_type_list *buf_map_list)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    LOGD("E camera_handle = %d, ch_id = %d",
          camera_handle, ch_id);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_map_stream_bufs(my_obj, ch_id, buf_map_list);
    }else{
        pthread_mutex_unlock(&g_intf_lock);
    }

    LOGD("X rc = %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_unmap_stream_buf
 *
 * DESCRIPTION: unmapping stream buffer via domain socket to server
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *   @s_id         : stream handle
 *   @buf_type     : type of buffer to be unmapped. could be following values:
 *                   CAM_MAPPING_BUF_TYPE_STREAM_BUF
 *                   CAM_MAPPING_BUF_TYPE_STREAM_INFO
 *                   CAM_MAPPING_BUF_TYPE_OFFLINE_INPUT_BUF
 *   @buf_idx      : index of buffer within the stream buffers, only valid if
 *                   buf_type is CAM_MAPPING_BUF_TYPE_STREAM_BUF or
 *                   CAM_MAPPING_BUF_TYPE_OFFLINE_INPUT_BUF
 *   @plane_idx    : plane index. If all planes share the same fd,
 *                   plane_idx = -1; otherwise, plean_idx is the
 *                   index to plane (0..num_of_planes)
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_unmap_stream_buf(uint32_t camera_handle,
                                               uint32_t ch_id,
                                               uint32_t stream_id,
                                               uint8_t buf_type,
                                               uint32_t buf_idx,
                                               int32_t plane_idx)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    LOGD("E camera_handle = %d, ch_id = %d, s_id = %d, buf_idx = %d, plane_idx = %d",
          camera_handle, ch_id, stream_id, buf_idx, plane_idx);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_unmap_stream_buf(my_obj, ch_id, stream_id,
                                        buf_type, buf_idx, plane_idx);
    }else{
        pthread_mutex_unlock(&g_intf_lock);
    }

    LOGD("X rc = %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_get_session_id
 *
 * DESCRIPTION: retrieve the session ID from the kernel for this HWI instance
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @sessionid: session id to be retrieved from server
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 * NOTE       : if this call succeeds, we will get a valid session id.
 *==========================================================================*/
static int32_t mm_camera_intf_get_session_id(uint32_t camera_handle,
                                                       uint32_t* sessionid)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        *sessionid = my_obj->sessionid;
        pthread_mutex_unlock(&my_obj->cam_lock);
        rc = 0;
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_sync_related_sensors
 *
 * DESCRIPTION: retrieve the session ID from the kernel for this HWI instance
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @related_cam_info: pointer to the related cam info to be sent to the server
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 * NOTE       : if this call succeeds, we will get linking established in back end
 *==========================================================================*/
static int32_t mm_camera_intf_sync_related_sensors(uint32_t camera_handle,
                              cam_sync_related_sensors_event_info_t* related_cam_info)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_sync_related_sensors(my_obj, related_cam_info);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : get_sensor_info
 *
 * DESCRIPTION: get sensor info like facing(back/front) and mount angle
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void get_sensor_info()
{
    int rc = 0;
    int dev_fd = -1;
    struct media_device_info mdev_info;
    int num_media_devices = 0;
    size_t num_cameras = 0;

    LOGD("E");
    while (1) {
        char dev_name[32];
        snprintf(dev_name, sizeof(dev_name), "/dev/media%d", num_media_devices);
        dev_fd = open(dev_name, O_RDWR | O_NONBLOCK);
        if (dev_fd < 0) {
            LOGD("Done discovering media devices\n");
            break;
        }
        num_media_devices++;
        memset(&mdev_info, 0, sizeof(mdev_info));
        rc = ioctl(dev_fd, MEDIA_IOC_DEVICE_INFO, &mdev_info);
        if (rc < 0) {
            LOGE("Error: ioctl media_dev failed: %s\n", strerror(errno));
            close(dev_fd);
            dev_fd = -1;
            num_cameras = 0;
            break;
        }

        if(strncmp(mdev_info.model,  MSM_CONFIGURATION_NAME, sizeof(mdev_info.model)) != 0) {
            close(dev_fd);
            dev_fd = -1;
            continue;
        }

        unsigned int num_entities = 1;
        while (1) {
            struct media_entity_desc entity;
            uint32_t temp;
            uint32_t mount_angle;
            uint32_t facing;
            int32_t type = 0;
            uint8_t is_yuv;

            memset(&entity, 0, sizeof(entity));
            entity.id = num_entities++;
            rc = ioctl(dev_fd, MEDIA_IOC_ENUM_ENTITIES, &entity);
            if (rc < 0) {
                LOGD("Done enumerating media entities\n");
                rc = 0;
                break;
            }
            if(entity.type == MEDIA_ENT_T_V4L2_SUBDEV &&
                entity.group_id == MSM_CAMERA_SUBDEV_SENSOR) {
                temp = entity.flags >> 8;
                mount_angle = (temp & 0xFF) * 90;
                facing = ((entity.flags & CAM_SENSOR_FACING_MASK) ?
                        CAMERA_FACING_FRONT:CAMERA_FACING_BACK);
                /* TODO: Need to revisit this logic if front AUX is available. */
                if ((unsigned int)facing == CAMERA_FACING_FRONT) {
                    type = CAM_TYPE_STANDALONE;
                } else if (entity.flags & CAM_SENSOR_TYPE_MASK) {
                    type = CAM_TYPE_AUX;
                } else {
                    type = CAM_TYPE_MAIN;
                }
                is_yuv = ((entity.flags & CAM_SENSOR_FORMAT_MASK) ?
                        CAM_SENSOR_YUV:CAM_SENSOR_RAW);
                LOGL("index = %u flag = %x mount_angle = %u "
                        "facing = %u type: %u is_yuv = %u\n",
                        (unsigned int)num_cameras, (unsigned int)temp,
                        (unsigned int)mount_angle, (unsigned int)facing,
                        (unsigned int)type, (uint8_t)is_yuv);
                g_cam_ctrl.info[num_cameras].facing = (int)facing;
                g_cam_ctrl.info[num_cameras].orientation = (int)mount_angle;
                g_cam_ctrl.cam_type[num_cameras] = type;
                g_cam_ctrl.is_yuv[num_cameras] = is_yuv;
                LOGD("dev_info[id=%zu,name='%s']\n",
                         num_cameras, g_cam_ctrl.video_dev_name[num_cameras]);
                num_cameras++;
                continue;
            }
        }
        close(dev_fd);
        dev_fd = -1;
    }

    LOGD("num_cameras=%d\n", g_cam_ctrl.num_cam);
    return;
}

/*===========================================================================
 * FUNCTION   : sort_camera_info
 *
 * DESCRIPTION: sort camera info to keep back cameras idx is smaller than front cameras idx
 *
 * PARAMETERS : number of cameras
 *
 * RETURN     :
 *==========================================================================*/
void sort_camera_info(int num_cam)
{
    int idx = 0, i;
    int8_t is_yuv_aux_cam_exposed = 0;
    char prop[PROPERTY_VALUE_MAX];
    struct camera_info temp_info[MM_CAMERA_MAX_NUM_SENSORS];
    cam_sync_type_t temp_type[MM_CAMERA_MAX_NUM_SENSORS];
    cam_sync_mode_t temp_mode[MM_CAMERA_MAX_NUM_SENSORS];
    uint8_t temp_is_yuv[MM_CAMERA_MAX_NUM_SENSORS];
    char temp_dev_name[MM_CAMERA_MAX_NUM_SENSORS][MM_CAMERA_DEV_NAME_LEN];

    memset(temp_info, 0, sizeof(temp_info));
    memset(temp_dev_name, 0, sizeof(temp_dev_name));
    memset(temp_type, 0, sizeof(temp_type));
    memset(temp_mode, 0, sizeof(temp_mode));
    memset(temp_is_yuv, 0, sizeof(temp_is_yuv));

    // Signifies whether YUV AUX camera has to be exposed as physical camera
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.aux.yuv", prop, "0");
    is_yuv_aux_cam_exposed = atoi(prop);
    LOGI("YUV Aux camera exposed %d",is_yuv_aux_cam_exposed);

    /* Order of the camera exposed is
    Back main, Front main, Back Aux and then Front Aux.
    It is because that lot of 3rd party cameras apps
    blindly assume 0th is Back and 1st is front */

    /* Firstly save the main back cameras info */
    for (i = 0; i < num_cam; i++) {
        if ((g_cam_ctrl.info[i].facing == CAMERA_FACING_BACK) &&
            (g_cam_ctrl.cam_type[i] != CAM_TYPE_AUX)) {
            temp_info[idx] = g_cam_ctrl.info[i];
            temp_type[idx] = g_cam_ctrl.cam_type[i];
            temp_mode[idx] = g_cam_ctrl.cam_mode[i];
            temp_is_yuv[idx] = g_cam_ctrl.is_yuv[i];
            LOGD("Found Back Main Camera: i: %d idx: %d", i, idx);
            memcpy(temp_dev_name[idx++],g_cam_ctrl.video_dev_name[i],
                MM_CAMERA_DEV_NAME_LEN);
        }
    }

    /* Save the main front cameras info */
    for (i = 0; i < num_cam; i++) {
        if ((g_cam_ctrl.info[i].facing == CAMERA_FACING_FRONT) &&
            (g_cam_ctrl.cam_type[i] != CAM_TYPE_AUX)) {
            temp_info[idx] = g_cam_ctrl.info[i];
            temp_type[idx] = g_cam_ctrl.cam_type[i];
            temp_mode[idx] = g_cam_ctrl.cam_mode[i];
            temp_is_yuv[idx] = g_cam_ctrl.is_yuv[i];
            LOGD("Found Front Main Camera: i: %d idx: %d", i, idx);
            memcpy(temp_dev_name[idx++],g_cam_ctrl.video_dev_name[i],
                    MM_CAMERA_DEV_NAME_LEN);
        }
    }

    /* Expose YUV AUX camera if persist.camera.aux.yuv is set to 1.
    Otherwsie expose AUX camera if it is not YUV. */
    for (i = 0; i < num_cam; i++) {
        if ((g_cam_ctrl.info[i].facing == CAMERA_FACING_BACK) &&
                (g_cam_ctrl.cam_type[i] == CAM_TYPE_AUX) &&
                (is_yuv_aux_cam_exposed || !(g_cam_ctrl.is_yuv[i]))) {
            temp_info[idx] = g_cam_ctrl.info[i];
            temp_type[idx] = g_cam_ctrl.cam_type[i];
            temp_mode[idx] = g_cam_ctrl.cam_mode[i];
            temp_is_yuv[idx] = g_cam_ctrl.is_yuv[i];
            LOGD("Found back Aux Camera: i: %d idx: %d", i, idx);
            memcpy(temp_dev_name[idx++],g_cam_ctrl.video_dev_name[i],
                MM_CAMERA_DEV_NAME_LEN);
        }
    }

    /* Expose YUV AUX camera if persist.camera.aux.yuv is set to 1.
    Otherwsie expose AUX camera if it is not YUV. */
    for (i = 0; i < num_cam; i++) {
        if ((g_cam_ctrl.info[i].facing == CAMERA_FACING_FRONT) &&
                (g_cam_ctrl.cam_type[i] == CAM_TYPE_AUX) &&
                (is_yuv_aux_cam_exposed || !(g_cam_ctrl.is_yuv[i]))) {
            temp_info[idx] = g_cam_ctrl.info[i];
            temp_type[idx] = g_cam_ctrl.cam_type[i];
            temp_mode[idx] = g_cam_ctrl.cam_mode[i];
            temp_is_yuv[idx] = g_cam_ctrl.is_yuv[i];
            LOGD("Found Front Aux Camera: i: %d idx: %d", i, idx);
            memcpy(temp_dev_name[idx++],g_cam_ctrl.video_dev_name[i],
                MM_CAMERA_DEV_NAME_LEN);
        }
    }

    if (idx <= num_cam) {
        memcpy(g_cam_ctrl.info, temp_info, sizeof(temp_info));
        memcpy(g_cam_ctrl.cam_type, temp_type, sizeof(temp_type));
        memcpy(g_cam_ctrl.cam_mode, temp_mode, sizeof(temp_mode));
        memcpy(g_cam_ctrl.is_yuv, temp_is_yuv, sizeof(temp_is_yuv));
        memcpy(g_cam_ctrl.video_dev_name, temp_dev_name, sizeof(temp_dev_name));
        //Set num cam based on the cameras exposed finally via dual/aux properties.
        g_cam_ctrl.num_cam = idx;
        for (i = 0; i < idx; i++) {
            LOGI("Camera id: %d facing: %d, type: %d is_yuv: %d",
                i, g_cam_ctrl.info[i].facing, g_cam_ctrl.cam_type[i], g_cam_ctrl.is_yuv[i]);
        }
    }
    LOGI("Number of cameras %d sorted %d", num_cam, idx);
    return;
}

/*===========================================================================
 * FUNCTION   : get_num_of_cameras
 *
 * DESCRIPTION: get number of cameras
 *
 * PARAMETERS :
 *
 * RETURN     : number of cameras supported
 *==========================================================================*/
uint8_t get_num_of_cameras()
{
    int rc = 0;
    int i = 0;
    int dev_fd = -1;
    struct media_device_info mdev_info;
    int num_media_devices = 0;
    int8_t num_cameras = 0;
    char subdev_name[32];
    int32_t sd_fd = -1;
    struct sensor_init_cfg_data cfg;
    char prop[PROPERTY_VALUE_MAX];

    LOGD("E");

    property_get("vold.decrypt", prop, "0");
    int decrypt = atoi(prop);
    if (decrypt == 1)
     return 0;
    pthread_mutex_lock(&g_intf_lock);

    memset (&g_cam_ctrl, 0, sizeof (g_cam_ctrl));
#ifndef DAEMON_PRESENT
    if (mm_camera_load_shim_lib() < 0) {
        LOGE ("Failed to module shim library");
        return 0;
    }
#endif /* DAEMON_PRESENT */

    while (1) {
        uint32_t num_entities = 1U;
        char dev_name[32];

        snprintf(dev_name, sizeof(dev_name), "/dev/media%d", num_media_devices);
        dev_fd = open(dev_name, O_RDWR | O_NONBLOCK);
        if (dev_fd < 0) {
            LOGD("Done discovering media devices\n");
            break;
        }
        num_media_devices++;
        rc = ioctl(dev_fd, MEDIA_IOC_DEVICE_INFO, &mdev_info);
        if (rc < 0) {
            LOGE("Error: ioctl media_dev failed: %s\n", strerror(errno));
            close(dev_fd);
            dev_fd = -1;
            break;
        }

        if (strncmp(mdev_info.model, MSM_CONFIGURATION_NAME,
          sizeof(mdev_info.model)) != 0) {
            close(dev_fd);
            dev_fd = -1;
            continue;
        }

        while (1) {
            struct media_entity_desc entity;
            memset(&entity, 0, sizeof(entity));
            entity.id = num_entities++;
            LOGD("entity id %d", entity.id);
            rc = ioctl(dev_fd, MEDIA_IOC_ENUM_ENTITIES, &entity);
            if (rc < 0) {
                LOGD("Done enumerating media entities");
                rc = 0;
                break;
            }
            LOGD("entity name %s type %d group id %d",
                entity.name, entity.type, entity.group_id);
            if (entity.type == MEDIA_ENT_T_V4L2_SUBDEV &&
                entity.group_id == MSM_CAMERA_SUBDEV_SENSOR_INIT) {
                snprintf(subdev_name, sizeof(dev_name), "/dev/%s", entity.name);
                break;
            }
        }
        close(dev_fd);
        dev_fd = -1;
    }

    /* Open sensor_init subdev */
    sd_fd = open(subdev_name, O_RDWR);
    if (sd_fd < 0) {
        LOGE("Open sensor_init subdev failed");
        return FALSE;
    }

    cfg.cfgtype = CFG_SINIT_PROBE_WAIT_DONE;
    cfg.cfg.setting = NULL;
    if (ioctl(sd_fd, VIDIOC_MSM_SENSOR_INIT_CFG, &cfg) < 0) {
        LOGI("failed...Camera Daemon may not up so try again");
        for(i = 0; i < (MM_CAMERA_EVT_ENTRY_MAX + EXTRA_ENTRY); i++) {
            if (ioctl(sd_fd, VIDIOC_MSM_SENSOR_INIT_CFG, &cfg) < 0) {
                LOGI("failed...Camera Daemon may not up so try again");
                continue;
            }
            else
                break;
        }
    }
    close(sd_fd);
    dev_fd = -1;


    num_media_devices = 0;
    while (1) {
        uint32_t num_entities = 1U;
        char dev_name[32];

        snprintf(dev_name, sizeof(dev_name), "/dev/media%d", num_media_devices);
        dev_fd = open(dev_name, O_RDWR | O_NONBLOCK);
        if (dev_fd < 0) {
            LOGD("Done discovering media devices: %s\n", strerror(errno));
            break;
        }
        num_media_devices++;
        memset(&mdev_info, 0, sizeof(mdev_info));
        rc = ioctl(dev_fd, MEDIA_IOC_DEVICE_INFO, &mdev_info);
        if (rc < 0) {
            LOGE("Error: ioctl media_dev failed: %s\n", strerror(errno));
            close(dev_fd);
            dev_fd = -1;
            num_cameras = 0;
            break;
        }

        if(strncmp(mdev_info.model, MSM_CAMERA_NAME, sizeof(mdev_info.model)) != 0) {
            close(dev_fd);
            dev_fd = -1;
            continue;
        }

        while (1) {
            struct media_entity_desc entity;
            memset(&entity, 0, sizeof(entity));
            entity.id = num_entities++;
            rc = ioctl(dev_fd, MEDIA_IOC_ENUM_ENTITIES, &entity);
            if (rc < 0) {
                LOGD("Done enumerating media entities\n");
                rc = 0;
                break;
            }
            if(entity.type == MEDIA_ENT_T_DEVNODE_V4L && entity.group_id == QCAMERA_VNODE_GROUP_ID) {
                strlcpy(g_cam_ctrl.video_dev_name[num_cameras],
                     entity.name, sizeof(entity.name));
                LOGE("dev_info[id=%d,name='%s']\n",
                    (int)num_cameras, g_cam_ctrl.video_dev_name[num_cameras]);
                num_cameras++;
                break;
            }
        }
        close(dev_fd);
        dev_fd = -1;
        if (num_cameras >= MM_CAMERA_MAX_NUM_SENSORS) {
            LOGW("Maximum number of camera reached %d", num_cameras);
            break;
        }
    }
    g_cam_ctrl.num_cam = num_cameras;

    get_sensor_info();
    sort_camera_info(g_cam_ctrl.num_cam);
    /* unlock the mutex */
    pthread_mutex_unlock(&g_intf_lock);
    LOGE("num_cameras=%d\n", (int)g_cam_ctrl.num_cam);
    return(uint8_t)g_cam_ctrl.num_cam;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_process_advanced_capture
 *
 * DESCRIPTION: Configures channel advanced capture mode
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @type : advanced capture type
 *   @ch_id        : channel handle
 *   @trigger  : 1 for start and 0 for cancel/stop
 *   @value  : input capture configaration
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_process_advanced_capture(uint32_t camera_handle,
        uint32_t ch_id, mm_camera_advanced_capture_t type,
        int8_t trigger, void *in_value)
{
    int32_t rc = -1;
    mm_camera_obj_t * my_obj = NULL;

    LOGD("E camera_handler = %d,ch_id = %d",
          camera_handle, ch_id);
    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_channel_advanced_capture(my_obj, ch_id, type,
                (uint32_t)trigger, in_value);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    LOGD("X ");
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_intf_register_stream_buf_cb
 *
 * DESCRIPTION: Register special callback for stream buffer
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id        : channel handle
 *   @stream_id    : stream handle
 *   @buf_cb       : callback function
 *   @buf_type     :SYNC/ASYNC
 *   @userdata     : userdata pointer
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              1 -- failure
 *==========================================================================*/
static int32_t mm_camera_intf_register_stream_buf_cb(uint32_t camera_handle,
        uint32_t ch_id, uint32_t stream_id, mm_camera_buf_notify_t buf_cb,
        mm_camera_stream_cb_type cb_type, void *userdata)
{
    int32_t rc = 0;
    mm_camera_obj_t * my_obj = NULL;

    LOGD("E handle = %u ch_id = %u",
          camera_handle, ch_id);

    pthread_mutex_lock(&g_intf_lock);
    my_obj = mm_camera_util_get_camera_by_handler(camera_handle);

    if(my_obj) {
        pthread_mutex_lock(&my_obj->cam_lock);
        pthread_mutex_unlock(&g_intf_lock);
        rc = mm_camera_reg_stream_buf_cb(my_obj, ch_id, stream_id,
                buf_cb, cb_type, userdata);
    } else {
        pthread_mutex_unlock(&g_intf_lock);
    }
    return (int32_t)rc;
}

struct camera_info *get_cam_info(uint32_t camera_id, cam_sync_type_t *pCamType)
{
    *pCamType = g_cam_ctrl.cam_type[camera_id];
    return &g_cam_ctrl.info[camera_id];
}

uint8_t is_yuv_sensor(uint32_t camera_id)
{
    return g_cam_ctrl.is_yuv[camera_id];
}

/* camera ops v-table */
static mm_camera_ops_t mm_camera_ops = {
    .query_capability = mm_camera_intf_query_capability,
    .register_event_notify = mm_camera_intf_register_event_notify,
    .close_camera = mm_camera_intf_close,
    .set_parms = mm_camera_intf_set_parms,
    .get_parms = mm_camera_intf_get_parms,
    .do_auto_focus = mm_camera_intf_do_auto_focus,
    .cancel_auto_focus = mm_camera_intf_cancel_auto_focus,
    .prepare_snapshot = mm_camera_intf_prepare_snapshot,
    .start_zsl_snapshot = mm_camera_intf_start_zsl_snapshot,
    .stop_zsl_snapshot = mm_camera_intf_stop_zsl_snapshot,
    .map_buf = mm_camera_intf_map_buf,
    .map_bufs = mm_camera_intf_map_bufs,
    .unmap_buf = mm_camera_intf_unmap_buf,
    .add_channel = mm_camera_intf_add_channel,
    .delete_channel = mm_camera_intf_del_channel,
    .get_bundle_info = mm_camera_intf_get_bundle_info,
    .add_stream = mm_camera_intf_add_stream,
    .link_stream = mm_camera_intf_link_stream,
    .delete_stream = mm_camera_intf_del_stream,
    .config_stream = mm_camera_intf_config_stream,
    .qbuf = mm_camera_intf_qbuf,
    .cancel_buffer = mm_camera_intf_cancel_buf,
    .get_queued_buf_count = mm_camera_intf_get_queued_buf_count,
    .map_stream_buf = mm_camera_intf_map_stream_buf,
    .map_stream_bufs = mm_camera_intf_map_stream_bufs,
    .unmap_stream_buf = mm_camera_intf_unmap_stream_buf,
    .set_stream_parms = mm_camera_intf_set_stream_parms,
    .get_stream_parms = mm_camera_intf_get_stream_parms,
    .start_channel = mm_camera_intf_start_channel,
    .stop_channel = mm_camera_intf_stop_channel,
    .request_super_buf = mm_camera_intf_request_super_buf,
    .cancel_super_buf_request = mm_camera_intf_cancel_super_buf_request,
    .flush_super_buf_queue = mm_camera_intf_flush_super_buf_queue,
    .configure_notify_mode = mm_camera_intf_configure_notify_mode,
    .process_advanced_capture = mm_camera_intf_process_advanced_capture,
    .get_session_id = mm_camera_intf_get_session_id,
    .sync_related_sensors = mm_camera_intf_sync_related_sensors,
    .flush = mm_camera_intf_flush,
    .register_stream_buf_cb = mm_camera_intf_register_stream_buf_cb
};

/*===========================================================================
 * FUNCTION   : camera_open
 *
 * DESCRIPTION: open a camera by camera index
 *
 * PARAMETERS :
 *   @camera_idx  : camera index. should within range of 0 to num_of_cameras
 *   @camera_vtbl : ptr to a virtual table containing camera handle and operation table.
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              non-zero error code -- failure
 *==========================================================================*/
int32_t camera_open(uint8_t camera_idx, mm_camera_vtbl_t **camera_vtbl)
{
    int32_t rc = 0;
    mm_camera_obj_t *cam_obj = NULL;

#ifdef QCAMERA_REDEFINE_LOG
    mm_camera_set_dbg_log_properties();
#endif

    LOGD("E camera_idx = %d\n", camera_idx);
    if (camera_idx >= g_cam_ctrl.num_cam) {
        LOGE("Invalid camera_idx (%d)", camera_idx);
        return -EINVAL;
    }

    pthread_mutex_lock(&g_intf_lock);
    /* opened already */
    if(NULL != g_cam_ctrl.cam_obj[camera_idx]) {
        /* Add reference */
        g_cam_ctrl.cam_obj[camera_idx]->ref_count++;
        pthread_mutex_unlock(&g_intf_lock);
        LOGD("opened alreadyn");
        *camera_vtbl = &g_cam_ctrl.cam_obj[camera_idx]->vtbl;
        return rc;
    }

    cam_obj = (mm_camera_obj_t *)malloc(sizeof(mm_camera_obj_t));
    if(NULL == cam_obj) {
        pthread_mutex_unlock(&g_intf_lock);
        LOGE("no mem");
        return -EINVAL;
    }

    /* initialize camera obj */
    memset(cam_obj, 0, sizeof(mm_camera_obj_t));
    cam_obj->ctrl_fd = -1;
    cam_obj->ds_fd = -1;
    cam_obj->ref_count++;
    cam_obj->my_hdl = mm_camera_util_generate_handler(camera_idx);
    cam_obj->vtbl.camera_handle = cam_obj->my_hdl; /* set handler */
    cam_obj->vtbl.ops = &mm_camera_ops;
    pthread_mutex_init(&cam_obj->cam_lock, NULL);
    /* unlock global interface lock, if not, in dual camera use case,
      * current open will block operation of another opened camera obj*/
    pthread_mutex_lock(&cam_obj->cam_lock);
    pthread_mutex_unlock(&g_intf_lock);

    rc = mm_camera_open(cam_obj);

    pthread_mutex_lock(&g_intf_lock);
    if (rc != 0) {
        LOGE("mm_camera_open err = %d", rc);
        pthread_mutex_destroy(&cam_obj->cam_lock);
        g_cam_ctrl.cam_obj[camera_idx] = NULL;
        free(cam_obj);
        cam_obj = NULL;
        pthread_mutex_unlock(&g_intf_lock);
        *camera_vtbl = NULL;
        return rc;
    } else {
        LOGD("Open succeded\n");
        g_cam_ctrl.cam_obj[camera_idx] = cam_obj;
        pthread_mutex_unlock(&g_intf_lock);
        *camera_vtbl = &cam_obj->vtbl;
        return 0;
    }
}

/*===========================================================================
 * FUNCTION   : mm_camera_load_shim_lib
 *
 * DESCRIPTION: Load shim layer library
 *
 * PARAMETERS :
 *
 * RETURN     : status of load shim library
 *==========================================================================*/
int32_t mm_camera_load_shim_lib()
{
    const char* error = NULL;
    void *qdaemon_lib = NULL;

    LOGD("E");
    qdaemon_lib = dlopen(SHIMLAYER_LIB, RTLD_NOW);
    if (!qdaemon_lib) {
        error = dlerror();
        LOGE("dlopen failed with error %s", error ? error : "");
        return -1;
    }

    *(void **)&mm_camera_shim_module_init =
            dlsym(qdaemon_lib, "mct_shimlayer_process_module_init");
    if (!mm_camera_shim_module_init) {
        error = dlerror();
        LOGE("dlsym failed with error code %s", error ? error: "");
        dlclose(qdaemon_lib);
        return -1;
    }

    return mm_camera_shim_module_init(&g_cam_ctrl.cam_shim_ops);
}

/*===========================================================================
 * FUNCTION   : mm_camera_module_open_session
 *
 * DESCRIPTION: wrapper function to call shim layer API to open session.
 *
 * PARAMETERS :
 *   @sessionid  : sessionID to open session
 *   @evt_cb     : Event callback function
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              non-zero error code -- failure
 *==========================================================================*/
cam_status_t mm_camera_module_open_session(int sessionid,
        mm_camera_shim_event_handler_func evt_cb)
{
    cam_status_t rc = -1;
    if(g_cam_ctrl.cam_shim_ops.mm_camera_shim_open_session) {
        rc = g_cam_ctrl.cam_shim_ops.mm_camera_shim_open_session(
                sessionid, evt_cb);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_module_close_session
 *
 * DESCRIPTION: wrapper function to call shim layer API to close session
 *
 * PARAMETERS :
 *   @sessionid  : sessionID to open session
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              non-zero error code -- failure
 *==========================================================================*/
int32_t mm_camera_module_close_session(int session)
{
    int32_t rc = -1;
    if(g_cam_ctrl.cam_shim_ops.mm_camera_shim_close_session) {
        rc = g_cam_ctrl.cam_shim_ops.mm_camera_shim_close_session(session);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_module_open_session
 *
 * DESCRIPTION: wrapper function to call shim layer API
 *
 * PARAMETERS :
 *   @sessionid  : sessionID to open session
 *   @evt_cb     : Event callback function
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              non-zero error code -- failure
 *==========================================================================*/
int32_t mm_camera_module_send_cmd(cam_shim_packet_t *event)
{
    int32_t rc = -1;
    if(g_cam_ctrl.cam_shim_ops.mm_camera_shim_send_cmd) {
        rc = g_cam_ctrl.cam_shim_ops.mm_camera_shim_send_cmd(event);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_camera_module_event_handler
 *
 * DESCRIPTION: call back function for shim layer
 *
 * PARAMETERS :
 *
 * RETURN     : status of call back function
 *==========================================================================*/
int mm_camera_module_event_handler(uint32_t session_id, cam_event_t *event)
{
    if (!event) {
        LOGE("null event");
        return FALSE;
    }
    mm_camera_event_t evt;

    LOGD("session_id:%d, cmd:0x%x", session_id, event->server_event_type);
    memset(&evt, 0, sizeof(mm_camera_event_t));

    evt = *event;
    mm_camera_obj_t *my_obj =
         mm_camera_util_get_camera_by_session_id(session_id);
    if (!my_obj) {
        LOGE("my_obj:%p", my_obj);
        return FALSE;
    }
    switch( evt.server_event_type) {
       case CAM_EVENT_TYPE_DAEMON_PULL_REQ:
       case CAM_EVENT_TYPE_CAC_DONE:
       case CAM_EVENT_TYPE_DAEMON_DIED:
       case CAM_EVENT_TYPE_INT_TAKE_JPEG:
       case CAM_EVENT_TYPE_INT_TAKE_RAW:
           mm_camera_enqueue_evt(my_obj, &evt);
           break;
       default:
           LOGE("cmd:%x from shim layer is not handled", evt.server_event_type);
           break;
   }
   return TRUE;
}

