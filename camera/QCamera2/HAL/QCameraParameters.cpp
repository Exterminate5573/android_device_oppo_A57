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

#define LOG_TAG "QCameraParameters"

// To remove
#include <cutils/properties.h>

// System dependencies
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <utils/Errors.h>
#define SYSINFO_H <SYSTEM_HEADER_PREFIX/sysinfo.h>
#include SYSINFO_H
#include "gralloc_priv.h"
#include "system/graphics.h"

// Camera dependencies
#include "QCameraBufferMaps.h"
#include "QCamera2HWI.h"
#include "QCameraParameters.h"
#include "QCameraTrace.h"

extern "C" {
#include "mm_camera_dbg.h"
}

#define PI 3.14159265
#define ASPECT_TOLERANCE 0.001
#define CAMERA_DEFAULT_LONGSHOT_STAGES 4
#define CAMERA_MIN_LONGSHOT_STAGES 2
#define FOCUS_PERCISION 0.0000001


namespace qcamera {
// Parameter keys to communicate between camera application and driver.
const char QCameraParameters::KEY_QC_SUPPORTED_HFR_SIZES[] = "hfr-size-values";
const char QCameraParameters::KEY_QC_PREVIEW_FRAME_RATE_MODE[] = "preview-frame-rate-mode";
const char QCameraParameters::KEY_QC_SUPPORTED_PREVIEW_FRAME_RATE_MODES[] = "preview-frame-rate-modes";
const char QCameraParameters::KEY_QC_PREVIEW_FRAME_RATE_AUTO_MODE[] = "frame-rate-auto";
const char QCameraParameters::KEY_QC_PREVIEW_FRAME_RATE_FIXED_MODE[] = "frame-rate-fixed";
const char QCameraParameters::KEY_QC_TOUCH_AF_AEC[] = "touch-af-aec";
const char QCameraParameters::KEY_QC_SUPPORTED_TOUCH_AF_AEC[] = "touch-af-aec-values";
const char QCameraParameters::KEY_QC_TOUCH_INDEX_AEC[] = "touch-index-aec";
const char QCameraParameters::KEY_QC_TOUCH_INDEX_AF[] = "touch-index-af";
const char QCameraParameters::KEY_QC_SCENE_DETECT[] = "scene-detect";
const char QCameraParameters::KEY_QC_SUPPORTED_SCENE_DETECT[] = "scene-detect-values";
const char QCameraParameters::KEY_QC_ISO_MODE[] = "iso";
const char QCameraParameters::KEY_QC_CONTINUOUS_ISO[] = "continuous-iso";
const char QCameraParameters::KEY_QC_MIN_ISO[] = "min-iso";
const char QCameraParameters::KEY_QC_MAX_ISO[] = "max-iso";
const char QCameraParameters::KEY_QC_SUPPORTED_ISO_MODES[] = "iso-values";
const char QCameraParameters::KEY_QC_EXPOSURE_TIME[] = "exposure-time";
const char QCameraParameters::KEY_QC_MIN_EXPOSURE_TIME[] = "min-exposure-time";
const char QCameraParameters::KEY_QC_MAX_EXPOSURE_TIME[] = "max-exposure-time";
const char QCameraParameters::KEY_QC_CURRENT_EXPOSURE_TIME[] = "cur-exposure-time";
const char QCameraParameters::KEY_QC_CURRENT_ISO[] = "cur-iso";
const char QCameraParameters::KEY_QC_LENSSHADE[] = "lensshade";
const char QCameraParameters::KEY_QC_SUPPORTED_LENSSHADE_MODES[] = "lensshade-values";
const char QCameraParameters::KEY_QC_AUTO_EXPOSURE[] = "auto-exposure";
const char QCameraParameters::KEY_QC_SUPPORTED_AUTO_EXPOSURE[] = "auto-exposure-values";
const char QCameraParameters::KEY_QC_DENOISE[] = "denoise";
const char QCameraParameters::KEY_QC_SUPPORTED_DENOISE[] = "denoise-values";
const char QCameraParameters::KEY_QC_FOCUS_ALGO[] = "selectable-zone-af";
const char QCameraParameters::KEY_QC_SUPPORTED_FOCUS_ALGOS[] = "selectable-zone-af-values";
const char QCameraParameters::KEY_QC_MANUAL_FOCUS_POSITION[] = "manual-focus-position";
const char QCameraParameters::KEY_QC_MANUAL_FOCUS_POS_TYPE[] = "manual-focus-pos-type";
const char QCameraParameters::KEY_QC_MIN_FOCUS_POS_INDEX[] = "min-focus-pos-index";
const char QCameraParameters::KEY_QC_MAX_FOCUS_POS_INDEX[] = "max-focus-pos-index";
const char QCameraParameters::KEY_QC_MIN_FOCUS_POS_DAC[] = "min-focus-pos-dac";
const char QCameraParameters::KEY_QC_MAX_FOCUS_POS_DAC[] = "max-focus-pos-dac";
const char QCameraParameters::KEY_QC_MIN_FOCUS_POS_RATIO[] = "min-focus-pos-ratio";
const char QCameraParameters::KEY_QC_MAX_FOCUS_POS_RATIO[] = "max-focus-pos-ratio";
const char QCameraParameters::KEY_QC_FOCUS_POSITION_SCALE[] = "cur-focus-scale";
const char QCameraParameters::KEY_QC_MIN_FOCUS_POS_DIOPTER[] = "min-focus-pos-diopter";
const char QCameraParameters::KEY_QC_MAX_FOCUS_POS_DIOPTER[] = "max-focus-pos-diopter";
const char QCameraParameters::KEY_QC_FOCUS_POSITION_DIOPTER[] = "cur-focus-diopter";
const char QCameraParameters::KEY_QC_FACE_DETECTION[] = "face-detection";
const char QCameraParameters::KEY_QC_SUPPORTED_FACE_DETECTION[] = "face-detection-values";
const char QCameraParameters::KEY_QC_FACE_RECOGNITION[] = "face-recognition";
const char QCameraParameters::KEY_QC_SUPPORTED_FACE_RECOGNITION[] = "face-recognition-values";
const char QCameraParameters::KEY_QC_MEMORY_COLOR_ENHANCEMENT[] = "mce";
const char QCameraParameters::KEY_QC_SUPPORTED_MEM_COLOR_ENHANCE_MODES[] = "mce-values";
const char QCameraParameters::KEY_QC_DIS[] = "dis";
const char QCameraParameters::KEY_QC_OIS[] = "ois";
const char QCameraParameters::KEY_QC_SUPPORTED_DIS_MODES[] = "dis-values";
const char QCameraParameters::KEY_QC_SUPPORTED_OIS_MODES[] = "ois-values";
const char QCameraParameters::KEY_QC_VIDEO_HIGH_FRAME_RATE[] = "video-hfr";
const char QCameraParameters::KEY_QC_VIDEO_HIGH_SPEED_RECORDING[] = "video-hsr";
const char QCameraParameters::KEY_QC_SUPPORTED_VIDEO_HIGH_FRAME_RATE_MODES[] = "video-hfr-values";
const char QCameraParameters::KEY_QC_REDEYE_REDUCTION[] = "redeye-reduction";
const char QCameraParameters::KEY_QC_SUPPORTED_REDEYE_REDUCTION[] = "redeye-reduction-values";
const char QCameraParameters::KEY_QC_HIGH_DYNAMIC_RANGE_IMAGING[] = "hdr";
const char QCameraParameters::KEY_QC_SUPPORTED_HDR_IMAGING_MODES[] = "hdr-values";
const char QCameraParameters::KEY_QC_ZSL[] = "zsl";
const char QCameraParameters::KEY_QC_SUPPORTED_ZSL_MODES[] = "zsl-values";
const char QCameraParameters::KEY_QC_ZSL_BURST_INTERVAL[] = "capture-burst-interval";
const char QCameraParameters::KEY_QC_ZSL_BURST_LOOKBACK[] = "capture-burst-retroactive";
const char QCameraParameters::KEY_QC_ZSL_QUEUE_DEPTH[] = "capture-burst-queue-depth";
const char QCameraParameters::KEY_QC_CAMERA_MODE[] = "camera-mode";
const char QCameraParameters::KEY_QC_AE_BRACKET_HDR[] = "ae-bracket-hdr";
const char QCameraParameters::KEY_QC_SUPPORTED_AE_BRACKET_MODES[] = "ae-bracket-hdr-values";
const char QCameraParameters::KEY_QC_SUPPORTED_RAW_FORMATS[] = "raw-format-values";
const char QCameraParameters::KEY_QC_RAW_FORMAT[] = "raw-format";
const char QCameraParameters::KEY_QC_ORIENTATION[] = "orientation";
const char QCameraParameters::KEY_QC_SELECTABLE_ZONE_AF[] = "selectable-zone-af";
const char QCameraParameters::KEY_QC_CAPTURE_BURST_EXPOSURE[] = "capture-burst-exposures";
const char QCameraParameters::KEY_QC_NUM_SNAPSHOT_PER_SHUTTER[] = "num-snaps-per-shutter";
const char QCameraParameters::KEY_QC_NUM_RETRO_BURST_PER_SHUTTER[] = "num-retro-burst-per-shutter";
const char QCameraParameters::KEY_QC_SNAPSHOT_BURST_LED_ON_PERIOD[] = "zsl-burst-led-on-period";
const char QCameraParameters::KEY_QC_NO_DISPLAY_MODE[] = "no-display-mode";
const char QCameraParameters::KEY_QC_RAW_PICUTRE_SIZE[] = "raw-size";
const char QCameraParameters::KEY_QC_SUPPORTED_SKIN_TONE_ENHANCEMENT_MODES[] = "skinToneEnhancement-values";
const char QCameraParameters::KEY_QC_SUPPORTED_LIVESNAPSHOT_SIZES[] = "supported-live-snapshot-sizes";
const char QCameraParameters::KEY_QC_SUPPORTED_HDR_NEED_1X[] = "hdr-need-1x-values";
const char QCameraParameters::KEY_QC_HDR_NEED_1X[] = "hdr-need-1x";
const char QCameraParameters::KEY_QC_PREVIEW_FLIP[] = "preview-flip";
const char QCameraParameters::KEY_QC_VIDEO_FLIP[] = "video-flip";
const char QCameraParameters::KEY_QC_SNAPSHOT_PICTURE_FLIP[] = "snapshot-picture-flip";
const char QCameraParameters::KEY_QC_SUPPORTED_FLIP_MODES[] = "flip-mode-values";
const char QCameraParameters::KEY_QC_VIDEO_HDR[] = "video-hdr";
const char QCameraParameters::KEY_QC_SENSOR_HDR[] = "sensor-hdr";
const char QCameraParameters::KEY_QC_VT_ENABLE[] = "avtimer";
const char QCameraParameters::KEY_QC_SUPPORTED_VIDEO_HDR_MODES[] = "video-hdr-values";
const char QCameraParameters::KEY_QC_SUPPORTED_SENSOR_HDR_MODES[] = "sensor-hdr-values";
const char QCameraParameters::KEY_QC_AUTO_HDR_ENABLE [] = "auto-hdr-enable";
const char QCameraParameters::KEY_QC_SNAPSHOT_BURST_NUM[] = "snapshot-burst-num";
const char QCameraParameters::KEY_QC_SNAPSHOT_FD_DATA[] = "snapshot-fd-data-enable";
const char QCameraParameters::KEY_QC_TINTLESS_ENABLE[] = "tintless";
const char QCameraParameters::KEY_QC_SCENE_SELECTION[] = "scene-selection";
const char QCameraParameters::KEY_QC_CDS_MODE[] = "cds-mode";
const char QCameraParameters::KEY_QC_VIDEO_CDS_MODE[] = "video-cds-mode";
const char QCameraParameters::KEY_QC_SUPPORTED_CDS_MODES[] = "cds-mode-values";
const char QCameraParameters::KEY_QC_SUPPORTED_VIDEO_CDS_MODES[] = "video-cds-mode-values";
const char QCameraParameters::KEY_QC_TNR_MODE[] = "tnr-mode";
const char QCameraParameters::KEY_QC_VIDEO_TNR_MODE[] = "video-tnr-mode";
const char QCameraParameters::KEY_QC_SUPPORTED_TNR_MODES[] = "tnr-mode-values";
const char QCameraParameters::KEY_QC_SUPPORTED_VIDEO_TNR_MODES[] = "video-tnr-mode-values";
const char QCameraParameters::KEY_QC_VIDEO_ROTATION[] = "video-rotation";
const char QCameraParameters::KEY_QC_SUPPORTED_VIDEO_ROTATION_VALUES[] = "video-rotation-values";
const char QCameraParameters::KEY_QC_AF_BRACKET[] = "af-bracket";
const char QCameraParameters::KEY_QC_SUPPORTED_AF_BRACKET_MODES[] = "af-bracket-values";
const char QCameraParameters::KEY_QC_RE_FOCUS[] = "re-focus";
const char QCameraParameters::KEY_QC_SUPPORTED_RE_FOCUS_MODES[] = "re-focus-values";
const char QCameraParameters::KEY_QC_CHROMA_FLASH[] = "chroma-flash";
const char QCameraParameters::KEY_QC_SUPPORTED_CHROMA_FLASH_MODES[] = "chroma-flash-values";
const char QCameraParameters::KEY_QC_OPTI_ZOOM[] = "opti-zoom";
const char QCameraParameters::KEY_QC_SEE_MORE[] = "see-more";
const char QCameraParameters::KEY_QC_STILL_MORE[] = "still-more";
const char QCameraParameters::KEY_QC_SUPPORTED_OPTI_ZOOM_MODES[] = "opti-zoom-values";
const char QCameraParameters::KEY_QC_HDR_MODE[] = "hdr-mode";
const char QCameraParameters::KEY_QC_SUPPORTED_KEY_QC_HDR_MODES[] = "hdr-mode-values";
const char QCameraParameters::KEY_QC_TRUE_PORTRAIT[] = "true-portrait";
const char QCameraParameters::KEY_QC_SUPPORTED_TRUE_PORTRAIT_MODES[] = "true-portrait-values";
const char QCameraParameters::KEY_QC_SUPPORTED_SEE_MORE_MODES[] = "see-more-values";
const char QCameraParameters::KEY_QC_SUPPORTED_STILL_MORE_MODES[] = "still-more-values";
const char QCameraParameters::KEY_INTERNAL_PERVIEW_RESTART[] = "internal-restart";
const char QCameraParameters::KEY_QC_RDI_MODE[] = "rdi-mode";
const char QCameraParameters::KEY_QC_SUPPORTED_RDI_MODES[] = "rdi-mode-values";
const char QCameraParameters::KEY_QC_SECURE_MODE[] = "secure-mode";
const char QCameraParameters::KEY_QC_SUPPORTED_SECURE_MODES[] = "secure-mode-values";
const char QCameraParameters::ISO_HJR[] = "ISO_HJR";
const char QCameraParameters::KEY_QC_AUTO_HDR_SUPPORTED[] = "auto-hdr-supported";
const char QCameraParameters::KEY_QC_LONGSHOT_SUPPORTED[] = "longshot-supported";
const char QCameraParameters::KEY_QC_ZSL_HDR_SUPPORTED[] = "zsl-hdr-supported";
const char QCameraParameters::KEY_QC_WB_MANUAL_CCT[] = "wb-manual-cct";
const char QCameraParameters::KEY_QC_MIN_WB_CCT[] = "min-wb-cct";
const char QCameraParameters::KEY_QC_MAX_WB_CCT[] = "max-wb-cct";

const char QCameraParameters::KEY_QC_MANUAL_WB_GAINS[] = "manual-wb-gains";
const char QCameraParameters::KEY_QC_MIN_WB_GAIN[] = "min-wb-gain";
const char QCameraParameters::KEY_QC_MAX_WB_GAIN[] = "max-wb-gain";

const char QCameraParameters::KEY_QC_MANUAL_WB_TYPE[] = "manual-wb-type";
const char QCameraParameters::KEY_QC_MANUAL_WB_VALUE[] = "manual-wb-value";

const char QCameraParameters::WHITE_BALANCE_MANUAL[] = "manual";
const char QCameraParameters::FOCUS_MODE_MANUAL_POSITION[] = "manual";
const char QCameraParameters::KEY_QC_CACHE_VIDEO_BUFFERS[] = "cache-video-buffers";

const char QCameraParameters::KEY_QC_LONG_SHOT[] = "long-shot";
const char QCameraParameters::KEY_QC_INITIAL_EXPOSURE_INDEX[] = "initial-exp-index";
const char QCameraParameters::KEY_QC_INSTANT_AEC[] = "instant-aec";
const char QCameraParameters::KEY_QC_INSTANT_CAPTURE[] = "instant-capture";
const char QCameraParameters::KEY_QC_INSTANT_AEC_SUPPORTED_MODES[] = "instant-aec-values";
const char QCameraParameters::KEY_QC_INSTANT_CAPTURE_SUPPORTED_MODES[] = "instant-capture-values";
const char QCameraParameters::KEY_QC_LED_CALIBRATION_MODES[] = "led-calibration-mode";

// Values for effect settings.
const char QCameraParameters::EFFECT_EMBOSS[] = "emboss";
const char QCameraParameters::EFFECT_SKETCH[] = "sketch";
const char QCameraParameters::EFFECT_NEON[] = "neon";
const char QCameraParameters::EFFECT_BEAUTY[] = "beauty";


// Values for auto exposure settings.
const char QCameraParameters::TOUCH_AF_AEC_OFF[] = "touch-off";
const char QCameraParameters::TOUCH_AF_AEC_ON[] = "touch-on";

// Values for scene mode settings.
const char QCameraParameters::SCENE_MODE_ASD[] = "asd";   // corresponds to CAMERA_BESTSHOT_AUTO in HAL
const char QCameraParameters::SCENE_MODE_BACKLIGHT[] = "backlight";
const char QCameraParameters::SCENE_MODE_FLOWERS[] = "flowers";
const char QCameraParameters::SCENE_MODE_AR[] = "AR";
const char QCameraParameters::SCENE_MODE_HDR[] = "hdr";

// Formats for setPreviewFormat and setPictureFormat.
const char QCameraParameters::PIXEL_FORMAT_YUV420SP_ADRENO[] = "yuv420sp-adreno";
const char QCameraParameters::PIXEL_FORMAT_YV12[] = "yuv420p";
const char QCameraParameters::PIXEL_FORMAT_NV12[] = "nv12";
const char QCameraParameters::QC_PIXEL_FORMAT_NV12_VENUS[] = "nv12-venus";

// Values for raw image formats
const char QCameraParameters::QC_PIXEL_FORMAT_YUV_RAW_8BIT_YUYV[] = "yuv-raw8-yuyv";
const char QCameraParameters::QC_PIXEL_FORMAT_YUV_RAW_8BIT_YVYU[] = "yuv-raw8-yvyu";
const char QCameraParameters::QC_PIXEL_FORMAT_YUV_RAW_8BIT_UYVY[] = "yuv-raw8-uyvy";
const char QCameraParameters::QC_PIXEL_FORMAT_YUV_RAW_8BIT_VYUY[] = "yuv-raw8-vyuy";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_QCOM_RAW_8GBRG[] = "bayer-qcom-8gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_QCOM_RAW_8GRBG[] = "bayer-qcom-8grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_QCOM_RAW_8RGGB[] = "bayer-qcom-8rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_QCOM_RAW_8BGGR[] = "bayer-qcom-8bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_QCOM_RAW_10GBRG[] = "bayer-qcom-10gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_QCOM_RAW_10GRBG[] = "bayer-qcom-10grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_QCOM_RAW_10RGGB[] = "bayer-qcom-10rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_QCOM_RAW_10BGGR[] = "bayer-qcom-10bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_QCOM_RAW_12GBRG[] = "bayer-qcom-12gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_QCOM_RAW_12GRBG[] = "bayer-qcom-12grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_QCOM_RAW_12RGGB[] = "bayer-qcom-12rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_QCOM_RAW_12BGGR[] = "bayer-qcom-12bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_QCOM_RAW_14GBRG[] = "bayer-qcom-14gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_QCOM_RAW_14GRBG[] = "bayer-qcom-14grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_QCOM_RAW_14RGGB[] = "bayer-qcom-14rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_QCOM_RAW_14BGGR[] = "bayer-qcom-14bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_MIPI_RAW_8GBRG[] = "bayer-mipi-8gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_MIPI_RAW_8GRBG[] = "bayer-mipi-8grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_MIPI_RAW_8RGGB[] = "bayer-mipi-8rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_MIPI_RAW_8BGGR[] = "bayer-mipi-8bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_MIPI_RAW_10GBRG[] = "bayer-mipi-10gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_MIPI_RAW_10GRBG[] = "bayer-mipi-10grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_MIPI_RAW_10RGGB[] = "bayer-mipi-10rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_MIPI_RAW_10BGGR[] = "bayer-mipi-10bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_MIPI_RAW_12GBRG[] = "bayer-mipi-12gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_MIPI_RAW_12GRBG[] = "bayer-mipi-12grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_MIPI_RAW_12RGGB[] = "bayer-mipi-12rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_MIPI_RAW_12BGGR[] = "bayer-mipi-12bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_MIPI_RAW_14GBRG[] = "bayer-mipi-14gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_MIPI_RAW_14GRBG[] = "bayer-mipi-14grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_MIPI_RAW_14RGGB[] = "bayer-mipi-14rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_MIPI_RAW_14BGGR[] = "bayer-mipi-14bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_8GBRG[] = "bayer-ideal-qcom-8gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_8GRBG[] = "bayer-ideal-qcom-8grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_8RGGB[] = "bayer-ideal-qcom-8rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_8BGGR[] = "bayer-ideal-qcom-8bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_10GBRG[] = "bayer-ideal-qcom-10gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_10GRBG[] = "bayer-ideal-qcom-10grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_10RGGB[] = "bayer-ideal-qcom-10rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_10BGGR[] = "bayer-ideal-qcom-10bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_12GBRG[] = "bayer-ideal-qcom-12gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_12GRBG[] = "bayer-ideal-qcom-12grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_12RGGB[] = "bayer-ideal-qcom-12rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_12BGGR[] = "bayer-ideal-qcom-12bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_14GBRG[] = "bayer-ideal-qcom-14gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_14GRBG[] = "bayer-ideal-qcom-14grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_14RGGB[] = "bayer-ideal-qcom-14rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_14BGGR[] = "bayer-ideal-qcom-14bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_8GBRG[] = "bayer-ideal-mipi-8gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_8GRBG[] = "bayer-ideal-mipi-8grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_8RGGB[] = "bayer-ideal-mipi-8rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_8BGGR[] = "bayer-ideal-mipi-8bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_10GBRG[] = "bayer-ideal-mipi-10gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_10GRBG[] = "bayer-ideal-mipi-10grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_10RGGB[] = "bayer-ideal-mipi-10rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_10BGGR[] = "bayer-ideal-mipi-10bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_12GBRG[] = "bayer-ideal-mipi-12gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_12GRBG[] = "bayer-ideal-mipi-12grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_12RGGB[] = "bayer-ideal-mipi-12rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_12BGGR[] = "bayer-ideal-mipi-12bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_14GBRG[] = "bayer-ideal-mipi-14gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_14GRBG[] = "bayer-ideal-mipi-14grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_14RGGB[] = "bayer-ideal-mipi-14rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_14BGGR[] = "bayer-ideal-mipi-14bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN8_8GBRG[] = "bayer-ideal-plain8-8gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN8_8GRBG[] = "bayer-ideal-plain8-8grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN8_8RGGB[] = "bayer-ideal-plain8-8rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN8_8BGGR[] = "bayer-ideal-plain8-8bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_8GBRG[] = "bayer-ideal-plain16-8gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_8GRBG[] = "bayer-ideal-plain16-8grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_8RGGB[] = "bayer-ideal-plain16-8rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_8BGGR[] = "bayer-ideal-plain16-8bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_10GBRG[] = "bayer-ideal-plain16-10gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_10GRBG[] = "bayer-ideal-plain16-10grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_10RGGB[] = "bayer-ideal-plain16-10rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_10BGGR[] = "bayer-ideal-plain16-10bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_12GBRG[] = "bayer-ideal-plain16-12gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_12GRBG[] = "bayer-ideal-plain16-12grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_12RGGB[] = "bayer-ideal-plain16-12rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_12BGGR[] = "bayer-ideal-plain16-12bggr";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_14GBRG[] = "bayer-ideal-plain16-14gbrg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_14GRBG[] = "bayer-ideal-plain16-14grbg";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_14RGGB[] = "bayer-ideal-plain16-14rggb";
const char QCameraParameters::QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_14BGGR[] = "bayer-ideal-plain16-14bggr";

// Values for ISO Settings
const char QCameraParameters::ISO_AUTO[] = "auto";
const char QCameraParameters::ISO_100[] = "ISO100";
const char QCameraParameters::ISO_200[] = "ISO200";
const char QCameraParameters::ISO_400[] = "ISO400";
const char QCameraParameters::ISO_800[] = "ISO800";
const char QCameraParameters::ISO_1600[] = "ISO1600";
const char QCameraParameters::ISO_3200[] = "ISO3200";
const char QCameraParameters::ISO_MANUAL[] = "manual";


// Values for auto exposure settings.
const char QCameraParameters::AUTO_EXPOSURE_FRAME_AVG[] = "frame-average";
const char QCameraParameters::AUTO_EXPOSURE_CENTER_WEIGHTED[] = "center-weighted";
const char QCameraParameters::AUTO_EXPOSURE_SPOT_METERING[] = "spot-metering";
const char QCameraParameters::AUTO_EXPOSURE_SMART_METERING[] = "smart-metering";
const char QCameraParameters::AUTO_EXPOSURE_USER_METERING[] = "user-metering";
const char QCameraParameters::AUTO_EXPOSURE_SPOT_METERING_ADV[] = "spot-metering-adv";
const char QCameraParameters::AUTO_EXPOSURE_CENTER_WEIGHTED_ADV[] = "center-weighted-adv";

// Values for instant AEC modes
const char QCameraParameters::KEY_QC_INSTANT_AEC_DISABLE[] = "0";
const char QCameraParameters::KEY_QC_INSTANT_AEC_AGGRESSIVE_AEC[] = "1";
const char QCameraParameters::KEY_QC_INSTANT_AEC_FAST_AEC[] = "2";

// Values for instant capture modes
const char QCameraParameters::KEY_QC_INSTANT_CAPTURE_DISABLE[] = "0";
const char QCameraParameters::KEY_QC_INSTANT_CAPTURE_AGGRESSIVE_AEC[] = "1";
const char QCameraParameters::KEY_QC_INSTANT_CAPTURE_FAST_AEC[] = "2";

//Values for led calibration mode
const char QCameraParameters::KEY_QC_LED_CALIBRATION_OFF[] = "0";
const char QCameraParameters::KEY_QC_LED_CALIBRATION_DUAL[] = "1";
const char QCameraParameters::KEY_QC_LED_CALIBRATION_SINGLE[] = "2";

const char QCameraParameters::KEY_QC_GPS_LATITUDE_REF[] = "gps-latitude-ref";
const char QCameraParameters::KEY_QC_GPS_LONGITUDE_REF[] = "gps-longitude-ref";
const char QCameraParameters::KEY_QC_GPS_ALTITUDE_REF[] = "gps-altitude-ref";
const char QCameraParameters::KEY_QC_GPS_STATUS[] = "gps-status";

const char QCameraParameters::KEY_QC_HISTOGRAM[] = "histogram";
const char QCameraParameters::KEY_QC_SUPPORTED_HISTOGRAM_MODES[] = "histogram-values";

const char QCameraParameters::VALUE_ENABLE[] = "enable";
const char QCameraParameters::VALUE_DISABLE[] = "disable";
const char QCameraParameters::VALUE_OFF[] = "off";
const char QCameraParameters::VALUE_ON[] = "on";
const char QCameraParameters::VALUE_TRUE[] = "true";
const char QCameraParameters::VALUE_FALSE[] = "false";

const char QCameraParameters::VALUE_FAST[] = "fast";
const char QCameraParameters::VALUE_HIGH_QUALITY[] = "high-quality";

const char QCameraParameters::KEY_QC_SHARPNESS[] = "sharpness";
const char QCameraParameters::KEY_QC_MIN_SHARPNESS[] = "min-sharpness";
const char QCameraParameters::KEY_QC_MAX_SHARPNESS[] = "max-sharpness";
const char QCameraParameters::KEY_QC_SHARPNESS_STEP[] = "sharpness-step";
const char QCameraParameters::KEY_QC_CONTRAST[] = "contrast";
const char QCameraParameters::KEY_QC_MIN_CONTRAST[] = "min-contrast";
const char QCameraParameters::KEY_QC_MAX_CONTRAST[] = "max-contrast";
const char QCameraParameters::KEY_QC_CONTRAST_STEP[] = "contrast-step";
const char QCameraParameters::KEY_QC_SATURATION[] = "saturation";
const char QCameraParameters::KEY_QC_MIN_SATURATION[] = "min-saturation";
const char QCameraParameters::KEY_QC_MAX_SATURATION[] = "max-saturation";
const char QCameraParameters::KEY_QC_SATURATION_STEP[] = "saturation-step";
const char QCameraParameters::KEY_QC_BRIGHTNESS[] = "luma-adaptation";
const char QCameraParameters::KEY_QC_MIN_BRIGHTNESS[] = "min-brightness";
const char QCameraParameters::KEY_QC_MAX_BRIGHTNESS[] = "max-brightness";
const char QCameraParameters::KEY_QC_BRIGHTNESS_STEP[] = "brightness-step";
const char QCameraParameters::KEY_QC_SCE_FACTOR[] = "skinToneEnhancement";
const char QCameraParameters::KEY_QC_MIN_SCE_FACTOR[] = "min-sce-factor";
const char QCameraParameters::KEY_QC_MAX_SCE_FACTOR[] = "max-sce-factor";
const char QCameraParameters::KEY_QC_SCE_FACTOR_STEP[] = "sce-factor-step";

const char QCameraParameters::KEY_QC_MAX_NUM_REQUESTED_FACES[] = "qc-max-num-requested-faces";

//Values for DENOISE
const char QCameraParameters::DENOISE_OFF[] = "denoise-off";
const char QCameraParameters::DENOISE_ON[] = "denoise-on";

// Values for selectable zone af Settings
const char QCameraParameters::FOCUS_ALGO_AUTO[] = "auto";
const char QCameraParameters::FOCUS_ALGO_SPOT_METERING[] = "spot-metering";
const char QCameraParameters::FOCUS_ALGO_CENTER_WEIGHTED[] = "center-weighted";
const char QCameraParameters::FOCUS_ALGO_FRAME_AVERAGE[] = "frame-average";

// Values for HFR settings.
const char QCameraParameters::VIDEO_HFR_OFF[] = "off";
const char QCameraParameters::VIDEO_HFR_2X[] = "60";
const char QCameraParameters::VIDEO_HFR_3X[] = "90";
const char QCameraParameters::VIDEO_HFR_4X[] = "120";
const char QCameraParameters::VIDEO_HFR_5X[] = "150";
const char QCameraParameters::VIDEO_HFR_6X[] = "180";
const char QCameraParameters::VIDEO_HFR_7X[] = "210";
const char QCameraParameters::VIDEO_HFR_8X[] = "240";
const char QCameraParameters::VIDEO_HFR_9X[] = "480";

// Values for HDR Bracketing settings.
const char QCameraParameters::AE_BRACKET_OFF[] = "Off";
const char QCameraParameters::AE_BRACKET[] = "AE-Bracket";

// Values for AF Bracketing setting.
const char QCameraParameters::AF_BRACKET_OFF[] = "af-bracket-off";
const char QCameraParameters::AF_BRACKET_ON[] = "af-bracket-on";

// Values for Refocus setting.
const char QCameraParameters::RE_FOCUS_OFF[] = "re-focus-off";
const char QCameraParameters::RE_FOCUS_ON[] = "re-focus-on";

// Values for Chroma Flash setting.
const char QCameraParameters::CHROMA_FLASH_OFF[] = "chroma-flash-off";
const char QCameraParameters::CHROMA_FLASH_ON[] = "chroma-flash-on";

// Values for Opti Zoom setting.
const char QCameraParameters::OPTI_ZOOM_OFF[] = "opti-zoom-off";
const char QCameraParameters::OPTI_ZOOM_ON[] = "opti-zoom-on";

// Values for Still More setting.
const char QCameraParameters::STILL_MORE_OFF[] = "still-more-off";
const char QCameraParameters::STILL_MORE_ON[] = "still-more-on";

// Values for HDR mode setting.
const char QCameraParameters::HDR_MODE_SENSOR[] = "hdr-mode-sensor";
const char QCameraParameters::HDR_MODE_MULTI_FRAME[] = "hdr-mode-multiframe";

// Values for True Portrait setting.
const char QCameraParameters::TRUE_PORTRAIT_OFF[] = "true-portrait-off";
const char QCameraParameters::TRUE_PORTRAIT_ON[] = "true-portrait-on";

// Values for FLIP settings.
const char QCameraParameters::FLIP_MODE_OFF[] = "off";
const char QCameraParameters::FLIP_MODE_V[] = "flip-v";
const char QCameraParameters::FLIP_MODE_H[] = "flip-h";
const char QCameraParameters::FLIP_MODE_VH[] = "flip-vh";

const char QCameraParameters::CDS_MODE_OFF[] = "off";
const char QCameraParameters::CDS_MODE_ON[] = "on";
const char QCameraParameters::CDS_MODE_AUTO[] = "auto";

// Values for video rotation settings.
const char QCameraParameters::VIDEO_ROTATION_0[] = "0";
const char QCameraParameters::VIDEO_ROTATION_90[] = "90";
const char QCameraParameters::VIDEO_ROTATION_180[] = "180";
const char QCameraParameters::VIDEO_ROTATION_270[] = "270";

const char QCameraParameters::KEY_QC_SUPPORTED_MANUAL_FOCUS_MODES[] = "manual-focus-modes";
const char QCameraParameters::KEY_QC_SUPPORTED_MANUAL_EXPOSURE_MODES[] = "manual-exposure-modes";
const char QCameraParameters::KEY_QC_SUPPORTED_MANUAL_WB_MODES[] = "manual-wb-modes";
const char QCameraParameters::KEY_QC_FOCUS_SCALE_MODE[] = "scale-mode";
const char QCameraParameters::KEY_QC_FOCUS_DIOPTER_MODE[] = "diopter-mode";
const char QCameraParameters::KEY_QC_ISO_PRIORITY[] = "iso-priority";
const char QCameraParameters::KEY_QC_EXP_TIME_PRIORITY[] = "exp-time-priority";
const char QCameraParameters::KEY_QC_USER_SETTING[] = "user-setting";
const char QCameraParameters::KEY_QC_WB_CCT_MODE[] = "color-temperature";
const char QCameraParameters::KEY_QC_WB_GAIN_MODE[] = "rbgb-gains";
const char QCameraParameters::KEY_QC_NOISE_REDUCTION_MODE[] = "noise-reduction-mode";
const char QCameraParameters::KEY_QC_NOISE_REDUCTION_MODE_VALUES[] = "noise-reduction-mode-values";

#ifdef TARGET_TS_MAKEUP
const char QCameraParameters::KEY_TS_MAKEUP[] = "tsmakeup";
const char QCameraParameters::KEY_TS_MAKEUP_WHITEN[] = "tsmakeup_whiten";
const char QCameraParameters::KEY_TS_MAKEUP_CLEAN[] = "tsmakeup_clean";
#endif

//KEY to share HFR batch size with video encoder.
const char QCameraParameters::KEY_QC_VIDEO_BATCH_SIZE[] = "video-batch-size";

static const char* portrait = "portrait";
static const char* landscape = "landscape";

const cam_dimension_t QCameraParameters::THUMBNAIL_SIZES_MAP[] = {
    { 256, 154 }, //1.66233
    { 240, 160 }, //1.5
    { 320, 320 }, //1.0
    { 320, 240 }, //1.33333
    { 256, 144 }, //1.777778
    { 240, 144 }, //1.666667
    { 176, 144 }, //1.222222
    /*Thumbnail sizes to match portrait picture size aspect ratio*/
    { 240, 320 }, //to match 480X640 & 240X320 picture size
    { 144, 176 }, //to match 144X176  picture size
    { 0, 0 }      // required by Android SDK
};

const QCameraParameters::QCameraMap<cam_auto_exposure_mode_type>
        QCameraParameters::AUTO_EXPOSURE_MAP[] = {
    { AUTO_EXPOSURE_FRAME_AVG,           CAM_AEC_MODE_FRAME_AVERAGE },
    { AUTO_EXPOSURE_CENTER_WEIGHTED,     CAM_AEC_MODE_CENTER_WEIGHTED },
    { AUTO_EXPOSURE_SPOT_METERING,       CAM_AEC_MODE_SPOT_METERING },
    { AUTO_EXPOSURE_SMART_METERING,      CAM_AEC_MODE_SMART_METERING },
    { AUTO_EXPOSURE_USER_METERING,       CAM_AEC_MODE_USER_METERING },
    { AUTO_EXPOSURE_SPOT_METERING_ADV,   CAM_AEC_MODE_SPOT_METERING_ADV },
    { AUTO_EXPOSURE_CENTER_WEIGHTED_ADV, CAM_AEC_MODE_CENTER_WEIGHTED_ADV },
};

const QCameraParameters::QCameraMap<cam_aec_convergence_type>
        QCameraParameters::INSTANT_AEC_MODES_MAP[] = {
    { KEY_QC_INSTANT_AEC_DISABLE,        CAM_AEC_NORMAL_CONVERGENCE },
    { KEY_QC_INSTANT_AEC_AGGRESSIVE_AEC, CAM_AEC_AGGRESSIVE_CONVERGENCE },
    { KEY_QC_INSTANT_AEC_FAST_AEC,       CAM_AEC_FAST_CONVERGENCE },
};

const QCameraParameters::QCameraMap<cam_aec_convergence_type>
        QCameraParameters::INSTANT_CAPTURE_MODES_MAP[] = {
    { KEY_QC_INSTANT_CAPTURE_DISABLE,        CAM_AEC_NORMAL_CONVERGENCE },
    { KEY_QC_INSTANT_CAPTURE_AGGRESSIVE_AEC, CAM_AEC_AGGRESSIVE_CONVERGENCE },
    { KEY_QC_INSTANT_CAPTURE_FAST_AEC,       CAM_AEC_FAST_CONVERGENCE },
};

const QCameraParameters::QCameraMap<cam_led_calibration_mode_t>
        QCameraParameters::LED_CALIBRATION_MODE_MAP[] = {
    {KEY_QC_LED_CALIBRATION_OFF,        CAM_LED_CALIBRATION_MODE_OFF},
    {KEY_QC_LED_CALIBRATION_DUAL,       CAM_LED_CALIBRATION_MODE_DUAL},
    {KEY_QC_LED_CALIBRATION_SINGLE,     CAM_LED_CALIBRATION_MODE_SINGLE},
};

const QCameraParameters::QCameraMap<cam_format_t>
        QCameraParameters::PREVIEW_FORMATS_MAP[] = {
    {PIXEL_FORMAT_YUV420SP,        CAM_FORMAT_YUV_420_NV21},
    {PIXEL_FORMAT_YUV420P,         CAM_FORMAT_YUV_420_YV12},
    {PIXEL_FORMAT_YUV420SP_ADRENO, CAM_FORMAT_YUV_420_NV21_ADRENO},
    {PIXEL_FORMAT_YV12,            CAM_FORMAT_YUV_420_YV12},
    {PIXEL_FORMAT_NV12,            CAM_FORMAT_YUV_420_NV12},
    {QC_PIXEL_FORMAT_NV12_VENUS,   CAM_FORMAT_YUV_420_NV12_VENUS}
};

const QCameraParameters::QCameraMap<cam_format_t>
        QCameraParameters::PICTURE_TYPES_MAP[] = {
    {PIXEL_FORMAT_JPEG,                          CAM_FORMAT_JPEG},
    {PIXEL_FORMAT_YUV420SP,                      CAM_FORMAT_YUV_420_NV21},
    {PIXEL_FORMAT_YUV422SP,                      CAM_FORMAT_YUV_422_NV16},
    {QC_PIXEL_FORMAT_YUV_RAW_8BIT_YUYV,          CAM_FORMAT_YUV_RAW_8BIT_YUYV},
    {QC_PIXEL_FORMAT_YUV_RAW_8BIT_YVYU,          CAM_FORMAT_YUV_RAW_8BIT_YVYU},
    {QC_PIXEL_FORMAT_YUV_RAW_8BIT_UYVY,          CAM_FORMAT_YUV_RAW_8BIT_UYVY},
    {QC_PIXEL_FORMAT_YUV_RAW_8BIT_VYUY,          CAM_FORMAT_YUV_RAW_8BIT_VYUY},
    {QC_PIXEL_FORMAT_BAYER_QCOM_RAW_8GBRG,       CAM_FORMAT_BAYER_QCOM_RAW_8BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_QCOM_RAW_8GRBG,       CAM_FORMAT_BAYER_QCOM_RAW_8BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_QCOM_RAW_8RGGB,       CAM_FORMAT_BAYER_QCOM_RAW_8BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_QCOM_RAW_8BGGR,       CAM_FORMAT_BAYER_QCOM_RAW_8BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_QCOM_RAW_10GBRG,      CAM_FORMAT_BAYER_QCOM_RAW_10BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_QCOM_RAW_10GRBG,      CAM_FORMAT_BAYER_QCOM_RAW_10BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_QCOM_RAW_10RGGB,      CAM_FORMAT_BAYER_QCOM_RAW_10BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_QCOM_RAW_10BGGR,      CAM_FORMAT_BAYER_QCOM_RAW_10BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_QCOM_RAW_12GBRG,      CAM_FORMAT_BAYER_QCOM_RAW_12BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_QCOM_RAW_12GRBG,      CAM_FORMAT_BAYER_QCOM_RAW_12BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_QCOM_RAW_12RGGB,      CAM_FORMAT_BAYER_QCOM_RAW_12BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_QCOM_RAW_12BGGR,      CAM_FORMAT_BAYER_QCOM_RAW_12BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_QCOM_RAW_14GBRG,      CAM_FORMAT_BAYER_QCOM_RAW_14BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_QCOM_RAW_14GRBG,      CAM_FORMAT_BAYER_QCOM_RAW_14BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_QCOM_RAW_14RGGB,      CAM_FORMAT_BAYER_QCOM_RAW_14BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_QCOM_RAW_14BGGR,      CAM_FORMAT_BAYER_QCOM_RAW_14BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_MIPI_RAW_8GBRG,       CAM_FORMAT_BAYER_MIPI_RAW_8BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_MIPI_RAW_8GRBG,       CAM_FORMAT_BAYER_MIPI_RAW_8BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_MIPI_RAW_8RGGB,       CAM_FORMAT_BAYER_MIPI_RAW_8BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_MIPI_RAW_8BGGR,       CAM_FORMAT_BAYER_MIPI_RAW_8BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_MIPI_RAW_10GBRG,      CAM_FORMAT_BAYER_MIPI_RAW_10BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_MIPI_RAW_10GRBG,      CAM_FORMAT_BAYER_MIPI_RAW_10BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_MIPI_RAW_10RGGB,      CAM_FORMAT_BAYER_MIPI_RAW_10BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_MIPI_RAW_10BGGR,      CAM_FORMAT_BAYER_MIPI_RAW_10BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_MIPI_RAW_12GBRG,      CAM_FORMAT_BAYER_MIPI_RAW_12BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_MIPI_RAW_12GRBG,      CAM_FORMAT_BAYER_MIPI_RAW_12BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_MIPI_RAW_12RGGB,      CAM_FORMAT_BAYER_MIPI_RAW_12BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_MIPI_RAW_12BGGR,      CAM_FORMAT_BAYER_MIPI_RAW_12BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_MIPI_RAW_14GBRG,      CAM_FORMAT_BAYER_MIPI_RAW_14BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_MIPI_RAW_14GRBG,      CAM_FORMAT_BAYER_MIPI_RAW_14BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_MIPI_RAW_14RGGB,      CAM_FORMAT_BAYER_MIPI_RAW_14BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_MIPI_RAW_14BGGR,      CAM_FORMAT_BAYER_MIPI_RAW_14BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_8GBRG,     CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_8BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_8GRBG,     CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_8BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_8RGGB,     CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_8BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_8BGGR,     CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_8BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_10GBRG,    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_10BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_10GRBG,    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_10BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_10RGGB,    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_10BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_10BGGR,    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_10BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_12GBRG,    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_12BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_12GRBG,    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_12BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_12RGGB,    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_12BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_12BGGR,    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_12BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_14GBRG,    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_14BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_14GRBG,    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_14BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_14RGGB,    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_14BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_14BGGR,    CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_14BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_8GBRG,     CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_8BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_8GRBG,     CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_8BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_8RGGB,     CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_8BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_8BGGR,     CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_8BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_10GBRG,    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_10BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_10GRBG,    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_10BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_10RGGB,    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_10BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_10BGGR,    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_10BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_12GBRG,    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_12BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_12GRBG,    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_12BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_12RGGB,    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_12BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_12BGGR,    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_12BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_14GBRG,    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_14BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_14GRBG,    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_14BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_14RGGB,    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_14BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_14BGGR,    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_14BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN8_8GBRG,   CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN8_8BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN8_8GRBG,   CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN8_8BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN8_8RGGB,   CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN8_8BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN8_8BGGR,   CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN8_8BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_8GBRG,  CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_8BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_8GRBG,  CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_8BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_8RGGB,  CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_8BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_8BGGR,  CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_8BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_10GBRG, CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_10BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_10GRBG, CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_10BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_10RGGB, CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_10BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_10BGGR, CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_10BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_12GBRG, CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_12BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_12GRBG, CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_12BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_12RGGB, CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_12BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_12BGGR, CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_12BPP_BGGR},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_14GBRG, CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_14BPP_GBRG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_14GRBG, CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_14BPP_GRBG},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_14RGGB, CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_14BPP_RGGB},
    {QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_14BGGR, CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_14BPP_BGGR}
};

const QCameraParameters::QCameraMap<cam_focus_mode_type>
        QCameraParameters::FOCUS_MODES_MAP[] = {
    { FOCUS_MODE_AUTO,               CAM_FOCUS_MODE_AUTO },
    { FOCUS_MODE_INFINITY,           CAM_FOCUS_MODE_INFINITY },
    { FOCUS_MODE_MACRO,              CAM_FOCUS_MODE_MACRO },
    { FOCUS_MODE_FIXED,              CAM_FOCUS_MODE_FIXED },
    { FOCUS_MODE_EDOF,               CAM_FOCUS_MODE_EDOF },
    { FOCUS_MODE_CONTINUOUS_PICTURE, CAM_FOCUS_MODE_CONTINOUS_PICTURE },
    { FOCUS_MODE_CONTINUOUS_VIDEO,   CAM_FOCUS_MODE_CONTINOUS_VIDEO },
    { FOCUS_MODE_MANUAL_POSITION,    CAM_FOCUS_MODE_MANUAL},
};

const QCameraParameters::QCameraMap<cam_effect_mode_type>
        QCameraParameters::EFFECT_MODES_MAP[] = {
    { EFFECT_NONE,       CAM_EFFECT_MODE_OFF },
    { EFFECT_MONO,       CAM_EFFECT_MODE_MONO },
    { EFFECT_NEGATIVE,   CAM_EFFECT_MODE_NEGATIVE },
    { EFFECT_SOLARIZE,   CAM_EFFECT_MODE_SOLARIZE },
    { EFFECT_SEPIA,      CAM_EFFECT_MODE_SEPIA },
    { EFFECT_POSTERIZE,  CAM_EFFECT_MODE_POSTERIZE },
    { EFFECT_WHITEBOARD, CAM_EFFECT_MODE_WHITEBOARD },
    { EFFECT_BLACKBOARD, CAM_EFFECT_MODE_BLACKBOARD },
    { EFFECT_AQUA,       CAM_EFFECT_MODE_AQUA },
    { EFFECT_EMBOSS,     CAM_EFFECT_MODE_EMBOSS },
    { EFFECT_SKETCH,     CAM_EFFECT_MODE_SKETCH },
    { EFFECT_NEON,       CAM_EFFECT_MODE_NEON },
    { EFFECT_BEAUTY,     CAM_EFFECT_MODE_BEAUTY }
};

const QCameraParameters::QCameraMap<cam_scene_mode_type>
        QCameraParameters::SCENE_MODES_MAP[] = {
    { SCENE_MODE_AUTO,           CAM_SCENE_MODE_OFF },
    { SCENE_MODE_ACTION,         CAM_SCENE_MODE_ACTION },
    { SCENE_MODE_PORTRAIT,       CAM_SCENE_MODE_PORTRAIT },
    { SCENE_MODE_LANDSCAPE,      CAM_SCENE_MODE_LANDSCAPE },
    { SCENE_MODE_NIGHT,          CAM_SCENE_MODE_NIGHT },
    { SCENE_MODE_NIGHT_PORTRAIT, CAM_SCENE_MODE_NIGHT_PORTRAIT },
    { SCENE_MODE_THEATRE,        CAM_SCENE_MODE_THEATRE },
    { SCENE_MODE_BEACH,          CAM_SCENE_MODE_BEACH },
    { SCENE_MODE_SNOW,           CAM_SCENE_MODE_SNOW },
    { SCENE_MODE_SUNSET,         CAM_SCENE_MODE_SUNSET },
    { SCENE_MODE_STEADYPHOTO,    CAM_SCENE_MODE_ANTISHAKE },
    { SCENE_MODE_FIREWORKS ,     CAM_SCENE_MODE_FIREWORKS },
    { SCENE_MODE_SPORTS ,        CAM_SCENE_MODE_SPORTS },
    { SCENE_MODE_PARTY,          CAM_SCENE_MODE_PARTY },
    { SCENE_MODE_CANDLELIGHT,    CAM_SCENE_MODE_CANDLELIGHT },
    { SCENE_MODE_ASD,            CAM_SCENE_MODE_AUTO },
    { SCENE_MODE_BACKLIGHT,      CAM_SCENE_MODE_BACKLIGHT },
    { SCENE_MODE_FLOWERS,        CAM_SCENE_MODE_FLOWERS },
    { SCENE_MODE_AR,             CAM_SCENE_MODE_AR },
    { SCENE_MODE_HDR,            CAM_SCENE_MODE_HDR },
};

const QCameraParameters::QCameraMap<cam_flash_mode_t>
        QCameraParameters::FLASH_MODES_MAP[] = {
    { FLASH_MODE_OFF,   CAM_FLASH_MODE_OFF },
    { FLASH_MODE_AUTO,  CAM_FLASH_MODE_AUTO },
    { FLASH_MODE_ON,    CAM_FLASH_MODE_ON },
    { FLASH_MODE_TORCH, CAM_FLASH_MODE_TORCH }
};

const QCameraParameters::QCameraMap<cam_focus_algorithm_type>
         QCameraParameters::FOCUS_ALGO_MAP[] = {
    { FOCUS_ALGO_AUTO,            CAM_FOCUS_ALGO_AUTO },
    { FOCUS_ALGO_SPOT_METERING,   CAM_FOCUS_ALGO_SPOT },
    { FOCUS_ALGO_CENTER_WEIGHTED, CAM_FOCUS_ALGO_CENTER_WEIGHTED },
    { FOCUS_ALGO_FRAME_AVERAGE,   CAM_FOCUS_ALGO_AVERAGE }
};

const QCameraParameters::QCameraMap<cam_wb_mode_type>
        QCameraParameters::WHITE_BALANCE_MODES_MAP[] = {
    { WHITE_BALANCE_AUTO,            CAM_WB_MODE_AUTO },
    { WHITE_BALANCE_INCANDESCENT,    CAM_WB_MODE_INCANDESCENT },
    { WHITE_BALANCE_FLUORESCENT,     CAM_WB_MODE_FLUORESCENT },
    { WHITE_BALANCE_WARM_FLUORESCENT,CAM_WB_MODE_WARM_FLUORESCENT},
    { WHITE_BALANCE_DAYLIGHT,        CAM_WB_MODE_DAYLIGHT },
    { WHITE_BALANCE_CLOUDY_DAYLIGHT, CAM_WB_MODE_CLOUDY_DAYLIGHT },
    { WHITE_BALANCE_TWILIGHT,        CAM_WB_MODE_TWILIGHT },
    { WHITE_BALANCE_SHADE,           CAM_WB_MODE_SHADE },
    { WHITE_BALANCE_MANUAL,          CAM_WB_MODE_MANUAL},
};

const QCameraParameters::QCameraMap<cam_antibanding_mode_type>
        QCameraParameters::ANTIBANDING_MODES_MAP[] = {
    { ANTIBANDING_OFF,  CAM_ANTIBANDING_MODE_OFF },
    { ANTIBANDING_50HZ, CAM_ANTIBANDING_MODE_50HZ },
    { ANTIBANDING_60HZ, CAM_ANTIBANDING_MODE_60HZ },
    { ANTIBANDING_AUTO, CAM_ANTIBANDING_MODE_AUTO }
};

const QCameraParameters::QCameraMap<cam_iso_mode_type>
        QCameraParameters::ISO_MODES_MAP[] = {
    { ISO_AUTO,  CAM_ISO_MODE_AUTO },
    { ISO_HJR,   CAM_ISO_MODE_AUTO },
    { ISO_100,   CAM_ISO_MODE_100 },
    { ISO_200,   CAM_ISO_MODE_200 },
    { ISO_400,   CAM_ISO_MODE_400 },
    { ISO_800,   CAM_ISO_MODE_800 },
    { ISO_1600,  CAM_ISO_MODE_1600 },
    { ISO_3200,  CAM_ISO_MODE_3200 }
};

const QCameraParameters::QCameraMap<cam_hfr_mode_t>
        QCameraParameters::HFR_MODES_MAP[] = {
    { VIDEO_HFR_OFF, CAM_HFR_MODE_OFF },
    { VIDEO_HFR_2X, CAM_HFR_MODE_60FPS },
    { VIDEO_HFR_3X, CAM_HFR_MODE_90FPS },
    { VIDEO_HFR_4X, CAM_HFR_MODE_120FPS },
    { VIDEO_HFR_5X, CAM_HFR_MODE_150FPS },
    { VIDEO_HFR_6X, CAM_HFR_MODE_180FPS },
    { VIDEO_HFR_7X, CAM_HFR_MODE_210FPS },
    { VIDEO_HFR_8X, CAM_HFR_MODE_240FPS },
    { VIDEO_HFR_9X, CAM_HFR_MODE_480FPS }
};

const QCameraParameters::QCameraMap<cam_bracket_mode>
        QCameraParameters::BRACKETING_MODES_MAP[] = {
    { AE_BRACKET_OFF, CAM_EXP_BRACKETING_OFF },
    { AE_BRACKET,     CAM_EXP_BRACKETING_ON }
};

const QCameraParameters::QCameraMap<int>
        QCameraParameters::ON_OFF_MODES_MAP[] = {
    { VALUE_OFF, 0 },
    { VALUE_ON,  1 }
};

const QCameraParameters::QCameraMap<int>
        QCameraParameters::TOUCH_AF_AEC_MODES_MAP[] = {
    { QCameraParameters::TOUCH_AF_AEC_OFF, 0 },
    { QCameraParameters::TOUCH_AF_AEC_ON, 1 }
};

const QCameraParameters::QCameraMap<int>
        QCameraParameters::ENABLE_DISABLE_MODES_MAP[] = {
    { VALUE_ENABLE,  1 },
    { VALUE_DISABLE, 0 }
};

const QCameraParameters::QCameraMap<int>
        QCameraParameters::DENOISE_ON_OFF_MODES_MAP[] = {
    { DENOISE_OFF, 0 },
    { DENOISE_ON,  1 }
};

const QCameraParameters::QCameraMap<int>
        QCameraParameters::TRUE_FALSE_MODES_MAP[] = {
    { VALUE_FALSE, 0},
    { VALUE_TRUE,  1}
};

const QCameraParameters::QCameraMap<cam_flip_t>
        QCameraParameters::FLIP_MODES_MAP[] = {
    {FLIP_MODE_OFF, FLIP_NONE},
    {FLIP_MODE_V, FLIP_V},
    {FLIP_MODE_H, FLIP_H},
    {FLIP_MODE_VH, FLIP_V_H}
};

const QCameraParameters::QCameraMap<int>
        QCameraParameters::AF_BRACKETING_MODES_MAP[] = {
    { AF_BRACKET_OFF, 0 },
    { AF_BRACKET_ON,  1 }
};

const QCameraParameters::QCameraMap<int>
        QCameraParameters::RE_FOCUS_MODES_MAP[] = {
    { RE_FOCUS_OFF, 0 },
    { RE_FOCUS_ON,  1 }
};

const QCameraParameters::QCameraMap<int>
        QCameraParameters::CHROMA_FLASH_MODES_MAP[] = {
    { CHROMA_FLASH_OFF, 0 },
    { CHROMA_FLASH_ON,  1 }
};

const QCameraParameters::QCameraMap<int>
        QCameraParameters::OPTI_ZOOM_MODES_MAP[] = {
    { OPTI_ZOOM_OFF, 0 },
    { OPTI_ZOOM_ON,  1 }
};

const QCameraParameters::QCameraMap<int>
        QCameraParameters::TRUE_PORTRAIT_MODES_MAP[] = {
    { TRUE_PORTRAIT_OFF, 0 },
    { TRUE_PORTRAIT_ON,  1 }
};

const QCameraParameters::QCameraMap<int>
        QCameraParameters::STILL_MORE_MODES_MAP[] = {
    { STILL_MORE_OFF, 0 },
    { STILL_MORE_ON,  1 }
};

const QCameraParameters::QCameraMap<cam_cds_mode_type_t>
        QCameraParameters::CDS_MODES_MAP[] = {
    { CDS_MODE_OFF, CAM_CDS_MODE_OFF },
    { CDS_MODE_ON, CAM_CDS_MODE_ON },
    { CDS_MODE_AUTO, CAM_CDS_MODE_AUTO}
};

const QCameraParameters::QCameraMap<int>
        QCameraParameters::HDR_MODES_MAP[] = {
    { HDR_MODE_SENSOR, 0 },
    { HDR_MODE_MULTI_FRAME, 1 }
};

const QCameraParameters::QCameraMap<int>
        QCameraParameters::VIDEO_ROTATION_MODES_MAP[] = {
    { VIDEO_ROTATION_0, 0 },
    { VIDEO_ROTATION_90, 90 },
    { VIDEO_ROTATION_180, 180 },
    { VIDEO_ROTATION_270, 270 }
};

const QCameraParameters::QCameraMap<int>
        QCameraParameters::NOISE_REDUCTION_MODES_MAP[] = {
    { VALUE_OFF, 0 },
    { VALUE_FAST,  1 },
    { VALUE_HIGH_QUALITY,  2 }
};

#define DEFAULT_CAMERA_AREA "(0, 0, 0, 0, 0)"
#define DATA_PTR(MEM_OBJ,INDEX) MEM_OBJ->getPtr( INDEX )
#define TOTAL_RAM_SIZE_512MB 536870912
#define PARAM_MAP_SIZE(MAP) (sizeof(MAP)/sizeof(MAP[0]))

/*===========================================================================
 * FUNCTION   : isOEMFeat1PropEnabled
 *
 * DESCRIPTION: inline function to check from property if custom feature
 *            is enabled
 *
 * PARAMETERS : none
 *
 * RETURN     : boolean true/false
 *==========================================================================*/
static inline bool isOEMFeat1PropEnabled()
{
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.camera.imglib.oemfeat1", value, "0");
    return atoi(value) > 0 ? true : false;
}

/*===========================================================================
 * FUNCTION   : QCameraParameters
 *
 * DESCRIPTION: default constructor of QCameraParameters
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
QCameraParameters::QCameraParameters()
    : CameraParameters(),
      m_reprocScaleParam(),
      mCommon(),
      m_pCapability(NULL),
      m_pCamOpsTbl(NULL),
      m_pParamHeap(NULL),
      m_pParamBuf(NULL),
      m_pRelCamSyncHeap(NULL),
      m_pRelCamSyncBuf(NULL),
      m_bFrameSyncEnabled(false),
      mIsTypeVideo(IS_TYPE_NONE),
      mIsTypePreview(IS_TYPE_NONE),
      m_bZslMode(false),
      m_bZslMode_new(false),
      m_bForceZslMode(false),
      m_bRecordingHint(false),
      m_bRecordingHint_new(false),
      m_bHistogramEnabled(false),
      m_bLongshotEnabled(false),
      m_nFaceProcMask(0),
      m_bFaceDetectionOn(0),
      m_bDebugFps(false),
      mFocusMode(CAM_FOCUS_MODE_MAX),
      mPreviewFormat(CAM_FORMAT_YUV_420_NV21),
      mAppPreviewFormat(CAM_FORMAT_YUV_420_NV21),
      mPictureFormat(CAM_FORMAT_JPEG),
      m_bNeedRestart(false),
      m_bNoDisplayMode(false),
      m_bWNROn(false),
      m_bTNRPreviewOn(false),
      m_bTNRVideoOn(false),
      m_bTNRSnapshotOn(false),
      m_bInited(false),
      m_nRetroBurstNum(0),
      m_nBurstLEDOnPeriod(100),
      m_bUpdateEffects(false),
      m_bSceneTransitionAuto(false),
      m_bPreviewFlipChanged(false),
      m_bVideoFlipChanged(false),
      m_bSnapshotFlipChanged(false),
      m_bFixedFrameRateSet(false),
      m_bHDREnabled(false),
      m_bLocalHDREnabled(false),
      m_bAVTimerEnabled(false),
      m_bDISEnabled(false),
      m_bMetaRawEnabled(false),
      m_MobiMask(0),
      m_AdjustFPS(NULL),
      m_bHDR1xFrameEnabled(false),
      m_HDRSceneEnabled(false),
      m_bHDRThumbnailProcessNeeded(false),
      m_bHDR1xExtraBufferNeeded(true),
      m_bHDROutputCropEnabled(false),
      m_tempMap(),
      m_bAFBracketingOn(false),
      m_bReFocusOn(false),
      m_bChromaFlashOn(false),
      m_bOptiZoomOn(false),
      m_bSceneSelection(false),
      m_SelectedScene(CAM_SCENE_MODE_MAX),
      m_bSeeMoreOn(false),
      m_bStillMoreOn(false),
      m_bHighQualityNoiseReductionMode(false),
      m_bHfrMode(false),
      m_bSensorHDREnabled(false),
      m_bRdiMode(false),
      m_bSecureMode(false),
      m_bAeBracketingEnabled(false),
      mFlashValue(CAM_FLASH_MODE_OFF),
      mFlashDaemonValue(CAM_FLASH_MODE_OFF),
      mHfrMode(CAM_HFR_MODE_OFF),
      m_bHDRModeSensor(true),
      mOfflineRAW(false),
      m_bTruePortraitOn(false),
      m_bIsLowMemoryDevice(false),
      mCds_mode(CAM_CDS_MODE_OFF),
      m_LLCaptureEnabled(FALSE),
      m_LowLightLevel(CAM_LOW_LIGHT_OFF),
      m_bLtmForSeeMoreEnabled(false),
      m_expTime(0),
      m_isoValue(0),
      m_ManualCaptureMode(CAM_MANUAL_CAPTURE_TYPE_OFF),
      m_ledCalibrationMode(CAM_LED_CALIBRATION_MODE_OFF),
      m_bInstantAEC(false),
      m_bInstantCapture(false),
      mAecFrameBound(0),
      mAecSkipDisplayFrameBound(0),
      m_bQuadraCfa(false)
{
    char value[PROPERTY_VALUE_MAX];
    // TODO: may move to parameter instead of sysprop
    property_get("persist.debug.sf.showfps", value, "0");
    m_bDebugFps = atoi(value) > 0 ? true : false;

    // For thermal mode, it should be set as system property
    // because system property applies to all applications, while
    // parameters only apply to specific app.
    property_get("persist.camera.thermal.mode", value, "fps");
    if (!strcmp(value, "frameskip")) {
        m_ThermalMode = QCAMERA_THERMAL_ADJUST_FRAMESKIP;
    } else {
        if (strcmp(value, "fps"))
            LOGW("Invalid camera thermal mode %s", value);
        m_ThermalMode = QCAMERA_THERMAL_ADJUST_FPS;
    }

    memset(value, 0, sizeof(value));
    // As per Power/Quality evaluation, LTM is enabled by default in SeeMore/StillMore usecase
    // to improve the quality as there is no much impact to power
    property_get("persist.camera.ltmforseemore", value, "1");
    m_bLtmForSeeMoreEnabled = atoi(value);

    memset(&m_LiveSnapshotSize, 0, sizeof(m_LiveSnapshotSize));
    memset(&m_default_fps_range, 0, sizeof(m_default_fps_range));
    memset(&m_hfrFpsRange, 0, sizeof(m_hfrFpsRange));
    memset(&m_stillmore_config, 0, sizeof(cam_still_more_t));
    memset(&m_captureFrameConfig, 0, sizeof(cam_capture_frame_config_t));
    memset(&m_relCamSyncInfo, 0, sizeof(cam_sync_related_sensors_event_info_t));
    mTotalPPCount = 1;
    mZoomLevel = 0;
    mParmZoomLevel = 0;
    mCurPPCount = 0;
    mBufBatchCnt = 0;
    mRotation = 0;
    mJpegRotation = 0;
    mVideoBatchSize = 0;
    m_bOEMFeatEnabled = isOEMFeat1PropEnabled();
}

/*===========================================================================
 * FUNCTION   : QCameraParameters
 *
 * DESCRIPTION: constructor of QCameraParameters
 *
 * PARAMETERS :
 *   @params  : parameters in string
 *
 * RETURN     : None
 *==========================================================================*/
QCameraParameters::QCameraParameters(const String8 &params)
    : CameraParameters(params),
    m_reprocScaleParam(),
    m_pCapability(NULL),
    m_pCamOpsTbl(NULL),
    m_pParamHeap(NULL),
    m_pParamBuf(NULL),
    m_pRelCamSyncHeap(NULL),
    m_pRelCamSyncBuf(NULL),
    m_bFrameSyncEnabled(false),
    m_bZslMode(false),
    m_bZslMode_new(false),
    m_bForceZslMode(false),
    m_bRecordingHint(false),
    m_bRecordingHint_new(false),
    m_bHistogramEnabled(false),
    m_bLongshotEnabled(false),
    m_nFaceProcMask(0),
    m_bDebugFps(false),
    mFocusMode(CAM_FOCUS_MODE_MAX),
    mPreviewFormat(CAM_FORMAT_YUV_420_NV21),
    mAppPreviewFormat(CAM_FORMAT_YUV_420_NV21),
    mPictureFormat(CAM_FORMAT_JPEG),
    m_bNeedRestart(false),
    m_bNoDisplayMode(false),
    m_bWNROn(false),
    m_bTNRPreviewOn(false),
    m_bTNRVideoOn(false),
    m_bTNRSnapshotOn(false),
    m_bInited(false),
    m_nRetroBurstNum(0),
    m_nBurstLEDOnPeriod(100),
    m_bPreviewFlipChanged(false),
    m_bVideoFlipChanged(false),
    m_bSnapshotFlipChanged(false),
    m_bFixedFrameRateSet(false),
    m_bHDREnabled(false),
    m_bLocalHDREnabled(false),
    m_bAVTimerEnabled(false),
    m_AdjustFPS(NULL),
    m_bHDR1xFrameEnabled(false),
    m_HDRSceneEnabled(false),
    m_bHDRThumbnailProcessNeeded(false),
    m_bHDR1xExtraBufferNeeded(true),
    m_bHDROutputCropEnabled(false),
    m_tempMap(),
    m_bAFBracketingOn(false),
    m_bReFocusOn(false),
    m_bChromaFlashOn(false),
    m_bOptiZoomOn(false),
    m_bSceneSelection(false),
    m_SelectedScene(CAM_SCENE_MODE_MAX),
    m_bSeeMoreOn(false),
    m_bStillMoreOn(false),
    m_bHighQualityNoiseReductionMode(false),
    m_bHfrMode(false),
    m_bSensorHDREnabled(false),
    m_bRdiMode(false),
    m_bSecureMode(false),
    m_bAeBracketingEnabled(false),
    mFlashValue(CAM_FLASH_MODE_OFF),
    mFlashDaemonValue(CAM_FLASH_MODE_OFF),
    mHfrMode(CAM_HFR_MODE_OFF),
    m_bHDRModeSensor(true),
    mOfflineRAW(false),
    m_bTruePortraitOn(false),
    m_bIsLowMemoryDevice(false),
    mCds_mode(CAM_CDS_MODE_OFF),
    mParmEffect(CAM_EFFECT_MODE_OFF),
    m_LLCaptureEnabled(FALSE),
    m_LowLightLevel(CAM_LOW_LIGHT_OFF),
    m_bLtmForSeeMoreEnabled(false),
    m_expTime(0),
    m_isoValue(0),
    m_ManualCaptureMode(CAM_MANUAL_CAPTURE_TYPE_OFF),
    m_ledCalibrationMode(CAM_LED_CALIBRATION_MODE_OFF),
    m_bInstantAEC(false),
    m_bInstantCapture(false),
    mAecFrameBound(0),
    mAecSkipDisplayFrameBound(0),
    m_bQuadraCfa(false)
{
    memset(&m_LiveSnapshotSize, 0, sizeof(m_LiveSnapshotSize));
    memset(&m_default_fps_range, 0, sizeof(m_default_fps_range));
    memset(&m_hfrFpsRange, 0, sizeof(m_hfrFpsRange));
    memset(&m_stillmore_config, 0, sizeof(cam_still_more_t));
    memset(&m_relCamSyncInfo, 0, sizeof(cam_sync_related_sensors_event_info_t));
    mTotalPPCount = 0;
    mZoomLevel = 0;
    mParmZoomLevel = 0;
    mCurPPCount = 0;
    mRotation = 0;
    mJpegRotation = 0;
    mBufBatchCnt = 0;
    mVideoBatchSize = 0;
    m_bOEMFeatEnabled = isOEMFeat1PropEnabled();
}

/*===========================================================================
 * FUNCTION   : ~QCameraParameters
 *
 * DESCRIPTION: deconstructor of QCameraParameters
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
QCameraParameters::~QCameraParameters()
{
    deinit();
}

/*===========================================================================
 * FUNCTION   : createSizesString
 *
 * DESCRIPTION: create string obj contains array of dimensions
 *
 * PARAMETERS :
 *   @sizes   : array of dimensions
 *   @len     : size of dimension array
 *
 * RETURN     : string obj
 *==========================================================================*/
String8 QCameraParameters::createSizesString(const cam_dimension_t *sizes, size_t len)
{
    String8 str;
    char buffer[32];

    if (len > 0) {
        snprintf(buffer, sizeof(buffer), "%dx%d", sizes[0].width, sizes[0].height);
        str.append(buffer);
    }
    for (size_t i = 1; i < len; i++) {
        snprintf(buffer, sizeof(buffer), ",%dx%d",
                sizes[i].width, sizes[i].height);
        str.append(buffer);
    }
    return str;
}

/*===========================================================================
 * FUNCTION   : createValuesString
 *
 * DESCRIPTION: create string obj contains array of values from map when matched
 *              from input values array
 *
 * PARAMETERS :
 *   @values  : array of values
 *   @len     : size of values array
 *   @map     : map contains the mapping between values and enums
 *   @map_len : size of the map
 *
 * RETURN     : string obj
 *==========================================================================*/
template <typename valuesType, class mapType> String8 createValuesString(
        const valuesType *values, size_t len, const mapType *map, size_t map_len)
{
    String8 str;
    int count = 0;

    for (size_t i = 0; i < len; i++ ) {
        for (size_t j = 0; j < map_len; j ++)
            if (map[j].val == values[i]) {
                if (NULL != map[j].desc) {
                    if (count > 0) {
                        str.append(",");
                    }
                    str.append(map[j].desc);
                    count++;
                    break; //loop j
                }
            }
    }
    return str;
}

/*===========================================================================
 * FUNCTION   : createValuesStringFromMap
 *
 * DESCRIPTION: create string obj contains array of values directly from map
 *
 * PARAMETERS :
 *   @map     : map contains the mapping between values and enums
 *   @map_len : size of the map
 *
 * RETURN     : string obj
 *==========================================================================*/
template <class mapType> String8 createValuesStringFromMap(
        const mapType *map, size_t map_len)
{
    String8 str;

    for (size_t i = 0; i < map_len; i++) {
        if (NULL != map[i].desc) {
            if (i > 0) {
                str.append(",");
            }
            str.append(map[i].desc);
        }
    }
    return str;
}

/*===========================================================================
 * FUNCTION   : createZoomRatioValuesString
 *
 * DESCRIPTION: create string obj contains array of zoom ratio values
 *
 * PARAMETERS :
 *   @zoomRaios  : array of zoom ratios
 *   @length     : size of the array
 *
 * RETURN     : string obj
 *==========================================================================*/
String8 QCameraParameters::createZoomRatioValuesString(uint32_t *zoomRatios,
        size_t length)
{
    String8 str;
    char buffer[32] = {0};

    if(length > 0){
        snprintf(buffer, sizeof(buffer), "%d", zoomRatios[0]);
        str.append(buffer);
    }

    for (size_t i = 1; i < length; i++) {
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), ",%d", zoomRatios[i]);
        str.append(buffer);
    }
    return str;
}

/*===========================================================================
 * FUNCTION   : createHfrValuesString
 *
 * DESCRIPTION: create string obj contains array of hfr values from map when
 *              matched from input hfr values
 *
 * PARAMETERS :
 *   @values  : array of hfr info
 *   @len     : size of the array
 *   @map     : map of hfr string value and enum
 *   map_len  : size of map
 *
 * RETURN     : string obj
 *==========================================================================*/
String8 QCameraParameters::createHfrValuesString(const cam_hfr_info_t *values,
        size_t len, const QCameraMap<cam_hfr_mode_t> *map, size_t map_len)
{
    String8 str;
    int count = 0;

    //Create HFR supported size string.
    for (size_t i = 0; i < len; i++ ) {
        for (size_t j = 0; j < map_len; j ++) {
            if (map[j].val == (int)values[i].mode) {
                if (NULL != map[j].desc) {
                    if (count > 0) {
                        str.append(",");
                    }
                     str.append(map[j].desc);
                     count++;
                     break; //loop j
                }
            }
        }
    }
    if (count > 0) {
        str.append(",");
    }
    str.append(VIDEO_HFR_OFF);
    return str;
}

/*===========================================================================
 * FUNCTION   : createHfrSizesString
 *
 * DESCRIPTION: create string obj contains array of hfr sizes
 *
 * PARAMETERS :
 *   @values  : array of hfr info
 *   @len     : size of the array
 *
 * RETURN     : string obj
 *==========================================================================*/
String8 QCameraParameters::createHfrSizesString(const cam_hfr_info_t *values, size_t len)
{
    String8 str;
    char buffer[32];

    if (len > 0) {
        snprintf(buffer, sizeof(buffer), "%dx%d",
                 values[0].dim[0].width, values[0].dim[0].height);
        str.append(buffer);
    }
    for (size_t i = 1; i < len; i++) {
        snprintf(buffer, sizeof(buffer), ",%dx%d",
                 values[i].dim[0].width, values[i].dim[0].height);
        str.append(buffer);
    }
    return str;
}

/*===========================================================================
 * FUNCTION   : createFpsString
 *
 * DESCRIPTION: create string obj contains array of FPS rates
 *
 * PARAMETERS :
 *   @fps     : default fps range
 *
 * RETURN     : string obj
 *==========================================================================*/
String8 QCameraParameters::createFpsString(cam_fps_range_t &fps)
{
    char buffer[32];
    String8 fpsValues;

    int min_fps = int(fps.min_fps);
    int max_fps = int(fps.max_fps);

    if (min_fps < fps.min_fps){
        min_fps++;
    }
    if (max_fps > fps.max_fps) {
        max_fps--;
    }
    if (min_fps <= max_fps) {
        snprintf(buffer, sizeof(buffer), "%d", min_fps);
        fpsValues.append(buffer);
    }

    for (int i = min_fps+1; i <= max_fps; i++) {
        snprintf(buffer, sizeof(buffer), ",%d", i);
        fpsValues.append(buffer);
    }

    return fpsValues;
}

/*===========================================================================
 * FUNCTION   : createFpsRangeString
 *
 * DESCRIPTION: create string obj contains array of FPS ranges
 *
 * PARAMETERS :
 *   @fps     : array of fps ranges
 *   @len     : size of the array
 *   @default_fps_index : reference to index of default fps range
 *
 * RETURN     : string obj
 *==========================================================================*/
String8 QCameraParameters::createFpsRangeString(const cam_fps_range_t* fps,
        size_t len, int &default_fps_index)
{
    String8 str;
    char buffer[32];
    int max_range = 0;
    int min_fps, max_fps;

    if (len > 0) {
        min_fps = int(fps[0].min_fps * 1000);
        max_fps = int(fps[0].max_fps * 1000);
        max_range = max_fps - min_fps;
        default_fps_index = 0;
        snprintf(buffer, sizeof(buffer), "(%d,%d)", min_fps, max_fps);
        str.append(buffer);
    }
    for (size_t i = 1; i < len; i++) {
        min_fps = int(fps[i].min_fps * 1000);
        max_fps = int(fps[i].max_fps * 1000);
        if (max_range < (max_fps - min_fps)) {
            max_range = max_fps - min_fps;
            default_fps_index = (int)i;
        }
        snprintf(buffer, sizeof(buffer), ",(%d,%d)", min_fps, max_fps);
        str.append(buffer);
    }
    return str;
}

/*===========================================================================
 * FUNCTION   : lookupAttr
 *
 * DESCRIPTION: lookup a value by its name
 *
 * PARAMETERS :
 *   @attr    : map contains <name, value>
 *   @len     : size of the map
 *   @name    : name to be looked up
 *
 * RETURN     : valid value if found
 *              NAME_NOT_FOUND if not found
 *==========================================================================*/
template <class mapType> int lookupAttr(const mapType *arr,
        size_t len, const char *name)
{
    if (name) {
        for (size_t i = 0; i < len; i++) {
            if (!strcmp(arr[i].desc, name))
                return arr[i].val;
        }
    }
    return NAME_NOT_FOUND;
}

/*===========================================================================
 * FUNCTION   : lookupNameByValue
 *
 * DESCRIPTION: lookup a name by its value
 *
 * PARAMETERS :
 *   @attr    : map contains <name, value>
 *   @len     : size of the map
 *   @value   : value to be looked up
 *
 * RETURN     : name str or NULL if not found
 *==========================================================================*/
template <class mapType> const char *lookupNameByValue(const mapType *arr,
        size_t len, int value)
{
    for (size_t i = 0; i < len; i++) {
        if (arr[i].val == value) {
            return arr[i].desc;
        }
    }
    return NULL;
}

/*===========================================================================
 * FUNCTION   : setPreviewSize
 *
 * DESCRIPTION: set preview size from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setPreviewSize(const QCameraParameters& params)
{
    int width = 0, height = 0;
    int old_width = 0, old_height = 0;
    params.getPreviewSize(&width, &height);
    CameraParameters::getPreviewSize(&old_width, &old_height);

    // Validate the preview size
    for (size_t i = 0; i < m_pCapability->preview_sizes_tbl_cnt; ++i) {
        if (width ==  m_pCapability->preview_sizes_tbl[i].width
           && height ==  m_pCapability->preview_sizes_tbl[i].height) {
            // check if need to restart preview in case of preview size change
            if (width != old_width || height != old_height) {
                LOGI("Requested preview size %d x %d", width, height);
                m_bNeedRestart = true;
            }
            // set the new value
            CameraParameters::setPreviewSize(width, height);
            return NO_ERROR;
        }
    }
    if (m_relCamSyncInfo.mode == CAM_MODE_SECONDARY) {
        char prop[PROPERTY_VALUE_MAX];
        // set prop to configure aux preview size
        property_get("persist.camera.aux.preview.size", prop, "0");
        parse_pair(prop, &width, &height, 'x', NULL);
        bool foundMatch = false;
        for (size_t i = 0; i < m_pCapability->preview_sizes_tbl_cnt; ++i) {
            if (width ==  m_pCapability->preview_sizes_tbl[i].width &&
                    height ==  m_pCapability->preview_sizes_tbl[i].height) {
               foundMatch = true;
            }
        }
        if (!foundMatch) {
            width = m_pCapability->preview_sizes_tbl[0].width;
            height = m_pCapability->preview_sizes_tbl[0].height;
        }
        // check if need to restart preview in case of preview size change
        if (width != old_width || height != old_height) {
            m_bNeedRestart = true;
        }
        CameraParameters::setPreviewSize(width, height);
        LOGH("Secondary Camera: preview size %d x %d", width, height);
        return NO_ERROR;
    }

    LOGE("Invalid preview size requested: %dx%d", width, height);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setPictureSize
 *
 * DESCRIPTION: set picture size from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setPictureSize(const QCameraParameters& params)
{
    int width, height;
    params.getPictureSize(&width, &height);
    int old_width, old_height;
    CameraParameters::getPictureSize(&old_width, &old_height);

    // Validate the picture size
    if(!m_reprocScaleParam.isScaleEnabled()){
        for (size_t i = 0; i < m_pCapability->picture_sizes_tbl_cnt; ++i) {
            if (width ==  m_pCapability->picture_sizes_tbl[i].width
               && height ==  m_pCapability->picture_sizes_tbl[i].height) {
                // check if need to restart preview in case of picture size change
                if ((m_bZslMode || m_bRecordingHint) &&
                    (width != old_width || height != old_height)) {
                    LOGI("Requested picture size %d x %d", width, height);
                    m_bNeedRestart = true;
                }
                // set the new value
                CameraParameters::setPictureSize(width, height);
                // Update View angles based on Picture Aspect ratio
                updateViewAngles();
                return NO_ERROR;
            }
        }
    }else{
        //should use scaled picture size table to validate
        if(m_reprocScaleParam.setValidatePicSize(width, height) == NO_ERROR){
            // check if need to restart preview in case of picture size change
            if ((m_bZslMode || m_bRecordingHint) &&
                (width != old_width || height != old_height)) {
                m_bNeedRestart = true;
            }
            // set the new value
            char val[32];
            snprintf(val, sizeof(val), "%dx%d", width, height);
            updateParamEntry(KEY_PICTURE_SIZE, val);
            LOGH("%s", val);
            // Update View angles based on Picture Aspect ratio
            updateViewAngles();
            return NO_ERROR;
        }
    }
    if (m_relCamSyncInfo.mode == CAM_MODE_SECONDARY) {
        char prop[PROPERTY_VALUE_MAX];
        // set prop to configure aux preview size
        property_get("persist.camera.aux.picture.size", prop, "0");
        parse_pair(prop, &width, &height, 'x', NULL);
        bool foundMatch = false;
        for (size_t i = 0; i < m_pCapability->picture_sizes_tbl_cnt; ++i) {
            if (width ==  m_pCapability->picture_sizes_tbl[i].width &&
                    height ==  m_pCapability->picture_sizes_tbl[i].height) {
               foundMatch = true;
            }
        }
        if (!foundMatch) {
            width = m_pCapability->picture_sizes_tbl[0].width;
            height = m_pCapability->picture_sizes_tbl[0].height;
        }
        // check if need to restart preview in case of preview size change
        if (width != old_width || height != old_height) {
            m_bNeedRestart = true;
        }
        char val[32];
        snprintf(val, sizeof(val), "%dx%d", width, height);
        set(KEY_PICTURE_SIZE, val);
        LOGH("Secondary Camera: picture size %s", val);
        return NO_ERROR;
    }
    LOGE("Invalid picture size requested: %dx%d", width, height);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : updateViewAngles
 *
 * DESCRIPTION: Update the Horizontal & Vertical based on the Aspect ratio of Preview and
 *                        Picture aspect ratio
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
void QCameraParameters::updateViewAngles()
{
    double stillAspectRatio, maxPictureAspectRatio;
    int stillWidth, stillHeight, maxWidth, maxHeight;
    // The crop factors from the full sensor array to the still picture crop region
    double horizCropFactor = 1.f,vertCropFactor = 1.f;
    float horizViewAngle, vertViewAngle, maxHfov, maxVfov;

    // Get current Picture & max Snapshot sizes
    getPictureSize(&stillWidth, &stillHeight);
    maxWidth  = m_pCapability->picture_sizes_tbl[0].width;
    maxHeight = m_pCapability->picture_sizes_tbl[0].height;

    // Get default maximum FOV from corresponding sensor driver
    maxHfov = m_pCapability->hor_view_angle;
    maxVfov = m_pCapability->ver_view_angle;

    stillAspectRatio = (double)stillWidth/stillHeight;
    maxPictureAspectRatio = (double)maxWidth/maxHeight;
    LOGD("Stillwidth: %d, height: %d", stillWidth, stillHeight);
    LOGD("Max width: %d, height: %d", maxWidth, maxHeight);
    LOGD("still aspect: %f, Max Pic Aspect: %f",
            stillAspectRatio, maxPictureAspectRatio);

    // crop as per the Maximum Snapshot aspect ratio
    if (stillAspectRatio < maxPictureAspectRatio)
        horizCropFactor = stillAspectRatio/maxPictureAspectRatio;
    else
        vertCropFactor = maxPictureAspectRatio/stillAspectRatio;

    LOGD("horizCropFactor %f, vertCropFactor %f",
             horizCropFactor, vertCropFactor);

    // Now derive the final FOV's based on field of view formula is i.e,
    // angle of view = 2 * arctangent ( d / 2f )
    // where d is the physical sensor dimension of interest, and f is
    // the focal length. This only applies to rectilinear sensors, for focusing
    // at distances >> f, etc.
    // Here d/2f is nothing but the Maximum Horizontal or Veritical FOV
    horizViewAngle = (180/PI)*2*atan(horizCropFactor*tan((maxHfov/2)*(PI/180)));
    vertViewAngle = (180/PI)*2*atan(horizCropFactor*tan((maxVfov/2)*(PI/180)));

    setFloat(QCameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, horizViewAngle);
    setFloat(QCameraParameters::KEY_VERTICAL_VIEW_ANGLE, vertViewAngle);
    LOGH("Final horizViewAngle %f, vertViewAngle %f",
            horizViewAngle, vertViewAngle);
}

/*===========================================================================
 * FUNCTION   : setVideoSize
 *
 * DESCRIPTION: set video size from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setVideoSize(const QCameraParameters& params)
{
    const char *str= NULL;
    int width, height;
    str = params.get(KEY_VIDEO_SIZE);
    int old_width, old_height;
    CameraParameters::getVideoSize(&old_width, &old_height);
    if(!str) {
        //If application didn't set this parameter string, use the values from
        //getPreviewSize() as video dimensions.
        params.getPreviewSize(&width, &height);
        LOGW("No Record Size requested, use the preview dimensions");
    } else {
        params.getVideoSize(&width, &height);
    }

    // Validate the video size
    for (size_t i = 0; i < m_pCapability->video_sizes_tbl_cnt; ++i) {
        if (width ==  m_pCapability->video_sizes_tbl[i].width
                && height ==  m_pCapability->video_sizes_tbl[i].height) {
            // check if need to restart preview in case of video size change
            if (m_bRecordingHint &&
               (width != old_width || height != old_height)) {
                m_bNeedRestart = true;
            }

            // set the new value
            LOGH("Requested video size %d x %d", width, height);
            CameraParameters::setVideoSize(width, height);
            return NO_ERROR;
        }
    }
    if (m_relCamSyncInfo.mode == CAM_MODE_SECONDARY) {
        // Set the default preview size for secondary camera
        width = m_pCapability->video_sizes_tbl[0].width;
        height = m_pCapability->video_sizes_tbl[0].height;
        // check if need to restart preview in case of preview size change
        if (width != old_width || height != old_height) {
            m_bNeedRestart = true;
        }

        CameraParameters::setVideoSize(width, height);
        LOGH("Secondary Camera: video size %d x %d",
                 width, height);
        return NO_ERROR;
    }

    LOGE("Error !! Invalid video size requested: %dx%d", width, height);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : getLiveSnapshotSize
 *
 * DESCRIPTION: get live snapshot size
 *
 * PARAMETERS : dim - Update dim with the liveshot size
 *
 *==========================================================================*/
void QCameraParameters::getLiveSnapshotSize(cam_dimension_t &dim)
{
    if(is4k2kVideoResolution()) {
        // We support maximum 8M liveshot @4K2K video resolution
        cam_dimension_t resolution = {0, 0};
        CameraParameters::getVideoSize(&resolution.width, &resolution.height);
        if((m_LiveSnapshotSize.width > resolution.width) ||
                (m_LiveSnapshotSize.height > resolution.height)) {
            m_LiveSnapshotSize.width = resolution.width;
            m_LiveSnapshotSize.height = resolution.height;
        }
    }
    dim = m_LiveSnapshotSize;
    LOGH("w x h: %d x %d", dim.width, dim.height);
}

/*===========================================================================
 * FUNCTION   : setLiveSnapshotSize
 *
 * DESCRIPTION: set live snapshot size
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setLiveSnapshotSize(const QCameraParameters& params)
{
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.camera.opt.livepic", value, "1");
    bool useOptimal = atoi(value) > 0 ? true : false;
    bool vHdrOn;
    int32_t liveSnapWidth = 0, liveSnapHeight = 0;
    // use picture size from user setting
    params.getPictureSize(&m_LiveSnapshotSize.width, &m_LiveSnapshotSize.height);

    size_t livesnapshot_sizes_tbl_cnt =
            m_pCapability->livesnapshot_sizes_tbl_cnt;
    cam_dimension_t *livesnapshot_sizes_tbl =
            &m_pCapability->livesnapshot_sizes_tbl[0];

    if(is4k2kVideoResolution()) {
        // We support maximum 8M liveshot @4K2K video resolution
        cam_dimension_t resolution = {0, 0};
        CameraParameters::getVideoSize(&resolution.width, &resolution.height);
        if((m_LiveSnapshotSize.width > resolution.width) ||
                (m_LiveSnapshotSize.height > resolution.height)) {
            m_LiveSnapshotSize.width = resolution.width;
            m_LiveSnapshotSize.height = resolution.height;
        }
    }

    // check if HFR is enabled
    const char *hfrStr = params.get(KEY_QC_VIDEO_HIGH_FRAME_RATE);
    cam_hfr_mode_t hfrMode = CAM_HFR_MODE_OFF;
    const char *hsrStr = params.get(KEY_QC_VIDEO_HIGH_SPEED_RECORDING);

    const char *vhdrStr = params.get(KEY_QC_VIDEO_HDR);
    vHdrOn = (vhdrStr != NULL && (0 == strcmp(vhdrStr,"on"))) ? true : false;
    if (vHdrOn) {
        livesnapshot_sizes_tbl_cnt = m_pCapability->vhdr_livesnapshot_sizes_tbl_cnt;
        livesnapshot_sizes_tbl = &m_pCapability->vhdr_livesnapshot_sizes_tbl[0];
    }
    if ((hsrStr != NULL) && strcmp(hsrStr, "off")) {
        int32_t hsr = lookupAttr(HFR_MODES_MAP, PARAM_MAP_SIZE(HFR_MODES_MAP), hsrStr);
        if ((hsr != NAME_NOT_FOUND) && (hsr > CAM_HFR_MODE_OFF)) {
            // if HSR is enabled, change live snapshot size
            for (size_t i = 0; i < m_pCapability->hfr_tbl_cnt; i++) {
                if (m_pCapability->hfr_tbl[i].mode == hsr) {
                    livesnapshot_sizes_tbl_cnt =
                            m_pCapability->hfr_tbl[i].livesnapshot_sizes_tbl_cnt;
                    livesnapshot_sizes_tbl =
                            &m_pCapability->hfr_tbl[i].livesnapshot_sizes_tbl[0];
                    hfrMode = m_pCapability->hfr_tbl[i].mode;
                    break;
                }
            }
        }
    } else if ((hfrStr != NULL) && strcmp(hfrStr, "off")) {
        int32_t hfr = lookupAttr(HFR_MODES_MAP, PARAM_MAP_SIZE(HFR_MODES_MAP), hfrStr);
        if ((hfr != NAME_NOT_FOUND) && (hfr > CAM_HFR_MODE_OFF)) {
            // if HFR is enabled, change live snapshot size
            for (size_t i = 0; i < m_pCapability->hfr_tbl_cnt; i++) {
                if (m_pCapability->hfr_tbl[i].mode == hfr) {
                    livesnapshot_sizes_tbl_cnt =
                            m_pCapability->hfr_tbl[i].livesnapshot_sizes_tbl_cnt;
                    livesnapshot_sizes_tbl =
                            &m_pCapability->hfr_tbl[i].livesnapshot_sizes_tbl[0];
                    hfrMode = m_pCapability->hfr_tbl[i].mode;
                    break;
                }
            }
        }
    }

    if (useOptimal || hfrMode != CAM_HFR_MODE_OFF || vHdrOn) {
        bool found = false;

        // first check if picture size is within the list of supported sizes
        for (size_t i = 0; i < livesnapshot_sizes_tbl_cnt; ++i) {
            if (m_LiveSnapshotSize.width == livesnapshot_sizes_tbl[i].width &&
                m_LiveSnapshotSize.height == livesnapshot_sizes_tbl[i].height) {
                found = true;
                break;
            }
        }

        if (!found) {
            // use optimal live snapshot size from supported list,
            // that has same preview aspect ratio
            int width = 0, height = 0;
            params.getPreviewSize(&width, &height);

            double previewAspectRatio = (double)width / height;
            for (size_t i = 0; i < livesnapshot_sizes_tbl_cnt; ++i) {
                double ratio = (double)livesnapshot_sizes_tbl[i].width /
                                livesnapshot_sizes_tbl[i].height;
                if (fabs(previewAspectRatio - ratio) <= ASPECT_TOLERANCE) {
                    m_LiveSnapshotSize = livesnapshot_sizes_tbl[i];
                    found = true;
                    break;
                }
            }

            if (!found && ((hfrMode != CAM_HFR_MODE_OFF) || vHdrOn)) {
                // Cannot find matching aspect ration from supported live snapshot list
                // choose the max dim from preview and video size
                LOGD("Cannot find matching aspect ratio, choose max of preview or video size");
                params.getVideoSize(&m_LiveSnapshotSize.width, &m_LiveSnapshotSize.height);
                if (m_LiveSnapshotSize.width < width && m_LiveSnapshotSize.height < height) {
                    m_LiveSnapshotSize.width = width;
                    m_LiveSnapshotSize.height = height;
                }
            }
        }
    }
    //To read liveshot resolution from setprop instead of matching aspect ratio.
    //The setprop resolution format should be WxH.
    //e.g: adb shell setprop persist.camera.liveshot.size 1280x720
    memset(value, 0, PROPERTY_VALUE_MAX);
    property_get("persist.camera.liveshot.size", value, "");
    if (strlen(value) > 0) {
        char *saveptr = NULL;
        char *token = strtok_r(value, "x", &saveptr);
        if (token != NULL) {
            liveSnapWidth = atoi(token);
        }
        token = strtok_r(NULL, "x", &saveptr);
        if (token != NULL) {
            liveSnapHeight = atoi(token);
        }
        if ((liveSnapWidth!=0) && (liveSnapHeight!=0)) {
            for (size_t i = 0; i < m_pCapability->picture_sizes_tbl_cnt; ++i) {
                if (liveSnapWidth ==  m_pCapability->picture_sizes_tbl[i].width
                        && liveSnapHeight ==  m_pCapability->picture_sizes_tbl[i].height) {
                   m_LiveSnapshotSize.width = liveSnapWidth;
                   m_LiveSnapshotSize.height = liveSnapHeight;
                   break;
                }
            }
        }
    }
    LOGH("live snapshot size %d x %d",
          m_LiveSnapshotSize.width, m_LiveSnapshotSize.height);

    return NO_ERROR;
}


/*===========================================================================
 * FUNCTION   : setRawSize
 *
 * DESCRIPTION: set live snapshot size
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setRawSize(cam_dimension_t &dim)
{
    m_rawSize = dim;
    return NO_ERROR;
}
/*===========================================================================
 * FUNCTION   : setPreviewFormat
 *
 * DESCRIPTION: set preview format from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setPreviewFormat(const QCameraParameters& params)
{
    const char *str = params.getPreviewFormat();
    int32_t previewFormat = lookupAttr(PREVIEW_FORMATS_MAP,
            PARAM_MAP_SIZE(PREVIEW_FORMATS_MAP), str);
    if (previewFormat != NAME_NOT_FOUND) {
        if (isUBWCEnabled()) {
            char prop[PROPERTY_VALUE_MAX];
            int pFormat;
            memset(prop, 0, sizeof(prop));
            property_get("persist.camera.preview.ubwc", prop, "1");

            pFormat = atoi(prop);
            if (pFormat == 1) {
                mPreviewFormat = CAM_FORMAT_YUV_420_NV12_UBWC;
                mAppPreviewFormat = (cam_format_t)previewFormat;
            } else {
                mPreviewFormat = (cam_format_t)previewFormat;
                mAppPreviewFormat = (cam_format_t)previewFormat;
            }
        } else {
            mPreviewFormat = (cam_format_t)previewFormat;
            mAppPreviewFormat = (cam_format_t)previewFormat;
        }
        CameraParameters::setPreviewFormat(str);
        LOGH("format %d\n", mPreviewFormat);
        return NO_ERROR;
    }
    LOGE("Invalid preview format value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setPictureFormat
 *
 * DESCRIPTION: set picture format from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setPictureFormat(const QCameraParameters& params)
{
    const char *str = params.getPictureFormat();
    int32_t pictureFormat = lookupAttr(PICTURE_TYPES_MAP, PARAM_MAP_SIZE(PICTURE_TYPES_MAP), str);
    if (pictureFormat != NAME_NOT_FOUND) {
        mPictureFormat = pictureFormat;

        CameraParameters::setPictureFormat(str);
        LOGH("format %d\n", mPictureFormat);
        return NO_ERROR;
    }
    LOGE("Invalid picture format value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setJpegThumbnailSize
 *
 * DESCRIPTION: set jpeg thumbnail size from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setJpegThumbnailSize(const QCameraParameters& params)
{
    int width = params.getInt(KEY_JPEG_THUMBNAIL_WIDTH);
    int height = params.getInt(KEY_JPEG_THUMBNAIL_HEIGHT);

    LOGD("requested jpeg thumbnail size %d x %d", width, height);
    int sizes_cnt = sizeof(THUMBNAIL_SIZES_MAP) / sizeof(cam_dimension_t);
    // Validate thumbnail size
    for (int i = 0; i < sizes_cnt; i++) {
        if (width == THUMBNAIL_SIZES_MAP[i].width &&
                height == THUMBNAIL_SIZES_MAP[i].height) {
           set(KEY_JPEG_THUMBNAIL_WIDTH, width);
           set(KEY_JPEG_THUMBNAIL_HEIGHT, height);
           return NO_ERROR;
        }
    }
    LOGE("error: setting jpeg thumbnail size (%d, %d)", width, height);
    return BAD_VALUE;
}

/*===========================================================================

 * FUNCTION   : setBurstLEDOnPeriod
 *
 * DESCRIPTION: set burst LED on period
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setBurstLEDOnPeriod(const QCameraParameters& params)
{
    int nBurstLEDOnPeriod = params.getInt(KEY_QC_SNAPSHOT_BURST_LED_ON_PERIOD);
    //Check if the LED ON period is within limits
    if ((nBurstLEDOnPeriod <= 0) || (nBurstLEDOnPeriod > 800)) {
        // if burst led on period is not set in parameters,
        // read from sys prop
        char prop[PROPERTY_VALUE_MAX];
        memset(prop, 0, sizeof(prop));
        property_get("persist.camera.led.on.period", prop, "0");
        nBurstLEDOnPeriod = atoi(prop);
        if (nBurstLEDOnPeriod <= 0) {
            nBurstLEDOnPeriod = 300;
        }
    }

    set(KEY_QC_SNAPSHOT_BURST_LED_ON_PERIOD, nBurstLEDOnPeriod);
    m_nBurstLEDOnPeriod = nBurstLEDOnPeriod;
    LOGH("Burst LED on period  %u", m_nBurstLEDOnPeriod);
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_BURST_LED_ON_PERIOD,
            (uint32_t)nBurstLEDOnPeriod)) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}



/*===========================================================================
 * FUNCTION   : setRetroActiveBurstNum
 *
 * DESCRIPTION: set retro active burst num
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setRetroActiveBurstNum(
        const QCameraParameters& params)
{
    int32_t nBurstNum = params.getInt(KEY_QC_NUM_RETRO_BURST_PER_SHUTTER);
    LOGH("m_nRetroBurstNum = %d", m_nRetroBurstNum);
    if (nBurstNum <= 0) {
        // if burst number is not set in parameters,
        // read from sys prop
        char prop[PROPERTY_VALUE_MAX];
        memset(prop, 0, sizeof(prop));
        property_get("persist.camera.retro.number", prop, "0");
        nBurstNum = atoi(prop);
        if (nBurstNum < 0) {
            nBurstNum = 0;
        }
    }

    set(KEY_QC_NUM_RETRO_BURST_PER_SHUTTER, nBurstNum);

    m_nRetroBurstNum = nBurstNum;
    LOGH("m_nRetroBurstNum = %d", m_nRetroBurstNum);
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setJpegQuality
 *
 * DESCRIPTION: set jpeg encpding quality from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setJpegQuality(const QCameraParameters& params)
{
    int32_t rc = NO_ERROR;
    int quality = params.getInt(KEY_JPEG_QUALITY);
    if (quality >= 0 && quality <= 100) {
        set(KEY_JPEG_QUALITY, quality);
    } else {
        LOGE("Invalid jpeg quality=%d", quality);
        rc = BAD_VALUE;
    }

    quality = params.getInt(KEY_JPEG_THUMBNAIL_QUALITY);
    if (quality >= 0 && quality <= 100) {
        set(KEY_JPEG_THUMBNAIL_QUALITY, quality);
    } else {
        LOGE("Invalid jpeg thumbnail quality=%d", quality);
        rc = BAD_VALUE;
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : setOrientaion
 *
 * DESCRIPTION: set orientaion from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setOrientation(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_ORIENTATION);

    if (str != NULL) {
        if (strcmp(str, portrait) == 0 || strcmp(str, landscape) == 0) {
            // Camera service needs this to decide if the preview frames and raw
            // pictures should be rotated.
            set(KEY_QC_ORIENTATION, str);
        } else {
            LOGE("Invalid orientation value: %s", str);
            return BAD_VALUE;
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setAutoExposure
 *
 * DESCRIPTION: set auto exposure value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setAutoExposure(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_AUTO_EXPOSURE);
    const char *prev_str = get(KEY_QC_AUTO_EXPOSURE);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setAutoExposure(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setPreviewFpsRange
 *
 * DESCRIPTION: set preview FPS range from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setPreviewFpsRange(const QCameraParameters& params)
{
    int minFps,maxFps;
    int prevMinFps, prevMaxFps, vidMinFps, vidMaxFps;
    int rc = NO_ERROR;
    bool found = false, updateNeeded = false;

    CameraParameters::getPreviewFpsRange(&prevMinFps, &prevMaxFps);
    params.getPreviewFpsRange(&minFps, &maxFps);

    LOGH("FpsRange Values:(%d, %d)", prevMinFps, prevMaxFps);
    LOGH("Requested FpsRange Values:(%d, %d)", minFps, maxFps);

    //first check if we need to change fps because of HFR mode change
    updateNeeded = UpdateHFRFrameRate(params);
    if (updateNeeded) {
        m_bNeedRestart = true;
        rc = setHighFrameRate(mHfrMode);
        if (rc != NO_ERROR) goto end;
    }
    LOGH("UpdateHFRFrameRate %d", updateNeeded);

    vidMinFps = (int)m_hfrFpsRange.video_min_fps;
    vidMaxFps = (int)m_hfrFpsRange.video_max_fps;

    if(minFps == prevMinFps && maxFps == prevMaxFps) {
        if ( m_bFixedFrameRateSet ) {
            minFps = params.getPreviewFrameRate() * 1000;
            maxFps = params.getPreviewFrameRate() * 1000;
            m_bFixedFrameRateSet = false;
        } else if (!updateNeeded) {
            LOGH("No change in FpsRange");
            rc = NO_ERROR;
            goto end;
        }
    }
    for(size_t i = 0; i < m_pCapability->fps_ranges_tbl_cnt; i++) {
        // if the value is in the supported list
        if (minFps >= m_pCapability->fps_ranges_tbl[i].min_fps * 1000 &&
                maxFps <= m_pCapability->fps_ranges_tbl[i].max_fps * 1000) {
            found = true;
            LOGH("FPS i=%d : minFps = %d, maxFps = %d"
                    " vidMinFps = %d, vidMaxFps = %d",
                     i, minFps, maxFps,
                    (int)m_hfrFpsRange.video_min_fps,
                    (int)m_hfrFpsRange.video_max_fps);
            if ((0.0f >= m_hfrFpsRange.video_min_fps) ||
                    (0.0f >= m_hfrFpsRange.video_max_fps)) {
                vidMinFps = minFps;
                vidMaxFps = maxFps;
            }
            else {
                vidMinFps = (int)m_hfrFpsRange.video_min_fps;
                vidMaxFps = (int)m_hfrFpsRange.video_max_fps;
            }

            setPreviewFpsRange(minFps, maxFps, vidMinFps, vidMaxFps);
            break;
        }
    }
    if(found == false){
        LOGE("error: FPS range value not supported");
        rc = BAD_VALUE;
    }
end:
    return rc;
}

/*===========================================================================
 * FUNCTION   : UpdateHFRFrameRate
 *
 * DESCRIPTION: set preview FPS range based on HFR setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : bool true/false
 *                  true -if HAL needs to overwrite FPS range set by app, false otherwise.
 *==========================================================================*/

bool QCameraParameters::UpdateHFRFrameRate(const QCameraParameters& params)
{
    bool updateNeeded = false;
    int min_fps, max_fps;
    int32_t hfrMode = CAM_HFR_MODE_OFF;
    int32_t newHfrMode = CAM_HFR_MODE_OFF;

    int parm_minfps,parm_maxfps;
    int prevMinFps, prevMaxFps;
    CameraParameters::getPreviewFpsRange(&prevMinFps, &prevMaxFps);
    params.getPreviewFpsRange(&parm_minfps, &parm_maxfps);
    LOGH("CameraParameters - : minFps = %d, maxFps = %d ",
                 prevMinFps, prevMaxFps);
    LOGH("Requested params - : minFps = %d, maxFps = %d ",
                 parm_minfps, parm_maxfps);

    const char *hfrStr = params.get(KEY_QC_VIDEO_HIGH_FRAME_RATE);
    const char *hsrStr = params.get(KEY_QC_VIDEO_HIGH_SPEED_RECORDING);

    const char *prev_hfrStr = CameraParameters::get(KEY_QC_VIDEO_HIGH_FRAME_RATE);
    const char *prev_hsrStr = CameraParameters::get(KEY_QC_VIDEO_HIGH_SPEED_RECORDING);

    if ((hfrStr != NULL) && (prev_hfrStr != NULL) && strcmp(hfrStr, prev_hfrStr)) {
        updateParamEntry(KEY_QC_VIDEO_HIGH_FRAME_RATE, hfrStr);
    }

    if ((hsrStr != NULL) && (prev_hsrStr != NULL) && strcmp(hsrStr, prev_hsrStr)) {
        updateParamEntry(KEY_QC_VIDEO_HIGH_SPEED_RECORDING, hsrStr);

    }

    // check if HFR is enabled
    if ((hfrStr != NULL) && strcmp(hfrStr, "off")) {
        hfrMode = lookupAttr(HFR_MODES_MAP, PARAM_MAP_SIZE(HFR_MODES_MAP), hfrStr);
        if (NAME_NOT_FOUND != hfrMode) newHfrMode = hfrMode;
    }
    // check if HSR is enabled
    else if ((hsrStr != NULL) && strcmp(hsrStr, "off")) {
        hfrMode = lookupAttr(HFR_MODES_MAP, PARAM_MAP_SIZE(HFR_MODES_MAP), hsrStr);
        if (NAME_NOT_FOUND != hfrMode) newHfrMode = hfrMode;
    }
    LOGH("prevHfrMode - %d, currentHfrMode = %d ",
                 mHfrMode, newHfrMode);

    if (mHfrMode != newHfrMode) {
        updateNeeded = true;
        mHfrMode = newHfrMode;
        switch (mHfrMode) {
            case CAM_HFR_MODE_60FPS:
                min_fps = 60000;
                max_fps = 60000;
                break;
            case CAM_HFR_MODE_90FPS:
                min_fps = 90000;
                max_fps = 90000;
                break;
            case CAM_HFR_MODE_120FPS:
                min_fps = 120000;
                max_fps = 120000;
                break;
            case CAM_HFR_MODE_150FPS:
                min_fps = 150000;
                max_fps = 150000;
                break;
            case CAM_HFR_MODE_180FPS:
                min_fps = 180000;
                max_fps = 180000;
                break;
            case CAM_HFR_MODE_210FPS:
                min_fps = 210000;
                max_fps = 210000;
                break;
            case CAM_HFR_MODE_240FPS:
                min_fps = 240000;
                max_fps = 240000;
                break;
            case CAM_HFR_MODE_480FPS:
                min_fps = 480000;
                max_fps = 480000;
                break;
            case CAM_HFR_MODE_OFF:
            default:
                // Set Video Fps to zero
                min_fps = 0;
                max_fps = 0;
                break;
        }
        m_hfrFpsRange.video_min_fps = (float)min_fps;
        m_hfrFpsRange.video_max_fps = (float)max_fps;

        LOGH("HFR mode (%d) Set video FPS : minFps = %d, maxFps = %d ",
                 mHfrMode, min_fps, max_fps);
    }

    // Remember if HFR mode is ON
    if ((mHfrMode > CAM_HFR_MODE_OFF) && (mHfrMode < CAM_HFR_MODE_MAX)) {
        LOGH("HFR mode is ON");
        m_bHfrMode = true;
    } else {
        m_hfrFpsRange.video_min_fps = 0;
        m_hfrFpsRange.video_max_fps = 0;
        m_bHfrMode = false;
        LOGH("HFR mode is OFF");
    }
    m_hfrFpsRange.min_fps = (float)parm_minfps;
    m_hfrFpsRange.max_fps = (float)parm_maxfps;

    if (m_bHfrMode && (mHfrMode > CAM_HFR_MODE_120FPS)
            && (parm_maxfps != 0)) {
        //Configure buffer batch count to use batch mode for higher fps
        setBufBatchCount((int8_t)(m_hfrFpsRange.video_max_fps / parm_maxfps));
    } else {
        //Reset batch count and update KEY for encoder
        setBufBatchCount(0);
    }
    return updateNeeded;
}

/*===========================================================================
 * FUNCTION   : setPreviewFrameRate
 *
 * DESCRIPTION: set preview frame rate from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setPreviewFrameRate(const QCameraParameters& params)
{
    const char *str = params.get(KEY_PREVIEW_FRAME_RATE);
    const char *prev_str = get(KEY_PREVIEW_FRAME_RATE);

    if ( str ) {
        if ( prev_str &&
             strcmp(str, prev_str)) {
            LOGD("Requested Fixed Frame Rate %s", str);
            updateParamEntry(KEY_PREVIEW_FRAME_RATE, str);
            m_bFixedFrameRateSet = true;
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setEffect
 *
 * DESCRIPTION: set effect value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setEffect(const QCameraParameters& params)
{
    const char *str = params.get(KEY_EFFECT);
    const char *prev_str = get(KEY_EFFECT);

    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.effect", prop, "none");

    if (strcmp(prop, "none")) {
        if ((prev_str == NULL) ||
                (strcmp(prop, prev_str) != 0) ||
                (m_bUpdateEffects == true)) {
            m_bUpdateEffects = false;
            return setEffect(prop);
        }
    } else if (str != NULL) {
        if ((prev_str == NULL) ||
                (strcmp(str, prev_str) != 0) ||
                (m_bUpdateEffects == true)) {
            m_bUpdateEffects = false;
            return setEffect(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setFocusMode
 *
 * DESCRIPTION: set focus mode from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setFocusMode(const QCameraParameters& params)
{
    const char *str = params.get(KEY_FOCUS_MODE);
    const char *prev_str = get(KEY_FOCUS_MODE);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setFocusMode(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setFocusPosition
 *
 * DESCRIPTION: set focus position from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t  QCameraParameters::setFocusPosition(const QCameraParameters& params)
{
    const char *focus_str = params.get(KEY_FOCUS_MODE);
    const char *prev_focus_str = get(KEY_FOCUS_MODE);

    if (NULL == focus_str) {
        return NO_ERROR;
    }

    LOGD("current focus mode: %s", focus_str);
    if (strcmp(focus_str, FOCUS_MODE_MANUAL_POSITION)) {
        LOGH(", dont set focus pos to back-end!");
        return NO_ERROR;
    }

    const char *pos = params.get(KEY_QC_MANUAL_FOCUS_POSITION);
    const char *prev_pos = get(KEY_QC_MANUAL_FOCUS_POSITION);
    const char *type = params.get(KEY_QC_MANUAL_FOCUS_POS_TYPE);
    const char *prev_type = get(KEY_QC_MANUAL_FOCUS_POS_TYPE);

    if ((pos != NULL) && (type != NULL) && (focus_str != NULL)) {
        if (prev_pos  == NULL || (strcmp(pos, prev_pos) != 0) ||
            prev_type == NULL || (strcmp(type, prev_type) != 0) ||
            prev_focus_str == NULL || (strcmp(focus_str, prev_focus_str) != 0)) {
            return setFocusPosition(type, pos);
        }
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setBrightness
 *
 * DESCRIPTION: set brightness control value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setBrightness(const QCameraParameters& params)
{
    int currentBrightness = getInt(KEY_QC_BRIGHTNESS);
    int brightness = params.getInt(KEY_QC_BRIGHTNESS);

    if(params.get(KEY_QC_BRIGHTNESS) == NULL) {
       LOGH("Brigtness not set by App ");
       return NO_ERROR;
    }
    if (currentBrightness !=  brightness) {
        if (brightness >= m_pCapability->brightness_ctrl.min_value &&
            brightness <= m_pCapability->brightness_ctrl.max_value) {
            LOGD("new brightness value : %d ", brightness);
            return setBrightness(brightness);
        } else {
            LOGE("invalid value %d out of (%d, %d)",
                   brightness,
                  m_pCapability->brightness_ctrl.min_value,
                  m_pCapability->brightness_ctrl.max_value);
            return BAD_VALUE;
        }
    } else {
        LOGD("No brightness value changed.");
        return NO_ERROR;
    }
}

/*===========================================================================
 * FUNCTION   : getBrightness
 *
 * DESCRIPTION: get brightness control value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCameraParameters::getBrightness()
{
    return getInt(KEY_QC_BRIGHTNESS);
}

/*===========================================================================
 * FUNCTION   : setSharpness
 *
 * DESCRIPTION: set sharpness control value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSharpness(const QCameraParameters& params)
{
    int shaprness = params.getInt(KEY_QC_SHARPNESS);
    int prev_sharp = getInt(KEY_QC_SHARPNESS);

    if(params.get(KEY_QC_SHARPNESS) == NULL) {
       LOGH("Sharpness not set by App ");
       return NO_ERROR;
    }
    if (prev_sharp !=  shaprness) {
        if((shaprness >= m_pCapability->sharpness_ctrl.min_value) &&
           (shaprness <= m_pCapability->sharpness_ctrl.max_value)) {
            LOGD("new sharpness value : %d ", shaprness);
            return setSharpness(shaprness);
        } else {
            LOGE("invalid value %d out of (%d, %d)",
                   shaprness,
                  m_pCapability->sharpness_ctrl.min_value,
                  m_pCapability->sharpness_ctrl.max_value);
            return BAD_VALUE;
        }
    } else {
        LOGD("No value change in shaprness");
        return NO_ERROR;
    }
}

/*===========================================================================
 * FUNCTION   : setSkintoneEnahancement
 *
 * DESCRIPTION: set skin tone enhancement factor from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSkinToneEnhancement(const QCameraParameters& params)
{
    int sceFactor = params.getInt(KEY_QC_SCE_FACTOR);
    int prev_sceFactor = getInt(KEY_QC_SCE_FACTOR);

    if(params.get(KEY_QC_SCE_FACTOR) == NULL) {
       LOGH("Skintone enhancement not set by App ");
       return NO_ERROR;
    }
    if (prev_sceFactor != sceFactor) {
        if((sceFactor >= m_pCapability->sce_ctrl.min_value) &&
           (sceFactor <= m_pCapability->sce_ctrl.max_value)) {
            LOGD("new Skintone Enhancement value : %d ", sceFactor);
            return setSkinToneEnhancement(sceFactor);
        } else {
            LOGE("invalid value %d out of (%d, %d)",
                   sceFactor,
                  m_pCapability->sce_ctrl.min_value,
                  m_pCapability->sce_ctrl.max_value);
            return BAD_VALUE;
        }
    } else {
        LOGD("No value change in skintone enhancement factor");
        return NO_ERROR;
    }
}

/*===========================================================================
 * FUNCTION   : setSaturation
 *
 * DESCRIPTION: set saturation control value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSaturation(const QCameraParameters& params)
{
    int saturation = params.getInt(KEY_QC_SATURATION);
    int prev_sat = getInt(KEY_QC_SATURATION);

    if(params.get(KEY_QC_SATURATION) == NULL) {
       LOGH("Saturation not set by App ");
       return NO_ERROR;
    }
    if (prev_sat !=  saturation) {
        if((saturation >= m_pCapability->saturation_ctrl.min_value) &&
           (saturation <= m_pCapability->saturation_ctrl.max_value)) {
            LOGD("new saturation value : %d ", saturation);
            return setSaturation(saturation);
        } else {
            LOGE("invalid value %d out of (%d, %d)",
                   saturation,
                  m_pCapability->saturation_ctrl.min_value,
                  m_pCapability->saturation_ctrl.max_value);
            return BAD_VALUE;
        }
    } else {
        LOGD("No value change in saturation factor");
        return NO_ERROR;
    }
}

/*===========================================================================
 * FUNCTION   : setContrast
 *
 * DESCRIPTION: set contrast control value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setContrast(const QCameraParameters& params)
{
    int contrast = params.getInt(KEY_QC_CONTRAST);
    int prev_contrast = getInt(KEY_QC_CONTRAST);

    if(params.get(KEY_QC_CONTRAST) == NULL) {
       LOGH("Contrast not set by App ");
       return NO_ERROR;
    }
    if (prev_contrast !=  contrast) {
        if((contrast >= m_pCapability->contrast_ctrl.min_value) &&
           (contrast <= m_pCapability->contrast_ctrl.max_value)) {
            LOGD("new contrast value : %d ", contrast);
            int32_t rc = setContrast(contrast);
            return rc;
        } else {
            LOGE("invalid value %d out of (%d, %d)",
                   contrast,
                  m_pCapability->contrast_ctrl.min_value,
                  m_pCapability->contrast_ctrl.max_value);
            return BAD_VALUE;
        }
    } else {
        LOGD("No value change in contrast");
        return NO_ERROR;
    }
}

/*===========================================================================
 * FUNCTION   : setExposureCompensation
 *
 * DESCRIPTION: set exposure compensation value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setExposureCompensation(const QCameraParameters & params)
{
    int expComp = params.getInt(KEY_EXPOSURE_COMPENSATION);
    int prev_expComp = getInt(KEY_EXPOSURE_COMPENSATION);

    if(params.get(KEY_EXPOSURE_COMPENSATION) == NULL) {
       LOGH("Exposure compensation not set by App ");
       return NO_ERROR;
    }
    if (prev_expComp != expComp) {
        if((expComp >= m_pCapability->exposure_compensation_min) &&
           (expComp <= m_pCapability->exposure_compensation_max)) {
            LOGD("new Exposure Compensation value : %d ", expComp);
            return setExposureCompensation(expComp);
        } else {
            LOGE("invalid value %d out of (%d, %d)",
                   expComp,
                  m_pCapability->exposure_compensation_min,
                  m_pCapability->exposure_compensation_max);
            return BAD_VALUE;
        }
    } else {
        LOGD("No value change in Exposure Compensation");
        return NO_ERROR;
    }
}

/*===========================================================================
 * FUNCTION   : setWhiteBalance
 *
 * DESCRIPTION: set white balance value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setWhiteBalance(const QCameraParameters& params)
{
    const char *str = params.get(KEY_WHITE_BALANCE);
    const char *prev_str = get(KEY_WHITE_BALANCE);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setWhiteBalance(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setManualWhiteBalance
 *
 * DESCRIPTION: set manual white balance from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t  QCameraParameters::setManualWhiteBalance(const QCameraParameters& params)
{
    int32_t rc = NO_ERROR;
    const char *wb_str = params.get(KEY_WHITE_BALANCE);
    const char *prev_wb_str = get(KEY_WHITE_BALANCE);
    LOGD("current wb mode: %s", wb_str);

    if (wb_str != NULL) {
        if (strcmp(wb_str, WHITE_BALANCE_MANUAL)) {
            LOGD("dont set cct to back-end.");
            return NO_ERROR;
        }
    }

    const char *value = params.get(KEY_QC_MANUAL_WB_VALUE);
    const char *prev_value = get(KEY_QC_MANUAL_WB_VALUE);
    const char *type = params.get(KEY_QC_MANUAL_WB_TYPE);
    const char *prev_type = get(KEY_QC_MANUAL_WB_TYPE);

    if ((value != NULL) && (type != NULL) && (wb_str != NULL)) {
        if (prev_value  == NULL || (strcmp(value, prev_value) != 0) ||
            prev_type == NULL || (strcmp(type, prev_type) != 0) ||
            prev_wb_str == NULL || (strcmp(wb_str, prev_wb_str) != 0)) {
            updateParamEntry(KEY_QC_MANUAL_WB_TYPE, type);
            updateParamEntry(KEY_QC_MANUAL_WB_VALUE, value);
            int32_t wb_type = atoi(type);
            if (wb_type == CAM_MANUAL_WB_MODE_CCT) {
                rc = setWBManualCCT(value);
            } else if (wb_type == CAM_MANUAL_WB_MODE_GAIN) {
                rc = setManualWBGains(value);
            } else {
                rc = BAD_VALUE;
            }
        }
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : setAntibanding
 *
 * DESCRIPTION: set antibanding value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setAntibanding(const QCameraParameters& params)
{
    const char *str = params.get(KEY_ANTIBANDING);
    const char *prev_str = get(KEY_ANTIBANDING);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setAntibanding(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setStatsDebugMask
 *
 * DESCRIPTION: get the value from persist file in Stats module that will
 *              control funtionality in the module
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setStatsDebugMask()
{
    uint32_t mask = 0;
    char value[PROPERTY_VALUE_MAX];

    property_get("persist.camera.stats.debug.mask", value, "0");
    mask = (uint32_t)atoi(value);

    LOGH("ctrl mask :%d", mask);

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_STATS_DEBUG_MASK, mask)) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setPAAF
 *
 * DESCRIPTION: get the value from persist file in Stats module that will
 *              control the preview assisted AF in the module
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setPAAF()
{
    uint32_t paaf = 0;
    char value[PROPERTY_VALUE_MAX];

    property_get("persist.camera.stats.af.paaf", value, "1");
    paaf = (uint32_t)atoi(value);

    LOGH("PAAF is: %s", paaf ? "ON": "OFF");

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_STATS_AF_PAAF, paaf)) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setSceneDetect
 *
 * DESCRIPTION: set scenen detect value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSceneDetect(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_SCENE_DETECT);
    const char *prev_str = get(KEY_QC_SCENE_DETECT);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setSceneDetect(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setVideoHDR
 *
 * DESCRIPTION: set video HDR value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setVideoHDR(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_VIDEO_HDR);
    const char *prev_str = get(KEY_QC_VIDEO_HDR);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setVideoHDR(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setVtEnable
 *
 * DESCRIPTION: set vt Time Stamp enable from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setVtEnable(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_VT_ENABLE);
    const char *prev_str = get(KEY_QC_VT_ENABLE);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setVtEnable(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setFaceRecognition
 *
 * DESCRIPTION: set face recognition mode from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setFaceRecognition(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_FACE_RECOGNITION);
    const char *prev_str = get(KEY_QC_FACE_RECOGNITION);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            uint32_t maxFaces = (uint32_t)params.getInt(KEY_QC_MAX_NUM_REQUESTED_FACES);
            return setFaceRecognition(str, maxFaces);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setZoom
 *
 * DESCRIPTION: set zoom value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setZoom(const QCameraParameters& params)
{
    if ((m_pCapability->zoom_supported == 0 ||
         m_pCapability->zoom_ratio_tbl_cnt == 0)) {
        LOGH("no zoom support");
        return NO_ERROR;
    }

    int zoomLevel = params.getInt(KEY_ZOOM);
    mParmZoomLevel = zoomLevel;
    if ((zoomLevel < 0) || (zoomLevel >= (int)m_pCapability->zoom_ratio_tbl_cnt)) {
        LOGE("invalid value %d out of (%d, %d)",
               zoomLevel,
              0, m_pCapability->zoom_ratio_tbl_cnt-1);
        return BAD_VALUE;
    }

    int prevZoomLevel = getInt(KEY_ZOOM);
    if (prevZoomLevel == zoomLevel) {
        LOGD("No value change in zoom %d %d", prevZoomLevel, zoomLevel);
        return NO_ERROR;
    }

    return setZoom(zoomLevel);
}

/*===========================================================================
 * FUNCTION   : setISOValue
 *
 * DESCRIPTION: set ISO value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t  QCameraParameters::setISOValue(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_ISO_MODE);
    const char *prev_str = get(KEY_QC_ISO_MODE);

    if(getManualCaptureMode()) {
        char iso_val[PROPERTY_VALUE_MAX];

        property_get("persist.camera.iso", iso_val, "");
        if (strlen(iso_val) > 0) {
            if (prev_str == NULL ||
                    strcmp(iso_val, prev_str) != 0) {
                return setISOValue(iso_val);
            }
        }
    } else if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setISOValue(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setContinuousISO
 *
 * DESCRIPTION: set ISO value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t  QCameraParameters::setContinuousISO(const char *isoValue)
{
    char iso[PROPERTY_VALUE_MAX];
    int32_t continous_iso = 0;

    // Check if continuous ISO is set through setproperty
    property_get("persist.camera.continuous.iso", iso, "");
    if (strlen(iso) > 0) {
        continous_iso = atoi(iso);
    } else {
        continous_iso = atoi(isoValue);
    }

    if ((continous_iso >= 0) &&
            (continous_iso <= m_pCapability->sensitivity_range.max_sensitivity)) {
        LOGH("Setting continuous ISO value %d", continous_iso);
        updateParamEntry(KEY_QC_CONTINUOUS_ISO, isoValue);

        cam_intf_parm_manual_3a_t iso_settings;
        memset(&iso_settings, 0, sizeof(cam_intf_parm_manual_3a_t));
        iso_settings.previewOnly = FALSE;
        iso_settings.value = continous_iso;
        if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_ISO, iso_settings)) {
            return BAD_VALUE;
        }
        return NO_ERROR;
    }
    LOGE("Invalid iso value: %d", continous_iso);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setExposureTime
 *
 * DESCRIPTION: set exposure time from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t  QCameraParameters::setExposureTime(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_EXPOSURE_TIME);
    const char *prev_str = get(KEY_QC_EXPOSURE_TIME);
    if (str != NULL) {
        if (prev_str == NULL ||
                strcmp(str, prev_str) != 0) {
            return setExposureTime(str);
        }
    } else if(getManualCaptureMode()) {
        char expTime[PROPERTY_VALUE_MAX];

        property_get("persist.camera.exposure.time", expTime, "");
        if (strlen(expTime) > 0) {
            if (prev_str == NULL ||
                    strcmp(expTime, prev_str) != 0) {
                return setExposureTime(expTime);
            }
        }
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setVideoRotation
 *
 * DESCRIPTION: set rotation value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setVideoRotation(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_VIDEO_ROTATION);
    if(str != NULL) {
        int value = lookupAttr(VIDEO_ROTATION_MODES_MAP,
                PARAM_MAP_SIZE(VIDEO_ROTATION_MODES_MAP), str);
        if (value != NAME_NOT_FOUND) {
            updateParamEntry(KEY_QC_VIDEO_ROTATION, str);
            LOGL("setVideoRotation:   %d: ", str, value);
        } else {
            LOGE("Invalid rotation value: %d", value);
            return BAD_VALUE;
        }

    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setRotation
 *
 * DESCRIPTION: set rotation value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setRotation(const QCameraParameters& params)
{
    int32_t rotation = params.getInt(KEY_ROTATION);
    if (rotation != -1) {
        if (rotation == 0 || rotation == 90 ||
            rotation == 180 || rotation == 270) {
            set(KEY_ROTATION, rotation);

            ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_META_JPEG_ORIENTATION,
                    rotation);
            mRotation = rotation;
        } else {
            LOGE("Invalid rotation value: %d", rotation);
            return BAD_VALUE;
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setFlash
 *
 * DESCRIPTION: set flash mode from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setFlash(const QCameraParameters& params)
{
    const char *str = params.get(KEY_FLASH_MODE);
    const char *prev_str = get(KEY_FLASH_MODE);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setFlash(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setAecLock
 *
 * DESCRIPTION: set AEC lock value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setAecLock(const QCameraParameters& params)
{
    const char *str = params.get(KEY_AUTO_EXPOSURE_LOCK);
    const char *prev_str = get(KEY_AUTO_EXPOSURE_LOCK);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setAecLock(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setAwbLock
 *
 * DESCRIPTION: set AWB lock from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setAwbLock(const QCameraParameters& params)
{
    const char *str = params.get(KEY_AUTO_WHITEBALANCE_LOCK);
    const char *prev_str = get(KEY_AUTO_WHITEBALANCE_LOCK);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setAwbLock(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setAutoHDR
 *
 * DESCRIPTION: Enable/disable auto HDR
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setAutoHDR(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_AUTO_HDR_ENABLE);
    const char *prev_str = get(KEY_QC_AUTO_HDR_ENABLE);
    char prop[PROPERTY_VALUE_MAX];

    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.auto.hdr.enable", prop, VALUE_DISABLE);
    if (str != NULL) {
       if (prev_str == NULL ||
           strcmp(str, prev_str) != 0) {
           LOGH("Auto HDR set to: %s", str);
           return updateParamEntry(KEY_QC_AUTO_HDR_ENABLE, str);
       }
    } else {
       if (prev_str == NULL ||
           strcmp(prev_str, prop) != 0 ) {
           LOGH("Auto HDR set to: %s", prop);
           updateParamEntry(KEY_QC_AUTO_HDR_ENABLE, prop);
       }
    }

       return NO_ERROR;
}

/*===========================================================================
* FUNCTION   : isAutoHDREnabled
*
* DESCRIPTION: Query auto HDR status
*
* PARAMETERS : None
*
* RETURN     : bool true/false
*==========================================================================*/
bool QCameraParameters::isAutoHDREnabled()
{
    const char *str = get(KEY_QC_AUTO_HDR_ENABLE);
    if (str != NULL) {
        int32_t value = lookupAttr(ENABLE_DISABLE_MODES_MAP,
                PARAM_MAP_SIZE(ENABLE_DISABLE_MODES_MAP), str);
        if (value == NAME_NOT_FOUND) {
            LOGE("Invalid Auto HDR value %s", str);
            return false;
        }

        LOGH("Auto HDR status is: %d", value);
        return value ? true : false;
    }

    LOGH("Auto HDR status not set!");
    return false;
}

/*===========================================================================
 * FUNCTION   : setMCEValue
 *
 * DESCRIPTION: set memory color enhancement value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setMCEValue(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_MEMORY_COLOR_ENHANCEMENT);
    const char *prev_str = get(KEY_QC_MEMORY_COLOR_ENHANCEMENT);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setMCEValue(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setDISValue
 *
 * DESCRIPTION: enable/disable DIS from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setDISValue(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_DIS);
    const char *prev_str = get(KEY_QC_DIS);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setDISValue(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setLensShadeValue
 *
 * DESCRIPTION: set lens shade value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setLensShadeValue(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_LENSSHADE);
    const char *prev_str = get(KEY_QC_LENSSHADE);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setLensShadeValue(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setFocusAreas
 *
 * DESCRIPTION: set focus areas from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setFocusAreas(const QCameraParameters& params)
{
    const char *str = params.get(KEY_FOCUS_AREAS);

    if (getRelatedCamSyncInfo()->mode == CAM_MODE_SECONDARY) {
        // Ignore focus areas for secondary camera
        LOGH("Ignore focus areas for secondary camera!! ");
        return NO_ERROR;
    }
    if (str != NULL) {
        int max_num_af_areas = getInt(KEY_MAX_NUM_FOCUS_AREAS);
        if(max_num_af_areas == 0) {
            LOGE("max num of AF area is 0, cannot set focus areas");
            return BAD_VALUE;
        }

        const char *prev_str = get(KEY_FOCUS_AREAS);
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setFocusAreas(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setMeteringAreas
 *
 * DESCRIPTION: set metering areas from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setMeteringAreas(const QCameraParameters& params)
{
    const char *str = params.get(KEY_METERING_AREAS);
    if (str != NULL) {
        int max_num_mtr_areas = getInt(KEY_MAX_NUM_METERING_AREAS);
        if(max_num_mtr_areas == 0) {
            LOGE("max num of metering areas is 0, cannot set focus areas");
            return BAD_VALUE;
        }

        const char *prev_str = get(KEY_METERING_AREAS);
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0 ||
            (m_bNeedRestart == true)) {
            return setMeteringAreas(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setSceneMode
 *
 * DESCRIPTION: set scenen mode from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSceneMode(const QCameraParameters& params)
{
    const char *str = params.get(KEY_SCENE_MODE);
    const char *prev_str = get(KEY_SCENE_MODE);
    LOGH("str - %s, prev_str - %s", str, prev_str);

    // HDR & Recording are mutually exclusive and so disable HDR if recording hint is set
    if (m_bRecordingHint_new && m_bHDREnabled) {
        LOGH("Disable the HDR and set it to Auto");
        str = SCENE_MODE_AUTO;
        m_bLocalHDREnabled = true;
    } else if (!m_bRecordingHint_new && m_bLocalHDREnabled) {
        LOGH("Restore the HDR from Auto scene mode");
        str = SCENE_MODE_HDR;
        m_bLocalHDREnabled = false;
    }

    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {

            if(strcmp(str, SCENE_MODE_AUTO) == 0) {
                m_bSceneTransitionAuto = true;
            }
            if (strcmp(str, SCENE_MODE_HDR) == 0) {

                // If HDR is set from client  and the feature is not enabled in the backend, ignore it.
                if (m_bHDRModeSensor && isSupportedSensorHdrSize(params)) {
                    m_bSensorHDREnabled = true;
                    m_bHDREnabled = false;
                    LOGH("Sensor HDR mode Enabled");
                } else {
                    m_bHDREnabled = true;
                    LOGH("S/W HDR Enabled");
                }
            } else {
                m_bHDREnabled = false;
                if (m_bSensorHDREnabled) {
                    m_bSensorHDREnabled = false;
                    m_bNeedRestart = true;
                    setSensorSnapshotHDR("off");
                }
            }

            if (m_bSensorHDREnabled) {
                setSensorSnapshotHDR("on");
                m_bNeedRestart = true;
            } else if ((m_bHDREnabled) ||
                ((prev_str != NULL) && (strcmp(prev_str, SCENE_MODE_HDR) == 0))) {
                LOGH("scene mode changed between HDR and non-HDR, need restart");
                m_bNeedRestart = true;
            }

            return setSceneMode(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setSelectableZoneAf
 *
 * DESCRIPTION: set selectable zone auto focus value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSelectableZoneAf(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_SELECTABLE_ZONE_AF);
    const char *prev_str = get(KEY_QC_SELECTABLE_ZONE_AF);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setSelectableZoneAf(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setAEBracket
 *
 * DESCRIPTION: set AE bracket from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setAEBracket(const QCameraParameters& params)
{
    if (isHDREnabled()) {
        LOGH("scene mode is HDR, overwrite AE bracket setting to off");
        return setAEBracket(AE_BRACKET_OFF);
    }

    const char *expStr = params.get(KEY_QC_CAPTURE_BURST_EXPOSURE);
    if (NULL != expStr && strlen(expStr) > 0) {
        set(KEY_QC_CAPTURE_BURST_EXPOSURE, expStr);
    } else {
        char prop[PROPERTY_VALUE_MAX];
        memset(prop, 0, sizeof(prop));
        property_get("persist.capture.burst.exposures", prop, "");
        if (strlen(prop) > 0) {
            set(KEY_QC_CAPTURE_BURST_EXPOSURE, prop);
        } else {
            remove(KEY_QC_CAPTURE_BURST_EXPOSURE);
        }
    }

    const char *str = params.get(KEY_QC_AE_BRACKET_HDR);
    const char *prev_str = get(KEY_QC_AE_BRACKET_HDR);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setAEBracket(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setAFBracket
 *
 * DESCRIPTION: set AF bracket from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setAFBracket(const QCameraParameters& params)
{
    if ((m_pCapability->qcom_supported_feature_mask &
            (CAM_QCOM_FEATURE_REFOCUS | CAM_QCOM_FEATURE_UBIFOCUS)) == 0) {
        LOGH("AF Bracketing is not supported");
        return NO_ERROR;
    }
    const char *str = params.get(KEY_QC_AF_BRACKET);
    const char *prev_str = get(KEY_QC_AF_BRACKET);
    LOGH("str =%s & prev_str =%s", str, prev_str);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            m_bNeedRestart = true;
            return setAFBracket(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setReFocus
 *
 * DESCRIPTION: set refocus from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setReFocus(const QCameraParameters& params)
{
    if ((m_pCapability->qcom_supported_feature_mask &
            (CAM_QCOM_FEATURE_REFOCUS | CAM_QCOM_FEATURE_UBIFOCUS)) == 0) {
        LOGD("AF Bracketing is not supported");
        return NO_ERROR;
    }
    const char *str = params.get(KEY_QC_RE_FOCUS);
    const char *prev_str = get(KEY_QC_RE_FOCUS);
    LOGH("str =%s & prev_str =%s", str, prev_str);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            m_bNeedRestart = true;
            return setReFocus(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setChromaFlash
 *
 * DESCRIPTION: set chroma flash from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setChromaFlash(const QCameraParameters& params)
{
    if ((m_pCapability->qcom_supported_feature_mask &
        CAM_QCOM_FEATURE_CHROMA_FLASH) == 0) {
        LOGH("Chroma Flash is not supported");
        return NO_ERROR;
    }
    const char *str = params.get(KEY_QC_CHROMA_FLASH);
    const char *prev_str = get(KEY_QC_CHROMA_FLASH);
    LOGH("str =%s & prev_str =%s", str, prev_str);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            m_bNeedRestart = true;
            return setChromaFlash(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setOptiZoom
 *
 * DESCRIPTION: set opti zoom from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setOptiZoom(const QCameraParameters& params)
{
    if ((m_pCapability->qcom_supported_feature_mask &
        CAM_QCOM_FEATURE_OPTIZOOM) == 0){
        LOGH("Opti Zoom is not supported");
        return NO_ERROR;
    }
    const char *str = params.get(KEY_QC_OPTI_ZOOM);
    const char *prev_str = get(KEY_QC_OPTI_ZOOM);
    LOGH("str =%s & prev_str =%s", str, prev_str);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            m_bNeedRestart = true;
            return setOptiZoom(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setTruePortrait
 *
 * DESCRIPTION: set true portrait from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setTruePortrait(const QCameraParameters& params)
{
    if ((m_pCapability->qcom_supported_feature_mask &
            CAM_QCOM_FEATURE_TRUEPORTRAIT) == 0) {
        LOGD("True Portrait is not supported");
        return NO_ERROR;
    }
    const char *str = params.get(KEY_QC_TRUE_PORTRAIT);
    const char *prev_str = get(KEY_QC_TRUE_PORTRAIT);
    LOGH("str =%s & prev_str =%s", str, prev_str);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setTruePortrait(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setHDRMode
 *
 * DESCRIPTION: set HDR mode from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setHDRMode(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_HDR_MODE);
    const char *prev_str = get(KEY_QC_HDR_MODE);
    uint32_t supported_hdr_modes = m_pCapability->qcom_supported_feature_mask &
          (CAM_QCOM_FEATURE_SENSOR_HDR | CAM_QCOM_FEATURE_HDR);

    LOGH("str =%s & prev_str =%s", str, prev_str);
    if (str != NULL) {
        if ((CAM_QCOM_FEATURE_SENSOR_HDR == supported_hdr_modes) &&
                (strncmp(str, HDR_MODE_SENSOR, strlen(HDR_MODE_SENSOR)))) {
            LOGH("Only sensor HDR is supported");
            return NO_ERROR;
        } else if  ((CAM_QCOM_FEATURE_HDR == supported_hdr_modes) &&
                (strncmp(str, HDR_MODE_SENSOR, strlen(HDR_MODE_MULTI_FRAME)))) {
            LOGH("Only multi frame HDR is supported");
            return NO_ERROR;
        } else if (!supported_hdr_modes) {
            LOGH("HDR is not supported");
            return NO_ERROR;
        }
        if (prev_str == NULL ||
                strcmp(str, prev_str) != 0) {
            return setHDRMode(str);
        }
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setHDRNeed1x
 *
 * DESCRIPTION: set HDR need 1x from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setHDRNeed1x(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_HDR_NEED_1X);
    const char *prev_str = get(KEY_QC_HDR_NEED_1X);

    LOGH("str =%s & prev_str =%s", str, prev_str);
    if (str != NULL) {
        if (m_bHDRModeSensor) {
            LOGH("Only multi frame HDR supports 1x frame");
            return NO_ERROR;
        }
        if ((prev_str == NULL) || (strcmp(str, prev_str) != 0)) {
            return setHDRNeed1x(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setQuadraCfaMode
 *
 * DESCRIPTION: enable or disable Quadra CFA mode
 *
 * PARAMETERS :
 *   @enable : enable: 1; disable 0
 *   @initCommit: if configuration list needs to be initialized and commited
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setQuadraCfaMode(uint32_t enable, bool initCommit) {

   int32_t rc = NO_ERROR;

    if (getQuadraCfa()) {
        if (enable) {
            setOfflineRAW(TRUE);
        } else  {
            setOfflineRAW(FALSE);
        }
        if (initCommit) {
            if (initBatchUpdate(m_pParamBuf) < 0) {
                LOGE("Failed to initialize group update table");
                return FAILED_TRANSACTION;
            }
        }
        if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_QUADRA_CFA, enable)) {
            LOGE("Failed to update Quadra CFA mode");
            return BAD_VALUE;
        }
        if (initCommit) {
            rc = commitSetBatch();
            if (rc != NO_ERROR) {
                LOGE("Failed to commit Quadra CFA mode");
                return rc;
            }
        }
        LOGI("Quadra CFA mode %d ", enable);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : setQuadraCFA
 *
 * DESCRIPTION: set Quadra CFA mode
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setQuadraCfa(const QCameraParameters& params)
{

    int32_t width = 0,height = 0;
    bool prev_quadracfa = getQuadraCfa();
    int32_t rc = NO_ERROR;
    int32_t value;

    if (!m_pCapability->is_remosaic_lib_present) {
        LOGD("Quadra CFA mode not supported");
        return rc;
    }

    /*Checking if the user selected dim is more than maximum dim supported by
    Quadra sensor in normal mode. If more then switch to Quadra CFA mode else
    remain in normal zsl mode */
    params.getPictureSize(&width, &height);
    if (width > m_pCapability->raw_dim[0].width ||
        height > m_pCapability->raw_dim[0].height) {
        LOGI("Quadra CFA mode selected");
        m_bQuadraCfa = TRUE;
    } else {
        LOGI("Quadra CFA mode not selected");
        m_bQuadraCfa = FALSE;
    }
    value = m_bQuadraCfa;
    if (prev_quadracfa == m_bQuadraCfa) {
        LOGD("No change in Quadra CFA mode");
    } else {
        if (m_bZslMode && m_bQuadraCfa) {
            m_bNeedRestart = TRUE;
            setZslMode(FALSE);
        } else {
            const char *str_val  = params.get(KEY_QC_ZSL);
            int32_t value = lookupAttr(ON_OFF_MODES_MAP, PARAM_MAP_SIZE(ON_OFF_MODES_MAP),
                    str_val);
            if (value != NAME_NOT_FOUND && value) {
                rc = setZslMode(value);
                // ZSL mode changed, need restart preview
                m_bNeedRestart = true;
            }
        }
        setReprocCount();
    }
    LOGH("Quadra CFA mode = %d", m_bQuadraCfa);
    return rc;
}
/*===========================================================================
 * FUNCTION   : setSeeMore
 *
 * DESCRIPTION: set see more (llvd) from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSeeMore(const QCameraParameters& params)
{
    if ((m_pCapability->qcom_supported_feature_mask &
            CAM_QCOM_FEATURE_LLVD) == 0) {
        LOGD("See more is not supported");
        return NO_ERROR;
    }
    const char *str = params.get(KEY_QC_SEE_MORE);
    const char *prev_str = get(KEY_QC_SEE_MORE);
    LOGH("str =%s & prev_str =%s", str, prev_str);
    if (str != NULL) {
        if (prev_str == NULL || strcmp(str, prev_str) != 0) {
            m_bNeedRestart = true;
            return setSeeMore(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setNoiseReductionMode
 *
 * DESCRIPTION: set noise reduction mode from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setNoiseReductionMode(const QCameraParameters& params)
{
    if ((m_pCapability->qcom_supported_feature_mask & CAM_QTI_FEATURE_SW_TNR) == 0) {
        LOGD("SW TNR is not supported");
        return NO_ERROR;
    }
    const char *str = params.get(KEY_QC_NOISE_REDUCTION_MODE);
    const char *prev_str = get(KEY_QC_NOISE_REDUCTION_MODE);
    LOGH("str =%s & prev_str =%s", str, prev_str);
    if (str != NULL) {
        if (prev_str == NULL || strcmp(str, prev_str) != 0) {
            m_bNeedRestart = true;
            return setNoiseReductionMode(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setStillMore
 *
 * DESCRIPTION: set stillmore from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setStillMore(const QCameraParameters& params)
{
    if ((m_pCapability->qcom_supported_feature_mask &
            CAM_QCOM_FEATURE_STILLMORE) == 0) {
        LOGD("Stillmore is not supported");
        return NO_ERROR;
    }
    const char *str = params.get(KEY_QC_STILL_MORE);
    const char *prev_str = get(KEY_QC_STILL_MORE);
    LOGH("str =%s & prev_str =%s", str, prev_str);
    if (str != NULL) {
        if (prev_str == NULL || strcmp(str, prev_str) != 0) {
            m_bNeedRestart = true;
            return setStillMore(str);
        }
    }
    return NO_ERROR;
}

#ifdef TARGET_TS_MAKEUP

/*===========================================================================
 * FUNCTION   : setTsMakeup
 *
 * DESCRIPTION: set setTsMakeup from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setTsMakeup(const QCameraParameters& params)
{
    const char *str = params.get(KEY_TS_MAKEUP);
    const char *prev_str = get(KEY_TS_MAKEUP);
    LOGH("str =%s & prev_str =%s", str, prev_str);
    if (str != NULL) {
        if (prev_str == NULL || strcmp(str, prev_str) != 0) {
            m_bNeedRestart = true;
            set(KEY_TS_MAKEUP, str);
        }
        str = params.get(KEY_TS_MAKEUP_WHITEN);
        prev_str = get(KEY_TS_MAKEUP_WHITEN);
        if (prev_str == NULL || strcmp(str, prev_str) != 0) {
            if (str != NULL) {
                set(KEY_TS_MAKEUP_WHITEN, str);
            }
        }
        str = params.get(KEY_TS_MAKEUP_CLEAN);
        prev_str = get(KEY_TS_MAKEUP_CLEAN);
        if (prev_str == NULL || strcmp(str, prev_str) != 0) {
            if (str != NULL) {
                set(KEY_TS_MAKEUP_CLEAN, str);
            }
        }
    }
    return NO_ERROR;
}

#endif

/*===========================================================================
 * FUNCTION   : setRedeyeReduction
 *
 * DESCRIPTION: set red eye reduction setting from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setRedeyeReduction(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_REDEYE_REDUCTION);
    const char *prev_str = get(KEY_QC_REDEYE_REDUCTION);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setRedeyeReduction(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setGpsLocation
 *
 * DESCRIPTION: set GPS location information from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setGpsLocation(const QCameraParameters& params)
{
    const char *method = params.get(KEY_GPS_PROCESSING_METHOD);
    if (method) {
        set(KEY_GPS_PROCESSING_METHOD, method);
    }else {
        remove(KEY_GPS_PROCESSING_METHOD);
    }

    const char *latitude = params.get(KEY_GPS_LATITUDE);
    if (latitude) {
        set(KEY_GPS_LATITUDE, latitude);
    }else {
        remove(KEY_GPS_LATITUDE);
    }

    const char *latitudeRef = params.get(KEY_QC_GPS_LATITUDE_REF);
    if (latitudeRef) {
        set(KEY_QC_GPS_LATITUDE_REF, latitudeRef);
    }else {
        remove(KEY_QC_GPS_LATITUDE_REF);
    }

    const char *longitude = params.get(KEY_GPS_LONGITUDE);
    if (longitude) {
        set(KEY_GPS_LONGITUDE, longitude);
    }else {
        remove(KEY_GPS_LONGITUDE);
    }

    const char *longitudeRef = params.get(KEY_QC_GPS_LONGITUDE_REF);
    if (longitudeRef) {
        set(KEY_QC_GPS_LONGITUDE_REF, longitudeRef);
    }else {
        remove(KEY_QC_GPS_LONGITUDE_REF);
    }

    const char *altitudeRef = params.get(KEY_QC_GPS_ALTITUDE_REF);
    if (altitudeRef) {
        set(KEY_QC_GPS_ALTITUDE_REF, altitudeRef);
    }else {
        remove(KEY_QC_GPS_ALTITUDE_REF);
    }

    const char *altitude = params.get(KEY_GPS_ALTITUDE);
    if (altitude) {
        set(KEY_GPS_ALTITUDE, altitude);
    }else {
        remove(KEY_GPS_ALTITUDE);
    }

    const char *status = params.get(KEY_QC_GPS_STATUS);
    if (status) {
        set(KEY_QC_GPS_STATUS, status);
    } else {
        remove(KEY_QC_GPS_STATUS);
    }

    const char *timestamp = params.get(KEY_GPS_TIMESTAMP);
    if (timestamp) {
        set(KEY_GPS_TIMESTAMP, timestamp);
    }else {
        remove(KEY_GPS_TIMESTAMP);
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setNumOfSnapshot
 *
 * DESCRIPTION: set number of snapshot per shutter from user setting
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setNumOfSnapshot()
{
    int nBurstNum = 1;
    int nExpnum = 0;

    const char *bracket_str = get(KEY_QC_AE_BRACKET_HDR);
    if (bracket_str != NULL && strlen(bracket_str) > 0) {
        int value = lookupAttr(BRACKETING_MODES_MAP, PARAM_MAP_SIZE(BRACKETING_MODES_MAP),
                bracket_str);
        switch (value) {
        case CAM_EXP_BRACKETING_ON:
            {
                nExpnum = 0;
                const char *str_val = get(KEY_QC_CAPTURE_BURST_EXPOSURE);
                if ((str_val != NULL) && (strlen(str_val) > 0)) {
                    char prop[PROPERTY_VALUE_MAX];
                    memset(prop, 0, sizeof(prop));
                    strlcpy(prop, str_val, PROPERTY_VALUE_MAX);
                    char *saveptr = NULL;
                    char *token = strtok_r(prop, ",", &saveptr);
                    while (token != NULL) {
                        token = strtok_r(NULL, ",", &saveptr);
                        nExpnum++;
                    }
                }
                if (nExpnum == 0) {
                    nExpnum = 1;
                }
            }
            break;
        default:
            nExpnum = 1 + getNumOfExtraHDROutBufsIfNeeded();
            break;
        }
    }

    if (isUbiRefocus()) {
        nBurstNum = m_pCapability->refocus_af_bracketing_need.output_count + 1;
    }

    LOGH("nBurstNum = %d, nExpnum = %d", nBurstNum, nExpnum);
    set(KEY_QC_NUM_SNAPSHOT_PER_SHUTTER, nBurstNum * nExpnum);
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setRecordingHint
 *
 * DESCRIPTION: set recording hint value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setRecordingHint(const QCameraParameters& params)
{
    const char * str = params.get(KEY_RECORDING_HINT);
    const char *prev_str = get(KEY_RECORDING_HINT);
    if (str != NULL) {
        if (prev_str == NULL || strcmp(str, prev_str) != 0) {
            int32_t value = lookupAttr(TRUE_FALSE_MODES_MAP, PARAM_MAP_SIZE(TRUE_FALSE_MODES_MAP),
                    str);
            if(value != NAME_NOT_FOUND){
                updateParamEntry(KEY_RECORDING_HINT, str);
                setRecordingHintValue(value);
                if (getFaceDetectionOption() == true) {
                    if (!fdModeInVideo()) {
                        setFaceDetection(value > 0 ? false : true, false);
                    } else {
                        setFaceDetection(true, false);
                    }
                }
                if (m_bDISEnabled) {
                    LOGH("Setting DIS value again");
                    setDISValue(VALUE_ENABLE);
                }
                return NO_ERROR;
            } else {
                LOGE("Invalid recording hint value: %s", str);
                return BAD_VALUE;
            }
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setNoDisplayMode
 *
 * DESCRIPTION: set no display mode from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setNoDisplayMode(const QCameraParameters& params)
{
    const char *str_val  = params.get(KEY_QC_NO_DISPLAY_MODE);
    const char *prev_str = get(KEY_QC_NO_DISPLAY_MODE);
    char prop[PROPERTY_VALUE_MAX];
    LOGD("str_val: %s, prev_str: %s", str_val, prev_str);

    // Aux Camera Mode, set no display mode
    if (m_relCamSyncInfo.mode == CAM_MODE_SECONDARY) {
        if (!m_bNoDisplayMode) {
            set(KEY_QC_NO_DISPLAY_MODE, 1);
            m_bNoDisplayMode = true;
            m_bNeedRestart = true;
        }
        return NO_ERROR;
    }

    if(str_val && strlen(str_val) > 0) {
        if (prev_str == NULL || strcmp(str_val, prev_str) != 0) {
            m_bNoDisplayMode = atoi(str_val);
            set(KEY_QC_NO_DISPLAY_MODE, str_val);
            m_bNeedRestart = true;
        }
    } else {
        memset(prop, 0, sizeof(prop));
        property_get("persist.camera.no-display", prop, "0");
        m_bNoDisplayMode = atoi(prop);
    }
    LOGH("Param m_bNoDisplayMode = %d", m_bNoDisplayMode);
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setZslMode
 *
 * DESCRIPTION: set ZSL mode from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setZslMode(const QCameraParameters& params)
{
    const char *str_val  = params.get(KEY_QC_ZSL);
    const char *prev_val  = get(KEY_QC_ZSL);
    int32_t rc = NO_ERROR;

    if(m_bForceZslMode) {
        if (!m_bZslMode) {
            // Force ZSL mode to ON
            set(KEY_QC_ZSL, VALUE_ON);
            setZslMode(TRUE);
            LOGH("ZSL Mode forced to be enabled");
        }
    } else if (str_val != NULL) {
        if (prev_val == NULL || strcmp(str_val, prev_val) != 0) {
            int32_t value = lookupAttr(ON_OFF_MODES_MAP, PARAM_MAP_SIZE(ON_OFF_MODES_MAP),
                    str_val);
            if (value != NAME_NOT_FOUND) {
                set(KEY_QC_ZSL, str_val);
                rc = setZslMode(value);
                // ZSL mode changed, need restart preview
                m_bNeedRestart = true;
            } else {
                LOGE("Invalid ZSL mode value: %s", str_val);
                rc = BAD_VALUE;
            }
        }
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : setZslMode
 *
 * DESCRIPTION: set ZSL mode from user setting
 *
 * PARAMETERS :
 *   @value  : ZSL mode value
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setZslMode(bool value)
{
    int32_t rc = NO_ERROR;
    if(m_bForceZslMode) {
        if (!m_bZslMode) {
            // Force ZSL mode to ON
            set(KEY_QC_ZSL, VALUE_ON);
            m_bZslMode_new = true;
            m_bZslMode = true;
            m_bNeedRestart = true;

            int32_t value = m_bForceZslMode;
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_ZSL_MODE, value)) {
                rc = BAD_VALUE;
            }

            LOGI("ZSL Mode forced to be enabled");
        }
    } else {
        LOGI("ZSL Mode  -> %s", m_bZslMode_new ? "Enabled" : "Disabled");
        m_bZslMode_new = (value > 0)? true : false;
        if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_ZSL_MODE, value)) {
            rc = BAD_VALUE;
        }
    }
    LOGH("enabled: %d rc = %d", m_bZslMode_new, rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : updateZSLModeValue
 *
 * DESCRIPTION: update zsl mode value locally and to daemon
 *
 * PARAMETERS :
 *   @value   : zsl mode value
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::updateZSLModeValue(bool value)
{
    int32_t rc = NO_ERROR;
    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    rc = setZslMode(value);
    if (rc != NO_ERROR) {
        LOGE("Failed to ZSL value");
        return rc;
    }

    rc = commitSetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to update recording hint");
        return rc;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : setWaveletDenoise
 *
 * DESCRIPTION: set wavelet denoise value from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setWaveletDenoise(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_DENOISE);
    const char *prev_str = get(KEY_QC_DENOISE);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setWaveletDenoise(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setTemporalDenoise
 *
 * DESCRIPTION: set temporal denoise value from properties
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setTemporalDenoise(const QCameraParameters& params)
{
    if ((m_pCapability->qcom_supported_feature_mask & CAM_QCOM_FEATURE_CPP_TNR) == 0) {
        LOGH("TNR is not supported");
        return NO_ERROR;
    }

    const char *str = params.get(KEY_QC_TNR_MODE);
    const char *prev_str = get(KEY_QC_TNR_MODE);
    const char *video_str = params.get(KEY_QC_VIDEO_TNR_MODE);
    const char *video_prev_str = get(KEY_QC_VIDEO_TNR_MODE);
    char video_value[PROPERTY_VALUE_MAX];
    char preview_value[PROPERTY_VALUE_MAX];
    bool prev_video_tnr = m_bTNRVideoOn;
    bool prev_preview_tnr = m_bTNRPreviewOn;
    bool prev_snap_tnr = m_bTNRSnapshotOn;

    char value[PROPERTY_VALUE_MAX];
    memset(value, 0, sizeof(value));
    property_get("persist.camera.tnr_cds", value, "0");
    uint8_t tnr_cds = (uint8_t)atoi(value);

    if (m_bRecordingHint_new == true) {
        if (video_str) {
            if ((video_prev_str == NULL) || (strcmp(video_str, video_prev_str) != 0)) {
                if (!strcmp(video_str, VALUE_ON)) {
                    m_bTNRVideoOn = true;
                    m_bTNRPreviewOn = true;
                } else {
                    m_bTNRVideoOn = false;
                    m_bTNRPreviewOn = false;
                }
                updateParamEntry(KEY_QC_VIDEO_TNR_MODE, video_str);
            } else {
                return NO_ERROR;
            }
        }
    } else {
        if (str) {
            if ((prev_str == NULL) || (strcmp(str, prev_str) != 0)) {
                if (!strcmp(str, VALUE_ON)) {
                    m_bTNRPreviewOn = true;
                } else {
                    m_bTNRPreviewOn = false;
                }
                updateParamEntry(KEY_QC_TNR_MODE, str);
            } else {
                return NO_ERROR;
            }
        }
    }

    //Read setprops only if UI is not present or disabled.
    if ((m_bRecordingHint_new == true)
            && ((video_str == NULL)
            || (strcmp(video_str, VALUE_ON)))) {
        memset(video_value, 0, sizeof(video_value));
        property_get("persist.camera.tnr.video", video_value, VALUE_OFF);
        if (!strcmp(video_value, VALUE_ON)) {
            m_bTNRVideoOn = true;
        } else {
            m_bTNRVideoOn = false;
        }
        updateParamEntry(KEY_QC_VIDEO_TNR_MODE, video_value);

        memset(preview_value, 0, sizeof(preview_value));
        property_get("persist.camera.tnr.preview", preview_value, VALUE_OFF);
        if (!strcmp(preview_value, VALUE_ON)) {
            m_bTNRPreviewOn = true;
        } else {
            m_bTNRPreviewOn = false;
        }
        updateParamEntry(KEY_QC_TNR_MODE, preview_value);
    } else if ((m_bRecordingHint_new != true)
            && ((str == NULL) || (strcmp(str, VALUE_ON)))) {
        memset(preview_value, 0, sizeof(preview_value));
        property_get("persist.camera.tnr.preview", preview_value, VALUE_OFF);
        if (!strcmp(preview_value, VALUE_ON)) {
            m_bTNRPreviewOn = true;
        } else {
            m_bTNRPreviewOn = false;
        }
        updateParamEntry(KEY_QC_TNR_MODE, preview_value);
    }

    memset(value, 0, sizeof(value));
    property_get("persist.camera.tnr.snapshot", value, VALUE_OFF);
    if (!strcmp(value, VALUE_ON)) {
        m_bTNRSnapshotOn = true;
        LOGD("TNR enabled for SNAPSHOT stream");
    } else {
        m_bTNRSnapshotOn = false;
    }

    cam_denoise_param_t temp;
    memset(&temp, 0, sizeof(temp));
    if (m_bTNRVideoOn || m_bTNRPreviewOn || m_bTNRSnapshotOn) {
        temp.denoise_enable = 1;
        temp.process_plates = getDenoiseProcessPlate(
                CAM_INTF_PARM_TEMPORAL_DENOISE);

        if (!tnr_cds) {
            int32_t cds_mode = lookupAttr(CDS_MODES_MAP,
                    PARAM_MAP_SIZE(CDS_MODES_MAP), CDS_MODE_OFF);

            if (cds_mode != NAME_NOT_FOUND) {
                updateParamEntry(KEY_QC_VIDEO_CDS_MODE, CDS_MODE_OFF);
                if (m_bTNRPreviewOn) {
                    updateParamEntry(KEY_QC_CDS_MODE, CDS_MODE_OFF);
                }
                if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf,
                        CAM_INTF_PARM_CDS_MODE, cds_mode)) {
                    LOGE("Failed CDS MODE to update table");
                    return BAD_VALUE;
                }
                LOGD("CDS is set to = %s when TNR is enabled",
                         CDS_MODE_OFF);
                mCds_mode = cds_mode;
            } else {
                LOGE("Invalid argument for video CDS MODE %d",
                         cds_mode);
            }
        } else {
            LOGH("Enabled TNR with CDS");
        }
    }

    if ((m_bTNRVideoOn != prev_video_tnr)
            || (m_bTNRPreviewOn != prev_preview_tnr)
            || (prev_snap_tnr != m_bTNRSnapshotOn)) {
        LOGD("TNR enabled = %d, plates = %d",
                temp.denoise_enable, temp.process_plates);
        if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf,
                CAM_INTF_PARM_TEMPORAL_DENOISE, temp)) {
            return BAD_VALUE;
        }
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setCameraMode
 *
 * DESCRIPTION: set camera mode from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setCameraMode(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_CAMERA_MODE);
    if (str != NULL) {
        set(KEY_QC_CAMERA_MODE, str);
    } else {
        remove(KEY_QC_CAMERA_MODE);
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setSceneSelectionMode
 *
 * DESCRIPTION: set scene selection mode from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSceneSelectionMode(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_SCENE_SELECTION);
    const char *prev_str = get(KEY_QC_SCENE_SELECTION);
    if (NULL != str) {
        if ((NULL == prev_str) || (strcmp(str, prev_str) != 0)) {
            int32_t value = lookupAttr(ENABLE_DISABLE_MODES_MAP,
                    PARAM_MAP_SIZE(ENABLE_DISABLE_MODES_MAP), str);
            if (value != NAME_NOT_FOUND) {
                LOGD("Setting selection value %s", str);
                if (value && m_bZslMode_new) {
                    updateParamEntry(KEY_QC_SCENE_SELECTION, str);
                    m_bNeedRestart = true;
                    m_bSceneSelection = true;
                } else if (!value) {
                    updateParamEntry(KEY_QC_SCENE_SELECTION, str);
                    m_bNeedRestart = true;
                    m_bSceneSelection = false;
                } else {
                    LOGE("Trying to enable scene selection in non ZSL mode!!!");
                    return BAD_VALUE;
                }
            } else {
                LOGE("Trying to configure invalid scene selection value: %s",
                        str);
                return BAD_VALUE;
            }
        }
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setSelectedScene
 *
 * DESCRIPTION: select specific scene
 *
 * PARAMETERS :
 *   @scene   : scene mode
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSelectedScene(cam_scene_mode_type scene)
{
    Mutex::Autolock l(m_SceneSelectLock);
    m_SelectedScene = scene;
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : getSelectedScene
 *
 * DESCRIPTION: get selected scene
 *
 * PARAMETERS :
 *
 * RETURN     : currently selected scene
 *==========================================================================*/
cam_scene_mode_type QCameraParameters::getSelectedScene()
{
    Mutex::Autolock l(m_SceneSelectLock);
    return m_SelectedScene;
}

/*==========================================================
 * FUNCTION   : setRdiMode
 *
 * DESCRIPTION: set Rdi mode from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *===========================================================*/
int32_t QCameraParameters::setRdiMode(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_RDI_MODE);
    const char *prev_str = get(KEY_QC_RDI_MODE);
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));

    property_get("persist.camera.rdi.mode", prop, VALUE_DISABLE);
    if ((str != NULL) && (prev_str == NULL || strcmp(str, prev_str) != 0)) {
        LOGD("RDI mode set to %s", str);
        setRdiMode(str);
    } else if (prev_str == NULL || strcmp(prev_str, prop) != 0 ) {
        LOGD("RDI mode set to prop: %s", prop);
        setRdiMode(prop);
    }
    return NO_ERROR;
}

/*==========================================================
 * FUNCTION   : setSecureMode
 *
 * DESCRIPTION: set secure mode from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *===========================================================*/

int32_t QCameraParameters::setSecureMode(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_SECURE_MODE);
    const char *prev_str = get(KEY_QC_SECURE_MODE);
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));

    property_get("persist.camera.secure.mode", prop, VALUE_DISABLE);
    if ((str != NULL) && (prev_str == NULL || strcmp(str, prev_str) != 0)) {
        LOGD("Secure mode set to KEY: %s", str);
        setSecureMode(str);
    } else if (prev_str == NULL || strcmp(prev_str, prop) != 0 ) {
        LOGD("Secure mode set to prop: %s", prop);
        setSecureMode(prop);
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setZslAttributes
 *
 * DESCRIPTION: set ZSL related attributes from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setZslAttributes(const QCameraParameters& params)
{
    // TODO: may switch to pure param instead of sysprop
    char prop[PROPERTY_VALUE_MAX];

    const char *str = params.get(KEY_QC_ZSL_BURST_INTERVAL);
    if (str != NULL) {
        set(KEY_QC_ZSL_BURST_INTERVAL, str);
    } else {
        memset(prop, 0, sizeof(prop));
        property_get("persist.camera.zsl.interval", prop, "1");
        set(KEY_QC_ZSL_BURST_INTERVAL, prop);
        LOGH("burst interval: %s", prop);
    }

    str = params.get(KEY_QC_ZSL_BURST_LOOKBACK);
    if (str != NULL) {
        set(KEY_QC_ZSL_BURST_LOOKBACK, str);
    } else {
        memset(prop, 0, sizeof(prop));
        property_get("persist.camera.zsl.backlookcnt", prop, "2");
        uint32_t look_back_cnt = atoi(prop);
        if (m_bFrameSyncEnabled) {
            look_back_cnt += EXTRA_FRAME_SYNC_BUFFERS;
        }
        set(KEY_QC_ZSL_BURST_LOOKBACK, look_back_cnt);
        LOGH("look back count: %s", prop);
    }

    str = params.get(KEY_QC_ZSL_QUEUE_DEPTH);
    if (str != NULL) {
        set(KEY_QC_ZSL_QUEUE_DEPTH, str);
    } else {
        memset(prop, 0, sizeof(prop));
        property_get("persist.camera.zsl.queuedepth", prop, "2");
        uint32_t queue_depth = atoi(prop);
        if (m_bFrameSyncEnabled) {
            queue_depth += EXTRA_FRAME_SYNC_BUFFERS;
        }
        set(KEY_QC_ZSL_QUEUE_DEPTH, queue_depth);
        LOGH("queue depth: %s", prop);
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setFlip
 *
 * DESCRIPTION: set preview/ video/ picture flip mode from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setFlip(const QCameraParameters& params)
{
    if ((m_pCapability->qcom_supported_feature_mask & CAM_QCOM_FEATURE_FLIP) == 0) {
        LOGH("flip is not supported.");
        return NO_ERROR;
    }

    //check preview flip setting
    const char *str = params.get(KEY_QC_PREVIEW_FLIP);
    const char *prev_val = get(KEY_QC_PREVIEW_FLIP);
    if(str != NULL){
        if (prev_val == NULL || strcmp(str, prev_val) != 0) {
            int32_t value = lookupAttr(FLIP_MODES_MAP, PARAM_MAP_SIZE(FLIP_MODES_MAP), str);
            if(value != NAME_NOT_FOUND){
                set(KEY_QC_PREVIEW_FLIP, str);
                m_bPreviewFlipChanged = true;
            }
        }
    }

    // check video filp setting
    str = params.get(KEY_QC_VIDEO_FLIP);
    prev_val = get(KEY_QC_VIDEO_FLIP);
    if(str != NULL){
        if (prev_val == NULL || strcmp(str, prev_val) != 0) {
            int32_t value = lookupAttr(FLIP_MODES_MAP, PARAM_MAP_SIZE(FLIP_MODES_MAP), str);
            if(value != NAME_NOT_FOUND){
                set(KEY_QC_VIDEO_FLIP, str);
                m_bVideoFlipChanged = true;
            }
        }
    }

    // check picture filp setting
    str = params.get(KEY_QC_SNAPSHOT_PICTURE_FLIP);
    prev_val = get(KEY_QC_SNAPSHOT_PICTURE_FLIP);
    if(str != NULL){
        if (prev_val == NULL || strcmp(str, prev_val) != 0) {
            int32_t value = lookupAttr(FLIP_MODES_MAP, PARAM_MAP_SIZE(FLIP_MODES_MAP), str);
            if(value != NAME_NOT_FOUND){
                set(KEY_QC_SNAPSHOT_PICTURE_FLIP, str);
                m_bSnapshotFlipChanged = true;
            }
        }
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setSnapshotFDReq
 *
 * DESCRIPTION: set requirement of Face Detection Metadata in Snapshot mode.
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSnapshotFDReq(const QCameraParameters& params)
{
    char prop[PROPERTY_VALUE_MAX];
    const char *str = params.get(KEY_QC_SNAPSHOT_FD_DATA);

    if(str != NULL){
        set(KEY_QC_SNAPSHOT_FD_DATA, str);
    }else{
        memset(prop, 0, sizeof(prop));
        property_get("persist.camera.snapshot.fd", prop, "0");
        set(KEY_QC_SNAPSHOT_FD_DATA, prop);
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setMobicat
 *
 * DESCRIPTION: set Mobicat on/off.
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setMobicat(const QCameraParameters& )
{
    char value [PROPERTY_VALUE_MAX];
    property_get("persist.camera.mobicat", value, "0");
    int32_t ret = NO_ERROR;
    uint8_t enableMobi = (uint8_t)atoi(value);

    if (enableMobi) {
        tune_cmd_t tune_cmd;
        tune_cmd.type = 2;
        tune_cmd.module = 0;
        tune_cmd.value = 1;
        if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_SET_VFE_COMMAND, tune_cmd)) {
            return BAD_VALUE;
        }
        if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_SET_PP_COMMAND, tune_cmd)) {
            ret = BAD_VALUE;
        }
    }
    m_MobiMask = enableMobi;

    return ret;
}

/*===========================================================================
 * FUNCTION   : setLongshotParam
 *
 * DESCRIPTION: set Longshot on/off.
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setLongshotParam(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_LONG_SHOT);
    const char *prev_str = get(KEY_QC_LONG_SHOT);

    if (str != NULL) {
        if (prev_str == NULL || strcmp(str, prev_str) != 0) {
            set(KEY_QC_LONG_SHOT, str);
            if (prev_str && !strcmp(str, "off") && !strcmp(prev_str, "on")) {
                // We restart here, to reset the FPS and no
                // of buffers as per the requirement of single snapshot usecase.
                // Here restart happens when continuous shot is changed to off from on.
                // In case of continuous shot on, restart is taken care when actual
                // longshot command is triggered through sendCommand.
                m_bNeedRestart = true;
            }
        }
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : checkFeatureConcurrency
 *
 * DESCRIPTION: check if there is a feature concurrency issue with advanced
 *              camera features
 *
 * PARAMETERS : None
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::checkFeatureConcurrency()
{
    int32_t rc = NO_ERROR;
    uint32_t advancedFeatEnableBit = 0;

    if (isStillMoreEnabled()) {
        advancedFeatEnableBit |= CAM_QCOM_FEATURE_STILLMORE;
    }
    if (isHDREnabled()) {
        advancedFeatEnableBit |= CAM_QCOM_FEATURE_HDR;
    }
    if (isChromaFlashEnabled()) {
        advancedFeatEnableBit |= CAM_QCOM_FEATURE_CHROMA_FLASH;
    }
    if (isUbiFocusEnabled()) {
        advancedFeatEnableBit |= CAM_QCOM_FEATURE_UBIFOCUS;
    }
    if (isTruePortraitEnabled()) {
        advancedFeatEnableBit |= CAM_QCOM_FEATURE_TRUEPORTRAIT;
    }
    if (isOptiZoomEnabled()) {
        advancedFeatEnableBit |= CAM_QCOM_FEATURE_OPTIZOOM;
    }
    if (isUbiRefocus()) {
        advancedFeatEnableBit |= CAM_QCOM_FEATURE_REFOCUS;
    }

   if (m_bLongshotEnabled && advancedFeatEnableBit) {
        LOGE("Failed Longshot mode bit 0x%x",
                    advancedFeatEnableBit);
        rc = BAD_TYPE;
        return rc;
    }

    if(m_bRecordingHint_new) {
        advancedFeatEnableBit &= ~CAM_QCOM_FEATURE_STILLMORE;

        if (advancedFeatEnableBit) {
            LOGE("Failed recording mode bit 0x%x",
                    advancedFeatEnableBit);
            rc = BAD_TYPE;
        }
    } else if (m_bZslMode_new) {
        /* ZSL mode check if 2 bits are set */
        if (advancedFeatEnableBit & (advancedFeatEnableBit - 1)) {
            LOGE("Failed ZSL mode bit 0x%x", advancedFeatEnableBit);
            rc = BAD_TYPE;
        }
    } else { /* non-ZSL mode */
        advancedFeatEnableBit &= ~CAM_QCOM_FEATURE_HDR;

        /* non-ZSL mode check if 1 bit is set */
        if (advancedFeatEnableBit) {
            LOGE("Failed non-ZSL mode bit 0x%x", advancedFeatEnableBit);
            rc = BAD_TYPE;
        }
    }
    LOGI("Advance feature enabled 0x%x", advancedFeatEnableBit);
    return rc;
}

/*===========================================================================
 * FUNCTION   : updateParameters
 *
 * DESCRIPTION: update parameters from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *   @needRestart : [output] if preview need restart upon setting changes
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::updateParameters(const String8& p,
        bool &needRestart)
{
    int32_t final_rc = NO_ERROR;
    int32_t rc;
    m_bNeedRestart = false;
    QCameraParameters params(p);

    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        rc = BAD_TYPE;
        goto UPDATE_PARAM_DONE;
    }

    if ((rc = setPreviewSize(params)))                  final_rc = rc;
    if ((rc = setVideoSize(params)))                    final_rc = rc;
    if ((rc = setPictureSize(params)))                  final_rc = rc;
    if ((rc = setPreviewFormat(params)))                final_rc = rc;
    if ((rc = setPictureFormat(params)))                final_rc = rc;
    if ((rc = setJpegQuality(params)))                  final_rc = rc;
    if ((rc = setOrientation(params)))                  final_rc = rc;
    if ((rc = setRotation(params)))                     final_rc = rc;
    if ((rc = setVideoRotation(params)))                final_rc = rc;
    if ((rc = setNoDisplayMode(params)))                final_rc = rc;
    if ((rc = setZslMode(params)))                      final_rc = rc;
    if ((rc = setZslAttributes(params)))                final_rc = rc;
    if ((rc = setCameraMode(params)))                   final_rc = rc;
    if ((rc = setSceneSelectionMode(params)))           final_rc = rc;
    if ((rc = setRecordingHint(params)))                final_rc = rc;
    if ((rc = setRdiMode(params)))                      final_rc = rc;
    if ((rc = setSecureMode(params)))                   final_rc = rc;
    if ((rc = setPreviewFrameRate(params)))             final_rc = rc;
    if ((rc = setPreviewFpsRange(params)))              final_rc = rc;
    if ((rc = setAutoExposure(params)))                 final_rc = rc;
    if ((rc = setEffect(params)))                       final_rc = rc;
    if ((rc = setBrightness(params)))                   final_rc = rc;
    if ((rc = setZoom(params)))                         final_rc = rc;
    if ((rc = setSharpness(params)))                    final_rc = rc;
    if ((rc = setSaturation(params)))                   final_rc = rc;
    if ((rc = setContrast(params)))                     final_rc = rc;
    if ((rc = setFocusMode(params)))                    final_rc = rc;
    if ((rc = setISOValue(params)))                     final_rc = rc;
    if ((rc = setContinuousISO(params)))                final_rc = rc;
    if ((rc = setExposureTime(params)))                 final_rc = rc;
    if ((rc = setSkinToneEnhancement(params)))          final_rc = rc;
    if ((rc = setFlash(params)))                        final_rc = rc;
    if ((rc = setAecLock(params)))                      final_rc = rc;
    if ((rc = setAwbLock(params)))                      final_rc = rc;
    if ((rc = setLensShadeValue(params)))               final_rc = rc;
    if ((rc = setMCEValue(params)))                     final_rc = rc;
    if ((rc = setDISValue(params)))                     final_rc = rc;
    if ((rc = setAntibanding(params)))                  final_rc = rc;
    if ((rc = setExposureCompensation(params)))         final_rc = rc;
    if ((rc = setWhiteBalance(params)))                 final_rc = rc;
    if ((rc = setHDRMode(params)))                      final_rc = rc;
    if ((rc = setHDRNeed1x(params)))                    final_rc = rc;
    if ((rc = setManualWhiteBalance(params)))           final_rc = rc;
    if ((rc = setSceneMode(params)))                    final_rc = rc;
    if ((rc = setFocusAreas(params)))                   final_rc = rc;
    if ((rc = setFocusPosition(params)))                final_rc = rc;
    if ((rc = setMeteringAreas(params)))                final_rc = rc;
    if ((rc = setSelectableZoneAf(params)))             final_rc = rc;
    if ((rc = setRedeyeReduction(params)))              final_rc = rc;
    if ((rc = setAEBracket(params)))                    final_rc = rc;
    if ((rc = setAutoHDR(params)))                      final_rc = rc;
    if ((rc = setGpsLocation(params)))                  final_rc = rc;
    if ((rc = setWaveletDenoise(params)))               final_rc = rc;
    if ((rc = setFaceRecognition(params)))              final_rc = rc;
    if ((rc = setFlip(params)))                         final_rc = rc;
    if ((rc = setVideoHDR(params)))                     final_rc = rc;
    if ((rc = setVtEnable(params)))                     final_rc = rc;
    if ((rc = setAFBracket(params)))                    final_rc = rc;
    if ((rc = setReFocus(params)))                      final_rc = rc;
    if ((rc = setChromaFlash(params)))                  final_rc = rc;
    if ((rc = setTruePortrait(params)))                 final_rc = rc;
    if ((rc = setOptiZoom(params)))                     final_rc = rc;
    if ((rc = setBurstLEDOnPeriod(params)))             final_rc = rc;
    if ((rc = setRetroActiveBurstNum(params)))          final_rc = rc;
    if ((rc = setSnapshotFDReq(params)))                final_rc = rc;
    if ((rc = setTintlessValue(params)))                final_rc = rc;
    if ((rc = setCDSMode(params)))                      final_rc = rc;
    if ((rc = setTemporalDenoise(params)))              final_rc = rc;
    if ((rc = setCacheVideoBuffers(params)))            final_rc = rc;
    if ((rc = setInitialExposureIndex(params)))         final_rc = rc;
    if ((rc = setInstantCapture(params)))               final_rc = rc;
    if ((rc = setInstantAEC(params)))                   final_rc = rc;

    // update live snapshot size after all other parameters are set
    if ((rc = setLiveSnapshotSize(params)))             final_rc = rc;
    if ((rc = setJpegThumbnailSize(params)))            final_rc = rc;
    if ((rc = setStatsDebugMask()))                     final_rc = rc;
    if ((rc = setPAAF()))                               final_rc = rc;
    if ((rc = setMobicat(params)))                      final_rc = rc;
    if ((rc = setSeeMore(params)))                      final_rc = rc;
    if ((rc = setStillMore(params)))                    final_rc = rc;
    if ((rc = setCustomParams(params)))                 final_rc = rc;
    if ((rc = setNoiseReductionMode(params)))           final_rc = rc;

    if ((rc = setLongshotParam(params)))                final_rc = rc;
    if ((rc = setLedCalibration(params)))               final_rc = rc;

    setQuadraCfa(params);
    setVideoBatchSize();
    setLowLightCapture();

    if ((rc = updateFlash(false)))                      final_rc = rc;
#ifdef TARGET_TS_MAKEUP
    if ((rc = setTsMakeup(params)))                     final_rc = rc;
#endif
    if ((rc = setAdvancedCaptureMode()))                final_rc = rc;
UPDATE_PARAM_DONE:
    needRestart = m_bNeedRestart;
    return final_rc;
}

/*===========================================================================
 * FUNCTION   : commitParameters
 *
 * DESCRIPTION: commit parameter changes to backend
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::commitParameters()
{
    return commitSetBatch();
}

/*===========================================================================
 * FUNCTION   : initDefaultParameters
 *
 * DESCRIPTION: initialize default parameters for the first time
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::initDefaultParameters()
{
    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }
    int32_t hal_version = CAM_HAL_V1;
    ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_HAL_VERSION, hal_version);

    /*************************Initialize Values******************************/
    // Set read only parameters from camera capability
    set(KEY_SMOOTH_ZOOM_SUPPORTED,
        m_pCapability->smooth_zoom_supported? VALUE_TRUE : VALUE_FALSE);
    set(KEY_ZOOM_SUPPORTED,
        m_pCapability->zoom_supported? VALUE_TRUE : VALUE_FALSE);
    set(KEY_VIDEO_SNAPSHOT_SUPPORTED,
        m_pCapability->video_snapshot_supported? VALUE_TRUE : VALUE_FALSE);
    set(KEY_VIDEO_STABILIZATION_SUPPORTED,
        m_pCapability->video_stablization_supported? VALUE_TRUE : VALUE_FALSE);
    set(KEY_AUTO_EXPOSURE_LOCK_SUPPORTED,
        m_pCapability->auto_exposure_lock_supported? VALUE_TRUE : VALUE_FALSE);
    set(KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED,
        m_pCapability->auto_wb_lock_supported? VALUE_TRUE : VALUE_FALSE);
    set(KEY_MAX_NUM_DETECTED_FACES_HW, m_pCapability->max_num_roi);
    set(KEY_MAX_NUM_DETECTED_FACES_SW, m_pCapability->max_num_roi);
    set(KEY_QC_MAX_NUM_REQUESTED_FACES, m_pCapability->max_num_roi);
    // Set focal length, horizontal view angle, and vertical view angle
    setFloat(KEY_FOCAL_LENGTH, m_pCapability->focal_length);
    setFloat(KEY_HORIZONTAL_VIEW_ANGLE, m_pCapability->hor_view_angle);
    setFloat(KEY_VERTICAL_VIEW_ANGLE, m_pCapability->ver_view_angle);
    set(QCameraParameters::KEY_FOCUS_DISTANCES, "Infinity,Infinity,Infinity");
    set(KEY_QC_AUTO_HDR_SUPPORTED,
        (m_pCapability->auto_hdr_supported)? VALUE_TRUE : VALUE_FALSE);
    // Set supported preview sizes
    if (m_pCapability->preview_sizes_tbl_cnt > 0 &&
        m_pCapability->preview_sizes_tbl_cnt <= MAX_SIZES_CNT) {
        String8 previewSizeValues = createSizesString(
                m_pCapability->preview_sizes_tbl, m_pCapability->preview_sizes_tbl_cnt);
        set(KEY_SUPPORTED_PREVIEW_SIZES, previewSizeValues.string());
        LOGH("supported preview sizes: %s", previewSizeValues.string());
        // Set default preview size
        CameraParameters::setPreviewSize(m_pCapability->preview_sizes_tbl[0].width,
                                         m_pCapability->preview_sizes_tbl[0].height);
    } else {
        LOGW("supported preview sizes cnt is 0 or exceeds max!!!");
    }

    // Set supported video sizes
    if (m_pCapability->video_sizes_tbl_cnt > 0 &&
        m_pCapability->video_sizes_tbl_cnt <= MAX_SIZES_CNT) {
        String8 videoSizeValues = createSizesString(
                m_pCapability->video_sizes_tbl, m_pCapability->video_sizes_tbl_cnt);
        set(KEY_SUPPORTED_VIDEO_SIZES, videoSizeValues.string());
        LOGH("supported video sizes: %s", videoSizeValues.string());
        // Set default video size
        CameraParameters::setVideoSize(m_pCapability->video_sizes_tbl[0].width,
                                       m_pCapability->video_sizes_tbl[0].height);

        //Set preferred Preview size for video
        String8 vSize = createSizesString(&m_pCapability->preview_sizes_tbl[0], 1);
        set(KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, vSize.string());
    } else {
        LOGW("supported video sizes cnt is 0 or exceeds max!!!");
    }

    // Set supported picture sizes
    if (m_pCapability->picture_sizes_tbl_cnt > 0 &&
        m_pCapability->picture_sizes_tbl_cnt <= MAX_SIZES_CNT) {
        String8 pictureSizeValues = createSizesString(
                m_pCapability->picture_sizes_tbl, m_pCapability->picture_sizes_tbl_cnt);
        set(KEY_SUPPORTED_PICTURE_SIZES, pictureSizeValues.string());
        LOGH("supported pic sizes: %s", pictureSizeValues.string());
        // Set default picture size to the smallest resolution
        CameraParameters::setPictureSize(
           m_pCapability->picture_sizes_tbl[m_pCapability->picture_sizes_tbl_cnt-1].width,
           m_pCapability->picture_sizes_tbl[m_pCapability->picture_sizes_tbl_cnt-1].height);
    } else {
        LOGW("supported picture sizes cnt is 0 or exceeds max!!!");
    }

    // Need check if scale should be enabled
    if (m_pCapability->scale_picture_sizes_cnt > 0 &&
        m_pCapability->scale_picture_sizes_cnt <= MAX_SCALE_SIZES_CNT){
        //get scale size, enable scaling. And re-set picture size table with scale sizes
        m_reprocScaleParam.setScaleEnable(true);
        int rc_s = m_reprocScaleParam.setScaleSizeTbl(
            m_pCapability->scale_picture_sizes_cnt, m_pCapability->scale_picture_sizes,
            m_pCapability->picture_sizes_tbl_cnt, m_pCapability->picture_sizes_tbl);
        if(rc_s == NO_ERROR){
            cam_dimension_t *totalSizeTbl = m_reprocScaleParam.getTotalSizeTbl();
            size_t totalSizeCnt = m_reprocScaleParam.getTotalSizeTblCnt();
            String8 pictureSizeValues = createSizesString(totalSizeTbl, totalSizeCnt);
            set(KEY_SUPPORTED_PICTURE_SIZES, pictureSizeValues.string());
            LOGH("scaled supported pic sizes: %s", pictureSizeValues.string());
        }else{
            m_reprocScaleParam.setScaleEnable(false);
            LOGW("reset scaled picture size table failed.");
        }
    }else{
        m_reprocScaleParam.setScaleEnable(false);
    }

    // Set supported thumbnail sizes
    String8 thumbnailSizeValues = createSizesString(
            THUMBNAIL_SIZES_MAP,
            PARAM_MAP_SIZE(THUMBNAIL_SIZES_MAP));
    set(KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES, thumbnailSizeValues.string());
    // Set default thumnail size
    set(KEY_JPEG_THUMBNAIL_WIDTH, THUMBNAIL_SIZES_MAP[0].width);
    set(KEY_JPEG_THUMBNAIL_HEIGHT, THUMBNAIL_SIZES_MAP[0].height);

    // Set supported livesnapshot sizes
    if (m_pCapability->livesnapshot_sizes_tbl_cnt > 0 &&
        m_pCapability->livesnapshot_sizes_tbl_cnt <= MAX_SIZES_CNT) {
        String8 liveSnpashotSizeValues = createSizesString(
                m_pCapability->livesnapshot_sizes_tbl,
                m_pCapability->livesnapshot_sizes_tbl_cnt);
        set(KEY_QC_SUPPORTED_LIVESNAPSHOT_SIZES, liveSnpashotSizeValues.string());
        LOGD("supported live snapshot sizes: %s", liveSnpashotSizeValues.string());
        m_LiveSnapshotSize =
            m_pCapability->livesnapshot_sizes_tbl[m_pCapability->livesnapshot_sizes_tbl_cnt-1];
    }

    // Set supported preview formats
    String8 previewFormatValues = createValuesString(
            m_pCapability->supported_preview_fmts,
            m_pCapability->supported_preview_fmt_cnt,
            PREVIEW_FORMATS_MAP,
            PARAM_MAP_SIZE(PREVIEW_FORMATS_MAP));
    set(KEY_SUPPORTED_PREVIEW_FORMATS, previewFormatValues.string());
    // Set default preview format
    CameraParameters::setPreviewFormat(PIXEL_FORMAT_YUV420SP);

    // Set default Video Format as OPAQUE
    // Internally both Video and Camera subsystems use NV21_VENUS
    set(KEY_VIDEO_FRAME_FORMAT, PIXEL_FORMAT_ANDROID_OPAQUE);

    // Set supported picture formats
    String8 pictureTypeValues(PIXEL_FORMAT_JPEG);
    String8 str = createValuesString(
            m_pCapability->supported_raw_fmts,
            m_pCapability->supported_raw_fmt_cnt,
            PICTURE_TYPES_MAP,
            PARAM_MAP_SIZE(PICTURE_TYPES_MAP));
    if (str.string() != NULL) {
        pictureTypeValues.append(",");
        pictureTypeValues.append(str);
    }

    set(KEY_SUPPORTED_PICTURE_FORMATS, pictureTypeValues.string());
    // Set default picture Format
    CameraParameters::setPictureFormat(PIXEL_FORMAT_JPEG);
    // Set raw image size
    char raw_size_str[32];
    snprintf(raw_size_str, sizeof(raw_size_str), "%dx%d",
             m_pCapability->raw_dim[0].width, m_pCapability->raw_dim[0].height);
    set(KEY_QC_RAW_PICUTRE_SIZE, raw_size_str);
    LOGD("KEY_QC_RAW_PICUTRE_SIZE: w: %d, h: %d ",
       m_pCapability->raw_dim[0].width, m_pCapability->raw_dim[0].height);

    //set default jpeg quality and thumbnail quality
    set(KEY_JPEG_QUALITY, 85);
    set(KEY_JPEG_THUMBNAIL_QUALITY, 85);

    // Set FPS ranges
    if (m_pCapability->fps_ranges_tbl_cnt > 0 &&
        m_pCapability->fps_ranges_tbl_cnt <= MAX_SIZES_CNT) {
        int default_fps_index = 0;
        String8 fpsRangeValues = createFpsRangeString(m_pCapability->fps_ranges_tbl,
                                                      m_pCapability->fps_ranges_tbl_cnt,
                                                      default_fps_index);
        set(KEY_SUPPORTED_PREVIEW_FPS_RANGE, fpsRangeValues.string());

        int min_fps =
            int(m_pCapability->fps_ranges_tbl[default_fps_index].min_fps * 1000);
        int max_fps =
            int(m_pCapability->fps_ranges_tbl[default_fps_index].max_fps * 1000);
        m_default_fps_range = m_pCapability->fps_ranges_tbl[default_fps_index];
        //Set video fps same as preview fps
        setPreviewFpsRange(min_fps, max_fps, min_fps, max_fps);

        // Set legacy preview fps
        String8 fpsValues = createFpsString(m_pCapability->fps_ranges_tbl[default_fps_index]);
        set(KEY_SUPPORTED_PREVIEW_FRAME_RATES, fpsValues.string());
        LOGH("supported fps rates: %s", fpsValues.string());
        CameraParameters::setPreviewFrameRate(int(m_pCapability->fps_ranges_tbl[default_fps_index].max_fps));
    } else {
        LOGW("supported fps ranges cnt is 0 or exceeds max!!!");
    }

    // Set supported focus modes
    if (m_pCapability->supported_focus_modes_cnt > 0) {
        String8 focusModeValues = createValuesString(
                m_pCapability->supported_focus_modes,
                m_pCapability->supported_focus_modes_cnt,
                FOCUS_MODES_MAP,
                PARAM_MAP_SIZE(FOCUS_MODES_MAP));
        set(KEY_SUPPORTED_FOCUS_MODES, focusModeValues);

        // Set default focus mode and update corresponding parameter buf
        const char *focusMode = lookupNameByValue(FOCUS_MODES_MAP,
                PARAM_MAP_SIZE(FOCUS_MODES_MAP),
                m_pCapability->supported_focus_modes[0]);
        if (focusMode != NULL) {
            setFocusMode(focusMode);
        } else {
            setFocusMode(FOCUS_MODE_FIXED);
        }
    } else {
        LOGW("supported focus modes cnt is 0!!!");
    }

    // Set focus areas
    if (m_pCapability->max_num_focus_areas > MAX_ROI) {
        m_pCapability->max_num_focus_areas = MAX_ROI;
    }
    set(KEY_MAX_NUM_FOCUS_AREAS, m_pCapability->max_num_focus_areas);
    if (m_pCapability->max_num_focus_areas > 0) {
        setFocusAreas(DEFAULT_CAMERA_AREA);
    }

    // Set metering areas
    if (m_pCapability->max_num_metering_areas > MAX_ROI) {
        m_pCapability->max_num_metering_areas = MAX_ROI;
    }
    set(KEY_MAX_NUM_METERING_AREAS, m_pCapability->max_num_metering_areas);
    if (m_pCapability->max_num_metering_areas > 0) {
        setMeteringAreas(DEFAULT_CAMERA_AREA);
    }

    // set focus position, we should get them from m_pCapability
    m_pCapability->min_focus_pos[CAM_MANUAL_FOCUS_MODE_INDEX] = 0;
    m_pCapability->max_focus_pos[CAM_MANUAL_FOCUS_MODE_INDEX] = 1023;
    set(KEY_QC_MIN_FOCUS_POS_INDEX,
            (int) m_pCapability->min_focus_pos[CAM_MANUAL_FOCUS_MODE_INDEX]);
    set(KEY_QC_MAX_FOCUS_POS_INDEX,
            (int) m_pCapability->max_focus_pos[CAM_MANUAL_FOCUS_MODE_INDEX]);

    m_pCapability->min_focus_pos[CAM_MANUAL_FOCUS_MODE_DAC_CODE] = 0;
    m_pCapability->max_focus_pos[CAM_MANUAL_FOCUS_MODE_DAC_CODE] = 1023;
    set(KEY_QC_MIN_FOCUS_POS_DAC,
            (int) m_pCapability->min_focus_pos[CAM_MANUAL_FOCUS_MODE_DAC_CODE]);
    set(KEY_QC_MAX_FOCUS_POS_DAC,
            (int) m_pCapability->max_focus_pos[CAM_MANUAL_FOCUS_MODE_DAC_CODE]);

    m_pCapability->min_focus_pos[CAM_MANUAL_FOCUS_MODE_RATIO] = 0;
    m_pCapability->max_focus_pos[CAM_MANUAL_FOCUS_MODE_RATIO] = 100;
    set(KEY_QC_MIN_FOCUS_POS_RATIO,
            (int) m_pCapability->min_focus_pos[CAM_MANUAL_FOCUS_MODE_RATIO]);
    set(KEY_QC_MAX_FOCUS_POS_RATIO,
            (int) m_pCapability->max_focus_pos[CAM_MANUAL_FOCUS_MODE_RATIO]);

    m_pCapability->min_focus_pos[CAM_MANUAL_FOCUS_MODE_DIOPTER] = 0;
    if (m_pCapability->min_focus_distance > 0) {
        m_pCapability->max_focus_pos[CAM_MANUAL_FOCUS_MODE_DIOPTER] =
                m_pCapability->min_focus_distance;
    } else {
        m_pCapability->max_focus_pos[CAM_MANUAL_FOCUS_MODE_DIOPTER] = 0;
    }
    setFloat(KEY_QC_MIN_FOCUS_POS_DIOPTER,
            m_pCapability->min_focus_pos[CAM_MANUAL_FOCUS_MODE_DIOPTER]);
    setFloat(KEY_QC_MAX_FOCUS_POS_DIOPTER,
            m_pCapability->max_focus_pos[CAM_MANUAL_FOCUS_MODE_DIOPTER]);

    //set supported manual focus modes
    String8 manualFocusModes(VALUE_OFF);
    if (m_pCapability->supported_focus_modes_cnt > 1 &&
        m_pCapability->min_focus_distance > 0) {
        manualFocusModes.append(",");
        manualFocusModes.append(KEY_QC_FOCUS_SCALE_MODE);
        manualFocusModes.append(",");
        manualFocusModes.append(KEY_QC_FOCUS_DIOPTER_MODE);
    }
    set(KEY_QC_SUPPORTED_MANUAL_FOCUS_MODES, manualFocusModes.string());

    // Set Saturation
    set(KEY_QC_MIN_SATURATION, m_pCapability->saturation_ctrl.min_value);
    set(KEY_QC_MAX_SATURATION, m_pCapability->saturation_ctrl.max_value);
    set(KEY_QC_SATURATION_STEP, m_pCapability->saturation_ctrl.step);
    setSaturation(m_pCapability->saturation_ctrl.def_value);

    // Set Sharpness
    set(KEY_QC_MIN_SHARPNESS, m_pCapability->sharpness_ctrl.min_value);
    set(KEY_QC_MAX_SHARPNESS, m_pCapability->sharpness_ctrl.max_value);
    set(KEY_QC_SHARPNESS_STEP, m_pCapability->sharpness_ctrl.step);
    setSharpness(m_pCapability->sharpness_ctrl.def_value);

    // Set Contrast
    set(KEY_QC_MIN_CONTRAST, m_pCapability->contrast_ctrl.min_value);
    set(KEY_QC_MAX_CONTRAST, m_pCapability->contrast_ctrl.max_value);
    set(KEY_QC_CONTRAST_STEP, m_pCapability->contrast_ctrl.step);
    setContrast(m_pCapability->contrast_ctrl.def_value);

    // Set SCE factor
    set(KEY_QC_MIN_SCE_FACTOR, m_pCapability->sce_ctrl.min_value); // -100
    set(KEY_QC_MAX_SCE_FACTOR, m_pCapability->sce_ctrl.max_value); // 100
    set(KEY_QC_SCE_FACTOR_STEP, m_pCapability->sce_ctrl.step);     // 10
    setSkinToneEnhancement(m_pCapability->sce_ctrl.def_value);     // 0

    // Set Brightness
    set(KEY_QC_MIN_BRIGHTNESS, m_pCapability->brightness_ctrl.min_value); // 0
    set(KEY_QC_MAX_BRIGHTNESS, m_pCapability->brightness_ctrl.max_value); // 6
    set(KEY_QC_BRIGHTNESS_STEP, m_pCapability->brightness_ctrl.step);     // 1
    setBrightness(m_pCapability->brightness_ctrl.def_value);

    // Set Auto exposure
    String8 autoExposureValues = createValuesString(
            m_pCapability->supported_aec_modes,
            m_pCapability->supported_aec_modes_cnt,
            AUTO_EXPOSURE_MAP,
            PARAM_MAP_SIZE(AUTO_EXPOSURE_MAP));
    set(KEY_QC_SUPPORTED_AUTO_EXPOSURE, autoExposureValues.string());
    setAutoExposure(AUTO_EXPOSURE_FRAME_AVG);

    // Set Exposure Compensation
    set(KEY_MAX_EXPOSURE_COMPENSATION, m_pCapability->exposure_compensation_max); // 12
    set(KEY_MIN_EXPOSURE_COMPENSATION, m_pCapability->exposure_compensation_min); // -12
    setFloat(KEY_EXPOSURE_COMPENSATION_STEP, m_pCapability->exposure_compensation_step); // 1/6
    setExposureCompensation(m_pCapability->exposure_compensation_default); // 0

    // Set Instant AEC modes
    String8 instantAECModes = createValuesString(
            m_pCapability->supported_instant_aec_modes,
            m_pCapability->supported_instant_aec_modes_cnt,
            INSTANT_AEC_MODES_MAP,
            PARAM_MAP_SIZE(INSTANT_AEC_MODES_MAP));
    set(KEY_QC_INSTANT_AEC_SUPPORTED_MODES, instantAECModes.string());

    // Set Instant Capture modes
    String8 instantCaptureModes = createValuesString(
            m_pCapability->supported_instant_aec_modes,
            m_pCapability->supported_instant_aec_modes_cnt,
            INSTANT_CAPTURE_MODES_MAP,
            PARAM_MAP_SIZE(INSTANT_CAPTURE_MODES_MAP));
    set(KEY_QC_INSTANT_CAPTURE_SUPPORTED_MODES, instantCaptureModes.string());


    // Set Antibanding
    String8 antibandingValues = createValuesString(
            m_pCapability->supported_antibandings,
            m_pCapability->supported_antibandings_cnt,
            ANTIBANDING_MODES_MAP,
            PARAM_MAP_SIZE(ANTIBANDING_MODES_MAP));
    set(KEY_SUPPORTED_ANTIBANDING, antibandingValues);
    setAntibanding(ANTIBANDING_OFF);

    // Set Effect
    String8 effectValues = createValuesString(
            m_pCapability->supported_effects,
            m_pCapability->supported_effects_cnt,
            EFFECT_MODES_MAP,
            PARAM_MAP_SIZE(EFFECT_MODES_MAP));

    if (m_pCapability->supported_effects_cnt > 0) {
        set(KEY_SUPPORTED_EFFECTS, effectValues);
    } else {
        LOGW("Color effects are not available");
        set(KEY_SUPPORTED_EFFECTS, EFFECT_NONE);
    }
    setEffect(EFFECT_NONE);

    // Set WhiteBalance
    String8 whitebalanceValues = createValuesString(
            m_pCapability->supported_white_balances,
            m_pCapability->supported_white_balances_cnt,
            WHITE_BALANCE_MODES_MAP,
            PARAM_MAP_SIZE(WHITE_BALANCE_MODES_MAP));
    set(KEY_SUPPORTED_WHITE_BALANCE, whitebalanceValues);
    setWhiteBalance(WHITE_BALANCE_AUTO);

    // set supported wb cct, we should get them from m_pCapability
    m_pCapability->min_wb_cct = 2000;
    m_pCapability->max_wb_cct = 8000;
    set(KEY_QC_MIN_WB_CCT, m_pCapability->min_wb_cct);
    set(KEY_QC_MAX_WB_CCT, m_pCapability->max_wb_cct);

    // set supported wb rgb gains, ideally we should get them from m_pCapability
    //but for now hardcode.
    m_pCapability->min_wb_gain = 1.0;
    m_pCapability->max_wb_gain = 4.0;
    setFloat(KEY_QC_MIN_WB_GAIN, m_pCapability->min_wb_gain);
    setFloat(KEY_QC_MAX_WB_GAIN, m_pCapability->max_wb_gain);

    //set supported manual wb modes
    String8 manualWBModes(VALUE_OFF);
    if(m_pCapability->sensor_type.sens_type != CAM_SENSOR_YUV) {
        manualWBModes.append(",");
        manualWBModes.append(KEY_QC_WB_CCT_MODE);
        manualWBModes.append(",");
        manualWBModes.append(KEY_QC_WB_GAIN_MODE);
    }
    set(KEY_QC_SUPPORTED_MANUAL_WB_MODES, manualWBModes.string());

    // Set Flash mode
    if(m_pCapability->supported_flash_modes_cnt > 0) {
       String8 flashValues = createValuesString(
               m_pCapability->supported_flash_modes,
               m_pCapability->supported_flash_modes_cnt,
               FLASH_MODES_MAP,
               PARAM_MAP_SIZE(FLASH_MODES_MAP));
       set(KEY_SUPPORTED_FLASH_MODES, flashValues);
       setFlash(FLASH_MODE_OFF);
    } else {
        LOGW("supported flash modes cnt is 0!!!");
    }

    // Set Scene Mode
    String8 sceneModeValues = createValuesString(
            m_pCapability->supported_scene_modes,
            m_pCapability->supported_scene_modes_cnt,
            SCENE_MODES_MAP,
            PARAM_MAP_SIZE(SCENE_MODES_MAP));
    set(KEY_SUPPORTED_SCENE_MODES, sceneModeValues);
    setSceneMode(SCENE_MODE_AUTO);

#if 0
    // Set CDS Mode
    String8 cdsModeValues = createValuesStringFromMap(
            CDS_MODES_MAP,
            PARAM_MAP_SIZE(CDS_MODES_MAP));
    set(KEY_QC_SUPPORTED_CDS_MODES, cdsModeValues);
#endif

    // Set video CDS Mode
    String8 videoCdsModeValues = createValuesStringFromMap(
            CDS_MODES_MAP,
            PARAM_MAP_SIZE(CDS_MODES_MAP));
    set(KEY_QC_SUPPORTED_VIDEO_CDS_MODES, videoCdsModeValues);

    // Set TNR Mode
    String8 tnrModeValues = createValuesStringFromMap(
            ON_OFF_MODES_MAP,
            PARAM_MAP_SIZE(ON_OFF_MODES_MAP));
    set(KEY_QC_SUPPORTED_TNR_MODES, tnrModeValues);

    // Set video TNR Mode
    String8 videoTnrModeValues = createValuesStringFromMap(
            ON_OFF_MODES_MAP,
            PARAM_MAP_SIZE(ON_OFF_MODES_MAP));
    set(KEY_QC_SUPPORTED_VIDEO_TNR_MODES, videoTnrModeValues);

    // Set ISO Mode
    String8 isoValues = createValuesString(
            m_pCapability->supported_iso_modes,
            m_pCapability->supported_iso_modes_cnt,
            ISO_MODES_MAP,
            PARAM_MAP_SIZE(ISO_MODES_MAP));
    set(KEY_QC_SUPPORTED_ISO_MODES, isoValues);
    setISOValue(ISO_AUTO);

    // Set exposure time
    String8 manualExpModes(VALUE_OFF);
    bool expTimeSupported = false;
    bool manualISOSupported = false;
    //capability values are in nano sec, convert to milli sec for upper layers
    char expTimeStr[20];
    double min_exp_time = (double) m_pCapability->exposure_time_range[0] / 1000000.0;
    double max_exp_time = (double) m_pCapability->exposure_time_range[1] / 1000000.0;
    snprintf(expTimeStr, sizeof(expTimeStr), "%f", min_exp_time);
    set(KEY_QC_MIN_EXPOSURE_TIME, expTimeStr);
    snprintf(expTimeStr, sizeof(expTimeStr), "%f", max_exp_time);
    set(KEY_QC_MAX_EXPOSURE_TIME, expTimeStr);
    if ((min_exp_time > 0) && (max_exp_time > min_exp_time)) {
        manualExpModes.append(",");
        manualExpModes.append(KEY_QC_EXP_TIME_PRIORITY);
        expTimeSupported = true;
    }
    LOGH(", Exposure time min %f ms, max %f ms",
            min_exp_time, max_exp_time);

    // Set iso
    set(KEY_QC_MIN_ISO, m_pCapability->sensitivity_range.min_sensitivity);
    set(KEY_QC_MAX_ISO, m_pCapability->sensitivity_range.max_sensitivity);
    LOGH(", ISO min %d, max %d",
            m_pCapability->sensitivity_range.min_sensitivity,
            m_pCapability->sensitivity_range.max_sensitivity);
    if ((m_pCapability->sensitivity_range.min_sensitivity > 0) &&
            (m_pCapability->sensitivity_range.max_sensitivity >
                    m_pCapability->sensitivity_range.min_sensitivity)) {
        manualExpModes.append(",");
        manualExpModes.append(KEY_QC_ISO_PRIORITY);
        manualISOSupported = true;
    }
    if (expTimeSupported && manualISOSupported) {
        manualExpModes.append(",");
        manualExpModes.append(KEY_QC_USER_SETTING);
    }
    //finally set supported manual exposure modes
    set(KEY_QC_SUPPORTED_MANUAL_EXPOSURE_MODES, manualExpModes.string());

    // Set HFR
    String8 hfrValues = createHfrValuesString(
            m_pCapability->hfr_tbl,
            m_pCapability->hfr_tbl_cnt,
            HFR_MODES_MAP,
            PARAM_MAP_SIZE(HFR_MODES_MAP));
    set(KEY_QC_SUPPORTED_VIDEO_HIGH_FRAME_RATE_MODES, hfrValues.string());
    set(KEY_QC_VIDEO_HIGH_SPEED_RECORDING, "off");
    set(KEY_QC_VIDEO_HIGH_FRAME_RATE, "off");
    String8 hfrSizeValues = createHfrSizesString(
            m_pCapability->hfr_tbl,
            m_pCapability->hfr_tbl_cnt);
    set(KEY_QC_SUPPORTED_HFR_SIZES, hfrSizeValues.string());
    LOGD("HFR values = %s HFR Sizes = %s", hfrValues.string(), hfrSizeValues.string());
    setHighFrameRate(CAM_HFR_MODE_OFF);

    // Set Focus algorithms
    String8 focusAlgoValues = createValuesString(
            m_pCapability->supported_focus_algos,
            m_pCapability->supported_focus_algos_cnt,
            FOCUS_ALGO_MAP,
            PARAM_MAP_SIZE(FOCUS_ALGO_MAP));
    set(KEY_QC_SUPPORTED_FOCUS_ALGOS, focusAlgoValues);
    setSelectableZoneAf(FOCUS_ALGO_AUTO);

    // Set Zoom Ratios
    if (m_pCapability->zoom_supported > 0) {
        String8 zoomRatioValues = createZoomRatioValuesString(
                m_pCapability->zoom_ratio_tbl,
                m_pCapability->zoom_ratio_tbl_cnt);
        set(KEY_ZOOM_RATIOS, zoomRatioValues);
        set(KEY_MAX_ZOOM, (int)(m_pCapability->zoom_ratio_tbl_cnt - 1));
        setZoom(0);
    }

    // Set Bracketing/HDR
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.capture.burst.exposures", prop, "");
    if (strlen(prop) > 0) {
        set(KEY_QC_CAPTURE_BURST_EXPOSURE, prop);
    }
    String8 bracketingValues = createValuesStringFromMap(
            BRACKETING_MODES_MAP,
            PARAM_MAP_SIZE(BRACKETING_MODES_MAP));
    set(KEY_QC_SUPPORTED_AE_BRACKET_MODES, bracketingValues);
    setAEBracket(AE_BRACKET_OFF);

    //Set AF Bracketing.
    for (size_t i = 0; i < m_pCapability->supported_focus_modes_cnt; i++) {
        if ((CAM_FOCUS_MODE_AUTO == m_pCapability->supported_focus_modes[i]) &&
                ((m_pCapability->qcom_supported_feature_mask &
                        CAM_QCOM_FEATURE_UBIFOCUS) > 0)) {
            String8 afBracketingValues = createValuesStringFromMap(
                    AF_BRACKETING_MODES_MAP,
                    PARAM_MAP_SIZE(AF_BRACKETING_MODES_MAP));
            set(KEY_QC_SUPPORTED_AF_BRACKET_MODES, afBracketingValues);
            setAFBracket(AF_BRACKET_OFF);
            break;
         }
    }

    //Set Refocus.
    //Re-use ubifocus flag for now.
    for (size_t i = 0; i < m_pCapability->supported_focus_modes_cnt; i++) {
        if ((CAM_FOCUS_MODE_AUTO == m_pCapability->supported_focus_modes[i]) &&
                (m_pCapability->qcom_supported_feature_mask &
                    CAM_QCOM_FEATURE_REFOCUS) > 0) {
            String8 reFocusValues = createValuesStringFromMap(
                    RE_FOCUS_MODES_MAP,
                    PARAM_MAP_SIZE(RE_FOCUS_MODES_MAP));
            set(KEY_QC_SUPPORTED_RE_FOCUS_MODES, reFocusValues);
            setReFocus(RE_FOCUS_OFF);
        }
    }

    //Set Chroma Flash.
    if ((m_pCapability->supported_flash_modes_cnt > 0) &&
            (m_pCapability->qcom_supported_feature_mask &
            CAM_QCOM_FEATURE_CHROMA_FLASH) > 0) {
        String8 chromaFlashValues = createValuesStringFromMap(
                CHROMA_FLASH_MODES_MAP,
                PARAM_MAP_SIZE(CHROMA_FLASH_MODES_MAP));
        set(KEY_QC_SUPPORTED_CHROMA_FLASH_MODES, chromaFlashValues);
        setChromaFlash(CHROMA_FLASH_OFF);
    }

    //Set Opti Zoom.
    if (m_pCapability->zoom_supported &&
            (m_pCapability->qcom_supported_feature_mask &
            CAM_QCOM_FEATURE_OPTIZOOM) > 0){
        String8 optiZoomValues = createValuesStringFromMap(
                OPTI_ZOOM_MODES_MAP,
                PARAM_MAP_SIZE(OPTI_ZOOM_MODES_MAP));
        set(KEY_QC_SUPPORTED_OPTI_ZOOM_MODES, optiZoomValues);
        setOptiZoom(OPTI_ZOOM_OFF);
    }

    //Set HDR Type
    uint32_t supported_hdr_modes = m_pCapability->qcom_supported_feature_mask &
            (CAM_QCOM_FEATURE_SENSOR_HDR | CAM_QCOM_FEATURE_HDR);
    if (supported_hdr_modes) {
        if (CAM_QCOM_FEATURE_SENSOR_HDR == supported_hdr_modes) {
            String8 hdrModeValues;
            hdrModeValues.append(HDR_MODE_SENSOR);
            set(KEY_QC_SUPPORTED_KEY_QC_HDR_MODES, hdrModeValues);
            setHDRMode(HDR_MODE_SENSOR);
        } else if (CAM_QCOM_FEATURE_HDR == supported_hdr_modes) {
            String8 hdrModeValues;
            hdrModeValues.append(HDR_MODE_MULTI_FRAME);
            set(KEY_QC_SUPPORTED_KEY_QC_HDR_MODES, hdrModeValues);
            setHDRMode(HDR_MODE_MULTI_FRAME);
        } else {
            String8 hdrModeValues = createValuesStringFromMap(
                    HDR_MODES_MAP,
                    PARAM_MAP_SIZE(HDR_MODES_MAP));
            set(KEY_QC_SUPPORTED_KEY_QC_HDR_MODES, hdrModeValues);
            setHDRMode(HDR_MODE_MULTI_FRAME);
        }
    }

    //Set HDR need 1x
    String8 hdrNeed1xValues;
    if (!m_bHDRModeSensor) {
        hdrNeed1xValues = createValuesStringFromMap(TRUE_FALSE_MODES_MAP,
                PARAM_MAP_SIZE(TRUE_FALSE_MODES_MAP));
    } else {
        hdrNeed1xValues.append(VALUE_FALSE);
    }
    setHDRNeed1x(VALUE_FALSE);
    set(KEY_QC_SUPPORTED_HDR_NEED_1X, hdrNeed1xValues);

    //Set True Portrait
    if ((m_pCapability->qcom_supported_feature_mask & CAM_QCOM_FEATURE_TRUEPORTRAIT) > 0) {
        String8 truePortraitValues = createValuesStringFromMap(
                TRUE_PORTRAIT_MODES_MAP,
                PARAM_MAP_SIZE(TRUE_PORTRAIT_MODES_MAP));
        set(KEY_QC_SUPPORTED_TRUE_PORTRAIT_MODES, truePortraitValues);
    }

    // Set Denoise
    if ((m_pCapability->qcom_supported_feature_mask & CAM_QCOM_FEATURE_DENOISE2D) > 0){
    String8 denoiseValues = createValuesStringFromMap(
        DENOISE_ON_OFF_MODES_MAP, PARAM_MAP_SIZE(DENOISE_ON_OFF_MODES_MAP));
    set(KEY_QC_SUPPORTED_DENOISE, denoiseValues.string());
#ifdef DEFAULT_DENOISE_MODE_ON
    setWaveletDenoise(DENOISE_ON);
#else
    setWaveletDenoise(DENOISE_OFF);
#endif
    }

    // Set feature enable/disable
    String8 enableDisableValues = createValuesStringFromMap(
            ENABLE_DISABLE_MODES_MAP, PARAM_MAP_SIZE(ENABLE_DISABLE_MODES_MAP));

    // Set Lens Shading
    set(KEY_QC_SUPPORTED_LENSSHADE_MODES, enableDisableValues);
    setLensShadeValue(VALUE_ENABLE);
    // Set MCE
    set(KEY_QC_SUPPORTED_MEM_COLOR_ENHANCE_MODES, enableDisableValues);
    setMCEValue(VALUE_ENABLE);

    // Set DIS
    set(KEY_QC_SUPPORTED_DIS_MODES, enableDisableValues);
    setDISValue(VALUE_DISABLE);

    // Set Histogram
    set(KEY_QC_SUPPORTED_HISTOGRAM_MODES,
        m_pCapability->histogram_supported ? enableDisableValues : "");
    set(KEY_QC_HISTOGRAM, VALUE_DISABLE);

    //Set Red Eye Reduction
    set(KEY_QC_SUPPORTED_REDEYE_REDUCTION, enableDisableValues);
    setRedeyeReduction(VALUE_DISABLE);

    //Set SkinTone Enhancement
    set(KEY_QC_SUPPORTED_SKIN_TONE_ENHANCEMENT_MODES, enableDisableValues);

    // Enable LTM by default and disable it in HDR & SeeMore usecases
    setToneMapMode(true, false);

    // Set feature on/off
    String8 onOffValues = createValuesStringFromMap(
            ON_OFF_MODES_MAP, PARAM_MAP_SIZE(ON_OFF_MODES_MAP));

    //Set See more (LLVD)
    if (m_pCapability->qcom_supported_feature_mask &
            CAM_QCOM_FEATURE_LLVD) {
        set(KEY_QC_SUPPORTED_SEE_MORE_MODES, onOffValues);
        setSeeMore(VALUE_OFF);
    }

    //Set Still more
    if (m_pCapability->qcom_supported_feature_mask &
            CAM_QCOM_FEATURE_STILLMORE) {
        String8 stillMoreValues = createValuesStringFromMap(
                STILL_MORE_MODES_MAP,
                PARAM_MAP_SIZE(STILL_MORE_MODES_MAP));
        set(KEY_QC_SUPPORTED_STILL_MORE_MODES, stillMoreValues);
        setStillMore(STILL_MORE_OFF);
    }

    //Set Noise Reduction mode
    if (m_pCapability->qcom_supported_feature_mask &
            CAM_QTI_FEATURE_SW_TNR) {
        String8 noiseReductionModesValues = createValuesStringFromMap(
                NOISE_REDUCTION_MODES_MAP, PARAM_MAP_SIZE(NOISE_REDUCTION_MODES_MAP));
        set(KEY_QC_NOISE_REDUCTION_MODE_VALUES, noiseReductionModesValues);
        setNoiseReductionMode(VALUE_OFF);
    }

    //Set Scene Detection
    set(KEY_QC_SUPPORTED_SCENE_DETECT, onOffValues);
    setSceneDetect(VALUE_OFF);
    m_bHDREnabled = false;
    m_bHDR1xFrameEnabled = false;

    m_bHDRThumbnailProcessNeeded = false;
    m_bHDR1xExtraBufferNeeded = true;
    for (uint32_t i=0; i<m_pCapability->hdr_bracketing_setting.num_frames; i++) {
        if (0 == m_pCapability->hdr_bracketing_setting.exp_val.values[i]) {
            m_bHDR1xExtraBufferNeeded = false;
            break;
        }
    }

    // Set HDR output scaling
    char value[PROPERTY_VALUE_MAX];

    property_get("persist.camera.hdr.outcrop", value, VALUE_DISABLE);
    if (strncmp(VALUE_ENABLE, value, sizeof(VALUE_ENABLE))) {
      m_bHDROutputCropEnabled = false;
    } else {
      m_bHDROutputCropEnabled = true;
    }

    //Set Face Detection
    set(KEY_QC_SUPPORTED_FACE_DETECTION, onOffValues);
    set(KEY_QC_FACE_DETECTION, VALUE_OFF);

    //Set Face Recognition
    //set(KEY_QC_SUPPORTED_FACE_RECOGNITION, onOffValues);
    //set(KEY_QC_FACE_RECOGNITION, VALUE_OFF);

    //Set ZSL
    set(KEY_QC_SUPPORTED_ZSL_MODES, onOffValues);
#ifdef DEFAULT_ZSL_MODE_ON
    set(KEY_QC_ZSL, VALUE_ON);
    m_bZslMode = true;
#else
    set(KEY_QC_ZSL, VALUE_OFF);
    m_bZslMode = false;
#endif

    // Check if zsl mode property is enabled.
    // If yes, force the camera to be in zsl mode
    // and force zsl mode to be enabled in dual camera mode.
    memset(value, 0x0, PROPERTY_VALUE_MAX);
    property_get("persist.camera.zsl.mode", value, "0");
    int32_t zsl_mode = atoi(value);
    if((zsl_mode == 1) ||
            (m_bZslMode == true) ||
            (m_relCamSyncInfo.sync_control == CAM_SYNC_RELATED_SENSORS_ON)) {
        LOGH("%d: Forcing Camera to ZSL mode enabled");
        set(KEY_QC_ZSL, VALUE_ON);
        m_bForceZslMode = true;
        m_bZslMode = true;
        int32_t value = m_bForceZslMode;
        ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_ZSL_MODE, value);
    }
    m_bZslMode_new = m_bZslMode;

    set(KEY_QC_SCENE_SELECTION, VALUE_DISABLE);

    // Rdi mode
    set(KEY_QC_SUPPORTED_RDI_MODES, enableDisableValues);
    setRdiMode(VALUE_DISABLE);

    // Secure mode
    set(KEY_QC_SUPPORTED_SECURE_MODES, enableDisableValues);
    setSecureMode(VALUE_DISABLE);

    //Set video HDR
    if ((m_pCapability->qcom_supported_feature_mask & CAM_QCOM_FEATURE_VIDEO_HDR) > 0) {
        set(KEY_QC_SUPPORTED_VIDEO_HDR_MODES, onOffValues);
        set(KEY_QC_VIDEO_HDR, VALUE_OFF);
    }

    //Set HW Sensor Snapshot HDR
    if ((m_pCapability->qcom_supported_feature_mask & CAM_QCOM_FEATURE_SENSOR_HDR)> 0) {
        set(KEY_QC_SUPPORTED_SENSOR_HDR_MODES, onOffValues);
        set(KEY_QC_SENSOR_HDR, VALUE_OFF);
        m_bSensorHDREnabled = false;
    }

    // Set VT TimeStamp
    set(KEY_QC_VT_ENABLE, VALUE_DISABLE);
    //Set Touch AF/AEC
    String8 touchValues = createValuesStringFromMap(
            TOUCH_AF_AEC_MODES_MAP, PARAM_MAP_SIZE(TOUCH_AF_AEC_MODES_MAP));

    set(KEY_QC_SUPPORTED_TOUCH_AF_AEC, touchValues);
    set(KEY_QC_TOUCH_AF_AEC, TOUCH_AF_AEC_OFF);

    //set flip mode
    if ((m_pCapability->qcom_supported_feature_mask & CAM_QCOM_FEATURE_FLIP) > 0) {
        String8 flipModes = createValuesStringFromMap(
                FLIP_MODES_MAP, PARAM_MAP_SIZE(FLIP_MODES_MAP));
        set(KEY_QC_SUPPORTED_FLIP_MODES, flipModes);
        set(KEY_QC_PREVIEW_FLIP, FLIP_MODE_OFF);
        set(KEY_QC_VIDEO_FLIP, FLIP_MODE_OFF);
        set(KEY_QC_SNAPSHOT_PICTURE_FLIP, FLIP_MODE_OFF);
    }

    // Set default Auto Exposure lock value
    setAecLock(VALUE_FALSE);

    // Set default AWB_LOCK lock value
    setAwbLock(VALUE_FALSE);

    // Set default Camera mode
    set(KEY_QC_CAMERA_MODE, 0);

    // Add support for internal preview restart
    set(KEY_INTERNAL_PERVIEW_RESTART, VALUE_TRUE);
    // Set default burst number
    set(KEY_QC_SNAPSHOT_BURST_NUM, 0);
    set(KEY_QC_NUM_RETRO_BURST_PER_SHUTTER, 0);

    //Get RAM size and disable features which are memory rich
    struct sysinfo info;
    sysinfo(&info);

    LOGH("totalram = %ld, freeram = %ld ", info.totalram,
        info.freeram);
    if (info.totalram > TOTAL_RAM_SIZE_512MB) {
        set(KEY_QC_ZSL_HDR_SUPPORTED, VALUE_TRUE);
    } else {
        m_bIsLowMemoryDevice = true;
        set(KEY_QC_ZSL_HDR_SUPPORTED, VALUE_FALSE);
    }

    setOfflineRAW();
    memset(mStreamPpMask, 0, sizeof(cam_feature_mask_t)*CAM_STREAM_TYPE_MAX);
    //Set video buffers as uncached by default
    set(KEY_QC_CACHE_VIDEO_BUFFERS, VALUE_DISABLE);

    // Set default longshot mode
    set(KEY_QC_LONG_SHOT, "off");
    //Enable longshot by default
    set(KEY_QC_LONGSHOT_SUPPORTED, VALUE_TRUE);

    int32_t rc = commitParameters();
    if (rc == NO_ERROR) {
        rc = setNumOfSnapshot();
    }

    //Set Video Rotation
    String8 videoRotationValues = createValuesStringFromMap(VIDEO_ROTATION_MODES_MAP,
            PARAM_MAP_SIZE(VIDEO_ROTATION_MODES_MAP));

    set(KEY_QC_SUPPORTED_VIDEO_ROTATION_VALUES, videoRotationValues.string());
    set(KEY_QC_VIDEO_ROTATION, VIDEO_ROTATION_0);

    //Check for EZTune
    setEztune();
    //Default set for video batch size
    set(KEY_QC_VIDEO_BATCH_SIZE, 0);

    //Setup dual-camera
    setDcrf();

    // For Aux Camera of dual camera Mode,
    // by default set no display mode
    if (m_relCamSyncInfo.mode == CAM_MODE_SECONDARY) {
        set(KEY_QC_NO_DISPLAY_MODE, 1);
        m_bNoDisplayMode = true;
    }

    cam_dimension_t pic_dim;
    pic_dim.width = 0;
    pic_dim.height = 0;

    if (m_pCapability->picture_sizes_tbl_cnt > 0 &&
        m_pCapability->picture_sizes_tbl_cnt <= MAX_SIZES_CNT) {
        for(uint32_t i = 0;
                i < m_pCapability->picture_sizes_tbl_cnt; i++) {
            if ((pic_dim.width * pic_dim.height) <
                    (int32_t)(m_pCapability->picture_sizes_tbl[i].width *
                    m_pCapability->picture_sizes_tbl[i].height)) {
                pic_dim.width =
                        m_pCapability->picture_sizes_tbl[i].width;
                pic_dim.height =
                        m_pCapability->picture_sizes_tbl[i].height;
            }
        }
        LOGD("max pic size = %d %d", pic_dim.width,
                pic_dim.height);
        setMaxPicSize(pic_dim);
    } else {
        LOGW("supported picture sizes cnt is 0 or exceeds max!!!");
    }

    setManualCaptureMode(CAM_MANUAL_CAPTURE_TYPE_OFF);

    return rc;
}

/*===========================================================================
 * FUNCTION   : allocate
 *
 * DESCRIPTION: Allocate buffer memory for parameter obj (if necessary)
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::allocate()
{
    int32_t rc = NO_ERROR;

    if (m_pParamHeap != NULL) {
        return rc;
    }

    //Allocate Set Param Buffer
    m_pParamHeap = new QCameraHeapMemory(QCAMERA_ION_USE_CACHE);
    if (m_pParamHeap == NULL) {
        return NO_MEMORY;
    }

    rc = m_pParamHeap->allocate(1, sizeof(parm_buffer_t), NON_SECURE);
    if(rc != OK) {
        rc = NO_MEMORY;
        LOGE("Error!! Param buffers have not been allocated");
        delete m_pParamHeap;
        m_pParamHeap = NULL;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : init
 *
 * DESCRIPTION: initialize parameter obj
 *
 * PARAMETERS :
 *   @capabilities  : ptr to camera capabilities
 *   @mmops         : ptr to memory ops table for mapping/unmapping
 *   @adjustFPS     : object reference for additional (possibly thermal)
 *                    framerate adjustment
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::init(cam_capability_t *capabilities,
        mm_camera_vtbl_t *mmOps, QCameraAdjustFPS *adjustFPS)
{
    int32_t rc = NO_ERROR;

    m_pCapability = capabilities;
    m_pCamOpsTbl = mmOps;
    m_AdjustFPS = adjustFPS;

    if (m_pParamHeap == NULL) {
        LOGE("Parameter buffers have not been allocated");
        rc = UNKNOWN_ERROR;
        goto TRANS_INIT_ERROR1;
    }

    //Map memory for parameters buffer
    cam_buf_map_type_list bufMapList;
    rc = QCameraBufferMaps::makeSingletonBufMapList(
            CAM_MAPPING_BUF_TYPE_PARM_BUF, 0 /*stream id*/,
            0 /*buffer index*/, -1 /*plane index*/, 0 /*cookie*/,
            m_pParamHeap->getFd(0), sizeof(parm_buffer_t), bufMapList,
                    m_pParamHeap->getPtr(0));

    if (rc == NO_ERROR) {
        rc = m_pCamOpsTbl->ops->map_bufs(m_pCamOpsTbl->camera_handle,
                &bufMapList);
    }

    if(rc < 0) {
        LOGE("failed to map SETPARM buffer");
        rc = FAILED_TRANSACTION;
        goto TRANS_INIT_ERROR2;
    }
    m_pParamBuf = (parm_buffer_t*) DATA_PTR(m_pParamHeap,0);

    // Check if it is dual camera mode
    if(m_relCamSyncInfo.sync_control == CAM_SYNC_RELATED_SENSORS_ON) {
        //Allocate related cam sync buffer
        //this is needed for the payload that goes along with bundling cmd for related
        //camera use cases
        m_pRelCamSyncHeap = new QCameraHeapMemory(QCAMERA_ION_USE_CACHE);
        rc = m_pRelCamSyncHeap->allocate(1,
                sizeof(cam_sync_related_sensors_event_info_t), NON_SECURE);
        if(rc != OK) {
            rc = NO_MEMORY;
            LOGE("Failed to allocate Related cam sync Heap memory");
            goto TRANS_INIT_ERROR3;
        }

        //Map memory for related cam sync buffer
        rc = m_pCamOpsTbl->ops->map_buf(m_pCamOpsTbl->camera_handle,
                CAM_MAPPING_BUF_TYPE_SYNC_RELATED_SENSORS_BUF,
                m_pRelCamSyncHeap->getFd(0),
                sizeof(cam_sync_related_sensors_event_info_t),
                (cam_sync_related_sensors_event_info_t*)DATA_PTR(m_pRelCamSyncHeap,0));
        if(rc < 0) {
            LOGE("failed to map Related cam sync buffer");
            rc = FAILED_TRANSACTION;
            goto TRANS_INIT_ERROR4;
        }
        m_pRelCamSyncBuf =
                (cam_sync_related_sensors_event_info_t*) DATA_PTR(m_pRelCamSyncHeap,0);
    }
    initDefaultParameters();
    mCommon.init(capabilities);
    m_bInited = true;

    goto TRANS_INIT_DONE;

TRANS_INIT_ERROR4:
    m_pRelCamSyncHeap->deallocate();

TRANS_INIT_ERROR3:
    delete m_pRelCamSyncHeap;
    m_pRelCamSyncHeap = NULL;

TRANS_INIT_ERROR2:
    m_pParamHeap->deallocate();
    delete m_pParamHeap;
    m_pParamHeap = NULL;

TRANS_INIT_ERROR1:
    m_pCapability = NULL;
    m_pCamOpsTbl = NULL;
    m_AdjustFPS = NULL;

TRANS_INIT_DONE:
    return rc;
}

/*===========================================================================
 * FUNCTION   : deinit
 *
 * DESCRIPTION: deinitialize
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
void QCameraParameters::deinit()
{
    if (!m_bInited) {
        return;
    }

    //clear all entries in the map
    String8 emptyStr;
    QCameraParameters::unflatten(emptyStr);

    if ((NULL != m_pCamOpsTbl) && (m_pCamOpsTbl->ops != NULL)) {
        m_pCamOpsTbl->ops->unmap_buf(
                             m_pCamOpsTbl->camera_handle,
                             CAM_MAPPING_BUF_TYPE_PARM_BUF);

        if (m_relCamSyncInfo.sync_control == CAM_SYNC_RELATED_SENSORS_ON) {
            m_pCamOpsTbl->ops->unmap_buf(
                    m_pCamOpsTbl->camera_handle,
                    CAM_MAPPING_BUF_TYPE_SYNC_RELATED_SENSORS_BUF);
        }
    }

    m_pCapability = NULL;
    if (NULL != m_pParamHeap) {
        m_pParamHeap->deallocate();
        delete m_pParamHeap;
        m_pParamHeap = NULL;
        m_pParamBuf = NULL;
    }
    if (NULL != m_pRelCamSyncHeap) {
        m_pRelCamSyncHeap->deallocate();
        delete m_pRelCamSyncHeap;
        m_pRelCamSyncHeap = NULL;
        m_pRelCamSyncBuf = NULL;
    }

    m_AdjustFPS = NULL;
    m_tempMap.clear();
    m_pCamOpsTbl = NULL;
    m_AdjustFPS = NULL;

    m_bInited = false;
}

/*===========================================================================
 * FUNCTION   : parse_pair
 *
 * DESCRIPTION: helper function to parse string like "640x480" or "10000,20000"
 *
 * PARAMETERS :
 *   @str     : input string to be parse
 *   @first   : [output] first value of the pair
 *   @second  : [output]  second value of the pair
 *   @delim   : [input] delimeter to seperate the pair
 *   @endptr  : [output] ptr to the end of the pair string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::parse_pair(const char *str,
                                      int *first,
                                      int *second,
                                      char delim,
                                      char **endptr = NULL)
{
    // Find the first integer.
    char *end;
    int w = (int)strtol(str, &end, 10);
    // If a delimeter does not immediately follow, give up.
    if (*end != delim) {
        LOGE("Cannot find delimeter (%c) in str=%s", delim, str);
        return BAD_VALUE;
    }

    // Find the second integer, immediately after the delimeter.
    int h = (int)strtol(end+1, &end, 10);

    *first = w;
    *second = h;

    if (endptr) {
        *endptr = end;
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : parseSizesList
 *
 * DESCRIPTION: helper function to parse string containing sizes
 *
 * PARAMETERS :
 *   @sizesStr: [input] input string to be parse
 *   @sizes   : [output] reference to store parsed sizes
 *
 * RETURN     : none
 *==========================================================================*/
void QCameraParameters::parseSizesList(const char *sizesStr, Vector<Size> &sizes)
{
    if (sizesStr == 0) {
        return;
    }

    char *sizeStartPtr = (char *)sizesStr;

    while (true) {
        int width, height;
        int success = parse_pair(sizeStartPtr, &width, &height, 'x',
                                 &sizeStartPtr);
        if (success == -1 || (*sizeStartPtr != ',' && *sizeStartPtr != '\0')) {
            LOGE("Picture sizes string \"%s\" contains invalid character.", sizesStr);
            return;
        }
        sizes.push(Size(width, height));

        if (*sizeStartPtr == '\0') {
            return;
        }
        sizeStartPtr++;
    }
}

/*===========================================================================
 * FUNCTION   : adjustPreviewFpsRange
 *
 * DESCRIPTION: adjust preview FPS ranges
 *              according to external events
 *
 * PARAMETERS :
 *   @minFPS  : min FPS value
 *   @maxFPS  : max FPS value
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::adjustPreviewFpsRange(cam_fps_range_t *fpsRange)
{
    if ( fpsRange == NULL ) {
        return BAD_VALUE;
    }

    if ( m_pParamBuf == NULL ) {
        return NO_INIT;
    }

    int32_t rc = initBatchUpdate(m_pParamBuf);
    if ( rc != NO_ERROR ) {
        LOGE("Failed to initialize group update table");
        return rc;
    }

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_FPS_RANGE, *fpsRange)) {
        LOGE("Parameters batch failed");
        return BAD_VALUE;
    }

    rc = commitSetBatch();
    if ( rc != NO_ERROR ) {
        LOGE("Failed to commit batch parameters");
        return rc;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : setPreviewFpsRanges
 *
 * DESCRIPTION: set preview FPS ranges
 *
 * PARAMETERS :
 *   @minFPS  : min FPS value
 *   @maxFPS  : max FPS value
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setPreviewFpsRange(int min_fps,
        int max_fps, int vid_min_fps,int vid_max_fps)
{
    char str[32];
    char value[PROPERTY_VALUE_MAX];
    int fixedFpsValue;
    /*This property get value should be the fps that user needs*/
    property_get("persist.debug.set.fixedfps", value, "0");
    fixedFpsValue = atoi(value);

    LOGD("E minFps = %d, maxFps = %d , vid minFps = %d, vid maxFps = %d",
                 min_fps, max_fps, vid_min_fps, vid_max_fps);

    if(fixedFpsValue != 0) {
        min_fps = max_fps = fixedFpsValue*1000;
        if (!isHfrMode()) {
             vid_min_fps = vid_max_fps = fixedFpsValue*1000;
        }
    }
    snprintf(str, sizeof(str), "%d,%d", min_fps, max_fps);
    LOGH("Setting preview fps range %s", str);
    updateParamEntry(KEY_PREVIEW_FPS_RANGE, str);
    cam_fps_range_t fps_range;
    memset(&fps_range, 0x00, sizeof(cam_fps_range_t));
    fps_range.min_fps = (float)min_fps / 1000.0f;
    fps_range.max_fps = (float)max_fps / 1000.0f;
    fps_range.video_min_fps = (float)vid_min_fps / 1000.0f;
    fps_range.video_max_fps = (float)vid_max_fps / 1000.0f;

    LOGH("Updated: minFps = %d, maxFps = %d ,"
            " vid minFps = %d, vid maxFps = %d",
             min_fps, max_fps, vid_min_fps, vid_max_fps);

    if ( NULL != m_AdjustFPS ) {
        if (m_ThermalMode == QCAMERA_THERMAL_ADJUST_FPS &&
                !m_bRecordingHint) {
            float minVideoFps = min_fps, maxVideoFps = max_fps;
            if (isHfrMode()) {
                minVideoFps = m_hfrFpsRange.video_min_fps;
                maxVideoFps = m_hfrFpsRange.video_max_fps;
            }
            m_AdjustFPS->recalcFPSRange(min_fps, max_fps, minVideoFps, maxVideoFps, fps_range);
            LOGH("Thermal adjusted Preview fps range %3.2f,%3.2f, %3.2f, %3.2f",
                   fps_range.min_fps, fps_range.max_fps,
                  fps_range.video_min_fps, fps_range.video_max_fps);
        }
    }

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_FPS_RANGE, fps_range)) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}



/*===========================================================================
 * FUNCTION   : setAutoExposure
 *
 * DESCRIPTION: set auto exposure
 *
 * PARAMETERS :
 *   @autoExp : auto exposure value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setAutoExposure(const char *autoExp)
{
    if (autoExp != NULL) {
        int32_t value = lookupAttr(AUTO_EXPOSURE_MAP, PARAM_MAP_SIZE(AUTO_EXPOSURE_MAP), autoExp);
        if (value != NAME_NOT_FOUND) {
            LOGH("Setting auto exposure %s", autoExp);
            updateParamEntry(KEY_QC_AUTO_EXPOSURE, autoExp);
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_AEC_ALGO_TYPE, value)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid auto exposure value: %s", (autoExp == NULL) ? "NULL" : autoExp);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setEffect
 *
 * DESCRIPTION: set effect
 *
 * PARAMETERS :
 *   @effect  : effect value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setEffect(const char *effect)
{
    if (effect != NULL) {
        int32_t value = lookupAttr(EFFECT_MODES_MAP, PARAM_MAP_SIZE(EFFECT_MODES_MAP), effect);
        if (value != NAME_NOT_FOUND) {
            LOGH("Setting effect %s", effect);
            updateParamEntry(KEY_EFFECT, effect);
            uint8_t prmEffect = static_cast<uint8_t>(value);
            mParmEffect = prmEffect;
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_EFFECT, prmEffect)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid effect value: %s", (effect == NULL) ? "NULL" : effect);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setBrightness
 *
 * DESCRIPTION: set brightness control value
 *
 * PARAMETERS :
 *   @brightness  : brightness control value
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setBrightness(int brightness)
{
    char val[16];
    snprintf(val, sizeof(val), "%d", brightness);
    updateParamEntry(KEY_QC_BRIGHTNESS, val);

    LOGH("Setting brightness %s", val);
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_BRIGHTNESS, brightness)) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setFocusMode
 *
 * DESCRIPTION: set focus mode
 *
 * PARAMETERS :
 *   @focusMode  : focus mode value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setFocusMode(const char *focusMode)
{
    if (focusMode != NULL) {
        int32_t value = lookupAttr(FOCUS_MODES_MAP, PARAM_MAP_SIZE(FOCUS_MODES_MAP), focusMode);
        if (value != NAME_NOT_FOUND) {
            int32_t rc = NO_ERROR;
            LOGH("Setting focus mode %s", focusMode);
            mFocusMode = (cam_focus_mode_type)value;

            updateParamEntry(KEY_FOCUS_MODE, focusMode);
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf,
                    CAM_INTF_PARM_FOCUS_MODE, (uint8_t)value)) {
                rc = BAD_VALUE;
            }
            if (strcmp(focusMode,"infinity")==0){
                set(QCameraParameters::KEY_FOCUS_DISTANCES, "Infinity,Infinity,Infinity");
            }
            return rc;
        }
    }
    LOGE("Invalid focus mode value: %s", (focusMode == NULL) ? "NULL" : focusMode);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setFocusPosition
 *
 * DESCRIPTION: set focus position
 *
 * PARAMETERS :
 *   @typeStr : focus position type, index or dac_code
 *   @posStr : focus positon.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t  QCameraParameters::setFocusPosition(const char *typeStr, const char *posStr)
{
    LOGH(", type:%s, pos: %s", typeStr, posStr);
    int32_t type = atoi(typeStr);
    float pos = (float) atof(posStr);

    if ((type >= CAM_MANUAL_FOCUS_MODE_INDEX) &&
            (type < CAM_MANUAL_FOCUS_MODE_MAX)) {
        // get max and min focus position from m_pCapability
        float minFocusPos = m_pCapability->min_focus_pos[type];
        float maxFocusPos = m_pCapability->max_focus_pos[type];
        LOGH(", focusPos min: %f, max: %f", minFocusPos, maxFocusPos);

        if (pos >= minFocusPos && pos <= maxFocusPos) {
            updateParamEntry(KEY_QC_MANUAL_FOCUS_POS_TYPE, typeStr);
            updateParamEntry(KEY_QC_MANUAL_FOCUS_POSITION, posStr);

            cam_manual_focus_parm_t manual_focus;
            manual_focus.flag = (cam_manual_focus_mode_type)type;
            if (manual_focus.flag == CAM_MANUAL_FOCUS_MODE_DIOPTER) {
                manual_focus.af_manual_diopter = pos;
            } else if (manual_focus.flag == CAM_MANUAL_FOCUS_MODE_RATIO) {
                manual_focus.af_manual_lens_position_ratio = (int32_t) pos;
            } else if (manual_focus.flag == CAM_MANUAL_FOCUS_MODE_INDEX) {
                manual_focus.af_manual_lens_position_index = (int32_t) pos;
            } else {
                manual_focus.af_manual_lens_position_dac = (int32_t) pos;
            }

            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_MANUAL_FOCUS_POS,
                    manual_focus)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }

    LOGE("invalid params, type:%d, pos: %f", type, pos);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : updateAEInfo
 *
 * DESCRIPTION: update exposure information from metadata callback
 *
 * PARAMETERS :
 *   @ae_params : auto exposure params
 *
 * RETURN     : void
 *==========================================================================*/
void  QCameraParameters::updateAEInfo(cam_3a_params_t &ae_params)
{
    const char *prevExpTime = get(KEY_QC_CURRENT_EXPOSURE_TIME);
    char newExpTime[15];
    snprintf(newExpTime, sizeof(newExpTime), "%f", ae_params.exp_time*1000.0);

    if (prevExpTime == NULL || strcmp(prevExpTime, newExpTime)) {
        LOGD("update exposure time: old: %s, new: %s", prevExpTime, newExpTime);
        set(KEY_QC_CURRENT_EXPOSURE_TIME, newExpTime);
    }

    int32_t prevISO = getInt(KEY_QC_CURRENT_ISO);
    int32_t newISO = ae_params.iso_value;
    if (prevISO != newISO) {
        LOGD("update iso: old:%d, new:%d", prevISO, newISO);
        set(KEY_QC_CURRENT_ISO, newISO);
    }
}

/*===========================================================================
 * FUNCTION   : updateCurrentFocusPosition
 *
 * DESCRIPTION: update current focus position from metadata callback
 *
 * PARAMETERS :
 *   @pos : current focus position
 *
 * RETURN     : void
 *==========================================================================*/
void  QCameraParameters::updateCurrentFocusPosition(cam_focus_pos_info_t &cur_pos_info)
{
    int prevScalePos = getInt(KEY_QC_FOCUS_POSITION_SCALE);
    int newScalePos = (int) cur_pos_info.scale;
    if (prevScalePos != newScalePos) {
        LOGD("update focus scale: old:%d, new:%d", prevScalePos, newScalePos);
        set(KEY_QC_FOCUS_POSITION_SCALE, newScalePos);
    }

    float prevDiopterPos = getFloat(KEY_QC_FOCUS_POSITION_DIOPTER);
    float newDiopterPos = cur_pos_info.diopter;
    if (prevDiopterPos != newDiopterPos) {
        LOGD("update focus diopter: old:%f, new:%f", prevDiopterPos, newDiopterPos);
        setFloat(KEY_QC_FOCUS_POSITION_DIOPTER, newDiopterPos);
    }
}

/*===========================================================================
 * FUNCTION   : setSharpness
 *
 * DESCRIPTION: set sharpness control value
 *
 * PARAMETERS :
 *   @sharpness  : sharpness control value
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSharpness(int sharpness)
{
    char val[16];
    snprintf(val, sizeof(val), "%d", sharpness);
    updateParamEntry(KEY_QC_SHARPNESS, val);
    LOGH("Setting sharpness %s", val);
    m_nSharpness = sharpness;
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_SHARPNESS, m_nSharpness)) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setSkinToneEnhancement
 *
 * DESCRIPTION: set skin tone enhancement value
 *
 * PARAMETERS :
 *   @sceFactore  : skin tone enhancement factor value
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSkinToneEnhancement(int sceFactor)
{
    char val[16];
    snprintf(val, sizeof(val), "%d", sceFactor);
    updateParamEntry(KEY_QC_SCE_FACTOR, val);
    LOGH("Setting skintone enhancement %s", val);

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_SCE_FACTOR, sceFactor)) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setSaturation
 *
 * DESCRIPTION: set saturation control value
 *
 * PARAMETERS :
 *   @saturation : saturation control value
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSaturation(int saturation)
{
    char val[16];
    snprintf(val, sizeof(val), "%d", saturation);
    updateParamEntry(KEY_QC_SATURATION, val);
    LOGH("Setting saturation %s", val);

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_SATURATION, saturation)) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setContrast
 *
 * DESCRIPTION: set contrast control value
 *
 * PARAMETERS :
 *   @contrast : contrast control value
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setContrast(int contrast)
{
    char val[16];
    snprintf(val, sizeof(val), "%d", contrast);
    updateParamEntry(KEY_QC_CONTRAST, val);
    LOGH("Setting contrast %s", val);

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_CONTRAST, contrast)) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setSceneDetect
 *
 * DESCRIPTION: set scenen detect value
 *
 * PARAMETERS :
 *   @sceneDetect  : scene detect value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSceneDetect(const char *sceneDetect)
{
    if (sceneDetect != NULL) {
        int32_t value = lookupAttr(ON_OFF_MODES_MAP, PARAM_MAP_SIZE(ON_OFF_MODES_MAP),
                sceneDetect);
        if (value != NAME_NOT_FOUND) {
            LOGH("Setting Scene Detect %s", sceneDetect);
            updateParamEntry(KEY_QC_SCENE_DETECT, sceneDetect);
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_ASD_ENABLE, value)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid Scene Detect value: %s",
          (sceneDetect == NULL) ? "NULL" : sceneDetect);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setSensorSnapshotHDR
 *
 * DESCRIPTION: set snapshot HDR value
 *
 * PARAMETERS :
 *   @snapshotHDR  : snapshot HDR value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSensorSnapshotHDR(const char *snapshotHDR)
{
    if (snapshotHDR != NULL) {
        int32_t value = (cam_sensor_hdr_type_t) lookupAttr(ON_OFF_MODES_MAP,
                PARAM_MAP_SIZE(ON_OFF_MODES_MAP), snapshotHDR);
        if (value != NAME_NOT_FOUND) {
            LOGH("Setting Sensor Snapshot HDR %s", snapshotHDR);
            updateParamEntry(KEY_QC_SENSOR_HDR, snapshotHDR);

            char zz_prop[PROPERTY_VALUE_MAX];
            memset(zz_prop, 0, sizeof(zz_prop));
            property_get("persist.camera.zzhdr.enable", zz_prop, "0");
            uint8_t zzhdr_enable = (uint8_t)atoi(zz_prop);

            if (zzhdr_enable && (value != CAM_SENSOR_HDR_OFF)) {
                value = CAM_SENSOR_HDR_ZIGZAG;
                LOGH("%s: Overriding to ZZ HDR Mode", __func__);
            }

            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_SENSOR_HDR, (cam_sensor_hdr_type_t)value)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid Snapshot HDR value: %s",
          (snapshotHDR == NULL) ? "NULL" : snapshotHDR);
    return BAD_VALUE;

}


/*===========================================================================
 * FUNCTION   : setVideoHDR
 *
 * DESCRIPTION: set video HDR value
 *
 * PARAMETERS :
 *   @videoHDR  : svideo HDR value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setVideoHDR(const char *videoHDR)
{
    if (videoHDR != NULL) {
        int32_t value = lookupAttr(ON_OFF_MODES_MAP, PARAM_MAP_SIZE(ON_OFF_MODES_MAP), videoHDR);
        if (value != NAME_NOT_FOUND) {

            char zz_prop[PROPERTY_VALUE_MAX];
            memset(zz_prop, 0, sizeof(zz_prop));
            property_get("persist.camera.zzhdr.video", zz_prop, "0");
            uint8_t use_zzhdr_video = (uint8_t)atoi(zz_prop);

            if (use_zzhdr_video) {
                LOGH("%s: Using ZZ HDR for video mode", __func__);
                if (value)
                    value = CAM_SENSOR_HDR_ZIGZAG;
                else
                    value = CAM_SENSOR_HDR_OFF;
                LOGH("%s: Overriding to sensor HDR Mode to:%d", __func__, value);
                if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_SENSOR_HDR, (cam_sensor_hdr_type_t) value)) {
                    LOGE("%s: Override to sensor HDR mode for video HDR failed", __func__);
                    return BAD_VALUE;
                }
                updateParamEntry(KEY_QC_VIDEO_HDR, videoHDR);
            } else {
                LOGH("%s: Setting Video HDR %s", __func__, videoHDR);
                updateParamEntry(KEY_QC_VIDEO_HDR, videoHDR);
                if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_VIDEO_HDR, value)) {
                    return BAD_VALUE;
                }
            }

            return NO_ERROR;
        }
    }
    LOGE("Invalid Video HDR value: %s",
          (videoHDR == NULL) ? "NULL" : videoHDR);
    return BAD_VALUE;
}



/*===========================================================================
 * FUNCTION   : setVtEnable
 *
 * DESCRIPTION: set vt Enable value
 *
 * PARAMETERS :
 *   @videoHDR  : svtEnable value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setVtEnable(const char *vtEnable)
{
    if (vtEnable != NULL) {
        int32_t value = lookupAttr(ENABLE_DISABLE_MODES_MAP,
                PARAM_MAP_SIZE(ENABLE_DISABLE_MODES_MAP), vtEnable);
        if (value != NAME_NOT_FOUND) {
            LOGH("Setting Vt Enable %s", vtEnable);
            m_bAVTimerEnabled = true;
            updateParamEntry(KEY_QC_VT_ENABLE, vtEnable);
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_VT, value)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid Vt Enable value: %s",
          (vtEnable == NULL) ? "NULL" : vtEnable);
    m_bAVTimerEnabled = false;
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setFaceRecognition
 *
 * DESCRIPTION: set face recognition value
 *
 * PARAMETERS :
 *   @faceRecog  : face recognition value string
 *   @maxFaces   : number of max faces to be detected/recognized
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setFaceRecognition(const char *faceRecog,
        uint32_t maxFaces)
{
    if (faceRecog != NULL) {
        int32_t value = lookupAttr(ON_OFF_MODES_MAP, PARAM_MAP_SIZE(ON_OFF_MODES_MAP), faceRecog);
        if (value != NAME_NOT_FOUND) {
            LOGH("Setting face recognition %s", faceRecog);
            updateParamEntry(KEY_QC_FACE_RECOGNITION, faceRecog);

            uint32_t faceProcMask = m_nFaceProcMask;
            if (value > 0) {
                faceProcMask |= CAM_FACE_PROCESS_MASK_RECOGNITION;
            } else {
                faceProcMask &= (uint32_t)(~CAM_FACE_PROCESS_MASK_RECOGNITION);
            }

            if(m_nFaceProcMask == faceProcMask) {
                LOGH("face process mask not changed, no ops here");
                return NO_ERROR;
            }
            m_nFaceProcMask = faceProcMask;
            LOGH("FaceProcMask -> %d", m_nFaceProcMask);

            // set parm for face process
            cam_fd_set_parm_t fd_set_parm;
            memset(&fd_set_parm, 0, sizeof(cam_fd_set_parm_t));
            fd_set_parm.fd_mode = m_nFaceProcMask;
            fd_set_parm.num_fd = maxFaces;

            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_FD, fd_set_parm)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid face recognition value: %s", (faceRecog == NULL) ? "NULL" : faceRecog);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setZoom
 *
 * DESCRIPTION: set zoom level
 *
 * PARAMETERS :
 *   @zoom_level : zoom level
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setZoom(int zoom_level)
{
    char val[16];
    snprintf(val, sizeof(val), "%d", zoom_level);
    updateParamEntry(KEY_ZOOM, val);
    LOGH("zoom level: %d", zoom_level);
    mZoomLevel = zoom_level;
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_ZOOM, zoom_level)) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setISOValue
 *
 * DESCRIPTION: set ISO value
 *
 * PARAMETERS :
 *   @isoValue : ISO value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t  QCameraParameters::setISOValue(const char *isoValue)
{
    if (isoValue != NULL) {
        if (!strcmp(isoValue, ISO_MANUAL)) {
            LOGD("iso manual mode - use continuous iso");
            updateParamEntry(KEY_QC_ISO_MODE, isoValue);
            return NO_ERROR;
        }
        int32_t value = lookupAttr(ISO_MODES_MAP, PARAM_MAP_SIZE(ISO_MODES_MAP), isoValue);
        if (value != NAME_NOT_FOUND) {
            LOGH("Setting ISO value %s", isoValue);
            updateParamEntry(KEY_QC_ISO_MODE, isoValue);

            cam_intf_parm_manual_3a_t iso_settings;
            memset(&iso_settings, 0, sizeof(cam_intf_parm_manual_3a_t));
            iso_settings.previewOnly = FALSE;
            iso_settings.value = value;
            if (getManualCaptureMode() != CAM_MANUAL_CAPTURE_TYPE_OFF) {
                iso_settings.previewOnly = TRUE;
            }

            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_ISO, iso_settings)) {
                return BAD_VALUE;
            }
            m_isoValue = value;
            return NO_ERROR;
        }
    }
    LOGE("Invalid ISO value: %s",
          (isoValue == NULL) ? "NULL" : isoValue);
    return BAD_VALUE;
}


/*===========================================================================
 * FUNCTION   : setContinuousISO
 *
 * DESCRIPTION: set continuous ISO value
 *
 * PARAMETERS :
 *   @params : ISO value parameter
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t  QCameraParameters::setContinuousISO(const QCameraParameters& params)
{
    const char *iso = params.get(KEY_QC_ISO_MODE);
    LOGD("current iso mode: %s", iso);

    if (iso != NULL) {
        if (strcmp(iso, ISO_MANUAL)) {
            LOGD("dont set iso to back-end.");
            return NO_ERROR;
        }
    }

    const char *str = params.get(KEY_QC_CONTINUOUS_ISO);
    const char *prev_str = get(KEY_QC_CONTINUOUS_ISO);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setContinuousISO(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setExposureTime
 *
 * DESCRIPTION: set exposure time
 *
 * PARAMETERS :
 *   @expTimeStr : string of exposure time in ms
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t  QCameraParameters::setExposureTime(const char *expTimeStr)
{
    if (expTimeStr != NULL) {
        double expTimeMs = atof(expTimeStr);
        //input is in milli seconds. Convert to nano sec for backend
        int64_t expTimeNs = (int64_t)(expTimeMs*1000000L);

        // expTime == 0 means not to use manual exposure time.
        if ((0 <= expTimeNs) &&
                ((expTimeNs == 0) ||
                ((expTimeNs >= m_pCapability->exposure_time_range[0]) &&
                (expTimeNs <= m_pCapability->exposure_time_range[1])))) {
            LOGH(", exposure time: %f ms", expTimeMs);
            updateParamEntry(KEY_QC_EXPOSURE_TIME, expTimeStr);

            cam_intf_parm_manual_3a_t exp_settings;
            memset(&exp_settings, 0, sizeof(cam_intf_parm_manual_3a_t));
            if (getManualCaptureMode() != CAM_MANUAL_CAPTURE_TYPE_OFF) {
                exp_settings.previewOnly = TRUE;
                if (expTimeMs < QCAMERA_MAX_EXP_TIME_LEVEL1) {
                    exp_settings.value = expTimeNs;
                } else {
                    exp_settings.value =
                            (int64_t)(QCAMERA_MAX_EXP_TIME_LEVEL1*1000000L);
                }
            } else {
                exp_settings.previewOnly = FALSE;
                exp_settings.value = expTimeNs;
            }

            //Based on exposure values we can decide the capture type here
            if (getManualCaptureMode() != CAM_MANUAL_CAPTURE_TYPE_OFF) {
                if (expTimeMs < QCAMERA_MAX_EXP_TIME_LEVEL1) {
                    setManualCaptureMode(CAM_MANUAL_CAPTURE_TYPE_1);
                } else if (expTimeMs < QCAMERA_MAX_EXP_TIME_LEVEL2) {
                    setManualCaptureMode(CAM_MANUAL_CAPTURE_TYPE_2);
                } else if (expTimeMs < QCAMERA_MAX_EXP_TIME_LEVEL4) {
                    setManualCaptureMode(CAM_MANUAL_CAPTURE_TYPE_3);
                } else {
                    setManualCaptureMode(CAM_MANUAL_CAPTURE_TYPE_OFF);
                }
            }

            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_EXPOSURE_TIME,
                    exp_settings)) {
                return BAD_VALUE;
            }
            m_expTime = expTimeNs;

            return NO_ERROR;
        }
    }

    LOGE("Invalid exposure time, value: %s",
          (expTimeStr == NULL) ? "NULL" : expTimeStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setLongshotEnable
 *
 * DESCRIPTION: set a flag indicating longshot mode
 *
 * PARAMETERS :
 *   @enable  : true - Longshot enabled
 *              false - Longshot disabled
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setLongshotEnable(bool enable)
{
    int32_t rc = NO_ERROR;
    int8_t value = enable ? 1 : 0;

    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_LONGSHOT_ENABLE, value)) {
        LOGE("Failed to update table");
        return BAD_VALUE;
    }

    rc = commitSetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to parameter changes");
        return rc;
    }

    m_bLongshotEnabled = enable;

    return rc;
}

/*===========================================================================
 * FUNCTION   : setFlash
 *
 * DESCRIPTION: set flash mode
 *
 * PARAMETERS :
 *   @flashStr : LED flash mode value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setFlash(const char *flashStr)
{
    if (flashStr != NULL) {
        int32_t value = lookupAttr(FLASH_MODES_MAP, PARAM_MAP_SIZE(FLASH_MODES_MAP), flashStr);
        if (value != NAME_NOT_FOUND) {
            LOGH("Setting Flash value %s", flashStr);
            updateParamEntry(KEY_FLASH_MODE, flashStr);
            mFlashValue = value;
            return NO_ERROR;
        }
    }
    LOGE("Invalid flash value: %s", (flashStr == NULL) ? "NULL" : flashStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : updateFlashMode
 *
 * DESCRIPTION: update flash mode
 *
 * PARAMETERS :
 *   @flashStr : LED flash mode value
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::updateFlashMode(cam_flash_mode_t flash_mode)
{
    int32_t rc = NO_ERROR;
    if (flash_mode >= CAM_FLASH_MODE_MAX) {
        LOGH("Error!! Invalid flash mode (%d)", flash_mode);
        return BAD_VALUE;
    }
    LOGH("Setting Flash mode from EZTune %d", flash_mode);

    const char *flash_mode_str = lookupNameByValue(FLASH_MODES_MAP,
            PARAM_MAP_SIZE(FLASH_MODES_MAP), flash_mode);
    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }
    rc = setFlash(flash_mode_str);
    if (rc != NO_ERROR) {
        LOGE("Failed to update Flash mode");
        return rc;
    }

    LOGH("Setting Flash mode %d", mFlashValue);
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_LED_MODE, mFlashValue)) {
        LOGE("Failed to set led mode");
        return BAD_VALUE;
    }

    rc = commitSetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to commit parameters");
        return rc;
    }

    return NO_ERROR;
}


/*===========================================================================
 * FUNCTION   : configureFlash
 *
 * DESCRIPTION: configure Flash Bracketing.
 *
 * PARAMETERS :
 *    @frame_config : output configuration structure to fill in.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::configureFlash(cam_capture_frame_config_t &frame_config)
{
    LOGH("E");
    int32_t rc = NO_ERROR;
    uint32_t i = 0;

    if (isChromaFlashEnabled()) {

        rc = setToneMapMode(false, false);
        if (rc != NO_ERROR) {
            LOGE("Failed to configure tone map");
            return rc;
        }

        rc = setCDSMode(CAM_CDS_MODE_OFF, false);
        if (rc != NO_ERROR) {
            LOGE("Failed to configure csd mode");
            return rc;
        }

        LOGH("Enable Chroma Flash capture");
        cam_flash_mode_t flash_mode = CAM_FLASH_MODE_OFF;
        frame_config.num_batch =
                m_pCapability->chroma_flash_settings_need.burst_count;
        if (frame_config.num_batch > CAM_MAX_FLASH_BRACKETING) {
            frame_config.num_batch = CAM_MAX_FLASH_BRACKETING;
        }
        for (i = 0; i < frame_config.num_batch; i++) {
            flash_mode = (m_pCapability->chroma_flash_settings_need.flash_bracketing[i]) ?
                    CAM_FLASH_MODE_ON:CAM_FLASH_MODE_OFF;
            frame_config.configs[i].num_frames = 1;
            frame_config.configs[i].type = CAM_CAPTURE_FLASH;
            frame_config.configs[i].flash_mode = flash_mode;
        }
    } else if (mFlashValue != CAM_FLASH_MODE_OFF) {
        frame_config.num_batch = 1;
        for (i = 0; i < frame_config.num_batch; i++) {
            frame_config.configs[i].num_frames = getNumOfSnapshots();
            frame_config.configs[i].type = CAM_CAPTURE_FLASH;
            frame_config.configs[i].flash_mode =(cam_flash_mode_t)mFlashValue;
        }
    }

    LOGD("Flash frame batch cnt = %d",frame_config.num_batch);
    return rc;
}

/*===========================================================================
 * FUNCTION   : configureHDRBracketing
 *
 * DESCRIPTION: configure HDR Bracketing.
 *
 * PARAMETERS :
 *    @frame_config : output configuration structure to fill in.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::configureHDRBracketing(cam_capture_frame_config_t &frame_config)
{
    LOGH("E");
    int32_t rc = NO_ERROR;
    uint32_t i = 0;

    uint32_t hdrFrameCount = m_pCapability->hdr_bracketing_setting.num_frames;
    LOGH("HDR values %d, %d frame count: %u",
          (int8_t) m_pCapability->hdr_bracketing_setting.exp_val.values[0],
          (int8_t) m_pCapability->hdr_bracketing_setting.exp_val.values[1],
          hdrFrameCount);

    frame_config.num_batch = hdrFrameCount;

    cam_bracket_mode mode =
            m_pCapability->hdr_bracketing_setting.exp_val.mode;
    if (mode == CAM_EXP_BRACKETING_ON) {
        rc = setToneMapMode(false, true);
        if (rc != NO_ERROR) {
            LOGW("Failed to disable tone map during HDR");
        }
    }
    for (i = 0; i < frame_config.num_batch; i++) {
        frame_config.configs[i].num_frames = 1;
        frame_config.configs[i].type = CAM_CAPTURE_BRACKETING;
        frame_config.configs[i].hdr_mode.mode = mode;
        frame_config.configs[i].hdr_mode.values =
                m_pCapability->hdr_bracketing_setting.exp_val.values[i];
        LOGD("exp values %d",
                (int)frame_config.configs[i].hdr_mode.values);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : configureAEBracketing
 *
 * DESCRIPTION: configure AE Bracketing.
 *
 * PARAMETERS :
 *    @frame_config : output configuration structure to fill in.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::configureAEBracketing(cam_capture_frame_config_t &frame_config)
{
    LOGH("E");
    int32_t rc = NO_ERROR;
    uint32_t i = 0;
    char exp_value[MAX_EXP_BRACKETING_LENGTH];

    rc = setToneMapMode(false, true);
    if (rc != NO_ERROR) {
        LOGH("Failed to disable tone map during AEBracketing");
    }

    uint32_t burstCount = 0;
    const char *str_val = m_AEBracketingClient.values;
    if ((str_val != NULL) && (strlen(str_val) > 0)) {
        char prop[PROPERTY_VALUE_MAX];
        memset(prop, 0, sizeof(prop));
        strlcpy(prop, str_val, PROPERTY_VALUE_MAX);
        char *saveptr = NULL;
        char *token = strtok_r(prop, ",", &saveptr);
        if (token != NULL) {
            exp_value[burstCount++] = (char)atoi(token);
            while (token != NULL) {
                token = strtok_r(NULL, ",", &saveptr);
                if (token != NULL) {
                    exp_value[burstCount++] = (char)atoi(token);
                }
            }
        }
    }

    frame_config.num_batch = burstCount;
    cam_bracket_mode mode = m_AEBracketingClient.mode;

    for (i = 0; i < frame_config.num_batch; i++) {
        frame_config.configs[i].num_frames = 1;
        frame_config.configs[i].type = CAM_CAPTURE_BRACKETING;
        frame_config.configs[i].hdr_mode.mode = mode;
        frame_config.configs[i].hdr_mode.values =
                m_AEBracketingClient.values[i];
        LOGD("exp values %d", (int)m_AEBracketingClient.values[i]);
    }

    LOGH("num_frame = %d X", burstCount);
    return rc;
}

/*===========================================================================
 * FUNCTION   : configureLowLight
 *
 * DESCRIPTION: configure low light frame capture use case.
 *
 * PARAMETERS :
 *    @frame_config : output configuration structure to fill in.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::configureLowLight(cam_capture_frame_config_t &frame_config)
{
    int32_t rc = NO_ERROR;

    frame_config.num_batch = 1;
    frame_config.configs[0].num_frames = getNumOfSnapshots();
    frame_config.configs[0].type = CAM_CAPTURE_LOW_LIGHT;
    frame_config.configs[0].low_light_mode = CAM_LOW_LIGHT_ON;
    LOGH("Snapshot Count: %d", frame_config.configs[0].num_frames);
    return rc;
}

/*===========================================================================
 * FUNCTION   : configureManualCapture
 *
 * DESCRIPTION: configure manual capture.
 *
 * PARAMETERS :
 *    @frame_config : output configaration structure to fill in.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::configureManualCapture(cam_capture_frame_config_t &frame_config)
{
    int32_t rc = NO_ERROR;
    uint32_t i = 0;

    LOGD("E");
    if (getManualCaptureMode()) {
        frame_config.num_batch = 1;
        for (i = 0; i < frame_config.num_batch; i++) {
            frame_config.configs[i].num_frames = getNumOfSnapshots();
            frame_config.configs[i].type = CAM_CAPTURE_MANUAL_3A;
            if (m_expTime != 0) {
                frame_config.configs[i].manual_3A_mode.exp_mode = CAM_SETTINGS_TYPE_ON;
                frame_config.configs[i].manual_3A_mode.exp_time = m_expTime;
            } else {
                frame_config.configs[i].manual_3A_mode.exp_mode = CAM_SETTINGS_TYPE_AUTO;
                frame_config.configs[i].manual_3A_mode.exp_time = 0;
            }

            if (m_isoValue != 0) {
                frame_config.configs[i].manual_3A_mode.iso_mode = CAM_SETTINGS_TYPE_ON;
                frame_config.configs[i].manual_3A_mode.iso_value = m_isoValue;
            } else {
                frame_config.configs[i].manual_3A_mode.iso_mode = CAM_SETTINGS_TYPE_AUTO;
                frame_config.configs[i].manual_3A_mode.iso_value = 0;
            }
        }
    }
    LOGD("X: batch cnt = %d", frame_config.num_batch);
    return rc;
}

/*===========================================================================
 * FUNCTION   : configFrameCapture
 *
 * DESCRIPTION: configuration for ZSL special captures (FLASH/HDR etc)
 *
 * PARAMETERS :
 *   @commitSettings : flag to enable or disable commit this this settings
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::configFrameCapture(bool commitSettings)
{
    int32_t rc = NO_ERROR;
    int32_t value;

    memset(&m_captureFrameConfig, 0, sizeof(cam_capture_frame_config_t));

    if (commitSettings) {
        if(initBatchUpdate(m_pParamBuf) < 0 ) {
            LOGE("Failed to initialize group update table");
            return BAD_TYPE;
        }
    }

    if (isHDREnabled() || m_bAeBracketingEnabled || m_bAFBracketingOn ||
          m_bOptiZoomOn || m_bReFocusOn || getManualCaptureMode()) {
        value = CAM_FLASH_MODE_OFF;
    } else if (isChromaFlashEnabled()) {
        value = CAM_FLASH_MODE_ON;
    } else {
        value = mFlashValue;
    }

    if (m_LowLightLevel && (value != CAM_FLASH_MODE_ON)) {
        configureLowLight (m_captureFrameConfig);

        //Added reset capture type as a last batch for back-end to restore settings.
        int32_t batch_count = m_captureFrameConfig.num_batch;
        m_captureFrameConfig.configs[batch_count].type = CAM_CAPTURE_RESET;
        m_captureFrameConfig.configs[batch_count].num_frames = 0;
        m_captureFrameConfig.num_batch++;
    } else if (value != CAM_FLASH_MODE_OFF) {
        configureFlash(m_captureFrameConfig);
    } else if(isHDREnabled()) {
        configureHDRBracketing (m_captureFrameConfig);
    } else if(isAEBracketEnabled()) {
        configureAEBracketing (m_captureFrameConfig);
    } else if (getManualCaptureMode() >= CAM_MANUAL_CAPTURE_TYPE_2){
        rc = configureManualCapture (m_captureFrameConfig);
        //Added reset capture type as a last batch for back-end to restore settings.
        int32_t batch_count = m_captureFrameConfig.num_batch;
        m_captureFrameConfig.configs[batch_count].type = CAM_CAPTURE_RESET;
        m_captureFrameConfig.configs[batch_count].num_frames = 0;
        m_captureFrameConfig.num_batch++;
    }

    rc = ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_CAPTURE_FRAME_CONFIG,
            (cam_capture_frame_config_t)m_captureFrameConfig);
    if (rc != NO_ERROR) {
        rc = BAD_VALUE;
        LOGE("Failed to set capture settings");
        return rc;
    }

    if (commitSettings) {
        rc = commitSetBatch();
        if (rc != NO_ERROR) {
            LOGE("Failed to commit parameters");
            return rc;
        }
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : resetFrameCapture
 *
 * DESCRIPTION: reset special captures settings(FLASH/HDR etc)
 *
 * PARAMETERS :
 *   @commitSettings : flag to enable or disable commit this this settings
 *   @lowLightEnabled: flag to indicate if low light scene detected
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::resetFrameCapture(bool commitSettings, bool lowLightEnabled)
{
    int32_t rc = NO_ERROR;
    memset(&m_captureFrameConfig, 0, sizeof(cam_capture_frame_config_t));

    if (commitSettings) {
        if(initBatchUpdate(m_pParamBuf) < 0 ) {
            LOGE("Failed to initialize group update table");
            return BAD_TYPE;
        }
    }

    if (isHDREnabled() || isAEBracketEnabled()) {
        rc = setToneMapMode(true, true);
        if (rc != NO_ERROR) {
            LOGH("Failed to enable tone map during HDR/AEBracketing");
        }
        rc = stopAEBracket();
    } else if ((isChromaFlashEnabled()) || (mFlashValue != CAM_FLASH_MODE_OFF)
            || (lowLightEnabled == true)) {
        rc = setToneMapMode(true, false);
        if (rc != NO_ERROR) {
            LOGH("Failed to enable tone map during chroma flash");
        }

        rc = setCDSMode(mCds_mode, false);
        if (rc != NO_ERROR) {
            LOGE("Failed to configure csd mode");
            return rc;
        }
    }

    rc = ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_CAPTURE_FRAME_CONFIG,
            (cam_capture_frame_config_t)m_captureFrameConfig);
    if (rc != NO_ERROR) {
        rc = BAD_VALUE;
        LOGE("Failed to set capture settings");
        return rc;
    }

    if (commitSettings) {
        rc = commitSetBatch();
        if (rc != NO_ERROR) {
            LOGE("Failed to commit parameters");
            return rc;
        }
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : setAecLock
 *
 * DESCRIPTION: set AEC lock value
 *
 * PARAMETERS :
 *   @aecLockStr : AEC lock value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setAecLock(const char *aecLockStr)
{
    if (aecLockStr != NULL) {
        int32_t value = lookupAttr(TRUE_FALSE_MODES_MAP, PARAM_MAP_SIZE(TRUE_FALSE_MODES_MAP),
                aecLockStr);
        if (value != NAME_NOT_FOUND) {
            LOGH("Setting AECLock value %s", aecLockStr);
            updateParamEntry(KEY_AUTO_EXPOSURE_LOCK, aecLockStr);
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf,
                    CAM_INTF_PARM_AEC_LOCK, (uint32_t)value)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid AECLock value: %s",
        (aecLockStr == NULL) ? "NULL" : aecLockStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setAwbLock
 *
 * DESCRIPTION: set AWB lock value
 *
 * PARAMETERS :
 *   @awbLockStr : AWB lock value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setAwbLock(const char *awbLockStr)
{
    if (awbLockStr != NULL) {
        int32_t value = lookupAttr(TRUE_FALSE_MODES_MAP, PARAM_MAP_SIZE(TRUE_FALSE_MODES_MAP),
                awbLockStr);
        if (value != NAME_NOT_FOUND) {
            LOGH("Setting AWBLock value %s", awbLockStr);
            updateParamEntry(KEY_AUTO_WHITEBALANCE_LOCK, awbLockStr);
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf,
                    CAM_INTF_PARM_AWB_LOCK, (uint32_t)value)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid AWBLock value: %s", (awbLockStr == NULL) ? "NULL" : awbLockStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setMCEValue
 *
 * DESCRIPTION: set memory color enhancement value
 *
 * PARAMETERS :
 *   @mceStr : MCE value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setMCEValue(const char *mceStr)
{
    if (mceStr != NULL) {
        int32_t value = lookupAttr(ENABLE_DISABLE_MODES_MAP,
                PARAM_MAP_SIZE(ENABLE_DISABLE_MODES_MAP), mceStr);
        if (value != NAME_NOT_FOUND) {
            LOGH("Setting AWBLock value %s", mceStr);
            updateParamEntry(KEY_QC_MEMORY_COLOR_ENHANCEMENT, mceStr);
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_MCE, value)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid MCE value: %s", (mceStr == NULL) ? "NULL" : mceStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setTintlessValue
 *
 * DESCRIPTION: enable/disable tintless from user setting
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setTintlessValue(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_TINTLESS_ENABLE);
    const char *prev_str = get(KEY_QC_TINTLESS_ENABLE);
    char prop[PROPERTY_VALUE_MAX];

    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.tintless", prop, VALUE_ENABLE);
    if (str != NULL) {
        if (prev_str == NULL ||
            strcmp(str, prev_str) != 0) {
            return setTintlessValue(str);
        }
    } else {
        if (prev_str == NULL ||
            strcmp(prev_str, prop) != 0 ) {
            setTintlessValue(prop);
        }
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setTintless
 *
 * DESCRIPTION: set tintless mode
 *
 * PARAMETERS :
 *   @enable : 1 = enable, 0 = disable
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
void QCameraParameters::setTintless(bool enable)
{
    if (enable) {
        setTintlessValue(VALUE_ENABLE);
    } else {
        setTintlessValue(VALUE_DISABLE);
    }
}

/*===========================================================================
 * FUNCTION   : setTintlessValue
 *
 * DESCRIPTION: set tintless value
 *
 * PARAMETERS :
 *   @tintStr : Tintless value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setTintlessValue(const char *tintStr)
{
    if (tintStr != NULL) {
        int32_t value = lookupAttr(ENABLE_DISABLE_MODES_MAP,
                PARAM_MAP_SIZE(ENABLE_DISABLE_MODES_MAP), tintStr);
        if (value != NAME_NOT_FOUND) {
            LOGH("Setting Tintless value %s", tintStr);
            updateParamEntry(KEY_QC_TINTLESS_ENABLE, tintStr);
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_TINTLESS, value)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid Tintless value: %s", (tintStr == NULL) ? "NULL" : tintStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setCDSMode
 *
 * DESCRIPTION: Set CDS mode
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setCDSMode(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_CDS_MODE);
    const char *prev_str = get(KEY_QC_CDS_MODE);
    const char *video_str = params.get(KEY_QC_VIDEO_CDS_MODE);
    const char *video_prev_str = get(KEY_QC_VIDEO_CDS_MODE);
    int32_t rc = NO_ERROR;

    if (m_bRecordingHint_new == true) {
        if (video_str) {
            if ((video_prev_str == NULL) || (strcmp(video_str, video_prev_str) != 0)) {
                int32_t cds_mode = lookupAttr(CDS_MODES_MAP, PARAM_MAP_SIZE(CDS_MODES_MAP),
                        video_str);
                if (cds_mode != NAME_NOT_FOUND) {
                    updateParamEntry(KEY_QC_VIDEO_CDS_MODE, video_str);
                    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_CDS_MODE, cds_mode)) {
                        LOGE("Failed CDS MODE to update table");
                        rc = BAD_VALUE;
                    } else {
                        LOGD("Set CDS in video mode = %d", cds_mode);
                        mCds_mode = cds_mode;
                        m_bNeedRestart = true;
                    }
                } else {
                    LOGE("Invalid argument for video CDS MODE %d",  cds_mode);
                    rc = BAD_VALUE;
                }
            }
        } else {
            char video_prop[PROPERTY_VALUE_MAX];
            memset(video_prop, 0, sizeof(video_prop));
            property_get("persist.camera.video.CDS", video_prop, CDS_MODE_OFF);
            int32_t cds_mode = lookupAttr(CDS_MODES_MAP, PARAM_MAP_SIZE(CDS_MODES_MAP),
                    video_prop);
            if (cds_mode != NAME_NOT_FOUND) {
                updateParamEntry(KEY_QC_VIDEO_CDS_MODE, video_prop);
                if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_CDS_MODE, cds_mode)) {
                    LOGE("Failed CDS MODE to update table");
                    rc = BAD_VALUE;
                } else {
                    LOGD("Set CDS in video mode from setprop = %d", cds_mode);
                    mCds_mode = cds_mode;
                }
            } else {
                LOGE("Invalid prop for video CDS MODE %d",  cds_mode);
                rc = BAD_VALUE;
            }
        }
    } else {
        if (str) {
            if ((prev_str == NULL) || (strcmp(str, prev_str) != 0)) {
                int32_t cds_mode = lookupAttr(CDS_MODES_MAP, PARAM_MAP_SIZE(CDS_MODES_MAP),
                        str);
                if (cds_mode != NAME_NOT_FOUND) {
                    updateParamEntry(KEY_QC_CDS_MODE, str);
                    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_CDS_MODE, cds_mode)) {
                        LOGE("Failed CDS MODE to update table");
                        rc = BAD_VALUE;
                    } else {
                        LOGD("Set CDS in capture mode = %d", cds_mode);
                        mCds_mode = cds_mode;
                        m_bNeedRestart = true;
                    }
                } else {
                    LOGE("Invalid argument for snapshot CDS MODE %d",  cds_mode);
                    rc = BAD_VALUE;
                }
            }
        } else {
            char prop[PROPERTY_VALUE_MAX];
            memset(prop, 0, sizeof(prop));
            property_get("persist.camera.CDS", prop, CDS_MODE_OFF);
            int32_t cds_mode = lookupAttr(CDS_MODES_MAP, PARAM_MAP_SIZE(CDS_MODES_MAP),
                    prop);
            if (cds_mode != NAME_NOT_FOUND) {
                updateParamEntry(KEY_QC_CDS_MODE, prop);
                if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_CDS_MODE, cds_mode)) {
                    LOGE("Failed CDS MODE to update table");
                    rc = BAD_VALUE;
                } else {
                    LOGD("Set CDS in snapshot mode from setprop = %d", cds_mode);
                    mCds_mode = cds_mode;
                }
            } else {
                LOGE("Invalid prop for snapshot CDS MODE %d",  cds_mode);
                rc = BAD_VALUE;
            }
        }
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : setInitialExposureIndex
 *
 * DESCRIPTION: Set initial exposure index value
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setInitialExposureIndex(const QCameraParameters& params)
{
    int32_t rc = NO_ERROR;
    int value = -1;
    const char *str = params.get(KEY_QC_INITIAL_EXPOSURE_INDEX);
    const char *prev_str = get(KEY_QC_INITIAL_EXPOSURE_INDEX);
    if (str) {
        if ((prev_str == NULL) || (strcmp(str, prev_str) != 0)) {
            value = atoi(str);
            LOGD("Set initial exposure index value from param = %d", value);
            if (value >= 0) {
                updateParamEntry(KEY_QC_INITIAL_EXPOSURE_INDEX, str);
            }
        }
    } else {
        char prop[PROPERTY_VALUE_MAX];
        memset(prop, 0, sizeof(prop));
        property_get("persist.camera.initial.exp.val", prop, "");
        if ((strlen(prop) > 0) &&
                ( (prev_str == NULL) || (strcmp(prop, prev_str) != 0))) {
            value = atoi(prop);
            LOGD("Set initial exposure index value from setprop = %d", value);
            if (value >= 0) {
                updateParamEntry(KEY_QC_INITIAL_EXPOSURE_INDEX, prop);
            }
        }
    }

    if (value >= 0) {
        if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf,
                CAM_INTF_PARM_INITIAL_EXPOSURE_INDEX, (uint32_t)value)) {
            LOGE("Failed to update initial exposure index value");
            rc = BAD_VALUE;
        }
    } else {
        LOGD("Invalid value for initial exposure index value %d", value);
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : setInstantCapture
 *
 * DESCRIPTION: Set Instant Capture related params
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setInstantCapture(const QCameraParameters& params)
{
    int32_t rc = NO_ERROR;
    int value = -1;
    // Check for instant capture, this will enable instant AEC as well.
    // This param will trigger the instant AEC param to backend
    // And also will be useful for instant capture.
    const char *str = params.get(KEY_QC_INSTANT_CAPTURE);
    const char *prev_str = get(KEY_QC_INSTANT_CAPTURE);
    if (str) {
        if ((prev_str == NULL) || (strcmp(str, prev_str) != 0)) {
            value = lookupAttr(INSTANT_CAPTURE_MODES_MAP,
                    PARAM_MAP_SIZE(INSTANT_CAPTURE_MODES_MAP), str);
            LOGD("Set instant Capture from param = %d", value);
            if(value != NAME_NOT_FOUND) {
                updateParamEntry(KEY_QC_INSTANT_CAPTURE, str);
            } else {
                LOGE("Invalid value for instant capture %s", str);
                return BAD_VALUE;
            }
        }
    } else {
        char prop[PROPERTY_VALUE_MAX];
        memset(prop, 0, sizeof(prop));
        property_get("persist.camera.instant.capture", prop, KEY_QC_INSTANT_CAPTURE_DISABLE);
        if ((prev_str == NULL) || (strcmp(prop, prev_str) != 0)) {
            value = lookupAttr(INSTANT_CAPTURE_MODES_MAP,
                    PARAM_MAP_SIZE(INSTANT_CAPTURE_MODES_MAP), prop);
            LOGD("Set instant capture from setprop = %d", value);
            if (value != NAME_NOT_FOUND) {
                updateParamEntry(KEY_QC_INSTANT_CAPTURE, prop);
            } else {
                LOGE("Invalid value for instant capture %s", prop);
                return BAD_VALUE;
            }
        }
    }

    // Set instant AEC param to the backend for either instant capture or instant AEC
    // 0 - disbale (normal AEC)
    // 1 - Aggressive AEC (algo used in backend)
    // 2 - Fast AEC (algo used in backend)
    if (value != NAME_NOT_FOUND && value != -1) {
        m_bInstantCapture = (value > 0)? true : false;
        setInstantAEC((uint8_t)value, false);
    }


    // get frame aec bound value from setprop.
    // This value indicates the number of frames, camera interface
    // will wait for getting the instant capture frame.
    // Default value set to 7.
    // This value also indicates the number of frames, that HAL
    // will not display and will not send preview frames to app
    // This will be applicable only if instant capture is set.
    if (m_bInstantCapture) {
        char prop[PROPERTY_VALUE_MAX];
        memset(prop, 0, sizeof(prop));
        property_get("persist.camera.ae.capture.bound", prop, "7");
        int32_t frame_bound = atoi(prop);
        if (frame_bound >= 0) {
            mAecFrameBound = (uint8_t)frame_bound;
        } else {
            LOGE("Invalid prop for aec frame bound %d", frame_bound);
            rc = BAD_VALUE;
        }
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : setInstantAEC
 *
 * DESCRIPTION: Set Instant AEC related params
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setInstantAEC(const QCameraParameters& params)
{
    int32_t rc = NO_ERROR;
    int value = -1;

    // Check for instant AEC only when instant capture is not enabled.
    // Instant capture already takes care of the instant AEC as well.
    if (!m_bInstantCapture) {
        // Check for instant AEC. Instant AEC will only enable fast AEC.
        // It will not enable instant capture.
        // This param will trigger the instant AEC param to backend
        // Instant AEC param is session based param,
        // the param change will be applicable for next camera open/close session.
        const char *str = params.get(KEY_QC_INSTANT_AEC);
        const char *prev_str = get(KEY_QC_INSTANT_AEC);
        if (str) {
            if ((prev_str == NULL) || (strcmp(str, prev_str) != 0)) {
                value = lookupAttr(INSTANT_AEC_MODES_MAP,
                        PARAM_MAP_SIZE(INSTANT_AEC_MODES_MAP), str);
                LOGD("Set instant AEC from param = %d", value);
                if(value != NAME_NOT_FOUND) {
                    updateParamEntry(KEY_QC_INSTANT_AEC, str);
                } else {
                    LOGE("Invalid value for instant AEC %s", str);
                    return BAD_VALUE;
                }
            }
        } else {
            char prop[PROPERTY_VALUE_MAX];
            memset(prop, 0, sizeof(prop));
            property_get("persist.camera.instant.aec", prop, KEY_QC_INSTANT_AEC_DISABLE);
            if ((prev_str == NULL) || (strcmp(prop, prev_str) != 0)) {
                value = lookupAttr(INSTANT_AEC_MODES_MAP,
                        PARAM_MAP_SIZE(INSTANT_AEC_MODES_MAP), prop);
                LOGD("Set instant AEC from setprop = %d", value);
                if(value != NAME_NOT_FOUND) {
                    updateParamEntry(KEY_QC_INSTANT_AEC, prop);
                } else {
                    LOGE("Invalid value for instant AEC %s", prop);
                    return BAD_VALUE;
                }
            }
        }

        // Set instant AEC param to the backend for either instant capture or instant AEC
        // 0 - disbale (normal AEC)
        // 1 - Aggressive AEC (algo used in backend)
        // 2 - Fast AEC (algo used in backend)
        if (value != NAME_NOT_FOUND && value != -1) {
            setInstantAEC((uint8_t)value, false);
        }

    }

    // get frame aec preview skip count from setprop.
    // This value indicates the number of frames, that HAL
    // will not display and will not send preview frames to app
    // Default value set to 7.
    // This will be applicable only if instant aec is set.
    if (m_bInstantAEC) {
        char prop[PROPERTY_VALUE_MAX];
        memset(prop, 0, sizeof(prop));
        property_get("persist.camera.ae.instant.bound", prop, "7");
        int32_t aec_frame_skip_cnt = atoi(prop);
        if (aec_frame_skip_cnt >= 0) {
            mAecSkipDisplayFrameBound = (uint8_t)aec_frame_skip_cnt;
        } else {
            LOGE("Invalid prop for aec frame bound %d", aec_frame_skip_cnt);
            rc = BAD_VALUE;
        }
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : setDISValue
 *
 * DESCRIPTION: set DIS value
 *
 * PARAMETERS :
 *   @disStr : DIS value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setDISValue(const char *disStr)
{
    if (disStr != NULL) {
        int32_t value = lookupAttr(ENABLE_DISABLE_MODES_MAP,
                PARAM_MAP_SIZE(ENABLE_DISABLE_MODES_MAP), disStr);
        if (value != NAME_NOT_FOUND) {
            //For some IS types (like EIS 2.0), when DIS value is changed, we need to restart
            //preview because of topology change in backend. But, for now, restart preview
            //for all IS types.
            m_bNeedRestart = true;
            LOGH("Setting DIS value %s", disStr);
            updateParamEntry(KEY_QC_DIS, disStr);
            if (!(strcmp(disStr,"enable"))) {
                m_bDISEnabled = true;
            } else {
                m_bDISEnabled = false;
            }
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_DIS_ENABLE, value)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid DIS value: %s", (disStr == NULL) ? "NULL" : disStr);
    m_bDISEnabled = false;
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : updateOisValue
 *
 * DESCRIPTION: update OIS value
 *
 * PARAMETERS :
 *   @oisValue : OIS value TRUE/FALSE
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::updateOisValue(bool oisValue)
{
    uint8_t enable = 0;
    int32_t rc = NO_ERROR;

    // Check for OIS disable
    char ois_prop[PROPERTY_VALUE_MAX];
    memset(ois_prop, 0, sizeof(ois_prop));
    property_get("persist.camera.ois.disable", ois_prop, "0");
    uint8_t ois_disable = (uint8_t)atoi(ois_prop);

    //Enable OIS if it is camera mode or Camcoder 4K mode
    if (!m_bRecordingHint || (is4k2kVideoResolution() && m_bRecordingHint)) {
        enable = 1;
        LOGH("Valid OIS mode!! ");
    }
    // Disable OIS if setprop is set
    if (ois_disable || !oisValue) {
        //Disable OIS
        enable = 0;
        LOGH("Disable OIS mode!! ois_disable(%d) oisValue(%d)",
                 ois_disable, oisValue);

    }
    m_bOISEnabled = enable;
    if (m_bOISEnabled) {
        updateParamEntry(KEY_QC_OIS, VALUE_ENABLE);
    } else {
        updateParamEntry(KEY_QC_OIS, VALUE_DISABLE);
    }

    if (initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    LOGH("Sending OIS mode (%d)", enable);
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_META_LENS_OPT_STAB_MODE, enable)) {
        LOGE("Failed to update table");
        return BAD_VALUE;
    }

    rc = commitSetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to parameter changes");
        return rc;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : setHighFrameRate
 *
 * DESCRIPTION: set high frame rate
 *
 * PARAMETERS :
 *   @hfrMode : HFR mode
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setHighFrameRate(const int32_t hfrMode)
{
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_HFR, hfrMode)) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setLensShadeValue
 *
 * DESCRIPTION: set lens shade value
 *
 * PARAMETERS :
 *   @lensSahdeStr : lens shade value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setLensShadeValue(const char *lensShadeStr)
{
    if (lensShadeStr != NULL) {
        int32_t value = lookupAttr(ENABLE_DISABLE_MODES_MAP,
                PARAM_MAP_SIZE(ENABLE_DISABLE_MODES_MAP), lensShadeStr);
        if (value != NAME_NOT_FOUND) {
            LOGH("Setting LensShade value %s", lensShadeStr);
            updateParamEntry(KEY_QC_LENSSHADE, lensShadeStr);
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_ROLLOFF, value)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid LensShade value: %s",
          (lensShadeStr == NULL) ? "NULL" : lensShadeStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setExposureCompensation
 *
 * DESCRIPTION: set exposure compensation value
 *
 * PARAMETERS :
 *   @expComp : exposure compensation value
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setExposureCompensation(int expComp)
{
    char val[16];
    snprintf(val, sizeof(val), "%d", expComp);
    updateParamEntry(KEY_EXPOSURE_COMPENSATION, val);

    // Don't need to pass step as part of setParameter because
    // camera daemon is already aware of it.
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_EXPOSURE_COMPENSATION, expComp)) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setWhiteBalance
 *
 * DESCRIPTION: set white balance mode
 *
 * PARAMETERS :
 *   @wbStr   : white balance mode value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setWhiteBalance(const char *wbStr)
{
    if (wbStr != NULL) {
        int32_t value = lookupAttr(WHITE_BALANCE_MODES_MAP,
                PARAM_MAP_SIZE(WHITE_BALANCE_MODES_MAP), wbStr);
        if (value != NAME_NOT_FOUND) {
            LOGH("Setting WhiteBalance value %s", wbStr);
            updateParamEntry(KEY_WHITE_BALANCE, wbStr);
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_WHITE_BALANCE, value)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid WhiteBalance value: %s", (wbStr == NULL) ? "NULL" : wbStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setWBManualCCT
 *
 * DESCRIPTION: set setWBManualCCT time
 *
 * PARAMETERS :
 *   @cctStr : string of wb cct, range (2000, 8000) in K.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t  QCameraParameters::setWBManualCCT(const char *cctStr)
{
    if (cctStr != NULL) {
        int32_t cctVal = atoi(cctStr);
        int32_t minCct = m_pCapability->min_wb_cct; /* 2000K */
        int32_t maxCct = m_pCapability->max_wb_cct; /* 8000K */

        if (cctVal >= minCct && cctVal <= maxCct) {
            LOGH(", cct value: %d", cctVal);
            updateParamEntry(KEY_QC_WB_MANUAL_CCT, cctStr);
            cam_manual_wb_parm_t manual_wb;
            manual_wb.type = CAM_MANUAL_WB_MODE_CCT;
            manual_wb.cct = cctVal;
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_WB_MANUAL, manual_wb)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }

    LOGE("Invalid cct, value: %s",
            (cctStr == NULL) ? "NULL" : cctStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : updateAWBParams
 *
 * DESCRIPTION: update CCT parameters key
 *
 * PARAMETERS :
 *   @awb_params : WB parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::updateAWBParams(cam_awb_params_t &awb_params)
{
    //check and update CCT
    int32_t prev_cct = getInt(KEY_QC_WB_MANUAL_CCT);
    if (prev_cct != awb_params.cct_value) {
        LOGD("update current cct value. old:%d, now:%d",
                prev_cct, awb_params.cct_value);
        set(KEY_QC_WB_MANUAL_CCT, awb_params.cct_value);
    }

    //check and update WB gains
    const char *prev_gains = get(KEY_QC_MANUAL_WB_GAINS);
    char gainStr[30];
    snprintf(gainStr, sizeof(gainStr), "%f,%f,%f", awb_params.rgb_gains.r_gain,
        awb_params.rgb_gains.g_gain, awb_params.rgb_gains.b_gain);

    if (prev_gains == NULL || strcmp(prev_gains, gainStr)) {
        set(KEY_QC_MANUAL_WB_GAINS, gainStr);
        LOGD("update currernt RGB gains: old %s new %s", prev_gains, gainStr);
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : parseGains
 *
 * DESCRIPTION: parse WB gains
 *
 * PARAMETERS :
 *   @gainStr : WB result string
 *   @r_gain  : WB red gain
 *   @g_gain  : WB green gain
 *   @b_gain  : WB blue gain
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::parseGains(const char *gainStr, double &r_gain,
                                          double &g_gain, double &b_gain)
{
    int32_t rc = NO_ERROR;
    char *saveptr = NULL;
    size_t gains_size = strlen(gainStr) + 1;
    char* gains = (char*) calloc(1, gains_size);
    if (NULL == gains) {
        LOGE("No memory for gains");
        return NO_MEMORY;
    }
    strlcpy(gains, gainStr, gains_size);
    char *token = strtok_r(gains, ",", &saveptr);

    if (NULL != token) {
        r_gain = (float) atof(token);
        token = strtok_r(NULL, ",", &saveptr);
    }

    if (NULL != token) {
        g_gain = (float) atof(token);
        token = strtok_r(NULL, ",", &saveptr);
    }

    if (NULL != token) {
        b_gain = (float) atof(token);
    } else {
        LOGE("Malformed string for gains");
        rc = BAD_VALUE;
    }

    free(gains);
    return rc;
}

/*===========================================================================
 * FUNCTION   : setManualWBGains
 *
 * DESCRIPTION: set manual wb gains for r,g,b
 *
 * PARAMETERS :
 *   @cctStr : string of wb gains, range (1.0, 4.0).
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setManualWBGains(const char *gainStr)
{
    int32_t rc = NO_ERROR;
    if (gainStr != NULL) {
        double r_gain,g_gain,b_gain;
        rc = parseGains(gainStr, r_gain, g_gain, b_gain);
        if (rc != NO_ERROR) {
            return rc;
        }

        double minGain = m_pCapability->min_wb_gain;
        double maxGain = m_pCapability->max_wb_gain;

        if (r_gain >= minGain && r_gain <= maxGain &&
            g_gain >= minGain && g_gain <= maxGain &&
            b_gain >= minGain && b_gain <= maxGain) {
            LOGH(", setting rgb gains: r = %lf g = %lf b = %lf",
                     r_gain, g_gain, b_gain);
            updateParamEntry(KEY_QC_MANUAL_WB_GAINS, gainStr);
            cam_manual_wb_parm_t manual_wb;
            manual_wb.type = CAM_MANUAL_WB_MODE_GAIN;
            manual_wb.gains.r_gain = r_gain;
            manual_wb.gains.g_gain = g_gain;
            manual_wb.gains.b_gain = b_gain;
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_WB_MANUAL, manual_wb)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }

    LOGH("Invalid manual wb gains: %s",
          (gainStr == NULL) ? "NULL" : gainStr);
    return BAD_VALUE;
}

int QCameraParameters::getAutoFlickerMode()
{
    /* Enable Advanced Auto Antibanding where we can set
       any of the following option
       ie. CAM_ANTIBANDING_MODE_AUTO
           CAM_ANTIBANDING_MODE_AUTO_50HZ
           CAM_ANTIBANDING_MODE_AUTO_60HZ
      Currently setting it to default    */
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.set.afd", prop, "3");
    return atoi(prop);
}

/*===========================================================================
 * FUNCTION   : setAntibanding
 *
 * DESCRIPTION: set antibanding value
 *
 * PARAMETERS :
 *   @antiBandingStr : antibanding value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setAntibanding(const char *antiBandingStr)
{
    if (antiBandingStr != NULL) {
        int32_t value = lookupAttr(ANTIBANDING_MODES_MAP, PARAM_MAP_SIZE(ANTIBANDING_MODES_MAP),
                antiBandingStr);
        if (value != NAME_NOT_FOUND) {
            LOGH("Setting AntiBanding value %s", antiBandingStr);
            updateParamEntry(KEY_ANTIBANDING, antiBandingStr);
            if(value == CAM_ANTIBANDING_MODE_AUTO) {
               value = getAutoFlickerMode();
            }
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf,
                    CAM_INTF_PARM_ANTIBANDING, (uint32_t)value)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid AntiBanding value: %s",
          (antiBandingStr == NULL) ? "NULL" : antiBandingStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setFocusAreas
 *
 * DESCRIPTION: set focus areas
 *
 * PARAMETERS :
 *   @focusAreasStr : focus areas value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setFocusAreas(const char *focusAreasStr)
{
    if (m_pCapability->max_num_focus_areas == 0 ||
        focusAreasStr == NULL) {
        LOGD("Parameter string is null");
        return NO_ERROR;
    }

    cam_area_t *areas = (cam_area_t *)malloc(sizeof(cam_area_t) * m_pCapability->max_num_focus_areas);
    if (NULL == areas) {
        LOGE("No memory for areas");
        return NO_MEMORY;
    }
    memset(areas, 0, sizeof(cam_area_t) * m_pCapability->max_num_focus_areas);
    int num_areas_found = 0;
    if (parseCameraAreaString(focusAreasStr,
                              m_pCapability->max_num_focus_areas,
                              areas,
                              num_areas_found) != NO_ERROR) {
        LOGE("Failed to parse the string: %s", focusAreasStr);
        free(areas);
        return BAD_VALUE;
    }

    if (validateCameraAreas(areas, num_areas_found) == false) {
        LOGE("invalid areas specified : %s", focusAreasStr);
        free(areas);
        return BAD_VALUE;
    }

    updateParamEntry(KEY_FOCUS_AREAS, focusAreasStr);

    //for special area string (0, 0, 0, 0, 0), set the num_areas_found to 0,
    //so no action is takenby the lower layer
    if (num_areas_found == 1 &&
        areas[0].rect.left == 0 &&
        areas[0].rect.top == 0 &&
        areas[0].rect.width == 0 &&
        areas[0].rect.height == 0 &&
        areas[0].weight == 0) {
        num_areas_found = 0;
    }

    int previewWidth, previewHeight;
    getPreviewSize(&previewWidth, &previewHeight);
    cam_roi_info_t af_roi_value;
    memset(&af_roi_value, 0, sizeof(cam_roi_info_t));
    af_roi_value.num_roi = (uint8_t)num_areas_found;
    for (int i = 0; i < num_areas_found; i++) {
        LOGH("FocusArea[%d] = (%d, %d, %d, %d)",
               i, (areas[i].rect.top), (areas[i].rect.left),
              (areas[i].rect.width), (areas[i].rect.height));

        // Transform the coords from (-1000, 1000)
        // to (0, previewWidth or previewHeight).
        af_roi_value.roi[i].left =
                (int32_t)(((double)areas[i].rect.left + 1000.0) *
                    ((double)previewWidth / 2000.0));
        af_roi_value.roi[i].top =
                (int32_t)(((double)areas[i].rect.top + 1000.0) *
                    ((double)previewHeight / 2000.0));
        af_roi_value.roi[i].width =
                (int32_t)((double)areas[i].rect.width *
                    (double)previewWidth / 2000.0);
        af_roi_value.roi[i].height =
                (int32_t)((double)areas[i].rect.height *
                    (double)previewHeight / 2000.0);
        af_roi_value.weight[i] = areas[i].weight;
    }
    free(areas);
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_AF_ROI, af_roi_value)) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setMeteringAreas
 *
 * DESCRIPTION: set metering areas value
 *
 * PARAMETERS :
 *   @meteringAreasStr : metering areas value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setMeteringAreas(const char *meteringAreasStr)
{
    if (m_pCapability->max_num_metering_areas == 0 ||
        meteringAreasStr == NULL) {
        LOGD("Parameter string is null");
        return NO_ERROR;
    }

    cam_area_t *areas = (cam_area_t *)malloc(sizeof(cam_area_t) * m_pCapability->max_num_metering_areas);
    if (NULL == areas) {
        LOGE("No memory for areas");
        return NO_MEMORY;
    }
    memset(areas, 0, sizeof(cam_area_t) * m_pCapability->max_num_metering_areas);
    int num_areas_found = 0;
    if (parseCameraAreaString(meteringAreasStr,
                              m_pCapability->max_num_metering_areas,
                              areas,
                              num_areas_found) < 0) {
        LOGE("Failed to parse the string: %s", meteringAreasStr);
        free(areas);
        return BAD_VALUE;
    }

    if (validateCameraAreas(areas, num_areas_found) == false) {
        LOGE("invalid areas specified : %s", meteringAreasStr);
        free(areas);
        return BAD_VALUE;
    }

    updateParamEntry(KEY_METERING_AREAS, meteringAreasStr);

    //for special area string (0, 0, 0, 0, 0), set the num_areas_found to 0,
    //so no action is takenby the lower layer
    if (num_areas_found == 1 &&
        areas[0].rect.left == 0 &&
        areas[0].rect.top == 0 &&
        areas[0].rect.width == 0 &&
        areas[0].rect.height == 0 &&
        areas[0].weight == 0) {
        num_areas_found = 0;
    }
    cam_set_aec_roi_t aec_roi_value;
    int previewWidth, previewHeight;
    getPreviewSize(&previewWidth, &previewHeight);

    memset(&aec_roi_value, 0, sizeof(cam_set_aec_roi_t));
    if (num_areas_found > 0) {
        aec_roi_value.aec_roi_enable = CAM_AEC_ROI_ON;
        aec_roi_value.aec_roi_type = CAM_AEC_ROI_BY_COORDINATE;

        for (int i = 0; i < num_areas_found; i++) {
            LOGH("MeteringArea[%d] = (%d, %d, %d, %d)",
                   i, (areas[i].rect.top), (areas[i].rect.left),
                  (areas[i].rect.width), (areas[i].rect.height));

            // Transform the coords from (-1000, 1000) to
            // (0, previewWidth or previewHeight).
            aec_roi_value.cam_aec_roi_position.coordinate[i].x =
                    (uint32_t)((((double)areas[i].rect.left +
                        (double)areas[i].rect.width / 2.0) + 1000.0) *
                            (double)previewWidth / 2000.0);
            aec_roi_value.cam_aec_roi_position.coordinate[i].y =
                    (uint32_t)((((double)areas[i].rect.top +
                        (double)areas[i].rect.height / 2.0) + 1000.0) *
                            (double)previewHeight / 2000.0);
        }
    } else {
        aec_roi_value.aec_roi_enable = CAM_AEC_ROI_OFF;
    }
    free(areas);
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_AEC_ROI, aec_roi_value)) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}


/*===========================================================================
 * FUNCTION   : isSupportedSensorHdrSize
 *
 * DESCRIPTION: Checks if the requested snapshot size is compatible with currently
 *              configured HDR mode, currently primary target for validation is
 *              zzhdr however this function can be extended in the future to vet
 *              all sensor based HDR configs
 *
 * PARAMETERS :
 *   @params  : CameraParameters object
 *
 * RETURN     : boolean type
 *              True  -- indicates supported config
 *              False -- indicated unsupported config should fallback to other
 *              available HDR modes
 *==========================================================================*/
bool QCameraParameters::isSupportedSensorHdrSize(const QCameraParameters& params)
{
    char value[PROPERTY_VALUE_MAX];
    memset(value, 0, sizeof(value));
    property_get("persist.camera.zzhdr.enable", value, "0");
    uint8_t zzhdr_enable = (uint8_t)atoi(value);

    if (zzhdr_enable) {

        int req_w, req_h;
        params.getPictureSize(&req_w, &req_h);

        // Check if requested w x h is in zzhdr supported list
        for (size_t i = 0; i< m_pCapability->zzhdr_sizes_tbl_cnt; ++i) {

            if (req_w == m_pCapability->zzhdr_sizes_tbl[i].width &&
                    req_h == m_pCapability->zzhdr_sizes_tbl[i].height) {
                LOGD("%s: Found match for %d x %d", __func__, req_w, req_h);
                return true;
            }
        }
        LOGH("%s: %d x %d is not supported for zzhdr mode", __func__, req_w, req_h);
        return false;
    }

    return true;
}

/*===========================================================================
 * FUNCTION   : setSceneMode
 *
 * DESCRIPTION: set scene mode
 *
 * PARAMETERS :
 *   @sceneModeStr : scene mode value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSceneMode(const char *sceneModeStr)
{
    if (sceneModeStr != NULL) {
        int32_t value = lookupAttr(SCENE_MODES_MAP, PARAM_MAP_SIZE(SCENE_MODES_MAP), sceneModeStr);
        if (value != NAME_NOT_FOUND) {
            LOGD("Setting SceneMode %s", sceneModeStr);
            updateParamEntry(KEY_SCENE_MODE, sceneModeStr);
            if (m_bSensorHDREnabled) {
              // Incase of HW HDR mode, we do not update the same as Best shot mode.
              LOGH("H/W HDR mode enabled. Do not set Best Shot Mode");
              return NO_ERROR;
            }
            if (m_bSceneSelection) {
                setSelectedScene((cam_scene_mode_type) value);
            }
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_BESTSHOT_MODE,
                    (uint32_t)value)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid Secene Mode: %s",
           (sceneModeStr == NULL) ? "NULL" : sceneModeStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setSelectableZoneAf
 *
 * DESCRIPTION: set selectable zone AF algorithm
 *
 * PARAMETERS :
 *   @selZoneAFStr : selectable zone AF algorithm value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSelectableZoneAf(const char *selZoneAFStr)
{
    if (selZoneAFStr != NULL) {
        int32_t value = lookupAttr(FOCUS_ALGO_MAP, PARAM_MAP_SIZE(FOCUS_ALGO_MAP), selZoneAFStr);
        if (value != NAME_NOT_FOUND) {
            LOGD("Setting Selectable Zone AF value %s", selZoneAFStr);
            updateParamEntry(KEY_QC_SELECTABLE_ZONE_AF, selZoneAFStr);
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_FOCUS_ALGO_TYPE, value)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid selectable zone af value: %s",
           (selZoneAFStr == NULL) ? "NULL" : selZoneAFStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : isAEBracketEnabled
 *
 * DESCRIPTION: checks if AE bracketing is enabled
 *
 * PARAMETERS :
 *
 * RETURN     : TRUE/FALSE
 *==========================================================================*/
bool QCameraParameters::isAEBracketEnabled()
{
    const char *str = get(KEY_QC_AE_BRACKET_HDR);
    if (str != NULL) {
        if (strcmp(str, AE_BRACKET_OFF) != 0) {
            return true;
        }
    }
    return false;
}

/*===========================================================================
 * FUNCTION   : setAEBracket
 *
 * DESCRIPTION: set AE bracket value
 *
 * PARAMETERS :
 *   @aecBracketStr : AE bracket value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setAEBracket(const char *aecBracketStr)
{
    if (aecBracketStr == NULL) {
        LOGD("setAEBracket with NULL value");
        return NO_ERROR;
    }

    cam_exp_bracketing_t expBracket;
    memset(&expBracket, 0, sizeof(expBracket));

    int value = lookupAttr(BRACKETING_MODES_MAP, PARAM_MAP_SIZE(BRACKETING_MODES_MAP),
            aecBracketStr);
    switch (value) {
    case CAM_EXP_BRACKETING_ON:
        {
            LOGD("EXP_BRACKETING_ON");
            const char *str_val = get(KEY_QC_CAPTURE_BURST_EXPOSURE);
            if ((str_val != NULL) && (strlen(str_val)>0)) {
                expBracket.mode = CAM_EXP_BRACKETING_ON;
                m_bAeBracketingEnabled = true;
                strlcpy(expBracket.values, str_val, MAX_EXP_BRACKETING_LENGTH);
                LOGD("setting Exposure Bracketing value of %s",
                       expBracket.values);
            }
            else {
                /* Apps not set capture-burst-exposures, error case fall into bracketing off mode */
                LOGD("capture-burst-exposures not set, back to HDR OFF mode");
                m_bAeBracketingEnabled = false;
                expBracket.mode = CAM_EXP_BRACKETING_OFF;
            }
        }
        break;
    default:
        {
            m_bAeBracketingEnabled = false;
            LOGH(", EXP_BRACKETING_OFF");
            expBracket.mode = CAM_EXP_BRACKETING_OFF;
        }
        break;
    }

    // Cache client AE bracketing configuration
    memcpy(&m_AEBracketingClient, &expBracket, sizeof(cam_exp_bracketing_t));

    /* save the value*/
    updateParamEntry(KEY_QC_AE_BRACKET_HDR, aecBracketStr);
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : set3ALock
 *
 * DESCRIPTION: enable/disable 3A lock.
 *
 * PARAMETERS :
 *   @lock3A  : lock or unlock
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::set3ALock(bool lock3A)
{
    int32_t rc = NO_ERROR;
    LOGH("Setting Lock %d", lock3A);
    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }
    uint32_t focus_mode = CAM_FOCUS_MODE_AUTO;
    if (lock3A) {
        if (isUbiFocusEnabled() || isUbiRefocus()) {
            //For Ubi focus move focus to infinity.
            focus_mode = CAM_FOCUS_MODE_INFINITY;
        } else if (isOptiZoomEnabled() || isStillMoreEnabled()) {
            //For optizoom and stillmore, set focus as fixed.
            focus_mode = CAM_FOCUS_MODE_FIXED;
        }
    } else {
        // retrieve previous focus value.
        const char *focus = get(KEY_FOCUS_MODE);
        int val = lookupAttr(FOCUS_MODES_MAP, PARAM_MAP_SIZE(FOCUS_MODES_MAP), focus);
        if (val != NAME_NOT_FOUND) {
            focus_mode = (uint32_t) val;
            LOGD("focus mode %s", focus);
        }
    }
    //Lock AWB
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_AWB_LOCK, (uint32_t)lock3A)) {
        return BAD_VALUE;
    }
    //Lock AEC
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_AEC_LOCK, (uint32_t)lock3A)) {
        return BAD_VALUE;
    }
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_FOCUS_MODE, focus_mode)) {
        return BAD_VALUE;
    }

    rc = commitSetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to commit batch");
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : setAndCommitZoom
 *
 * DESCRIPTION: set zoom.
 *
 * PARAMETERS :
 *     @zoom_level : zoom level to set.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setAndCommitZoom(int zoom_level)
{
    LOGH("E");
    int32_t rc = NO_ERROR;
    if (initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_ZOOM, zoom_level)) {
        LOGE("Failed to update table");
        return BAD_VALUE;
    }

    rc = commitSetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to set Flash value");
    }

    mZoomLevel = zoom_level;
    LOGH("X");

    return rc;
}

/*===========================================================================
 * FUNCTION   : isOptiZoomEnabled
 *
 * DESCRIPTION: checks whether optizoom is enabled
 *
 * PARAMETERS :
 *
 * RETURN     : true - enabled, false - disabled
 *
 *==========================================================================*/
bool QCameraParameters::isOptiZoomEnabled()
{
    if (m_bOptiZoomOn && (0 <= mParmZoomLevel)) {
        uint32_t zoom_level = (uint32_t) mParmZoomLevel;
        cam_opti_zoom_t *opti_zoom_settings_need =
                &(m_pCapability->opti_zoom_settings_need);
        uint32_t zoom_threshold = (uint32_t) opti_zoom_settings_need->zoom_threshold;
        LOGH("current zoom level =%u & zoom_threshold =%u",
                 zoom_level, zoom_threshold);

        if (zoom_level >= zoom_threshold) {
            return true;
        }
    }

    return false;
}

/*===========================================================================
 * FUNCTION   : setNoiseReductionMode
 *
 * DESCRIPTION: set noise reduction mode
 *
 * PARAMETERS :
 *   @noiseReductionModeStr : noise reduction mode
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setNoiseReductionMode(const char *noiseReductionModeStr)
{
    LOGH("noiseReductionModeStr = %s", noiseReductionModeStr);
    if (noiseReductionModeStr != NULL) {
        int value = lookupAttr(NOISE_REDUCTION_MODES_MAP, PARAM_MAP_SIZE(NOISE_REDUCTION_MODES_MAP),
                noiseReductionModeStr);
        if (value != NAME_NOT_FOUND) {
            m_bHighQualityNoiseReductionMode =
                    !strncmp(VALUE_HIGH_QUALITY, noiseReductionModeStr, strlen(VALUE_HIGH_QUALITY));
            updateParamEntry(KEY_QC_NOISE_REDUCTION_MODE, noiseReductionModeStr);
            return NO_ERROR;
        }
    }
    LOGE("Invalid noise reduction mode value: %s",
            (noiseReductionModeStr == NULL) ? "NULL" : noiseReductionModeStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : commitAFBracket
 *
 * DESCRIPTION: commit AF Bracket.
 *
 * PARAMETERS :
 *   @AFBracket : AF bracketing configuration
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::commitAFBracket(cam_af_bracketing_t afBracket)
{

    int32_t rc = NO_ERROR;
    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_FOCUS_BRACKETING, afBracket)) {
        LOGE("Failed to update table");
        return BAD_VALUE;
    }

    rc = commitSetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to commit batch");
        return rc;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : setAFBracket
 *
 * DESCRIPTION: set AF bracket value
 *
 * PARAMETERS :
 *   @afBracketStr : AF bracket value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setAFBracket(const char *afBracketStr)
{
    LOGH("afBracketStr =%s",afBracketStr);

    if(afBracketStr != NULL) {
        int value = lookupAttr(AF_BRACKETING_MODES_MAP, PARAM_MAP_SIZE(AF_BRACKETING_MODES_MAP),
                afBracketStr);
        if (value != NAME_NOT_FOUND) {
            m_bAFBracketingOn = (value != 0);
            updateParamEntry(KEY_QC_AF_BRACKET, afBracketStr);

            return NO_ERROR;
        }
    }

    LOGE("Invalid af bracket value: %s",
        (afBracketStr == NULL) ? "NULL" : afBracketStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setReFocus
 *
 * DESCRIPTION: set refocus value
 *
 * PARAMETERS :
 *   @afBracketStr : refocus value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setReFocus(const char *reFocusStr)
{
    LOGH("reFocusStr =%s",reFocusStr);

    if (reFocusStr != NULL) {
        int value = lookupAttr(RE_FOCUS_MODES_MAP, PARAM_MAP_SIZE(RE_FOCUS_MODES_MAP),
                reFocusStr);
        if (value != NAME_NOT_FOUND) {
            m_bReFocusOn = (value != 0);
            updateParamEntry(KEY_QC_RE_FOCUS, reFocusStr);
            return NO_ERROR;
        }
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setChromaFlash
 *
 * DESCRIPTION: set chroma flash value
 *
 * PARAMETERS :
 *   @aecBracketStr : chroma flash value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setChromaFlash(const char *chromaFlashStr)
{
    LOGH("chromaFlashStr =%s",chromaFlashStr);
    if(chromaFlashStr != NULL) {
        int value = lookupAttr(CHROMA_FLASH_MODES_MAP, PARAM_MAP_SIZE(CHROMA_FLASH_MODES_MAP),
                chromaFlashStr);
        if(value != NAME_NOT_FOUND) {
            m_bChromaFlashOn = (value != 0);
            updateParamEntry(KEY_QC_CHROMA_FLASH, chromaFlashStr);

            return NO_ERROR;
        }
    }

    LOGE("Invalid chroma flash value: %s",
        (chromaFlashStr == NULL) ? "NULL" : chromaFlashStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setOptiZoom
 *
 * DESCRIPTION: set opti zoom value
 *
 * PARAMETERS :
 *   @optiZoomStr : opti zoom value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setOptiZoom(const char *optiZoomStr)
{
    LOGH("optiZoomStr =%s",optiZoomStr);
    if(optiZoomStr != NULL) {
        int value = lookupAttr(OPTI_ZOOM_MODES_MAP, PARAM_MAP_SIZE(OPTI_ZOOM_MODES_MAP),
                optiZoomStr);
        if(value != NAME_NOT_FOUND) {
            m_bOptiZoomOn = (value != 0);
            updateParamEntry(KEY_QC_OPTI_ZOOM, optiZoomStr);

            return NO_ERROR;
        }
    }
    LOGE("Invalid opti zoom value: %s",
        (optiZoomStr == NULL) ? "NULL" : optiZoomStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setTruePortrait
 *
 * DESCRIPTION: set true portrait value
 *
 * PARAMETERS :
 *   @optiZoomStr : true portrait value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setTruePortrait(const char *truePortraitStr)
{
    LOGH("truePortraitStr =%s", truePortraitStr);
    if (truePortraitStr != NULL) {
        int value = lookupAttr(TRUE_PORTRAIT_MODES_MAP,
                PARAM_MAP_SIZE(TRUE_PORTRAIT_MODES_MAP),
                truePortraitStr);
        if (value != NAME_NOT_FOUND) {
            m_bTruePortraitOn = (value != 0);
            updateParamEntry(KEY_QC_TRUE_PORTRAIT, truePortraitStr);
            setFaceDetection(m_bFaceDetectionOn, false);
            return NO_ERROR;
        }
    }
    LOGH("Invalid true portrait value: %s",
            (truePortraitStr == NULL) ? "NULL" : truePortraitStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setHDRMode
 *
 * DESCRIPTION: set hdr mode value
 *
 * PARAMETERS :
 *   @hdrModeStr : hdr mode value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setHDRMode(const char *hdrModeStr)
{
    LOGH("hdrModeStr =%s", hdrModeStr);
    if (hdrModeStr != NULL) {
        int value = lookupAttr(HDR_MODES_MAP, PARAM_MAP_SIZE(HDR_MODES_MAP), hdrModeStr);
        if (value != NAME_NOT_FOUND) {
            const char *str = get(KEY_SCENE_MODE);

            m_bHDRModeSensor = !strncmp(hdrModeStr, HDR_MODE_SENSOR, strlen(HDR_MODE_SENSOR));

            updateParamEntry(KEY_QC_HDR_MODE, hdrModeStr);

            // If hdr is already selected, need to deselect it in local cache
            // So the new hdr mode will be applied
            if (str && !strncmp(str, SCENE_MODE_HDR, strlen(SCENE_MODE_HDR))) {
                updateParamEntry(KEY_SCENE_MODE, SCENE_MODE_AUTO);
                m_bNeedRestart = true;
            }

            return NO_ERROR;
        }
    }
    LOGH("Invalid hdr mode value: %s",
            (hdrModeStr == NULL) ? "NULL" : hdrModeStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setSeeMore
 *
 * DESCRIPTION: set see more value
 *
 * PARAMETERS :
 *   @seeMoreStr : see more value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSeeMore(const char *seeMoreStr)
{
    int32_t rc = NO_ERROR;

    LOGH("seeMoreStr =%s", seeMoreStr);
    if (seeMoreStr != NULL) {
        int value = lookupAttr(ON_OFF_MODES_MAP,
                PARAM_MAP_SIZE(ON_OFF_MODES_MAP),
                seeMoreStr);
        if (value != NAME_NOT_FOUND) {
            m_bSeeMoreOn = (value != 0);

            // If SeeMore is enabled, enable StillMore for live snapshot
            // and disable tone map
            if (m_bSeeMoreOn) {
                m_bStillMoreOn = TRUE;
                if (!m_bLtmForSeeMoreEnabled) {
                    rc = setToneMapMode(false, false);
                }
                if (rc != NO_ERROR) {
                    LOGH("Failed to disable tone map during SeeMore");
                }
            } else {
                m_bStillMoreOn = FALSE;
                if (!m_bLtmForSeeMoreEnabled) {
                    rc = setToneMapMode(true, false);
                }
                if (rc != NO_ERROR) {
                    LOGH("Failed to enable tone map during SeeMore");
                }
            }
            updateParamEntry(KEY_QC_SEE_MORE, seeMoreStr);
            return NO_ERROR;
        }
    }
    LOGE("Invalid see more value: %s",
            (seeMoreStr == NULL) ? "NULL" : seeMoreStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setStillMore
 *
 * DESCRIPTION: set still more value
 *
 * PARAMETERS :
 *   @seeMoreStr : still more value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setStillMore(const char *stillMoreStr)
{
    LOGH("stillMoreStr =%s", stillMoreStr);
    if (stillMoreStr != NULL) {
        int value = lookupAttr(STILL_MORE_MODES_MAP, PARAM_MAP_SIZE(STILL_MORE_MODES_MAP),
                stillMoreStr);
        if (value != NAME_NOT_FOUND) {
            m_bStillMoreOn = (value != 0);
            updateParamEntry(KEY_QC_STILL_MORE, stillMoreStr);

            return NO_ERROR;
        }
    }
    LOGE("Invalid still more value: %s",
            (stillMoreStr == NULL) ? "NULL" : stillMoreStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setHDRNeed1x
 *
 * DESCRIPTION: set hdr need 1x value
 *
 * PARAMETERS :
 *   @hdrModeStr : hdr need 1x value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setHDRNeed1x(const char *hdrNeed1xStr)
{
    LOGH("hdrNeed1xStr =%s", hdrNeed1xStr);
    if (hdrNeed1xStr != NULL) {
        int value = lookupAttr(TRUE_FALSE_MODES_MAP, PARAM_MAP_SIZE(TRUE_FALSE_MODES_MAP),
                hdrNeed1xStr);
        if (value != NAME_NOT_FOUND) {
            updateParamEntry(KEY_QC_HDR_NEED_1X, hdrNeed1xStr);
            m_bHDR1xFrameEnabled = !strncmp(hdrNeed1xStr, VALUE_TRUE, strlen(VALUE_TRUE));
            m_bNeedRestart = true;

            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_HDR_NEED_1X,
                    m_bHDR1xFrameEnabled)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }

    LOGH("Invalid hdr need 1x value: %s",
            (hdrNeed1xStr == NULL) ? "NULL" : hdrNeed1xStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setAEBracketing
 *
 * DESCRIPTION: enables AE bracketing
 *
 * PARAMETERS :
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setAEBracketing()
{
    int32_t rc = NO_ERROR;
    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_HDR, m_AEBracketingClient)) {
        LOGE("Failed to update AE bracketing");
        return BAD_VALUE;
    }

    rc = commitSetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to configure AE bracketing");
        return rc;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : setHDRAEBracket
 *
 * DESCRIPTION: enables AE bracketing for HDR
 *
 * PARAMETERS :
 *   @hdrBracket : HDR bracketing configuration
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setHDRAEBracket(cam_exp_bracketing_t hdrBracket)
{
    int32_t rc = NO_ERROR;
    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_HDR, hdrBracket)) {
        LOGE("Failed to update table");
        return BAD_TYPE;
    }

    rc = commitSetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to configure HDR bracketing");
        return rc;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : setCacheVideoBuffers
 *
 * DESCRIPTION: set cache video buffers value
 *
 * PARAMETERS :
 *   @cacheVideoStr : cache video buffer value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setCacheVideoBuffers(const char *cacheVideoBufStr)
{
    if (cacheVideoBufStr != NULL) {
        int8_t cacheVideoBuf = lookupAttr(ENABLE_DISABLE_MODES_MAP,
                PARAM_MAP_SIZE(ENABLE_DISABLE_MODES_MAP), cacheVideoBufStr);
        char prop[PROPERTY_VALUE_MAX];
        memset(prop, 0, sizeof(prop));
        property_get("persist.camera.mem.usecache", prop, "");
        if (strlen(prop) > 0) {
            cacheVideoBuf = atoi(prop);
        }
        if (cacheVideoBuf != NAME_NOT_FOUND) {
            const char *cacheStr = (strlen(prop)>0) ? prop : cacheVideoBufStr;
            LOGD("Setting video buffer %s",
                    (cacheVideoBuf == 0) ? "UnCached" : "Cached");
            return updateParamEntry(KEY_QC_CACHE_VIDEO_BUFFERS, cacheStr);
        }
        LOGE("Cache video buffers not set correctly");
    }
    return BAD_VALUE;
}


/*===========================================================================
 * FUNCTION   : setCacheVideoBuffers
 *
 * DESCRIPTION: Set buffers as Cache/Uncache Memory
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setCacheVideoBuffers(const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_CACHE_VIDEO_BUFFERS);;
    const char *prev_str = get(KEY_QC_CACHE_VIDEO_BUFFERS);

    if (str != NULL) {
        if (prev_str == NULL ||
                strcmp(str, prev_str) != 0) {
            return setCacheVideoBuffers(str);
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : restoreAEBracket
 *
 * DESCRIPTION: restores client AE bracketing configuration after HDR is done
 *
 * PARAMETERS :
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::stopAEBracket()
{
  cam_exp_bracketing_t bracketing;

  bracketing.mode = CAM_EXP_BRACKETING_OFF;

  return setHDRAEBracket(bracketing);
}

/*===========================================================================
 * FUNCTION   : updateFlash
 *
 * DESCRIPTION: restores client flash configuration or disables flash
 *
 * PARAMETERS :
 *   @commitSettings : flag indicating whether settings need to be commited
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::updateFlash(bool commitSettings)
{
    int32_t rc = NO_ERROR;
    int32_t value;

    if (commitSettings) {
      if(initBatchUpdate(m_pParamBuf) < 0 ) {
          LOGE("Failed to initialize group update table");
          return BAD_TYPE;
      }
    }
    // Turn Off Flash if any of the below AOST features are enabled
    if (isHDREnabled() || m_bAeBracketingEnabled || m_bAFBracketingOn ||
          m_bOptiZoomOn || m_bReFocusOn || (m_bStillMoreOn && !m_bSeeMoreOn)) {
        value = CAM_FLASH_MODE_OFF;
    } else if (m_bChromaFlashOn) {
        value = CAM_FLASH_MODE_ON;
    } else {
        value = mFlashValue;
    }

    if (value != mFlashDaemonValue) {
        LOGD("Setting Flash value %d", value);
        if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_LED_MODE, value)) {
            LOGE("Failed to set led mode");
            return BAD_VALUE;
        }
        mFlashDaemonValue = value;
    } else {
        rc = NO_ERROR;
    }

    if (commitSettings) {
        rc = commitSetBatch();
        if (rc != NO_ERROR) {
            LOGE("Failed to configure HDR bracketing");
            return rc;
        }
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : setRedeyeReduction
 *
 * DESCRIPTION: set red eye reduction value
 *
 * PARAMETERS :
 *   @redeyeStr : red eye reduction value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setRedeyeReduction(const char *redeyeStr)
{
    if (redeyeStr != NULL) {
        int32_t value = lookupAttr(ENABLE_DISABLE_MODES_MAP,
                PARAM_MAP_SIZE(ENABLE_DISABLE_MODES_MAP), redeyeStr);
        if (value != NAME_NOT_FOUND) {
            LOGD("Setting RedEye Reduce value %s", redeyeStr);
            updateParamEntry(KEY_QC_REDEYE_REDUCTION, redeyeStr);
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf,
                    CAM_INTF_PARM_REDEYE_REDUCTION, value)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid RedEye Reduce value: %s",
           (redeyeStr == NULL) ? "NULL" : redeyeStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : getDenoiseProcessPlate
 *
 * DESCRIPTION: query denoise process plate
 *
 * PARAMETERS : None
 *
 * RETURN     : NR process plate vlaue
 *==========================================================================*/
cam_denoise_process_type_t
        QCameraParameters::getDenoiseProcessPlate(cam_intf_parm_type_t type)
{
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    cam_denoise_process_type_t processPlate = CAM_WAVELET_DENOISE_CBCR_ONLY;
    if (CAM_INTF_PARM_WAVELET_DENOISE == type) {
        property_get("persist.denoise.process.plates", prop, "");
    } else if (CAM_INTF_PARM_TEMPORAL_DENOISE == type) {
        property_get("persist.tnr.process.plates", prop, "");
    } else {
        LOGW("Type not supported");
        prop[0] = '\0';
    }
    if (strlen(prop) > 0) {
        switch(atoi(prop)) {
        case 0:
            processPlate = CAM_WAVELET_DENOISE_YCBCR_PLANE;
            break;
        case 1:
            processPlate = CAM_WAVELET_DENOISE_CBCR_ONLY;
            break;
        case 2:
            processPlate = CAM_WAVELET_DENOISE_STREAMLINE_YCBCR;
            break;
        case 3:
            processPlate = CAM_WAVELET_DENOISE_STREAMLINED_CBCR;
            break;
        default:
            processPlate = CAM_WAVELET_DENOISE_CBCR_ONLY;
            break;
        }
    }
    return processPlate;
}

/*===========================================================================
 * FUNCTION   : setWaveletDenoise
 *
 * DESCRIPTION: set wavelet denoise value
 *
 * PARAMETERS :
 *   @wnrStr : wavelet denoise value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setWaveletDenoise(const char *wnrStr)
{
    if ((m_pCapability->qcom_supported_feature_mask & CAM_QCOM_FEATURE_DENOISE2D) == 0){
        LOGH("WNR is not supported");
        return NO_ERROR;
    }

    if (wnrStr != NULL) {
        int value = lookupAttr(DENOISE_ON_OFF_MODES_MAP,
                PARAM_MAP_SIZE(DENOISE_ON_OFF_MODES_MAP), wnrStr);
        if (value != NAME_NOT_FOUND) {
            updateParamEntry(KEY_QC_DENOISE, wnrStr);

            cam_denoise_param_t temp;
            memset(&temp, 0, sizeof(temp));
            temp.denoise_enable = (uint8_t)value;
            m_bWNROn = (value != 0);
            if (m_bWNROn) {
                temp.process_plates = getDenoiseProcessPlate(CAM_INTF_PARM_WAVELET_DENOISE);
            }
            LOGD("Denoise enable=%d, plates=%d",
                   temp.denoise_enable, temp.process_plates);
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_WAVELET_DENOISE, temp)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid Denoise value: %s", (wnrStr == NULL) ? "NULL" : wnrStr);
    return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : setRdiMode
 *
 * DESCRIPTION: set rdi mode value
 *
 * PARAMETERS :
 *   @str     : rdi mode value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setRdiMode(const char *str)
{
    LOGD("RDI_DEBUG  rdi mode value: %s", str);

    if (str != NULL) {
        int32_t value = lookupAttr(ENABLE_DISABLE_MODES_MAP,
                PARAM_MAP_SIZE(ENABLE_DISABLE_MODES_MAP), str);
        if (value != NAME_NOT_FOUND) {
            updateParamEntry(KEY_QC_RDI_MODE, str);
            m_bRdiMode = (value == 0) ? false : true;
            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_RDI_MODE, value)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid rdi mode value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}


/*===========================================================================
 * FUNCTION   : setSecureMode
 *
 * DESCRIPTION: set secure mode value
 *
 * PARAMETERS :
 *   @str     : secure mode value string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setSecureMode(const char *str)
{
  LOGD("Secure mode value: %s", str);

  if (str != NULL) {
    int32_t value = lookupAttr(ENABLE_DISABLE_MODES_MAP,
            PARAM_MAP_SIZE(ENABLE_DISABLE_MODES_MAP), str);
    if (value != NAME_NOT_FOUND) {
        updateParamEntry(KEY_QC_SECURE_MODE, str);
        m_bSecureMode = (value == 0)? false : true;
        return NO_ERROR;
    }
  }
  LOGE("Invalid Secure mode value: %s",
     (str == NULL) ? "NULL" : str);
  return BAD_VALUE;
}

/*===========================================================================
 * FUNCTION   : getStreamRotation
 *
 * DESCRIPTION: get stream rotation by its type
 *
 * PARAMETERS :
 *   @streamType        : stream type
 *   @featureConfig     : stream feature config structure
 *   @dim               : stream dimension
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::getStreamRotation(cam_stream_type_t streamType,
                                            cam_pp_feature_config_t &featureConfig,
                                            cam_dimension_t &dim)
{
    int32_t ret = NO_ERROR;
    const char *str = get(KEY_QC_VIDEO_ROTATION);
    int rotationParam = lookupAttr(VIDEO_ROTATION_MODES_MAP,
            PARAM_MAP_SIZE(VIDEO_ROTATION_MODES_MAP), str);
    featureConfig.rotation = ROTATE_0;
    int swapDim = 0;
    switch (streamType) {
        case CAM_STREAM_TYPE_VIDEO:
            switch(rotationParam) {
                case 90:
                    featureConfig.feature_mask |= CAM_QCOM_FEATURE_ROTATION;
                    featureConfig.rotation = ROTATE_90;
                    swapDim = 1;
                    break;
                case 180:
                    featureConfig.feature_mask |= CAM_QCOM_FEATURE_ROTATION;
                    featureConfig.rotation = ROTATE_180;
                    break;
                case 270:
                    featureConfig.feature_mask |= CAM_QCOM_FEATURE_ROTATION;
                    featureConfig.rotation = ROTATE_270;
                    swapDim = 1;
                    break;
                default:
                    featureConfig.rotation = ROTATE_0;
            }
            break;
        case CAM_STREAM_TYPE_PREVIEW:
        case CAM_STREAM_TYPE_POSTVIEW:
        case CAM_STREAM_TYPE_SNAPSHOT:
        case CAM_STREAM_TYPE_RAW:
        case CAM_STREAM_TYPE_METADATA:
        case CAM_STREAM_TYPE_OFFLINE_PROC:
        case CAM_STREAM_TYPE_DEFAULT:
        default:
            break;
    }

    if (swapDim > 0) {
        int w = 0;
        w = dim.width;
        dim.width = dim.height;
        dim.height = w;
    }
    return ret;
}

int32_t QCameraParameters::getStreamSubFormat(cam_stream_type_t streamType,
                            cam_sub_format_type_t &sub_format)
{
    int32_t ret = NO_ERROR;
    sub_format = CAM_FORMAT_SUBTYPE_MAX;

    switch (streamType) {
        case CAM_STREAM_TYPE_RAW: {
          char raw_sub_format[PROPERTY_VALUE_MAX];
          int rawSubFormat;
          memset(raw_sub_format, 0, sizeof(raw_sub_format));
          /*Default value is CAM_FORMAT_SUBTYPE_PDAF_STATS*/
          property_get("persist.camera.raw.subformat", raw_sub_format, "1");
          rawSubFormat = atoi(raw_sub_format);
          sub_format = (cam_sub_format_type_t)rawSubFormat;
          LOGH("Subformat for raw stream = %d", sub_format);
        }
            break;
        default:
            break;
    }
    return ret;
}
/*===========================================================================
 * FUNCTION   : getStreamFormat
 *
 * DESCRIPTION: get stream format by its type
 *
 * PARAMETERS :
 *   @streamType : [input] stream type
 *   @format     : [output] stream format
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::getStreamFormat(cam_stream_type_t streamType,
                                            cam_format_t &format)
{
    int32_t ret = NO_ERROR;
    format = CAM_FORMAT_MAX;
    switch (streamType) {
    case CAM_STREAM_TYPE_PREVIEW:
        if (!isUBWCEnabled()) {
#if VENUS_PRESENT
            cam_dimension_t preview;
            cam_dimension_t video;
            getStreamDimension(CAM_STREAM_TYPE_VIDEO , video);
            getStreamDimension(CAM_STREAM_TYPE_PREVIEW, preview);
            if (getRecordingHintValue() == true &&
                    video.width == preview.width &&
                    video.height == preview.height &&
                    mPreviewFormat == CAM_FORMAT_YUV_420_NV21) {
                format = CAM_FORMAT_YUV_420_NV21_VENUS;
            } else
#endif
            format = mPreviewFormat;
        } else {
            format = mPreviewFormat;
        }
        break;
    case CAM_STREAM_TYPE_POSTVIEW:
    case CAM_STREAM_TYPE_CALLBACK:
        format = mAppPreviewFormat;
        break;
    case CAM_STREAM_TYPE_ANALYSIS:
        cam_analysis_info_t analysisInfo;
        cam_feature_mask_t featureMask;

        featureMask = 0;
        getStreamPpMask(CAM_STREAM_TYPE_ANALYSIS, featureMask);
        ret = getAnalysisInfo(
                ((getRecordingHintValue() == true) && fdModeInVideo()),
                FALSE,
                featureMask,
                &analysisInfo);
        if (ret != NO_ERROR) {
            LOGE("getAnalysisInfo failed, ret = %d", ret);
            return ret;
        }

        if (analysisInfo.hw_analysis_supported &&
                analysisInfo.analysis_format == CAM_FORMAT_Y_ONLY) {
            format = analysisInfo.analysis_format;
        } else {
            if (analysisInfo.hw_analysis_supported) {
                LOGW("Invalid analysis_format %d\n",
                        analysisInfo.analysis_format);
            }
            format = mAppPreviewFormat;
        }
        break;
    case CAM_STREAM_TYPE_SNAPSHOT:
        if ( mPictureFormat == CAM_FORMAT_YUV_422_NV16 ) {
            format = CAM_FORMAT_YUV_422_NV16;
        } else {
            char prop[PROPERTY_VALUE_MAX];
            int snapshotFormat;
            memset(prop, 0, sizeof(prop));
            property_get("persist.camera.snap.format", prop, "0");
            snapshotFormat = atoi(prop);
            if(snapshotFormat == 1) {
                format = CAM_FORMAT_YUV_422_NV61;
            } else {
                format = CAM_FORMAT_YUV_420_NV21;
            }
        }
        break;
    case CAM_STREAM_TYPE_VIDEO:
        if (isUBWCEnabled()) {
            char prop[PROPERTY_VALUE_MAX];
            int pFormat;
            memset(prop, 0, sizeof(prop));
            property_get("persist.camera.video.ubwc", prop, "1");
            pFormat = atoi(prop);
            if (pFormat == 1) {
                format = CAM_FORMAT_YUV_420_NV12_UBWC;
            } else {
                format = CAM_FORMAT_YUV_420_NV21_VENUS;
            }
        } else {
#if VENUS_PRESENT
            format = CAM_FORMAT_YUV_420_NV21_VENUS;
#else
            format = CAM_FORMAT_YUV_420_NV21;
#endif
        }
        break;
    case CAM_STREAM_TYPE_RAW:
        if ((isRdiMode()) || (getofflineRAW())|| (getQuadraCfa())) {
            format = m_pCapability->rdi_mode_stream_fmt;
        } else if (mPictureFormat >= CAM_FORMAT_YUV_RAW_8BIT_YUYV) {
            format = (cam_format_t)mPictureFormat;
        } else {
            char raw_format[PROPERTY_VALUE_MAX];
            int rawFormat;
            memset(raw_format, 0, sizeof(raw_format));
            /*Default value is CAM_FORMAT_BAYER_QCOM_RAW_10BPP_GBRG*/
            property_get("persist.camera.raw.format", raw_format, "17");
            rawFormat = atoi(raw_format);
            format = (cam_format_t)rawFormat;
            LOGH("Raw stream format %d bundled with snapshot",
                    format);
        }
        break;
    case CAM_STREAM_TYPE_OFFLINE_PROC:
        if (getQuadraCfa()) {
            if (m_pCapability->color_arrangement == CAM_FILTER_ARRANGEMENT_BGGR) {
                format = CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_10BPP_BGGR;
            } else if (m_pCapability->color_arrangement == CAM_FILTER_ARRANGEMENT_GBRG) {
                format = CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_10BPP_GBRG;
            } else if (m_pCapability->color_arrangement == CAM_FILTER_ARRANGEMENT_GRBG) {
                format = CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_10BPP_GRBG;
            } else if (m_pCapability->color_arrangement == CAM_FILTER_ARRANGEMENT_RGGB) {
                format = CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_10BPP_RGGB;
            } else {
                LOGW("Unrecognized format set by sensor, setting default");
                format = m_pCapability->quadra_cfa_format;
            }
        }
        break;
    case CAM_STREAM_TYPE_METADATA:
    case CAM_STREAM_TYPE_DEFAULT:
    default:
        break;
    }

    LOGD("Stream type = %d Stream Format = %d", streamType, format);
    return ret;
}

/*===========================================================================
 * FUNCTION   : getFlipMode
 *
 * DESCRIPTION: get flip mode
 *
 * PARAMETERS :
 *   @cam_intf_parm_type_t : [input] stream type
 *
 * RETURN     : int type of flip mode
 *              0 - no filp
 *              1 - FLIP_H
 *              2 - FLIP_V
 *              3 - FLIP_H | FLIP_V
 *==========================================================================*/
int QCameraParameters::getFlipMode(cam_stream_type_t type)
{
    const char *str = NULL;
    int flipMode = 0; // no flip

    switch(type){
    case CAM_STREAM_TYPE_PREVIEW:
        if (!isRdiMode()) {
            str = get(KEY_QC_PREVIEW_FLIP);
        }
        break;
    case CAM_STREAM_TYPE_VIDEO:
        str = get(KEY_QC_VIDEO_FLIP);
        break;
    case CAM_STREAM_TYPE_SNAPSHOT:
    case CAM_STREAM_TYPE_POSTVIEW:
        str = get(KEY_QC_SNAPSHOT_PICTURE_FLIP);
        break;
    default:
        LOGD("No flip mode for stream type %d", type);
        break;
    }

    if(str != NULL){
        //Need give corresponding filp value based on flip mode strings
        int value = lookupAttr(FLIP_MODES_MAP, PARAM_MAP_SIZE(FLIP_MODES_MAP), str);
        if(value != NAME_NOT_FOUND)
            flipMode = value;
        }

    LOGH("the filp mode of stream type %d is %d .", type, flipMode);
    return flipMode;
}

/*===========================================================================
 * FUNCTION   : isSnapshotFDNeeded
 *
 * DESCRIPTION: check whether Face Detection Metadata is needed
 *
 * PARAMETERS : none
 *
 * RETURN     : bool type of status
 *              0 - need
 *              1 - not need
 *==========================================================================*/
bool QCameraParameters::isSnapshotFDNeeded()
{
    return getInt(KEY_QC_SNAPSHOT_FD_DATA);
}

/*===========================================================================
 * FUNCTION   : getStreamDimension
 *
 * DESCRIPTION: get stream dimension by its type
 *
 * PARAMETERS :
 *   @streamType : [input] stream type
 *   @dim        : [output] stream dimension
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::getStreamDimension(cam_stream_type_t streamType,
                                               cam_dimension_t &dim)
{
    int32_t ret = NO_ERROR;
    memset(&dim, 0, sizeof(cam_dimension_t));

    switch (streamType) {
    case CAM_STREAM_TYPE_PREVIEW:
    case CAM_STREAM_TYPE_CALLBACK:
        getPreviewSize(&dim.width, &dim.height);
        break;
    case CAM_STREAM_TYPE_POSTVIEW:
        getPreviewSize(&dim.width, &dim.height);
        break;
    case CAM_STREAM_TYPE_SNAPSHOT:
        if (isPostProcScaling()) {
            getMaxPicSize(dim);
        } else if (getRecordingHintValue()) {
            // live snapshot
            getLiveSnapshotSize(dim);
        } else {
            getPictureSize(&dim.width, &dim.height);
        }
        break;
    case CAM_STREAM_TYPE_VIDEO:
        getVideoSize(&dim.width, &dim.height);
        break;
    case CAM_STREAM_TYPE_RAW:
        //dim = m_pCapability->raw_dim;
        getRawSize(dim);
        break;
    case CAM_STREAM_TYPE_METADATA:
        dim.width = (int32_t)sizeof(metadata_buffer_t);
        dim.height = 1;
        break;
    case CAM_STREAM_TYPE_OFFLINE_PROC:
        if (isPostProcScaling()) {
            if (getRecordingHintValue()) {
                // live snapshot
                getLiveSnapshotSize(dim);
            } else {
                getPictureSize(&dim.width, &dim.height);
            }
        }
        break;
    case CAM_STREAM_TYPE_ANALYSIS:
        cam_dimension_t prv_dim, max_dim;

        /* Analysis stream need aspect ratio as preview stream */
        getPreviewSize(&prv_dim.width, &prv_dim.height);

        cam_analysis_info_t analysisInfo;
        cam_feature_mask_t featureMask;

        featureMask = 0;
        getStreamPpMask(CAM_STREAM_TYPE_ANALYSIS, featureMask);
        ret = getAnalysisInfo(
                ((getRecordingHintValue() == true) && fdModeInVideo()),
                FALSE,
                featureMask,
                &analysisInfo);
        if (ret != NO_ERROR) {
            LOGE("getAnalysisInfo failed, ret = %d", ret);
            return ret;
        }

        max_dim.width = analysisInfo.analysis_max_res.width;
        max_dim.height = analysisInfo.analysis_max_res.height;

        if (prv_dim.width > max_dim.width || prv_dim.height > max_dim.height) {
            double max_ratio, requested_ratio;

            max_ratio = (double)max_dim.width / (double)max_dim.height;
            requested_ratio = (double)prv_dim.width / (double)prv_dim.height;

            if (max_ratio < requested_ratio) {
                dim.width = max_dim.width;
                dim.height = (int32_t)((double)dim.width / requested_ratio);
            } else {
                dim.height = max_dim.height;
                dim.width = (int32_t)((double)max_dim.height * requested_ratio);
            }
            dim.width &= ~0x1;
            dim.height &= ~0x1;
        } else {
            dim.width = prv_dim.width;
            dim.height = prv_dim.height;
        }
      break;
    case CAM_STREAM_TYPE_DEFAULT:
    default:
        LOGE("no dimension for unsupported stream type %d",
               streamType);
        ret = BAD_VALUE;
        break;
    }

    LOGD("Stream type = %d Stream Dimension = %d X %d",
             streamType, dim.width, dim.height);
    return ret;
}

/*===========================================================================
 * FUNCTION   : getParameters
 *
 * DESCRIPTION: Return a C string containing the parameters
 *
 * PARAMETERS : none
 *
 * RETURN     : a string containing parameter pairs
 *==========================================================================*/
char* QCameraParameters::getParameters()
{
    char* strParams = NULL;
    String8 str;

    int cur_width, cur_height;
    //Need take care Scale picture size
    if(m_reprocScaleParam.isScaleEnabled() &&
        m_reprocScaleParam.isUnderScaling()){
        int scale_width, scale_height;

        m_reprocScaleParam.getPicSizeFromAPK(scale_width,scale_height);
        getPictureSize(&cur_width, &cur_height);

        String8 pic_size;
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%dx%d", scale_width, scale_height);
        pic_size.append(buffer);
        set(CameraParameters::KEY_PICTURE_SIZE, pic_size);
    }

    str = flatten();
    strParams = (char *)malloc(sizeof(char)*(str.length()+1));
    if(strParams != NULL){
        memset(strParams, 0, sizeof(char)*(str.length()+1));
        strlcpy(strParams, str.string(), str.length()+1);
        strParams[str.length()] = 0;
    }

    if(m_reprocScaleParam.isScaleEnabled() &&
        m_reprocScaleParam.isUnderScaling()){
        //need set back picture size
        String8 pic_size;
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%dx%d", cur_width, cur_height);
        pic_size.append(buffer);
        set(CameraParameters::KEY_PICTURE_SIZE, pic_size);
    }
    return strParams;
}

#ifdef TARGET_TS_MAKEUP
/*===========================================================================
 * FUNCTION   : getTsMakeupInfo
 *
 * DESCRIPTION: get TsMakeup info
 *
 * PARAMETERS :
 *   @whiteLevel : [output] white level
 *   @cleanLevel : [output] clean level

 * RETURN     : Whether makeup is enabled or not
 *==========================================================================*/
bool QCameraParameters::getTsMakeupInfo(int &whiteLevel, int &cleanLevel) const
{
    const char* pch_makeup_enable = get(QCameraParameters::KEY_TS_MAKEUP);
    if (pch_makeup_enable == NULL) {
        LOGH("pch_makeup_enable = null");
        return false;
    }
    bool enableMakeup =
            (strcmp(pch_makeup_enable,"On") == 0);
    if (enableMakeup) {
        whiteLevel = getInt(QCameraParameters::KEY_TS_MAKEUP_WHITEN);
        cleanLevel = getInt(QCameraParameters::KEY_TS_MAKEUP_CLEAN);
    }
    return enableMakeup;
}
#endif

/*===========================================================================
 * FUNCTION   : getPreviewHalPixelFormat
 *
 * DESCRIPTION: get preview HAL pixel format
 *
 * PARAMETERS : none
 *
 * RETURN     : HAL pixel format
 *==========================================================================*/
int QCameraParameters::getPreviewHalPixelFormat()
{
    int32_t halPixelFormat;
    cam_format_t fmt;
    getStreamFormat(CAM_STREAM_TYPE_PREVIEW,fmt);

    switch (fmt) {
    case CAM_FORMAT_YUV_420_NV12:
        halPixelFormat = HAL_PIXEL_FORMAT_YCbCr_420_SP;
        break;
    case CAM_FORMAT_YUV_420_NV21:
        halPixelFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        break;
    case CAM_FORMAT_YUV_420_NV21_ADRENO:
        halPixelFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO;
        break;
    case CAM_FORMAT_YUV_420_YV12:
        halPixelFormat = HAL_PIXEL_FORMAT_YV12;
        break;
    case CAM_FORMAT_YUV_420_NV12_VENUS:
        halPixelFormat = HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS;
        break;
    case CAM_FORMAT_YUV_420_NV21_VENUS:
        halPixelFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP_VENUS;
        break;
    case CAM_FORMAT_YUV_420_NV12_UBWC:
        halPixelFormat = HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS_UBWC;
        break;
    case CAM_FORMAT_YUV_422_NV16:
    case CAM_FORMAT_YUV_422_NV61:
    default:
        halPixelFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        break;
    }
    LOGH("format %d\n", halPixelFormat);
    return halPixelFormat;
}

/*===========================================================================
 * FUNCTION   : getQuadraCFA
 *
 * DESCRIPTION: get QuadraCFA mode
 *
 * PARAMETERS :
 *
 * RETURN     : none
 *==========================================================================*/
bool QCameraParameters::getQuadraCfa()
{
    return m_bQuadraCfa;
}
/*===========================================================================
 * FUNCTION   : getthumbnailSize
 *
 * DESCRIPTION: get thumbnail size
 *
 * PARAMETERS :
 *   @width, height : [output] thumbnail width and height
 *
 * RETURN     : none
 *==========================================================================*/
void QCameraParameters::getThumbnailSize(int *width, int *height) const
{
    *width = getInt(KEY_JPEG_THUMBNAIL_WIDTH);
    *height = getInt(KEY_JPEG_THUMBNAIL_HEIGHT);
}

/*===========================================================================
 * FUNCTION   : getZSLBurstInterval
 *
 * DESCRIPTION: get ZSL burst interval setting
 *
 * PARAMETERS : none
 *
 * RETURN     : ZSL burst interval value
 *==========================================================================*/
uint8_t QCameraParameters::getZSLBurstInterval()
{
    int interval = getInt(KEY_QC_ZSL_BURST_INTERVAL);
    if (interval < 0) {
        interval = 1;
    }
    return (uint8_t)interval;
}

/*===========================================================================
 * FUNCTION   : getZSLQueueDepth
 *
 * DESCRIPTION: get ZSL queue depth
 *
 * PARAMETERS : none
 *
 * RETURN     : ZSL queue depth value
 *==========================================================================*/
uint8_t QCameraParameters::getZSLQueueDepth()
{
    int qdepth = getInt(KEY_QC_ZSL_QUEUE_DEPTH);
    if (qdepth < 0) {
        qdepth = 2;
    }
    if (isLowMemoryDevice()) {
        qdepth = 1;
    }
    return (uint8_t)qdepth;
}

/*===========================================================================
 * FUNCTION   : getZSLBackLookCount
 *
 * DESCRIPTION: get ZSL backlook count setting
 *
 * PARAMETERS : none
 *
 * RETURN     : ZSL backlook count value
 *==========================================================================*/
uint8_t QCameraParameters::getZSLBackLookCount()
{
    int look_back = getInt(KEY_QC_ZSL_BURST_LOOKBACK);
    if (look_back < 0) {
        look_back = 2;
    }
    if (isLowMemoryDevice()) {
        look_back = 1;
    }
    return (uint8_t)look_back;
}
/*===========================================================================
 * FUNCTION   : isVideoBuffersCached
 *
 * DESCRIPTION: Query buffers are cached /un cached
 *
 * PARAMETERS : None
 *
 * RETURN     : buffers are cached /un cached
 *==========================================================================*/
bool QCameraParameters::isVideoBuffersCached()
{
    const char *cached_mem  = get(KEY_QC_CACHE_VIDEO_BUFFERS);
    if (cached_mem != NULL) {
        if (strcmp(cached_mem, VALUE_DISABLE) != 0) {
            return true;
        }
    }
    return false;
}
/*===========================================================================
 * FUNCTION   : getZSLMaxUnmatchedFrames
 *
 * DESCRIPTION: get allowed ZSL max unmatched frames number
 *
 * PARAMETERS : none
 *
 * RETURN     : ZSL backlook count value
 *==========================================================================*/
uint8_t QCameraParameters::getMaxUnmatchedFramesInQueue()
{
    return (uint8_t)(m_pCapability->min_num_pp_bufs);
}

/*===========================================================================
 * FUNCTION   : setRecordingHintValue
 *
 * DESCRIPTION: set recording hint
 *
 * PARAMETERS :
 *   @value   : video hint value
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCameraParameters::setRecordingHintValue(int32_t value)
{
    LOGH("VideoHint = %d", value);
    bool newValue = (value > 0)? true : false;

    if ( m_bRecordingHint != newValue ) {
        m_bNeedRestart = true;
        m_bRecordingHint_new = newValue;
    } else {
        m_bRecordingHint_new = m_bRecordingHint;
    }
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_RECORDING_HINT, value)) {
        return BAD_VALUE;
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : getNumOfSnapshots
 *
 * DESCRIPTION: get number of snapshot per shutter
 *
 * PARAMETERS : none
 *
 * RETURN     : number of snapshot per shutter
 *==========================================================================*/
uint8_t QCameraParameters::getNumOfSnapshots()
{
    uint8_t numOfSnapshot = 1;
    int val = getInt(KEY_QC_NUM_SNAPSHOT_PER_SHUTTER);
    if (0 < val) {
        numOfSnapshot = (uint8_t)val;
    }

    return (uint8_t)numOfSnapshot;
}

/*===========================================================================
 * FUNCTION   : getBurstCountForAdvancedCapture
 *
 * DESCRIPTION: get burst count for advanced capture.
 *
 * PARAMETERS : none
 *
 * RETURN     : number of snapshot required for advanced capture.
 *==========================================================================*/
uint8_t QCameraParameters::getBurstCountForAdvancedCapture()
{
    uint32_t burstCount = 0;
    if (isUbiFocusEnabled()) {
        //number of snapshots required for Ubi Focus.
        burstCount = m_pCapability->ubifocus_af_bracketing_need.burst_count;
    } else if (isUbiRefocus()) {
        //number of snapshots required for Opti Zoom.
        burstCount = m_pCapability->refocus_af_bracketing_need.burst_count;
    } else if (isOptiZoomEnabled()) {
        //number of snapshots required for Opti Zoom.
        burstCount = m_pCapability->opti_zoom_settings_need.burst_count;
    } else if (isChromaFlashEnabled()) {
        //number of snapshots required for Chroma Flash.
        burstCount = m_pCapability->chroma_flash_settings_need.burst_count;
    } else if (isStillMoreEnabled()) {
        //number of snapshots required for Still More.
        if (isSeeMoreEnabled()) {
            burstCount = 1;
        } else if ((m_stillmore_config.burst_count >=
                m_pCapability->stillmore_settings_need.min_burst_count) &&
                (m_stillmore_config.burst_count <=
                m_pCapability->stillmore_settings_need.max_burst_count)) {
            burstCount = m_stillmore_config.burst_count;
        } else {
            burstCount = m_pCapability->stillmore_settings_need.burst_count;
        }
    } else if (isHDREnabled()) {
        //number of snapshots required for HDR.
        burstCount = m_pCapability->hdr_bracketing_setting.num_frames;
    } else if (isAEBracketEnabled()) {
      burstCount = 0;
      const char *str_val = m_AEBracketingClient.values;
      if ((str_val != NULL) && (strlen(str_val) > 0)) {
          char prop[PROPERTY_VALUE_MAX];
          memset(prop, 0, sizeof(prop));
          strlcpy(prop, str_val, PROPERTY_VALUE_MAX);
          char *saveptr = NULL;
          char *token = strtok_r(prop, ",", &saveptr);
          while (token != NULL) {
              token = strtok_r(NULL, ",", &saveptr);
              burstCount++;
          }
      }
    }

    if (burstCount <= 0) {
        burstCount = getNumOfSnapshots();
    }

    LOGH("Snapshot burst count = %d", burstCount);
    return (uint8_t)burstCount;
}

/*===========================================================================
 * FUNCTION   : getNumOfRetroSnapshots
 *
 * DESCRIPTION: get number of retro active snapshots per shutter
 *
 * PARAMETERS : none
 *
 * RETURN     : number of retro active snapshots per shutter
 *==========================================================================*/
uint8_t QCameraParameters::getNumOfRetroSnapshots()
{
    int numOfRetroSnapshots = getInt(KEY_QC_NUM_RETRO_BURST_PER_SHUTTER);
    if (numOfRetroSnapshots < 0) {
        numOfRetroSnapshots = 0;
    }
    LOGH("numOfRetroSnaps - %d", numOfRetroSnapshots);
    return (uint8_t)numOfRetroSnapshots;
}

/*===========================================================================
 * FUNCTION   : getNumOfExtraHDRInBufsIfNeeded
 *
 * DESCRIPTION: get number of extra input buffers needed by HDR
 *
 * PARAMETERS : none
 *
 * RETURN     : number of extra buffers needed by HDR; 0 if not HDR enabled
 *==========================================================================*/
uint8_t QCameraParameters::getNumOfExtraHDRInBufsIfNeeded()
{
    unsigned int numOfBufs = 0;

    if (isHDREnabled()) {
        numOfBufs += m_pCapability->hdr_bracketing_setting.num_frames;
        if (isHDR1xFrameEnabled() && isHDR1xExtraBufferNeeded()) {
            numOfBufs++;
        }
        numOfBufs--; // Only additional buffers need to be returned
    }

    return (uint8_t)(numOfBufs);
}

/*===========================================================================
 * FUNCTION   : getNumOfExtraHDROutBufsIfNeeded
 *
 * DESCRIPTION: get number of extra output buffers needed by HDR
 *
 * PARAMETERS : none
 *
 * RETURN     : number of extra buffers needed by HDR; 0 if not HDR enabled
 *==========================================================================*/
uint8_t QCameraParameters::getNumOfExtraHDROutBufsIfNeeded()
{
    int numOfBufs = 0;

    if (isHDREnabled() && isHDR1xFrameEnabled()) {
        numOfBufs++;
    }

    return (uint8_t)(numOfBufs);
}

/*===========================================================================
 * FUNCTION   : getJpegQuality
 *
 * DESCRIPTION: get jpeg encoding quality
 *
 * PARAMETERS : none
 *
 * RETURN     : jpeg encoding quality
 *==========================================================================*/
uint32_t QCameraParameters::getJpegQuality()
{
    int quality = getInt(KEY_JPEG_QUALITY);
    if (quality < 0) {
        quality = 85; // set to default quality value
    }
    return (uint32_t)quality;
}

/*===========================================================================
 * FUNCTION   : getRotation
 *
 * DESCRIPTION: get application configured rotation
 *
 * PARAMETERS : none
 *
 * RETURN     : rotation value
 *==========================================================================*/
uint32_t QCameraParameters::getRotation() {
    int rotation = 0;

    //If exif rotation is set, do not rotate captured image
    if (!useJpegExifRotation()) {
        rotation = mRotation;
        if (rotation < 0) {
            rotation = 0;
        }
    }
    return (uint32_t)rotation;
}

/*===========================================================================
 * FUNCTION   : setJpegRotation
 *
 * DESCRIPTION: set jpeg rotation value configured internally
 *
 * PARAMETERS : none
 *
 * RETURN     : jpeg rotation value
 *==========================================================================*/
void QCameraParameters::setJpegRotation(int rotation) {
    if (rotation == 0 || rotation == 90 ||
            rotation == 180 || rotation == 270) {
        mJpegRotation = (uint32_t)rotation;
    }
}

/*===========================================================================
 * FUNCTION   : getDeviceRotation
 *
 * DESCRIPTION: get device rotation value
 *
 * PARAMETERS : none
 *
 * RETURN     : device rotation value
 *==========================================================================*/
uint32_t QCameraParameters::getDeviceRotation() {
    int rotation = 0;

    rotation = mRotation;
    if (rotation < 0) {
        rotation = 0;
    }

    return (uint32_t)rotation;
}

/*===========================================================================
 * FUNCTION   : getJpegExifRotation
 *
 * DESCRIPTION: get exif rotation value
 *
 * PARAMETERS : none
 *
 * RETURN     : rotation value
 *==========================================================================*/
uint32_t QCameraParameters::getJpegExifRotation() {
    int rotation = 0;

    if (useJpegExifRotation()) {
        rotation = mRotation;
        if (rotation < 0) {
            rotation = 0;
        }
    }
    return (uint32_t)rotation;
}

/*===========================================================================
 * FUNCTION   : useJpegExifRotation
 *
 * DESCRIPTION: Check if jpeg exif rotation need to be used
 *
 * PARAMETERS : none
 *
 * RETURN     : true if jpeg exif rotation need to be used
 *==========================================================================*/
bool QCameraParameters::useJpegExifRotation() {
    char exifRotation[PROPERTY_VALUE_MAX];

    property_get("persist.camera.exif.rotation", exifRotation, "off");

    if (!strcmp(exifRotation, "on")) {
        return true;
    }

    if (!(m_pCapability->qcom_supported_feature_mask & CAM_QCOM_FEATURE_ROTATION)) {
        return true;
    }

    return false;
}

/*===========================================================================
 * FUNCTION   : getEffectValue
 *
 * DESCRIPTION: get effect value
 *
 * PARAMETERS : none
 *
 * RETURN     : effect value
 *==========================================================================*/
int32_t QCameraParameters::getEffectValue()
{
    uint32_t cnt = 0;
    const char *effect = get(KEY_EFFECT);
    if (effect) {
        while (NULL != EFFECT_MODES_MAP[cnt].desc) {
            if (!strcmp(EFFECT_MODES_MAP[cnt].desc, effect)) {
                return EFFECT_MODES_MAP[cnt].val;
            }
            cnt++;
        }
    } else {
        LOGW("Missing effect value");
    }
    return CAM_EFFECT_MODE_OFF;
}

/*===========================================================================
 * FUNCTION   : parseGPSCoordinate
 *
 * DESCRIPTION: parse GPS coordinate string
 *
 * PARAMETERS :
 *   @coord_str : [input] coordinate string
 *   @coord     : [output]  ptr to struct to store coordinate
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCameraParameters::parseGPSCoordinate(const char *coord_str, rat_t* coord)
{
    if(coord == NULL) {
        LOGE("error, invalid argument coord == NULL");
        return BAD_VALUE;
    }
    double degF = atof(coord_str);
    if (degF < 0) {
        degF = -degF;
    }
    double minF = (degF - (double)(int) degF) * 60.0;
    double secF = (minF - (double)(int) minF) * 60.0;

    getRational(&coord[0], (int)degF, 1);
    getRational(&coord[1], (int)minF, 1);
    getRational(&coord[2], (int)(secF * 10000.0), 10000);
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : getExifDateTime
 *
 * DESCRIPTION: query exif date time
 *
 * PARAMETERS :
 *   @dateTime    : String to store exif date time.
 *                  Should be leaved unchanged in case of error.
 *   @subsecTime  : String to store exif time nanoseconds.
 *                  Should be leaved unchanged in case of error.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::getExifDateTime(String8 &dateTime, String8 &subsecTime)
{
    int32_t ret = NO_ERROR;

    //get time and date from system
    struct timeval tv;
    struct tm timeinfo_data;

    int res = gettimeofday(&tv, NULL);
    if (0 == res) {
        struct tm *timeinfo = localtime_r(&tv.tv_sec, &timeinfo_data);
        if (NULL != timeinfo) {
            //Write datetime according to EXIF Spec
            //"YYYY:MM:DD HH:MM:SS" (20 chars including \0)
            dateTime = String8::format("%04d:%02d:%02d %02d:%02d:%02d",
                    timeinfo->tm_year + 1900, timeinfo->tm_mon + 1,
                    timeinfo->tm_mday, timeinfo->tm_hour,
                    timeinfo->tm_min, timeinfo->tm_sec);
            //Write subsec according to EXIF Sepc
            subsecTime = String8::format("%06ld", tv.tv_usec);
        } else {
            LOGE("localtime_r() error");
            ret = UNKNOWN_ERROR;
        }
    } else if (-1 == res) {
        LOGE("gettimeofday() error: %s", strerror(errno));
        ret = UNKNOWN_ERROR;
    } else {
        LOGE("gettimeofday() unexpected return code: %d", res);
        ret = UNKNOWN_ERROR;
    }

    return ret;
}

/*===========================================================================
 * FUNCTION   : getRational
 *
 * DESCRIPTION: compose rational struct
 *
 * PARAMETERS :
 *   @rat     : ptr to struct to store rational info
 *   @num     :num of the rational
 *   @denom   : denom of the rational
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::getRational(rat_t *rat, int num, int denom)
{
    if ((0 > num) || (0 > denom)) {
        LOGE("Negative values");
        return BAD_VALUE;
    }
    if (NULL == rat) {
        LOGE("NULL rat input");
        return BAD_VALUE;
    }
    rat->num = (uint32_t)num;
    rat->denom = (uint32_t)denom;
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : getExifFocalLength
 *
 * DESCRIPTION: get exif focal lenght
 *
 * PARAMETERS :
 *   @focalLength : ptr to rational strcut to store focal lenght
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::getExifFocalLength(rat_t *focalLength)
{
    int focalLengthValue =
        (int)(getFloat(QCameraParameters::KEY_FOCAL_LENGTH) * FOCAL_LENGTH_DECIMAL_PRECISION);
    return getRational(focalLength, focalLengthValue, FOCAL_LENGTH_DECIMAL_PRECISION);
}

/*===========================================================================
 * FUNCTION   : getExifIsoSpeed
 *
 * DESCRIPTION: get exif ISO speed
 *
 * PARAMETERS : none
 *
 * RETURN     : ISO speed value
 *==========================================================================*/
uint16_t QCameraParameters::getExifIsoSpeed()
{
    uint16_t isoSpeed = 0;
    const char *iso_str = get(QCameraParameters::KEY_QC_ISO_MODE);
    int iso_index = lookupAttr(ISO_MODES_MAP, PARAM_MAP_SIZE(ISO_MODES_MAP), iso_str);
    switch (iso_index) {
    case CAM_ISO_MODE_AUTO:
        isoSpeed = 0;
        break;
    case CAM_ISO_MODE_DEBLUR:
        isoSpeed = 1;
        break;
    case CAM_ISO_MODE_100:
        isoSpeed = 100;
        break;
    case CAM_ISO_MODE_200:
        isoSpeed = 200;
        break;
    case CAM_ISO_MODE_400:
        isoSpeed = 400;
        break;
    case CAM_ISO_MODE_800:
        isoSpeed = 800;
        break;
    case CAM_ISO_MODE_1600:
        isoSpeed = 1600;
        break;
    case CAM_ISO_MODE_3200:
        isoSpeed = 3200;
        break;
    }
    return isoSpeed;
}

/*===========================================================================
 * FUNCTION   : getExifGpsProcessingMethod
 *
 * DESCRIPTION: get GPS processing method
 *
 * PARAMETERS :
 *   @gpsProcessingMethod : string to store GPS process method
 *   @count               : lenght of the string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::getExifGpsProcessingMethod(char *gpsProcessingMethod,
                                                      uint32_t &count)
{
    const char *str = get(KEY_GPS_PROCESSING_METHOD);
    if(str != NULL) {
        memcpy(gpsProcessingMethod, ExifAsciiPrefix, EXIF_ASCII_PREFIX_SIZE);
        count = EXIF_ASCII_PREFIX_SIZE;
        strlcpy(gpsProcessingMethod + EXIF_ASCII_PREFIX_SIZE, str, GPS_PROCESSING_METHOD_SIZE);
        count += (uint32_t)strlen(str);
        gpsProcessingMethod[count++] = '\0'; // increase 1 for the last NULL char
        return NO_ERROR;
    } else {
        return BAD_VALUE;
    }
}

/*===========================================================================
 * FUNCTION   : getExifLatitude
 *
 * DESCRIPTION: get exif latitude
 *
 * PARAMETERS :
 *   @latitude : ptr to rational struct to store latitude info
 *   @ladRef   : charater to indicate latitude reference
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::getExifLatitude(rat_t *latitude,
                                           char *latRef)
{
    const char *str = get(KEY_GPS_LATITUDE);
    if(str != NULL) {
        parseGPSCoordinate(str, latitude);

        //set Latitude Ref
        float latitudeValue = getFloat(KEY_GPS_LATITUDE);
        if(latitudeValue < 0.0f) {
            latRef[0] = 'S';
        } else {
            latRef[0] = 'N';
        }
        latRef[1] = '\0';
        return NO_ERROR;
    }else{
        return BAD_VALUE;
    }
}

/*===========================================================================
 * FUNCTION   : getExifLongitude
 *
 * DESCRIPTION: get exif longitude
 *
 * PARAMETERS :
 *   @longitude : ptr to rational struct to store longitude info
 *   @lonRef    : charater to indicate longitude reference
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::getExifLongitude(rat_t *longitude,
                                            char *lonRef)
{
    const char *str = get(KEY_GPS_LONGITUDE);
    if(str != NULL) {
        parseGPSCoordinate(str, longitude);

        //set Longitude Ref
        float longitudeValue = getFloat(KEY_GPS_LONGITUDE);
        if(longitudeValue < 0.0f) {
            lonRef[0] = 'W';
        } else {
            lonRef[0] = 'E';
        }
        lonRef[1] = '\0';
        return NO_ERROR;
    }else{
        return BAD_VALUE;
    }
}

/*===========================================================================
 * FUNCTION   : getExifAltitude
 *
 * DESCRIPTION: get exif altitude
 *
 * PARAMETERS :
 *   @altitude : ptr to rational struct to store altitude info
 *   @altRef   : charater to indicate altitude reference
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::getExifAltitude(rat_t *altitude,
                                           char *altRef)
{
    const char *str = get(KEY_GPS_ALTITUDE);
    if(str != NULL) {
        double value = atof(str);
        *altRef = 0;
        if(value < 0){
            *altRef = 1;
            value = -value;
        }
        return getRational(altitude, (int)(value*1000), 1000);
    }else{
        return BAD_VALUE;
    }
}

/*===========================================================================
 * FUNCTION   : getExifGpsDateTimeStamp
 *
 * DESCRIPTION: get exif GPS date time stamp
 *
 * PARAMETERS :
 *   @gpsDateStamp : GPS date time stamp string
 *   @bufLen       : length of the string
 *   @gpsTimeStamp : ptr to rational struct to store time stamp info
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::getExifGpsDateTimeStamp(char *gpsDateStamp,
                                                   uint32_t bufLen,
                                                   rat_t *gpsTimeStamp)
{
    const char *str = get(KEY_GPS_TIMESTAMP);
    if(str != NULL) {
        time_t unixTime = (time_t)atol(str);
        struct tm *UTCTimestamp = gmtime(&unixTime);

        if(!UTCTimestamp) {
            LOGE("UTCTimestamp is null\n");
            return BAD_VALUE;
        }

        strftime(gpsDateStamp, bufLen, "%Y:%m:%d", UTCTimestamp);

        getRational(&gpsTimeStamp[0], UTCTimestamp->tm_hour, 1);
        getRational(&gpsTimeStamp[1], UTCTimestamp->tm_min, 1);
        getRational(&gpsTimeStamp[2], UTCTimestamp->tm_sec, 1);

        return NO_ERROR;
    } else {
        return BAD_VALUE;
    }
}

/*===========================================================================
 * FUNCTION   : updateFocusDistances
 *
 * DESCRIPTION: update focus distances
 *
 * PARAMETERS :
 *   @focusDistances : ptr to focus distance info
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::updateFocusDistances(cam_focus_distances_info_t *focusDistances)
{
    String8 str;
    char buffer[32] = {0};
    //set all distances to infinity if focus mode is infinity
    if(mFocusMode == CAM_FOCUS_MODE_INFINITY) {
        str.append("Infinity,Infinity,Infinity");
    } else {
        if (focusDistances->focus_distance[0] < FOCUS_PERCISION) {
            str.append("Infinity");
        } else {
            snprintf(buffer, sizeof(buffer), "%f", 1.0/focusDistances->focus_distance[0]);
            str.append(buffer);
        }
        if (focusDistances->focus_distance[1] < FOCUS_PERCISION) {
            str.append(",Infinity");
        } else {
            snprintf(buffer, sizeof(buffer), ",%f", 1.0/focusDistances->focus_distance[1]);
            str.append(buffer);
        }
        if (focusDistances->focus_distance[2] < FOCUS_PERCISION) {
            str.append(",Infinity");
        } else {
            snprintf(buffer, sizeof(buffer), ",%f", 1.0/focusDistances->focus_distance[2]);
            str.append(buffer);
        }
    }
    LOGH("setting KEY_FOCUS_DISTANCES as %s", __FUNCTION__, str.string());
    set(QCameraParameters::KEY_FOCUS_DISTANCES, str.string());
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : updateRecordingHintValue
 *
 * DESCRIPTION: update recording hint locally and to daemon
 *
 * PARAMETERS :
 *   @value   : video hint value
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::updateRecordingHintValue(int32_t value)
{
    int32_t rc = NO_ERROR;
    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    rc = setRecordingHintValue(value);
    if (rc != NO_ERROR) {
        LOGE("Failed to update table");
        return rc;
    }

    if(m_bDISEnabled && (value==1)) {
        LOGH("%d: Setting DIS value again!!");
        setDISValue(VALUE_ENABLE);
    }

    rc = commitSetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to update recording hint");
        return rc;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : setHistogram
 *
 * DESCRIPTION: set histogram
 *
 * PARAMETERS :
 *   @enabled : if histogram is enabled
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setHistogram(bool enabled)
{
    if(m_bHistogramEnabled == enabled) {
        LOGH("histogram flag not changed, no ops here");
        return NO_ERROR;
    }

    // set parm for histogram
    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    int32_t value = enabled ? 1 : 0;
    int32_t rc = NO_ERROR;
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_HISTOGRAM, value)) {
        LOGE("Failed to update table");
        return BAD_VALUE;
    }

    rc = commitSetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to set histogram");
        return rc;
    }

    m_bHistogramEnabled = enabled;

    LOGH("Histogram -> %s", m_bHistogramEnabled ? "Enabled" : "Disabled");

    return rc;
}

/*===========================================================================
 * FUNCTION   : setIntEvent
 *
 * DESCRIPTION: set setIntEvent
 *
 * PARAMETERS :
 *   @params : image size and dimensions
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setIntEvent(cam_int_evt_params_t params)
{
    int32_t rc = NO_ERROR;

    if ( m_pParamBuf == NULL ) {
        return NO_INIT;
    }

    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    //Sending snapshot taken notification back to Eztune"
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_INT_EVT, params)) {
        LOGE("Failed to update table");
        return BAD_VALUE;
    }

    rc = commitSetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to set frameskip info parm");
        return rc;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : setFaceDetectionOption
 *
 * DESCRIPTION: set if face detection is enabled by SendCommand
 *
 * PARAMETERS :
 *   @enabled : bool flag if face detection should be enabled
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
 int32_t QCameraParameters::setFaceDetectionOption(bool enabled)
{
    m_bFaceDetectionOn = enabled;
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setFaceDetection
 *
 * DESCRIPTION: set face detection
 *
 * PARAMETERS :
 *   @enabled : if face detection is enabled
 *   @initCommit : if configuration list need to be initialized and commited
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setFaceDetection(bool enabled, bool initCommit)
{
    uint32_t faceProcMask = m_nFaceProcMask;
    // set face detection mask
    if (enabled) {
        faceProcMask |= CAM_FACE_PROCESS_MASK_DETECTION;
        if (getRecordingHintValue() > 0) {
            faceProcMask = 0;
            faceProcMask |= CAM_FACE_PROCESS_MASK_FOCUS;
            if (fdModeInVideo() == CAM_FACE_PROCESS_MASK_DETECTION) {
                faceProcMask |= CAM_FACE_PROCESS_MASK_DETECTION;
            }
        } else {
            faceProcMask |= CAM_FACE_PROCESS_MASK_FOCUS;
            faceProcMask |= CAM_FACE_PROCESS_MASK_DETECTION;
        }
        if (isTruePortraitEnabled()) {
            LOGL("QCameraParameters::setFaceDetection trueportrait enabled");
            faceProcMask |= CAM_FACE_PROCESS_MASK_GAZE;
        } else {
            LOGL("QCameraParameters::setFaceDetection trueportrait disabled");
            faceProcMask &= ~CAM_FACE_PROCESS_MASK_GAZE;
        }
    } else {
        faceProcMask &= ~(CAM_FACE_PROCESS_MASK_DETECTION
                | CAM_FACE_PROCESS_MASK_FOCUS
                | CAM_FACE_PROCESS_MASK_GAZE);
    }

    if(m_nFaceProcMask == faceProcMask) {
        LOGH("face process mask not changed, no ops here");
        return NO_ERROR;
    }

    m_nFaceProcMask = faceProcMask;

    // set parm for face detection
    uint32_t requested_faces = (uint32_t)getInt(KEY_QC_MAX_NUM_REQUESTED_FACES);
    cam_fd_set_parm_t fd_set_parm;
    memset(&fd_set_parm, 0, sizeof(cam_fd_set_parm_t));
    fd_set_parm.fd_mode = faceProcMask;
    fd_set_parm.num_fd = requested_faces;

    LOGH("[KPI Perf]: PROFILE_FACE_DETECTION_VALUE = %d num_fd = %d",
           faceProcMask,requested_faces);

    if (initCommit) {
        if(initBatchUpdate(m_pParamBuf) < 0 ) {
            LOGE("Failed to initialize group update table");
            return BAD_TYPE;
        }
    }

    int32_t rc = NO_ERROR;

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_FD, fd_set_parm)) {
        LOGE("Failed to update table");
        return BAD_VALUE;
    }

    if (initCommit) {
        rc = commitSetBatch();
        if (rc != NO_ERROR) {
            LOGE("Failed to set face detection parm");
            return rc;
        }
    }

    LOGH("FaceProcMask -> %d", m_nFaceProcMask);

    return rc;
}

/*===========================================================================
 * FUNCTION   : setFrameSkip
 *
 * DESCRIPTION: send ISP frame skip pattern to camera daemon
 *
 * PARAMETERS :
 *   @pattern : skip pattern for ISP
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setFrameSkip(enum msm_vfe_frame_skip_pattern pattern)
{
    int32_t rc = NO_ERROR;

    if ( m_pParamBuf == NULL ) {
        return NO_INIT;
    }

    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_FRAMESKIP, (int32_t)pattern)) {
        LOGE("Failed to update table");
        return BAD_VALUE;
    }

    rc = commitSetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to set frameskip info parm");
        return rc;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : updateRAW
 *
 * DESCRIPTION: Query sensor output size based on maximum stream dimension
 *
 * PARAMETERS :
 *   @max_dim : maximum stream dimension
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::updateRAW(cam_dimension_t max_dim)
{
    int32_t rc = NO_ERROR;
    cam_dimension_t raw_dim, pic_dim;

    //No need to update RAW dimensions if meta raw is enabled.
    if (m_bMetaRawEnabled) {
        return rc;
    }
    // If offline raw is enabled, check the dimensions from Picture size since snapshot
    // stream is not added but final JPEG is required of snapshot size
    if (getofflineRAW()) {
        if (getQuadraCfa()) {
            max_dim.width = m_pCapability->quadra_cfa_dim[0].width;
            max_dim.height = m_pCapability->quadra_cfa_dim[0].height;
        } else {
            getStreamDimension(CAM_STREAM_TYPE_SNAPSHOT, pic_dim);
            if (pic_dim.width > max_dim.width) {
                max_dim.width = pic_dim.width;
            }
            if (pic_dim.height > max_dim.height) {
                max_dim.height = pic_dim.height;
            }
        }
    }

    if (max_dim.width == 0 || max_dim.height == 0) {
        max_dim = m_pCapability->raw_dim[0];
    }

    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_MAX_DIMENSION, max_dim)) {
        LOGE("Failed to update table for CAM_INTF_PARM_MAX_DIMENSION ");
        return BAD_VALUE;
    }

    rc = commitSetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to set lock CAM_INTF_PARM_MAX_DIMENSION parm");
        return rc;
    }

    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    ADD_GET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_RAW_DIMENSION);

    rc = commitGetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to get commit CAM_INTF_PARM_RAW_DIMENSION");
        return rc;
    }

    READ_PARAM_ENTRY(m_pParamBuf, CAM_INTF_PARM_RAW_DIMENSION, raw_dim);

    LOGH("RAW Dimension = %d X %d",raw_dim.width,raw_dim.height);
    if (raw_dim.width == 0 || raw_dim.height == 0) {
        LOGW("Error getting RAW size. Setting to Capability value");
        if (getQuadraCfa()) {
            raw_dim = m_pCapability->quadra_cfa_dim[0];
        } else {
            raw_dim = m_pCapability->raw_dim[0];
        }
    }
    setRawSize(raw_dim);
    return rc;
}

/*===========================================================================
 * FUNCTION   : setHDRSceneEnable
 *
 * DESCRIPTION: sets hdr scene deteced flag
 *
 * PARAMETERS :
 *   @bflag : hdr scene deteced
 *
 * RETURN     : nothing
 *==========================================================================*/
void QCameraParameters::setHDRSceneEnable(bool bflag)
{
    bool bupdate = false;
    if (m_HDRSceneEnabled != bflag) {
        bupdate = true;
    }
    m_HDRSceneEnabled = bflag;

    if (bupdate) {
        updateFlash(true);
    }
}

/*===========================================================================
 * FUNCTION   : getASDStateString
 *
 * DESCRIPTION: get ASD result in string format
 *
 * PARAMETERS :
 *   @scene : selected scene mode
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
 const char *QCameraParameters::getASDStateString(cam_auto_scene_t scene)
{
    switch (scene) {
      case S_NORMAL :
        return "Normal";
      case S_SCENERY:
        return "Scenery";
      case S_PORTRAIT:
        return "Portrait";
      case S_PORTRAIT_BACKLIGHT:
        return "Portrait-Backlight";
      case S_SCENERY_BACKLIGHT:
        return "Scenery-Backlight";
      case S_BACKLIGHT:
        return "Backlight";
      default:
        return "<Unknown!>";
      }
}

/*===========================================================================
 * FUNCTION   : parseNDimVector
 *
 * DESCRIPTION: helper function to parse a string like "(1, 2, 3, 4, ..., N)"
 *              into N-dimension vector
 *
 * PARAMETERS :
 *   @str     : string to be parsed
 *   @num     : output array of size N to store vector element values
 *   @N       : number of dimension
 *   @delim   : delimeter to seperete string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::parseNDimVector(const char *str, int *num, int N, char delim = ',')
{
    char *start, *end;
    if (num == NULL) {
        LOGE("Invalid output array (num == NULL)");
        return BAD_VALUE;
    }

    //check if string starts and ends with parantheses
    if(str[0] != '(' || str[strlen(str)-1] != ')') {
        LOGE("Invalid format of string %s, valid format is (n1, n2, n3, n4 ...)",
               str);
        return BAD_VALUE;
    }
    start = (char*) str;
    start++;
    for(int i=0; i<N; i++) {
        *(num+i) = (int) strtol(start, &end, 10);
        if(*end != delim && i < N-1) {
            LOGE("Cannot find delimeter '%c' in string \"%s\". end = %c",
                   delim, str, *end);
            return -1;
        }
        start = end+1;
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : parseCameraAreaString
 *
 * DESCRIPTION: helper function to parse a string of camera areas like
 *              "(1, 2, 3, 4, 5),(1, 2, 3, 4, 5),..."
 *
 * PARAMETERS :
 *   @str             : string to be parsed
 *   @max_num_areas   : max number of areas
 *   @pAreas          : ptr to struct to store areas
 *   @num_areas_found : number of areas found
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::parseCameraAreaString(const char *str,
                                                 int max_num_areas,
                                                 cam_area_t *pAreas,
                                                 int& num_areas_found)
{
    char area_str[32];
    const char *start, *end, *p;
    start = str; end = NULL;
    int values[5], index=0;
    num_areas_found = 0;

    memset(values, 0, sizeof(values));
    while(start != NULL) {
       if(*start != '(') {
            LOGE("error: Ill formatted area string: %s", str);
            return BAD_VALUE;
       }
       end = strchr(start, ')');
       if(end == NULL) {
            LOGE("error: Ill formatted area string: %s", str);
            return BAD_VALUE;
       }
       int i;
       for (i=0,p=start; p<=end; p++, i++) {
           area_str[i] = *p;
       }
       area_str[i] = '\0';
       if(parseNDimVector(area_str, values, 5) < 0){
            LOGE("error: Failed to parse the area string: %s", area_str);
            return BAD_VALUE;
       }
       // no more areas than max_num_areas are accepted.
       if(index >= max_num_areas) {
            LOGE("error: too many areas specified %s", str);
            return BAD_VALUE;
       }
       pAreas[index].rect.left = values[0];
       pAreas[index].rect.top = values[1];
       pAreas[index].rect.width = values[2] - values[0];
       pAreas[index].rect.height = values[3] - values[1];
       pAreas[index].weight = values[4];

       index++;
       start = strchr(end, '('); // serach for next '('
    }
    num_areas_found = index;
    return 0;
}

/*===========================================================================
 * FUNCTION   : validateCameraAreas
 *
 * DESCRIPTION: helper function to validate camera areas within (-1000, 1000)
 *
 * PARAMETERS :
 *   @areas     : ptr to array of areas
 *   @num_areas : number of areas
 *
 * RETURN     : true --  area is in valid range
 *              false -- not valid
 *==========================================================================*/
bool QCameraParameters::validateCameraAreas(cam_area_t *areas, int num_areas)
{
    // special case: default area
    if (num_areas == 1 &&
        areas[0].rect.left == 0 &&
        areas[0].rect.top == 0 &&
        areas[0].rect.width == 0 &&
        areas[0].rect.height == 0 &&
        areas[0].weight == 0) {
        return true;
    }

    for(int i = 0; i < num_areas; i++) {
        // left should be >= -1000
        if(areas[i].rect.left < -1000) {
            return false;
        }

        // top  should be >= -1000
        if(areas[i].rect.top < -1000) {
            return false;
        }

        // width or height should be > 0
        if (areas[i].rect.width <= 0 || areas[i].rect.height <= 0) {
            return false;
        }

        // right  should be <= 1000
        if(areas[i].rect.left + areas[i].rect.width > 1000) {
            return false;
        }

        // bottom should be <= 1000
        if(areas[i].rect.top + areas[i].rect.height > 1000) {
            return false;
        }

        // weight should be within (1, 1000)
        if (areas[i].weight < 1 || areas[i].weight > 1000) {
            return false;
        }
    }
    return true;
}

/*===========================================================================
 * FUNCTION   : isYUVFrameInfoNeeded
 *
 * DESCRIPTION: In AE-Bracket mode, we need set yuv buffer information for up-layer
 *
 * PARAMETERS : none
 *
 * RETURN     : true: needed
 *              false: no need
 *==========================================================================*/
bool QCameraParameters::isYUVFrameInfoNeeded()
{
    //In AE-Bracket mode, we need set raw buffer information for up-layer
    if(!isNV21PictureFormat() && !isNV16PictureFormat()){
        return false;
    }
    const char *aecBracketStr =  get(KEY_QC_AE_BRACKET_HDR);

    int value = lookupAttr(BRACKETING_MODES_MAP, PARAM_MAP_SIZE(BRACKETING_MODES_MAP),
            aecBracketStr);
    LOGH("aecBracketStr=%s, value=%d.", aecBracketStr, value);
    return (value == CAM_EXP_BRACKETING_ON);
}

/*===========================================================================
 * FUNCTION   : getFrameFmtString
 *
 * DESCRIPTION: get string name of frame format
 *
 * PARAMETERS :
 *   @frame   : frame format
 *
 * RETURN     : string name of frame format
 *==========================================================================*/
const char *QCameraParameters::getFrameFmtString(cam_format_t fmt)
{
    return lookupNameByValue(PICTURE_TYPES_MAP, PARAM_MAP_SIZE(PICTURE_TYPES_MAP), fmt);
}

/*===========================================================================
 * FUNCTION   : setDcrf
 *
 * DESCRIPTION: Enable/Disable DCRF (dual-camera-range-finding)
 *
 * RETURN     : none
 *==========================================================================*/
void QCameraParameters::setDcrf()
{
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));

    // Set DCRF to off by default (assuming single-camera mode)
    m_bDcrfEnabled = 0;

    // In dual-cam mode, get sysprop and set it to on by default
    if(m_relCamSyncInfo.sync_control == CAM_SYNC_RELATED_SENSORS_ON) {
        property_get("persist.camera.dcrf.enable", prop, "1");
        m_bDcrfEnabled = atoi(prop);
    }
}

/*===========================================================================
 * FUNCTION   : setRelatedCamSyncInfo
 *
 * DESCRIPTION: set the related cam info parameters
 * the related cam info is cached into params to make some decisions beforehand
 *
 * PARAMETERS :
 *   @info  : ptr to related cam info parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setRelatedCamSyncInfo(
        cam_sync_related_sensors_event_info_t* info)
{
    if(info != NULL){
        memcpy(&m_relCamSyncInfo, info,
                sizeof(cam_sync_related_sensors_event_info_t));
        return NO_ERROR;
    } else {
        LOGE("info buffer is null");
        return UNKNOWN_ERROR;
    }
}

/*===========================================================================
 * FUNCTION   : getRelatedCamSyncInfo
 *
 * DESCRIPTION:returns the related cam sync info for this HWI instance
 *
 * PARAMETERS :none
 *
 * RETURN     : const pointer to cam_sync_related_sensors_event_info_t
 *==========================================================================*/
const cam_sync_related_sensors_event_info_t*
        QCameraParameters::getRelatedCamSyncInfo(void)
{
    return &m_relCamSyncInfo;
}

/*===========================================================================
 * FUNCTION   : setFrameSyncEnabled
 *
 * DESCRIPTION: sets whether frame sync is enabled
 *
 * PARAMETERS :
 *   @enable  : flag whether to enable or disable frame sync
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setFrameSyncEnabled(bool enable)
{
    m_bFrameSyncEnabled = enable;
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : isFrameSyncEnabled
 *
 * DESCRIPTION: returns whether frame sync is enabled
 *
 * PARAMETERS :none
 *
 * RETURN     : bool indicating whether frame sync is enabled
 *==========================================================================*/
bool QCameraParameters::isFrameSyncEnabled(void)
{
    return m_bFrameSyncEnabled;
}

/*===========================================================================
 * FUNCTION   : bundleRelatedCameras
 *
 * DESCRIPTION: send trigger for bundling related camera sessions in the server
 *
 * PARAMETERS :
 *   @sync        :indicates whether syncing is On or Off
 *   @sessionid  :session id for other camera session
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::bundleRelatedCameras(bool sync,
        uint32_t sessionid)
{
    int32_t rc = NO_ERROR;

    if (NULL == m_pCamOpsTbl) {
        LOGE("Ops not initialized");
        return NO_INIT;
    }

    LOGD("Sending Bundling cmd sync %d, SessionId %d ",
            sync, sessionid);

    if(m_pRelCamSyncBuf) {
        if(sync) {
            m_pRelCamSyncBuf->sync_control = CAM_SYNC_RELATED_SENSORS_ON;
        }
        else {
            m_pRelCamSyncBuf->sync_control = CAM_SYNC_RELATED_SENSORS_OFF;
        }
        m_pRelCamSyncBuf->mode = m_relCamSyncInfo.mode;
        m_pRelCamSyncBuf->type = m_relCamSyncInfo.type;
        m_pRelCamSyncBuf->related_sensor_session_id = sessionid;
        rc = m_pCamOpsTbl->ops->sync_related_sensors(
                m_pCamOpsTbl->camera_handle, m_pRelCamSyncBuf);
    } else {
        LOGE("Related Cam SyncBuffer not allocated", rc);
        return NO_INIT;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : getRelatedCamCalibration
 *
 * DESCRIPTION: fetch the related camera subsystem calibration data
 *
 * PARAMETERS :
 *   @calib  : calibration data fetched
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::getRelatedCamCalibration(
        cam_related_system_calibration_data_t* calib)
{
    int32_t rc = NO_ERROR;

    if(!calib) {
        return BAD_TYPE;
    }

    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    ADD_GET_PARAM_ENTRY_TO_BATCH(m_pParamBuf,
            CAM_INTF_PARM_RELATED_SENSORS_CALIBRATION);

    rc = commitGetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to get related cam calibration info");
        return rc;
    }

    READ_PARAM_ENTRY(m_pParamBuf,
            CAM_INTF_PARM_RELATED_SENSORS_CALIBRATION, *calib);

    LOGD("CALIB version %d ", calib->calibration_format_version);
    LOGD("CALIB normalized_focal_length %f ",
            calib->main_cam_specific_calibration.normalized_focal_length);
    LOGD("CALIB native_sensor_resolution_width %d ",
            calib->main_cam_specific_calibration.native_sensor_resolution_width);
    LOGD("CALIB native_sensor_resolution_height %d ",
            calib->main_cam_specific_calibration.native_sensor_resolution_height);
    LOGD("CALIB sensor_resolution_width %d ",
            calib->main_cam_specific_calibration.calibration_sensor_resolution_width);
    LOGD("CALIB sensor_resolution_height %d ",
            calib->main_cam_specific_calibration.calibration_sensor_resolution_height);
    LOGD("CALIB focal_length_ratio %f ",
            calib->main_cam_specific_calibration.focal_length_ratio);

    return rc;
}

/*===========================================================================
 * FUNCTION   : initBatchUpdate
 *
 * DESCRIPTION: init camera parameters buf entries
 *
 * PARAMETERS :
 *   @p_table : ptr to parameter buffer
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::initBatchUpdate(parm_buffer_t *p_table)
{
    m_tempMap.clear();
    clear_metadata_buffer(p_table);
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : commitSetBatch
 *
 * DESCRIPTION: commit all set parameters in the batch work to backend
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::commitSetBatch()
{
    int32_t rc = NO_ERROR;
    int32_t i = 0;

    if (NULL == m_pParamBuf) {
        LOGE("Params not initialized");
        return NO_INIT;
    }

    /* Loop to check if atleast one entry is valid */
    for(i = 0; i < CAM_INTF_PARM_MAX; i++){
        if(m_pParamBuf->is_valid[i])
            break;
    }

    if (NULL == m_pCamOpsTbl) {
        LOGE("Ops not initialized");
        return NO_INIT;
    }

    if (i < CAM_INTF_PARM_MAX) {
        rc = m_pCamOpsTbl->ops->set_parms(m_pCamOpsTbl->camera_handle, m_pParamBuf);
    }
    if (rc == NO_ERROR) {
        // commit change from temp storage into param map
        rc = commitParamChanges();
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : commitGetBatch
 *
 * DESCRIPTION: commit all get parameters in the batch work to backend
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::commitGetBatch()
{
    int32_t rc = NO_ERROR;
    int32_t i = 0;

    if (NULL == m_pParamBuf) {
        LOGE("Params not initialized");
        return NO_INIT;
    }

    /* Loop to check if atleast one entry is valid */
    for(i = 0; i < CAM_INTF_PARM_MAX; i++){
        if(m_pParamBuf->is_valid[i])
            break;
    }

    if (NULL == m_pCamOpsTbl) {
        LOGE("Ops not initialized");
        return NO_INIT;
    }

    if (i < CAM_INTF_PARM_MAX) {
        return m_pCamOpsTbl->ops->get_parms(m_pCamOpsTbl->camera_handle, m_pParamBuf);
    } else {
        return NO_ERROR;
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : updateParamEntry
 *
 * DESCRIPTION: update a parameter entry in the local temp map obj
 *
 * PARAMETERS :
 *   @key     : key of the entry
 *   @value   : value of the entry
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::updateParamEntry(const char *key, const char *value)
{
    m_tempMap.replaceValueFor(String8(key), String8(value));
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : commitParamChanges
 *
 * DESCRIPTION: commit all changes in local temp map obj into parameter obj
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::commitParamChanges()
{
    size_t size = m_tempMap.size();
    for (size_t i = 0; i < size; i++) {
        String8 k, v;
        k = m_tempMap.keyAt(i);
        v = m_tempMap.valueAt(i);
        set(k, v);
    }
    m_tempMap.clear();

    // update local changes
    m_bRecordingHint = m_bRecordingHint_new;
    m_bZslMode = m_bZslMode_new;

    /* After applying scene mode auto,
      Camera effects need to be reapplied */
    if ( m_bSceneTransitionAuto ) {
        m_bUpdateEffects = true;
        m_bSceneTransitionAuto = false;
    }


    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : QCameraReprocScaleParam
 *
 * DESCRIPTION: constructor of QCameraReprocScaleParam
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCameraParameters::QCameraReprocScaleParam::QCameraReprocScaleParam()
  : mScaleEnabled(false),
    mIsUnderScaling(false),
    mNeedScaleCnt(0),
    mSensorSizeTblCnt(0),
    mSensorSizeTbl(NULL),
    mTotalSizeTblCnt(0)
{
    mPicSizeFromAPK.width = 0;
    mPicSizeFromAPK.height = 0;
    mPicSizeSetted.width = 0;
    mPicSizeSetted.height = 0;
    memset(mNeedScaledSizeTbl, 0, sizeof(mNeedScaledSizeTbl));
    memset(mTotalSizeTbl, 0, sizeof(mTotalSizeTbl));
}

/*===========================================================================
 * FUNCTION   : ~~QCameraReprocScaleParam
 *
 * DESCRIPTION: destructor of QCameraReprocScaleParam
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCameraParameters::QCameraReprocScaleParam::~QCameraReprocScaleParam()
{
    //do nothing now.
}

/*===========================================================================
 * FUNCTION   : setScaledSizeTbl
 *
 * DESCRIPTION: re-set picture size table with dimensions that need scaling if Reproc Scale is enabled
 *
 * PARAMETERS :
 *   @scale_cnt   : count of picture sizes that want scale
 *   @scale_tbl    : picture size table that want scale
 *   @org_cnt     : sensor supported picture size count
 *   @org_tbl      : sensor supported picture size table
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::QCameraReprocScaleParam::setScaleSizeTbl(size_t scale_cnt,
        cam_dimension_t *scale_tbl, size_t org_cnt, cam_dimension_t *org_tbl)
{
    int32_t rc = NO_ERROR;
    size_t i;
    mNeedScaleCnt = 0;

    if(!mScaleEnabled || scale_cnt <=0 || scale_tbl == NULL || org_cnt <=0 || org_tbl == NULL){
        return BAD_VALUE;    // Do not need scale, so also need not reset picture size table
    }

    mSensorSizeTblCnt = org_cnt;
    mSensorSizeTbl = org_tbl;
    mNeedScaleCnt = checkScaleSizeTable(scale_cnt, scale_tbl, org_cnt, org_tbl);
    if(mNeedScaleCnt <= 0){
        LOGE("do not have picture sizes need scaling.");
        return BAD_VALUE;
    }

    if(mNeedScaleCnt + org_cnt > MAX_SIZES_CNT){
        LOGE("picture size list exceed the max count.");
        return BAD_VALUE;
    }

    //get the total picture size table
    mTotalSizeTblCnt = mNeedScaleCnt + org_cnt;

    if (mNeedScaleCnt > MAX_SCALE_SIZES_CNT) {
        LOGE("Error!! mNeedScaleCnt (%d) is more than MAX_SCALE_SIZES_CNT",
                 mNeedScaleCnt);
        return BAD_VALUE;
    }

    for(i = 0; i < mNeedScaleCnt; i++){
        mTotalSizeTbl[i].width = mNeedScaledSizeTbl[i].width;
        mTotalSizeTbl[i].height = mNeedScaledSizeTbl[i].height;
        LOGH("scale picture size: i =%d, width=%d, height=%d.",
            i, mTotalSizeTbl[i].width, mTotalSizeTbl[i].height);
    }
    for(; i < mTotalSizeTblCnt; i++){
        mTotalSizeTbl[i].width = org_tbl[i-mNeedScaleCnt].width;
        mTotalSizeTbl[i].height = org_tbl[i-mNeedScaleCnt].height;
        LOGH("sensor supportted picture size: i =%d, width=%d, height=%d.",
            i, mTotalSizeTbl[i].width, mTotalSizeTbl[i].height);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : getScaledSizeTblCnt
 *
 * DESCRIPTION: get picture size cnt that need scale
 *
 * PARAMETERS : none
 *
 * RETURN     : uint8_t type of picture size count
 *==========================================================================*/
size_t QCameraParameters::QCameraReprocScaleParam::getScaleSizeTblCnt()
{
    return mNeedScaleCnt;
}

/*===========================================================================
 * FUNCTION   : getScaledSizeTbl
 *
 * DESCRIPTION: get picture size table that need scale
 *
 * PARAMETERS :  none
 *
 * RETURN     : cam_dimension_t list of picture size table
 *==========================================================================*/
cam_dimension_t *QCameraParameters::QCameraReprocScaleParam::getScaledSizeTbl()
{
    if(!mScaleEnabled)
        return NULL;

    return mNeedScaledSizeTbl;
}

/*===========================================================================
 * FUNCTION   : setScaleEnable
 *
 * DESCRIPTION: enable or disable Reproc Scale
 *
 * PARAMETERS :
 *   @enabled : enable: 1; disable 0
 *
 * RETURN     : none
 *==========================================================================*/
void QCameraParameters::QCameraReprocScaleParam::setScaleEnable(bool enabled)
{
    mScaleEnabled = enabled;
}

/*===========================================================================
 * FUNCTION   : isScaleEnabled
 *
 * DESCRIPTION: check if Reproc Scale is enabled
 *
 * PARAMETERS :  none
 *
 * RETURN     : bool type of status
 *==========================================================================*/
bool QCameraParameters::QCameraReprocScaleParam::isScaleEnabled()
{
    return mScaleEnabled;
}

/*===========================================================================
 * FUNCTION   : isScalePicSize
 *
 * DESCRIPTION: check if current picture size is from Scale Table
 *
 * PARAMETERS :
 *   @width     : current picture width
 *   @height    : current picture height
 *
 * RETURN     : bool type of status
 *==========================================================================*/
bool QCameraParameters::QCameraReprocScaleParam::isScalePicSize(int width, int height)
{
    //Check if the picture size is in scale table
    if(mNeedScaleCnt <= 0)
        return FALSE;

    for (size_t i = 0; i < mNeedScaleCnt; i++) {
        if ((mNeedScaledSizeTbl[i].width == width) && (mNeedScaledSizeTbl[i].height == height)) {
            //found match
            return TRUE;
        }
    }

    LOGE("Not in scale picture size table.");
    return FALSE;
}

/*===========================================================================
 * FUNCTION   : isValidatePicSize
 *
 * DESCRIPTION: check if current picture size is validate
 *
 * PARAMETERS :
 *   @width     : current picture width
 *   @height    : current picture height
 *
 * RETURN     : bool type of status
 *==========================================================================*/
bool QCameraParameters::QCameraReprocScaleParam::isValidatePicSize(int width, int height)
{
    size_t i = 0;

    for(i = 0; i < mSensorSizeTblCnt; i++){
        if(mSensorSizeTbl[i].width == width
            && mSensorSizeTbl[i].height== height){
            return TRUE;
        }
    }

    for(i = 0; i < mNeedScaleCnt; i++){
        if(mNeedScaledSizeTbl[i].width == width
            && mNeedScaledSizeTbl[i].height== height){
            return TRUE;
        }
    }

    LOGE("Invalidate input picture size.");
    return FALSE;
}

/*===========================================================================
 * FUNCTION   : setSensorSupportedPicSize
 *
 * DESCRIPTION: set sensor supported picture size.
 *    For Snapshot stream size configuration, we need use sensor supported size.
 *    We will use CPP to do Scaling based on output Snapshot stream.
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::QCameraReprocScaleParam::setSensorSupportedPicSize()
{
    //will find a suitable picture size (here we leave a prossibility to add other scale requirement)
    //Currently we only focus on upscaling, and checkScaleSizeTable() has guaranteed the dimension ratio.

    if(!mIsUnderScaling || mSensorSizeTblCnt <= 0)
        return BAD_VALUE;

    //We just get the max sensor supported size here.
    mPicSizeSetted.width = mSensorSizeTbl[0].width;
    mPicSizeSetted.height = mSensorSizeTbl[0].height;

    return NO_ERROR;
}


/*===========================================================================
 * FUNCTION   : setValidatePicSize
 *
 * DESCRIPTION: set sensor supported size and change scale status.
 *
 * PARAMETERS :
 *   @width    : input picture width
 *   @height   : input picture height
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::QCameraReprocScaleParam::setValidatePicSize(int &width,int &height)
{
    if(!mScaleEnabled)
        return BAD_VALUE;

    mIsUnderScaling = FALSE; //default: not under scale

    if(isScalePicSize(width, height)){
        // input picture size need scaling operation. Record size from APK and setted
        mIsUnderScaling = TRUE;
        mPicSizeFromAPK.width = width;
        mPicSizeFromAPK.height = height;

        if(setSensorSupportedPicSize() != NO_ERROR)
            return BAD_VALUE;

        //re-set picture size to sensor supported size
        width = mPicSizeSetted.width;
        height = mPicSizeSetted.height;
        LOGH("mPicSizeFromAPK- with=%d, height=%d, mPicSizeSetted- with =%d, height=%d.",
             mPicSizeFromAPK.width, mPicSizeFromAPK.height, mPicSizeSetted.width, mPicSizeSetted.height);
    }else{
        mIsUnderScaling = FALSE;
        //no scale is needed for input picture size
        if(!isValidatePicSize(width, height)){
            LOGE("invalidate input picture size.");
            return BAD_VALUE;
        }
        mPicSizeSetted.width = width;
        mPicSizeSetted.height = height;
    }

    LOGH("X. mIsUnderScaling=%d, width=%d, height=%d.", mIsUnderScaling, width, height);
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : getPicSizeFromAPK
 *
 * DESCRIPTION: get picture size that get from APK
 *
 * PARAMETERS :
 *   @width     : input width
 *   @height    : input height
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::QCameraReprocScaleParam::getPicSizeFromAPK(int &width, int &height)
{
    if(!mIsUnderScaling)
        return BAD_VALUE;

    width = mPicSizeFromAPK.width;
    height = mPicSizeFromAPK.height;
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : getPicSizeSetted
 *
 * DESCRIPTION: get picture size that setted into mm-camera
 *
 * PARAMETERS :
 *   @width     : input width
 *   @height    : input height
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::QCameraReprocScaleParam::getPicSizeSetted(int &width, int &height)
{
    width = mPicSizeSetted.width;
    height = mPicSizeSetted.height;
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : isUnderScaling
 *
 * DESCRIPTION: check if we are in Reproc Scaling requirment
 *
 * PARAMETERS :  none
 *
 * RETURN     : bool type of status
 *==========================================================================*/
bool QCameraParameters::QCameraReprocScaleParam::isUnderScaling()
{
    return mIsUnderScaling;
}

/*===========================================================================
 * FUNCTION   : checkScaleSizeTable
 *
 * DESCRIPTION: check PICTURE_SIZE_NEED_SCALE to choose
 *
 * PARAMETERS :
 *   @scale_cnt   : count of picture sizes that want scale
 *   @scale_tbl    : picture size table that want scale
 *   @org_cnt     : sensor supported picture size count
 *   @org_tbl      : sensor supported picture size table
 *
 * RETURN     : bool type of status
 *==========================================================================*/
size_t QCameraParameters::QCameraReprocScaleParam::checkScaleSizeTable(size_t scale_cnt,
        cam_dimension_t *scale_tbl, size_t org_cnt, cam_dimension_t *org_tbl)
{
    size_t stbl_cnt = 0;
    size_t temp_cnt = 0;
    ssize_t i = 0;
    if(scale_cnt <=0 || scale_tbl == NULL || org_tbl == NULL || org_cnt <= 0)
        return stbl_cnt;

    //get validate scale size table. Currently we only support:
    // 1. upscale. The scale size must larger than max sensor supported size
    // 2. Scale dimension ratio must be same as the max sensor supported size.
    temp_cnt = scale_cnt;
    for (i = (ssize_t)(scale_cnt - 1); i >= 0; i--) {
        if (scale_tbl[i].width > org_tbl[0].width ||
                (scale_tbl[i].width == org_tbl[0].width &&
                    scale_tbl[i].height > org_tbl[0].height)) {
            //get the smallest scale size
            break;
        }
        temp_cnt--;
    }

    //check dimension ratio
    double supported_ratio = (double)org_tbl[0].width / (double)org_tbl[0].height;
    for (i = 0; i < (ssize_t)temp_cnt; i++) {
        double cur_ratio = (double)scale_tbl[i].width / (double)scale_tbl[i].height;
        if (fabs(supported_ratio - cur_ratio) > ASPECT_TOLERANCE) {
            continue;
        }
        mNeedScaledSizeTbl[stbl_cnt].width = scale_tbl[i].width;
        mNeedScaledSizeTbl[stbl_cnt].height= scale_tbl[i].height;
        stbl_cnt++;
    }

    return stbl_cnt;
}

/*===========================================================================
 * FUNCTION   : getTotalSizeTblCnt
 *
 * DESCRIPTION: get total picture size count after adding dimensions that need scaling
 *
 * PARAMETERS : none
 *
 * RETURN     : uint8_t type of picture size count
 *==========================================================================*/
size_t QCameraParameters::QCameraReprocScaleParam::getTotalSizeTblCnt()
{
    return mTotalSizeTblCnt;
}

/*===========================================================================
 * FUNCTION   : getTotalSizeTbl
 *
 * DESCRIPTION: get picture size table after adding dimensions that need scaling
 *
 * PARAMETERS :  none
 *
 * RETURN     : cam_dimension_t list of picture size table
 *==========================================================================*/
cam_dimension_t *QCameraParameters::QCameraReprocScaleParam::getTotalSizeTbl()
{
    if(!mScaleEnabled)
        return NULL;

    return mTotalSizeTbl;
}

/*===========================================================================
 * FUNCTION   : setEztune
 *
 * DESCRIPTION: Enable/Disable EZtune
 *
 *==========================================================================*/
int32_t QCameraParameters::setEztune()
{
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.eztune.enable", prop, "0");
    m_bEztuneEnabled = atoi(prop);
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : isHDREnabled
 *
 * DESCRIPTION: if HDR is enabled
 *
 * PARAMETERS : none
 *
 * RETURN     : true: needed
 *              false: no need
 *==========================================================================*/
bool QCameraParameters::isHDREnabled()
{
    return ((m_bHDREnabled || m_HDRSceneEnabled));
}

/*===========================================================================
 * FUNCTION   : isAVTimerEnabled
 *
 * DESCRIPTION: if AVTimer is enabled
 *
 * PARAMETERS : none
 *
 * RETURN     : true: needed
 *              false: no need
 *==========================================================================*/
bool QCameraParameters::isAVTimerEnabled()
{
    return m_bAVTimerEnabled;
}

/*===========================================================================
* FUNCTION   : isDISEnabled
*
* DESCRIPTION: if DIS is enabled
*
* PARAMETERS : none
*
* RETURN    : true: needed
*               false: no need
*==========================================================================*/
bool QCameraParameters::isDISEnabled()
{
    return m_bDISEnabled;
}

/*===========================================================================
* FUNCTION   : setISType
*
* DESCRIPTION: Set both Preview & Video IS type by reading the correspoding setprop's
*
* PARAMETERS : none
*
* RETURN     : IS type
*
*==========================================================================*/
int32_t QCameraParameters::setISType()
{
    bool eisSupported = false, eis3Supported = false;
    for (size_t i = 0; i < m_pCapability->supported_is_types_cnt; i++) {
        if ((m_pCapability->supported_is_types[i] == IS_TYPE_EIS_2_0) ||
                (m_pCapability->supported_is_types[i] == IS_TYPE_EIS_3_0)) {
            eisSupported = true;
        }
        if (m_pCapability->supported_is_types[i] == IS_TYPE_EIS_3_0) {
            eis3Supported = TRUE;
        }
    }
    if (m_bDISEnabled && eisSupported) {
        char value[PROPERTY_VALUE_MAX];
        // Make default value for Video IS_TYPE as IS_TYPE_EIS_2_0
        property_get("persist.camera.is_type", value, "4");
        mIsTypeVideo = static_cast<cam_is_type_t>(atoi(value));
        if ( (mIsTypeVideo == IS_TYPE_EIS_3_0) && (eis3Supported == FALSE) ) {
            LOGW("EIS_3.0 is not supported and so setting EIS_2.0");
            mIsTypeVideo = IS_TYPE_EIS_2_0;
        }
        // Make default value for preview IS_TYPE as IS_TYPE_EIS_2_0
        property_get("persist.camera.is_type_preview", value, "4");
        mIsTypePreview = static_cast<cam_is_type_t>(atoi(value));
    } else {
        mIsTypeVideo = IS_TYPE_NONE;
        mIsTypePreview = IS_TYPE_NONE;
    }
    return NO_ERROR;
}

/*===========================================================================
* FUNCTION   : getISType
*
* DESCRIPTION: returns IS type
*
* PARAMETERS : none
*
* RETURN     : IS type
*
*==========================================================================*/
cam_is_type_t QCameraParameters::getVideoISType()
{
    return mIsTypeVideo;
}

/*===========================================================================
* FUNCTION   : getPreviewISType
*
* DESCRIPTION: returns IS type for preview
*
* PARAMETERS : none
*
* RETURN     : IS type
*
*==========================================================================*/
cam_is_type_t QCameraParameters::getPreviewISType()
{
    return mIsTypePreview;
}

/*===========================================================================
 * FUNCTION   : MobicatMask
 *
 * DESCRIPTION: returns mobicat mask
 *
 * PARAMETERS : none
 *
 * RETURN     : mobicat mask
 *
 *==========================================================================*/
uint8_t QCameraParameters::getMobicatMask()
{
    return m_MobiMask;
}

/*===========================================================================
 * FUNCTION   : sendStreamConfigInfo
 *
 * DESCRIPTION: send Stream config info.
 *
 * PARAMETERS :
 *   @stream_config_info: Stream config information
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
bool QCameraParameters::sendStreamConfigInfo(cam_stream_size_info_t &stream_config_info) {
    int32_t rc = NO_ERROR;
    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf,
            CAM_INTF_META_STREAM_INFO, stream_config_info)) {
        LOGE("Failed to update table");
        return BAD_VALUE;
    }

    rc = commitSetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to set stream info parm");
        return rc;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : setStreamConfigure
 *
 * DESCRIPTION: set stream type, stream dimension for all configured streams.
 *
 * PARAMETERS :
 *   @isCapture: Whether this configureation is for an image capture
 *   @previewAsPostview: Use preview as postview
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
bool QCameraParameters::setStreamConfigure(bool isCapture,
        bool previewAsPostview, bool resetConfig) {

    int32_t rc = NO_ERROR;
    cam_stream_size_info_t stream_config_info;
    char value[PROPERTY_VALUE_MAX];
    bool raw_yuv = false;
    bool raw_capture = false;

    if ( m_pParamBuf == NULL ) {
        return NO_INIT;
    }

    memset(&stream_config_info, 0, sizeof(stream_config_info));
    stream_config_info.num_streams = 0;

    if (resetConfig) {
        LOGH("Reset stream config!!");
        rc = sendStreamConfigInfo(stream_config_info);
        LOGH("Done Resetting stream config!!");
        return rc;
    }

    stream_config_info.hfr_mode       = static_cast<cam_hfr_mode_t>(mHfrMode);
    stream_config_info.buf_alignment  = m_pCapability->buf_alignment;
    stream_config_info.min_stride     = m_pCapability->min_stride;
    stream_config_info.min_scanline   = m_pCapability->min_scanline;
    stream_config_info.batch_size = getBufBatchCount();

    LOGH("buf_alignment=%d stride X scan=%dx%d batch size = %d\n",
            m_pCapability->buf_alignment,
            m_pCapability->min_stride,
            m_pCapability->min_scanline,
            stream_config_info.batch_size);

    property_get("persist.camera.raw_yuv", value, "0");
    raw_yuv = atoi(value) > 0 ? true : false;

    if (isZSLMode() && getRecordingHintValue() != true) {
        stream_config_info.type[stream_config_info.num_streams] =
            CAM_STREAM_TYPE_PREVIEW;
        getStreamDimension(CAM_STREAM_TYPE_PREVIEW,
                stream_config_info.stream_sizes[stream_config_info.num_streams]);
        updatePpFeatureMask(CAM_STREAM_TYPE_PREVIEW);
        stream_config_info.postprocess_mask[stream_config_info.num_streams] =
                mStreamPpMask[CAM_STREAM_TYPE_PREVIEW];
        getStreamFormat(CAM_STREAM_TYPE_PREVIEW,
                stream_config_info.format[stream_config_info.num_streams]);
        stream_config_info.num_streams++;

        stream_config_info.type[stream_config_info.num_streams] =
                CAM_STREAM_TYPE_ANALYSIS;
        updatePpFeatureMask(CAM_STREAM_TYPE_ANALYSIS);
        getStreamDimension(CAM_STREAM_TYPE_ANALYSIS,
                stream_config_info.stream_sizes[stream_config_info.num_streams]);
        stream_config_info.postprocess_mask[stream_config_info.num_streams] =
                mStreamPpMask[CAM_STREAM_TYPE_ANALYSIS];
        getStreamFormat(CAM_STREAM_TYPE_ANALYSIS,
                stream_config_info.format[stream_config_info.num_streams]);
        stream_config_info.num_streams++;

        stream_config_info.type[stream_config_info.num_streams] =
                CAM_STREAM_TYPE_SNAPSHOT;
        getStreamDimension(CAM_STREAM_TYPE_SNAPSHOT,
                stream_config_info.stream_sizes[stream_config_info.num_streams]);
        updatePpFeatureMask(CAM_STREAM_TYPE_SNAPSHOT);
        stream_config_info.postprocess_mask[stream_config_info.num_streams] =
                mStreamPpMask[CAM_STREAM_TYPE_SNAPSHOT];
        getStreamFormat(CAM_STREAM_TYPE_SNAPSHOT,
                stream_config_info.format[stream_config_info.num_streams]);
        stream_config_info.num_streams++;

        if (isUBWCEnabled() && getRecordingHintValue() != true) {
            cam_format_t fmt;
            getStreamFormat(CAM_STREAM_TYPE_PREVIEW,fmt);
            if (fmt == CAM_FORMAT_YUV_420_NV12_UBWC) {
                stream_config_info.type[stream_config_info.num_streams] =
                        CAM_STREAM_TYPE_CALLBACK;
                getStreamDimension(CAM_STREAM_TYPE_CALLBACK,
                        stream_config_info.stream_sizes[stream_config_info.num_streams]);
                updatePpFeatureMask(CAM_STREAM_TYPE_CALLBACK);
                stream_config_info.postprocess_mask[stream_config_info.num_streams] =
                        mStreamPpMask[CAM_STREAM_TYPE_CALLBACK];
                getStreamFormat(CAM_STREAM_TYPE_CALLBACK,
                        stream_config_info.format[stream_config_info.num_streams]);
                stream_config_info.num_streams++;
            }
        }

    } else if (!isCapture) {
        if (m_bRecordingHint) {
            setISType();
            mIsTypeVideo = getVideoISType();
            mIsTypePreview = getPreviewISType();
            stream_config_info.is_type[stream_config_info.num_streams] = IS_TYPE_NONE;
            stream_config_info.type[stream_config_info.num_streams] =
                    CAM_STREAM_TYPE_SNAPSHOT;
            getStreamDimension(CAM_STREAM_TYPE_SNAPSHOT,
                    stream_config_info.stream_sizes[stream_config_info.num_streams]);
            updatePpFeatureMask(CAM_STREAM_TYPE_SNAPSHOT);
            stream_config_info.postprocess_mask[stream_config_info.num_streams] =
                    mStreamPpMask[CAM_STREAM_TYPE_SNAPSHOT];
            getStreamFormat(CAM_STREAM_TYPE_SNAPSHOT,
                        stream_config_info.format[stream_config_info.num_streams]);
            stream_config_info.num_streams++;
            stream_config_info.is_type[stream_config_info.num_streams] = mIsTypeVideo;
            stream_config_info.type[stream_config_info.num_streams] =
                    CAM_STREAM_TYPE_VIDEO;
            getStreamDimension(CAM_STREAM_TYPE_VIDEO,
                    stream_config_info.stream_sizes[stream_config_info.num_streams]);
            updatePpFeatureMask(CAM_STREAM_TYPE_VIDEO);
            stream_config_info.postprocess_mask[stream_config_info.num_streams] =
                    mStreamPpMask[CAM_STREAM_TYPE_VIDEO];
            getStreamFormat(CAM_STREAM_TYPE_VIDEO,
                    stream_config_info.format[stream_config_info.num_streams]);
            stream_config_info.num_streams++;
        }

        /* Analysis stream is needed by DCRF regardless of recording hint */
        if ((getDcrf() == true) ||
                (getRecordingHintValue() != true) ||
                (fdModeInVideo())) {
            stream_config_info.type[stream_config_info.num_streams] =
                    CAM_STREAM_TYPE_ANALYSIS;
            updatePpFeatureMask(CAM_STREAM_TYPE_ANALYSIS);
            getStreamDimension(CAM_STREAM_TYPE_ANALYSIS,
                    stream_config_info.stream_sizes[stream_config_info.num_streams]);
            stream_config_info.postprocess_mask[stream_config_info.num_streams] =
                    mStreamPpMask[CAM_STREAM_TYPE_ANALYSIS];
            getStreamFormat(CAM_STREAM_TYPE_ANALYSIS,
                    stream_config_info.format[stream_config_info.num_streams]);
            stream_config_info.num_streams++;
        }

        stream_config_info.type[stream_config_info.num_streams] =
                CAM_STREAM_TYPE_PREVIEW;
        getStreamDimension(CAM_STREAM_TYPE_PREVIEW,
                stream_config_info.stream_sizes[stream_config_info.num_streams]);
        updatePpFeatureMask(CAM_STREAM_TYPE_PREVIEW);
        stream_config_info.postprocess_mask[stream_config_info.num_streams] =
                mStreamPpMask[CAM_STREAM_TYPE_PREVIEW];
        getStreamFormat(CAM_STREAM_TYPE_PREVIEW,
                    stream_config_info.format[stream_config_info.num_streams]);
        stream_config_info.is_type[stream_config_info.num_streams] = mIsTypePreview;
        stream_config_info.num_streams++;

        if (isUBWCEnabled() && getRecordingHintValue() != true) {
            cam_format_t fmt;
            getStreamFormat(CAM_STREAM_TYPE_PREVIEW,fmt);
            if (fmt == CAM_FORMAT_YUV_420_NV12_UBWC) {
                stream_config_info.type[stream_config_info.num_streams] =
                        CAM_STREAM_TYPE_CALLBACK;
                getStreamDimension(CAM_STREAM_TYPE_CALLBACK,
                        stream_config_info.stream_sizes[stream_config_info.num_streams]);
                updatePpFeatureMask(CAM_STREAM_TYPE_CALLBACK);
                stream_config_info.postprocess_mask[stream_config_info.num_streams] =
                        mStreamPpMask[CAM_STREAM_TYPE_CALLBACK];
                getStreamFormat(CAM_STREAM_TYPE_CALLBACK,
                        stream_config_info.format[stream_config_info.num_streams]);
                stream_config_info.is_type[stream_config_info.num_streams] = IS_TYPE_NONE;
                stream_config_info.num_streams++;
            }
        }

    } else {
        if (isJpegPictureFormat() || isNV16PictureFormat() || isNV21PictureFormat()) {
            if (!getofflineRAW()) {
                stream_config_info.type[stream_config_info.num_streams] =
                        CAM_STREAM_TYPE_SNAPSHOT;
                getStreamDimension(CAM_STREAM_TYPE_SNAPSHOT,
                        stream_config_info.stream_sizes[stream_config_info.num_streams]);
                updatePpFeatureMask(CAM_STREAM_TYPE_SNAPSHOT);
                stream_config_info.postprocess_mask[stream_config_info.num_streams] =
                        mStreamPpMask[CAM_STREAM_TYPE_SNAPSHOT];
                getStreamFormat(CAM_STREAM_TYPE_SNAPSHOT,
                        stream_config_info.format[stream_config_info.num_streams]);
                stream_config_info.is_type[stream_config_info.num_streams] = IS_TYPE_NONE;
                stream_config_info.num_streams++;
            }

            if (previewAsPostview) {
                stream_config_info.type[stream_config_info.num_streams] =
                        CAM_STREAM_TYPE_PREVIEW;
                getStreamDimension(CAM_STREAM_TYPE_PREVIEW,
                        stream_config_info.stream_sizes[stream_config_info.num_streams]);
                updatePpFeatureMask(CAM_STREAM_TYPE_PREVIEW);
                stream_config_info.postprocess_mask[stream_config_info.num_streams] =
                        mStreamPpMask[CAM_STREAM_TYPE_PREVIEW];
                getStreamFormat(CAM_STREAM_TYPE_PREVIEW,
                        stream_config_info.format[stream_config_info.num_streams]);
                stream_config_info.is_type[stream_config_info.num_streams] = IS_TYPE_NONE;
                stream_config_info.num_streams++;
            } else if(!getQuadraCfa()) {
                stream_config_info.type[stream_config_info.num_streams] =
                        CAM_STREAM_TYPE_POSTVIEW;
                getStreamDimension(CAM_STREAM_TYPE_POSTVIEW,
                        stream_config_info.stream_sizes[stream_config_info.num_streams]);
                updatePpFeatureMask(CAM_STREAM_TYPE_POSTVIEW);
                stream_config_info.postprocess_mask[stream_config_info.num_streams] =
                        mStreamPpMask[CAM_STREAM_TYPE_POSTVIEW];
                getStreamFormat(CAM_STREAM_TYPE_POSTVIEW,
                        stream_config_info.format[stream_config_info.num_streams]);
                stream_config_info.is_type[stream_config_info.num_streams] = IS_TYPE_NONE;
                stream_config_info.num_streams++;
            }
        } else {
            raw_capture = true;
            stream_config_info.type[stream_config_info.num_streams] =
                    CAM_STREAM_TYPE_RAW;
            getStreamDimension(CAM_STREAM_TYPE_RAW,
                    stream_config_info.stream_sizes[stream_config_info.num_streams]);
            updatePpFeatureMask(CAM_STREAM_TYPE_RAW);
            stream_config_info.postprocess_mask[stream_config_info.num_streams] =
                    mStreamPpMask[CAM_STREAM_TYPE_RAW];
            getStreamFormat(CAM_STREAM_TYPE_RAW,
                    stream_config_info.format[stream_config_info.num_streams]);
            stream_config_info.is_type[stream_config_info.num_streams] = IS_TYPE_NONE;
            stream_config_info.num_streams++;
        }
    }

    if ((!raw_capture) && ((getofflineRAW() && !getRecordingHintValue())
            || (raw_yuv))) {
        cam_dimension_t max_dim = {0,0};

        if (!getQuadraCfa()) {
            // Find the Maximum dimension admong all the streams
            for (uint32_t j = 0; j < stream_config_info.num_streams; j++) {
                if (stream_config_info.stream_sizes[j].width > max_dim.width) {
                    max_dim.width = stream_config_info.stream_sizes[j].width;
                }
                if (stream_config_info.stream_sizes[j].height > max_dim.height) {
                    max_dim.height = stream_config_info.stream_sizes[j].height;
                }
            }
        } else {
            max_dim.width = m_pCapability->quadra_cfa_dim[0].width;
            max_dim.height = m_pCapability->quadra_cfa_dim[0].height;
        }
        LOGH("Max Dimension = %d X %d", max_dim.width, max_dim.height);
        stream_config_info.type[stream_config_info.num_streams] =
            CAM_STREAM_TYPE_RAW;
        getStreamFormat(CAM_STREAM_TYPE_RAW,
                stream_config_info.format[stream_config_info.num_streams]);
        if (CAM_FORMAT_META_RAW_10BIT ==
            stream_config_info.format[stream_config_info.num_streams]) {
            int32_t dt = 0;
            int32_t vc = 0;
            cam_stream_size_info_t temp_stream_config_info;
            getStreamSubFormat(CAM_STREAM_TYPE_RAW,
                stream_config_info.sub_format_type[
                stream_config_info.num_streams]);
            /* Sending separate meta_stream_info so that other modules do
             * not confuse with original sendStreamConfigInfo(). This is only
             * for sensor where sensor can run pick resolusion for meta raw.
             */
            updateDtVc(&dt, &vc);
            stream_config_info.dt[stream_config_info.num_streams] = dt;
            stream_config_info.vc[stream_config_info.num_streams] = vc;
            memcpy(&temp_stream_config_info, &stream_config_info,
                sizeof(temp_stream_config_info));
            temp_stream_config_info.num_streams++;
            sendStreamConfigForPickRes(temp_stream_config_info);
            getMetaRawInfo();
        } else {
            updateRAW(max_dim);
        }
        getStreamDimension(CAM_STREAM_TYPE_RAW, stream_config_info.stream_sizes[
                stream_config_info.num_streams]);
        updatePpFeatureMask(CAM_STREAM_TYPE_RAW);
        stream_config_info.postprocess_mask[stream_config_info.num_streams] =
                mStreamPpMask[CAM_STREAM_TYPE_RAW];
        stream_config_info.num_streams++;
    }

    for (uint32_t k = 0; k < stream_config_info.num_streams; k++) {
        LOGI("STREAM INFO : type %d, wxh: %d x %d, pp_mask: 0x%llx \
                Format = %d, dt =%d cid =%d subformat =%d, is_type %d",
                stream_config_info.type[k],
                stream_config_info.stream_sizes[k].width,
                stream_config_info.stream_sizes[k].height,
                stream_config_info.postprocess_mask[k],
                stream_config_info.format[k],
                stream_config_info.dt[k],
                stream_config_info.vc[k],
                stream_config_info.sub_format_type[k],
                stream_config_info.is_type[k]);
    }

    rc = sendStreamConfigInfo(stream_config_info);
    return rc;
}

/*===========================================================================
 * FUNCTION   : addOnlineRotation
 *
 * DESCRIPTION: send additional rotation information for specific stream
 *
 * PARAMETERS :
 *   @rotation: rotation
 *   @streamId: internal stream id
 *   @device_rotation: device rotation
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::addOnlineRotation(uint32_t rotation, uint32_t streamId,
        int32_t device_rotation)
{
    int32_t rc = NO_ERROR;
    cam_rotation_info_t rotation_info;
    memset(&rotation_info, 0, sizeof(cam_rotation_info_t));

    /* Add jpeg rotation information */
    if (rotation == 0) {
        rotation_info.rotation = ROTATE_0;
    } else if (rotation == 90) {
        rotation_info.rotation = ROTATE_90;
    } else if (rotation == 180) {
        rotation_info.rotation = ROTATE_180;
    } else if (rotation == 270) {
        rotation_info.rotation = ROTATE_270;
    } else {
        rotation_info.rotation = ROTATE_0;
    }
    rotation_info.streamId = streamId;

    /* Add device rotation information */
    if (device_rotation == 0) {
        rotation_info.device_rotation = ROTATE_0;
    } else if (device_rotation == 90) {
        rotation_info.device_rotation = ROTATE_90;
    } else if (device_rotation == 180) {
        rotation_info.device_rotation = ROTATE_180;
    } else if (device_rotation == 270) {
        rotation_info.device_rotation = ROTATE_270;
    } else {
        rotation_info.device_rotation = ROTATE_0;
    }

    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_ROTATION, rotation_info)) {
        LOGE("Failed to update table");
        return BAD_VALUE;
    }

    rc = commitSetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to set stream info parm");
        return rc;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : needThumbnailReprocess
 *
 * DESCRIPTION: Check if thumbnail reprocessing is needed
 *
 * PARAMETERS : @pFeatureMask - feature mask
 *
 * RETURN     : true: needed
 *              false: no need
 *==========================================================================*/
bool QCameraParameters::needThumbnailReprocess(cam_feature_mask_t *pFeatureMask)
{
    if (isUbiFocusEnabled() || isChromaFlashEnabled() ||
            isOptiZoomEnabled() || isUbiRefocus() ||
            isStillMoreEnabled() ||
            (isHDREnabled() && !isHDRThumbnailProcessNeeded())
            || isUBWCEnabled()|| getQuadraCfa()) {
        *pFeatureMask &= ~CAM_QCOM_FEATURE_CHROMA_FLASH;
        *pFeatureMask &= ~CAM_QCOM_FEATURE_UBIFOCUS;
        *pFeatureMask &= ~CAM_QCOM_FEATURE_REFOCUS;
        *pFeatureMask &= ~CAM_QCOM_FEATURE_OPTIZOOM;
        *pFeatureMask &= ~CAM_QCOM_FEATURE_STILLMORE;
        *pFeatureMask &= ~CAM_QCOM_FEATURE_HDR;
        return false;
    } else {
        cam_dimension_t thumb_dim;
        getThumbnailSize(&(thumb_dim.width), &(thumb_dim.height));
        if (thumb_dim.width == 0 || thumb_dim.height == 0) {
            return false;
        }
        else {
            return true;
        }
    }
}

/*===========================================================================
 * FUNCTION   : getNumOfExtraBuffersForImageProc
 *
 * DESCRIPTION: get number of extra input buffers needed by image processing
 *
 * PARAMETERS : none
 *
 * RETURN     : number of extra buffers needed by ImageProc;
 *              0 if not ImageProc enabled
 *==========================================================================*/
uint8_t QCameraParameters::getNumOfExtraBuffersForImageProc()
{
    int numOfBufs = 0;

    if (isUbiRefocus()) {
        return (uint8_t)(m_pCapability->refocus_af_bracketing_need.burst_count - 1);
    } else if (isUbiFocusEnabled()) {
        numOfBufs += m_pCapability->ubifocus_af_bracketing_need.burst_count - 1;
    } else if (m_bOptiZoomOn) {
        numOfBufs += m_pCapability->opti_zoom_settings_need.burst_count - 1;
    } else if (isChromaFlashEnabled()) {
        numOfBufs += m_pCapability->chroma_flash_settings_need.burst_count - 1;
    } else if (isStillMoreEnabled()) {
        if (isSeeMoreEnabled()) {
            m_stillmore_config.burst_count = 1;
        } else if ((m_stillmore_config.burst_count >=
                m_pCapability->stillmore_settings_need.min_burst_count) &&
                (m_stillmore_config.burst_count <=
                m_pCapability->stillmore_settings_need.max_burst_count)) {
            numOfBufs += m_stillmore_config.burst_count - 1;
        } else {
            numOfBufs += m_pCapability->stillmore_settings_need.burst_count - 1;
        }
    } else if (isOEMFeatEnabled()) {
        numOfBufs += 1;
    }

    if (getQuadraCfa()) {
        numOfBufs += 1;
    }

    return (uint8_t)(numOfBufs);
}

/*===========================================================================
 * FUNCTION   : getExifBufIndex
 *
 * DESCRIPTION: get index of metadata to be used for EXIF
 *
 * PARAMETERS : @captureIndex - index of current captured frame
 *
 * RETURN     : index of metadata to be used for EXIF
 *==========================================================================*/
uint32_t QCameraParameters::getExifBufIndex(uint32_t captureIndex)
{
    uint32_t index = captureIndex;

    if (isUbiRefocus()) {
        if (captureIndex < m_pCapability->refocus_af_bracketing_need.burst_count) {
            index = captureIndex;
        } else {
            index = 0;
        }
    } else if (isChromaFlashEnabled()) {
        index = m_pCapability->chroma_flash_settings_need.metadata_index;
    } else if (isHDREnabled()) {
        if (isHDR1xFrameEnabled() && isHDR1xExtraBufferNeeded()) {
            index = m_pCapability->hdr_bracketing_setting.num_frames;
        } else {
            for (index = 0; index < m_pCapability->hdr_bracketing_setting.num_frames; index++) {
                if (0 == m_pCapability->hdr_bracketing_setting.exp_val.values[index]) {
                    break;
                }
            }
            if (index == m_pCapability->hdr_bracketing_setting.num_frames) {
                index = captureIndex;
            }
        }
    }

    return index;
}

/*===========================================================================
 * FUNCTION   : getNumberInBufsForSingleShot
 *
 * DESCRIPTION: get number of input buffers for single shot
 *
 * PARAMETERS : none
 *
 * RETURN     : number of input buffers for single shot
 *==========================================================================*/
uint32_t QCameraParameters::getNumberInBufsForSingleShot()
{
    uint32_t numOfBufs = 1;

    if (isUbiRefocus()) {
        numOfBufs = m_pCapability->refocus_af_bracketing_need.burst_count;
    } else if (isUbiFocusEnabled()) {
        numOfBufs = m_pCapability->ubifocus_af_bracketing_need.burst_count;
    } else if (m_bOptiZoomOn) {
        numOfBufs = m_pCapability->opti_zoom_settings_need.burst_count;
    } else if (isChromaFlashEnabled()) {
        numOfBufs = m_pCapability->chroma_flash_settings_need.burst_count;
    } else if (isHDREnabled()) {
        numOfBufs = m_pCapability->hdr_bracketing_setting.num_frames;
        if (isHDR1xFrameEnabled() && isHDR1xExtraBufferNeeded()) {
            numOfBufs++;
        }
    } else if (isStillMoreEnabled()) {
        if (isSeeMoreEnabled()) {
            m_stillmore_config.burst_count = 1;
            numOfBufs = m_stillmore_config.burst_count;
        } else if ((m_stillmore_config.burst_count >=
                m_pCapability->stillmore_settings_need.min_burst_count) &&
                (m_stillmore_config.burst_count <=
                m_pCapability->stillmore_settings_need.max_burst_count)) {
            numOfBufs = m_stillmore_config.burst_count;
        } else {
            numOfBufs = m_pCapability->stillmore_settings_need.burst_count;
        }
    }

    return numOfBufs;
}

/*===========================================================================
 * FUNCTION   : getNumberOutBufsForSingleShot
 *
 * DESCRIPTION: get number of output buffers for single shot
 *
 * PARAMETERS : none
 *
 * RETURN     : number of output buffers for single shot
 *==========================================================================*/
uint32_t QCameraParameters::getNumberOutBufsForSingleShot()
{
    uint32_t numOfBufs = 1;

    if (isUbiRefocus()) {
        numOfBufs = m_pCapability->refocus_af_bracketing_need.output_count;
    } else if (isHDREnabled()) {
        if (isHDR1xFrameEnabled()) {
            numOfBufs++;
        }
    }

    return numOfBufs;
}

/*===========================================================================
 * FUNCTION   : is4k2kVideoResolution
 *
 * DESCRIPTION: if resolution is 4k x 2k or true 4k x 2k
 *
 * PARAMETERS : none
 *
 * RETURN     : true: video resolution is 4k x 2k
 *              false: video resolution is not 4k x 2k
 *==========================================================================*/
bool QCameraParameters::is4k2kVideoResolution()
{
   bool enabled = false;
   cam_dimension_t resolution;
   getVideoSize(&resolution.width, &resolution.height);
   if (!(resolution.width < 3840 && resolution.height < 2160)) {
      enabled = true;
   }

   return enabled;
}

/*===========================================================================
 * FUNCTION   : isPreviewSeeMoreRequired
 *
 * DESCRIPTION: This function checks whether SeeMmore(SW TNR) needs to be applied for
 *              preview stream depending on video resoluion and setprop
 *
 * PARAMETERS : none
 *
 * RETURN     : true: If SeeMore needs to apply
 *              false: No need to apply
 *==========================================================================*/
bool QCameraParameters::isPreviewSeeMoreRequired()
{
   cam_dimension_t dim;
   char prop[PROPERTY_VALUE_MAX];

   getVideoSize(&dim.width, &dim.height);
   memset(prop, 0, sizeof(prop));
   property_get("persist.camera.preview.seemore", prop, "0");
   int enable = atoi(prop);

   // Enable SeeMore for preview stream if :
   // 1. Video resolution <= (1920x1080)  (or)
   // 2. persist.camera.preview.seemore is set
   LOGD("width=%d, height=%d, enable=%d", dim.width, dim.height, enable);
   return (((dim.width * dim.height) <= (1920 * 1080)) || enable);
}

/*===========================================================================
 * FUNCTION   : updateDebugLevel
 *
 * DESCRIPTION: send CAM_INTF_PARM_UPDATE_DEBUG_LEVEL to backend
 *
 * PARAMETERS : none
 *
 * RETURN     : NO_ERROR --success
 *              int32_t type of status
 *==========================================================================*/
int32_t QCameraParameters::updateDebugLevel()
{
    if ( m_pParamBuf == NULL ) {
        return NO_INIT;
    }

    int32_t rc = initBatchUpdate(m_pParamBuf);
    if ( rc != NO_ERROR ) {
        LOGE("Failed to initialize group update table");
        return rc;
    }

    uint32_t dummyDebugLevel = 0;
    /* The value of dummyDebugLevel is irrelavent. On
     * CAM_INTF_PARM_UPDATE_DEBUG_LEVEL, read debug property */
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_UPDATE_DEBUG_LEVEL, dummyDebugLevel)) {
        LOGE("Parameters batch failed");
        return BAD_VALUE;
    }

    rc = commitSetBatch();
    if ( rc != NO_ERROR ) {
        LOGE("Failed to commit batch parameters");
        return rc;
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setOfflineRAW
 *
 * DESCRIPTION: Function to decide Offline RAW feature.
 *
 * PARAMETERS :
 *  @raw_value: offline raw value to set.
 *
 * RETURN     : none
 *==========================================================================*/
void QCameraParameters::setOfflineRAW(bool raw_value)
{
    char value[PROPERTY_VALUE_MAX];
    bool raw_yuv = false;
    bool offlineRaw = false;

    if (raw_value) {
        mOfflineRAW = true;
        LOGH("Offline Raw  %d", mOfflineRAW);
        return;
    }

    property_get("persist.camera.raw_yuv", value, "0");
    raw_yuv = atoi(value) > 0 ? true : false;
    property_get("persist.camera.offlineraw", value, "0");
    offlineRaw = atoi(value) > 0 ? true : false;
    if ((raw_yuv || isRdiMode()) && offlineRaw) {
        mOfflineRAW = true;
    } else {
        mOfflineRAW = false;
    }
    LOGH("Offline Raw  %d", mOfflineRAW);
}

/*===========================================================================
 * FUNCTION   : updatePpFeatureMask
 *
 * DESCRIPTION: Updates the feature mask for a particular stream depending
 *              on current client configuration.
 *
 * PARAMETERS :
 *  @stream_type: Camera stream type
 *
 * RETURN     : NO_ERROR --success
 *              int32_t type of status
 *==========================================================================*/
int32_t QCameraParameters::updatePpFeatureMask(cam_stream_type_t stream_type) {

    cam_feature_mask_t feature_mask = 0;

    if (stream_type >= CAM_STREAM_TYPE_MAX) {
        LOGE("Error!! stream type: %d not valid", stream_type);
        return -1;
    }

    // Update feature mask for SeeMore in video and video preview
    if (isSeeMoreEnabled() && ((stream_type == CAM_STREAM_TYPE_VIDEO) ||
            (stream_type == CAM_STREAM_TYPE_PREVIEW && getRecordingHintValue() &&
            isPreviewSeeMoreRequired()))) {
       feature_mask |= CAM_QCOM_FEATURE_LLVD;
    }

    if (isHighQualityNoiseReductionMode() &&
            ((stream_type == CAM_STREAM_TYPE_VIDEO) ||
            (stream_type == CAM_STREAM_TYPE_PREVIEW && getRecordingHintValue() &&
            isPreviewSeeMoreRequired()))) {
        feature_mask |= CAM_QTI_FEATURE_SW_TNR;
    }

    // Do not enable feature mask for ZSL/non-ZSL/liveshot snapshot except for 4K2k case
    if ((getRecordingHintValue() &&
            (stream_type == CAM_STREAM_TYPE_SNAPSHOT) && is4k2kVideoResolution()) ||
            (stream_type != CAM_STREAM_TYPE_SNAPSHOT)) {
        if ((m_nMinRequiredPpMask & CAM_QCOM_FEATURE_SHARPNESS) &&
                !isOptiZoomEnabled()) {
            feature_mask |= CAM_QCOM_FEATURE_SHARPNESS;
        }

        if (m_nMinRequiredPpMask & CAM_QCOM_FEATURE_EFFECT) {
            feature_mask |= CAM_QCOM_FEATURE_EFFECT;
        }
        if (isWNREnabled()) {
            feature_mask |= CAM_QCOM_FEATURE_DENOISE2D;
        }

        //Set flip mode based on Stream type;
        int flipMode = getFlipMode(stream_type);
        if (flipMode > 0) {
            feature_mask |= CAM_QCOM_FEATURE_FLIP;
        }
    }

    if ((isTNRVideoEnabled() && (CAM_STREAM_TYPE_VIDEO == stream_type))
            || (isTNRPreviewEnabled() && (CAM_STREAM_TYPE_PREVIEW == stream_type))) {
        feature_mask |= CAM_QCOM_FEATURE_CPP_TNR;
    }
    if (isEztuneEnabled() &&
            ((CAM_STREAM_TYPE_PREVIEW == stream_type) ||
            (CAM_STREAM_TYPE_SNAPSHOT == stream_type))) {
        feature_mask |= CAM_QCOM_FEATURE_EZTUNE;
    }

    if ((getCDSMode() != CAM_CDS_MODE_OFF) &&
            ((CAM_STREAM_TYPE_PREVIEW == stream_type) ||
            (CAM_STREAM_TYPE_VIDEO == stream_type) ||
            (CAM_STREAM_TYPE_CALLBACK == stream_type) ||
            ((CAM_STREAM_TYPE_SNAPSHOT == stream_type) &&
            getRecordingHintValue() && is4k2kVideoResolution()))) {
         if (m_nMinRequiredPpMask & CAM_QCOM_FEATURE_DSDN) {
             feature_mask |= CAM_QCOM_FEATURE_DSDN;
         } else {
             feature_mask |= CAM_QCOM_FEATURE_CDS;
         }
    }

    if (isTNRSnapshotEnabled() && (CAM_STREAM_TYPE_SNAPSHOT == stream_type)
            && (isZSLMode() || getRecordingHintValue())) {
        feature_mask |= CAM_QCOM_FEATURE_CPP_TNR;
    }

    //Rotation could also have an effect on pp feature mask
    cam_pp_feature_config_t config;
    cam_dimension_t dim;
    memset(&config, 0, sizeof(cam_pp_feature_config_t));
    getStreamRotation(stream_type, config, dim);
    feature_mask |= config.feature_mask;

    // Dual Camera scenarios
    // all feature masks are disabled for preview and analysis streams for aux session
    // all required feature masks for aux session preview and analysis streams need
    // to be enabled explicitly here
    ///@note When aux camera is of bayer type, keep pp mask as is or we'd run
    ///      into stream mapping problems. YUV sensor is marked as interleaved and has
    ///      preferred mapping setup so we don't see any mapping issues.
    if (m_relCamSyncInfo.sync_control == CAM_SYNC_RELATED_SENSORS_ON) {
        if (((CAM_STREAM_TYPE_ANALYSIS == stream_type) ||
                (CAM_STREAM_TYPE_PREVIEW == stream_type)) &&
                (m_relCamSyncInfo.mode == CAM_MODE_SECONDARY) &&
                (m_pCapability->sensor_type.sens_type == CAM_SENSOR_YUV)) {
            LOGH("Disabling all pp feature masks for aux preview and "
                    "analysis streams");
            feature_mask = 0;
        }

        // all feature masks need to be enabled here
        // enable DCRF feature mask on analysis stream in case of dual camera
        if (m_bDcrfEnabled && (CAM_STREAM_TYPE_ANALYSIS == stream_type)) {
            feature_mask |= CAM_QCOM_FEATURE_DCRF;
        } else {
            feature_mask &= ~CAM_QCOM_FEATURE_DCRF;
        }
    }

    // Preview assisted autofocus needs to be supported for
    // callback, preview, or video streams
    cam_color_filter_arrangement_t filter_arrangement;
    filter_arrangement = m_pCapability->color_arrangement;
    switch (filter_arrangement) {
    case CAM_FILTER_ARRANGEMENT_RGGB:
    case CAM_FILTER_ARRANGEMENT_GRBG:
    case CAM_FILTER_ARRANGEMENT_GBRG:
    case CAM_FILTER_ARRANGEMENT_BGGR:
        if ((stream_type == CAM_STREAM_TYPE_CALLBACK) ||
            (stream_type == CAM_STREAM_TYPE_PREVIEW)) {
            feature_mask |= CAM_QCOM_FEATURE_PAAF;
        } else if (stream_type == CAM_STREAM_TYPE_VIDEO) {
            if (getVideoISType() != IS_TYPE_EIS_3_0)
                feature_mask |= CAM_QCOM_FEATURE_PAAF;
        }
        break;
    case CAM_FILTER_ARRANGEMENT_Y:
        if (stream_type == CAM_STREAM_TYPE_ANALYSIS) {
            feature_mask |= CAM_QCOM_FEATURE_PAAF;
            LOGH("add PAAF mask to feature_mask for mono device");
        }
        break;
    default:
        break;
    }

    // Enable PPEISCORE for EIS 3.0
    if ((stream_type == CAM_STREAM_TYPE_VIDEO) &&
            (getVideoISType() == IS_TYPE_EIS_3_0)) {
        feature_mask |= CAM_QTI_FEATURE_PPEISCORE;
    }

    // Store stream feature mask
    setStreamPpMask(stream_type, feature_mask);
    LOGH("stream type: %d, pp_mask: 0x%llx", stream_type, feature_mask);

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setStreamPpMask
 *
 * DESCRIPTION: Stores a particular feature mask for a given camera stream
 *
 * PARAMETERS :
 *  @stream_type: Camera stream type
 *  @pp_mask  : Feature mask
 *
 * RETURN     : NO_ERROR --success
 *              int32_t type of status
 *==========================================================================*/
int32_t QCameraParameters::setStreamPpMask(cam_stream_type_t stream_type,
        cam_feature_mask_t pp_mask) {

    if(stream_type >= CAM_STREAM_TYPE_MAX) {
        return BAD_TYPE;
    }

    mStreamPpMask[stream_type] = pp_mask;
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : getStreamPpMask
 *
 * DESCRIPTION: Retrieves the feature mask for a given camera stream
 *
 * PARAMETERS :
 *  @stream_type: Camera stream type
 *  @pp_mask  : Feature mask
 *
 * RETURN     : NO_ERROR --success
 *              int32_t type of status
 *==========================================================================*/
int32_t QCameraParameters::getStreamPpMask(cam_stream_type_t stream_type,
        cam_feature_mask_t &pp_mask) {

    if(stream_type >= CAM_STREAM_TYPE_MAX) {
        return BAD_TYPE;
    }

    pp_mask = mStreamPpMask[stream_type];
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : isMultiPassReprocessing
 *
 * DESCRIPTION: Read setprop to enable/disable multipass
 *
 * PARAMETERS : none
 *
 * RETURN     : TRUE  -- If enabled
 *              FALSE  -- disabled
 *==========================================================================*/
bool QCameraParameters::isMultiPassReprocessing()
{
    char value[PROPERTY_VALUE_MAX];
    int multpass = 0;

    if (getQuadraCfa()) {
        multpass = TRUE;
        return TRUE;
    }

    property_get("persist.camera.multi_pass", value, "0");
    multpass = atoi(value);

    return (multpass == 0)? FALSE : TRUE;
}

/*===========================================================================
 * FUNCTION   : setReprocCount
 *
 * DESCRIPTION: Set total reprocessing pass count
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void QCameraParameters::setReprocCount()
{
    mTotalPPCount = 1; //Default reprocessing Pass count

    if (getManualCaptureMode() >=
            CAM_MANUAL_CAPTURE_TYPE_3) {
        LOGD("Additional post processing enabled for manual capture");
        mTotalPPCount++;
    }

    if (!isMultiPassReprocessing()) {
        return;
    }

    if ((getZoomLevel() != 0 && !getQuadraCfa())
            && (getBurstCountForAdvancedCapture()
            == getNumOfSnapshots())) {
        LOGD("2 Pass postprocessing enabled");
        mTotalPPCount++;
    }

    if (getQuadraCfa()) {
        mTotalPPCount++;
    }
}

/*===========================================================================
 * FUNCTION   : isUBWCEnabled
 *
 * DESCRIPTION: Function to get UBWC hardware support.
 *
 * PARAMETERS : None
 *
 * RETURN     : TRUE -- UBWC format supported
 *              FALSE -- UBWC is not supported.
 *==========================================================================*/
bool QCameraParameters::isUBWCEnabled()
{
#ifdef UBWC_PRESENT
    char value[PROPERTY_VALUE_MAX];
    int prop_value = 0;
    memset(value, 0, sizeof(value));
    property_get("debug.gralloc.gfx_ubwc_disable", value, "0");
    prop_value = atoi(value);
    if (prop_value) {
        return FALSE;
    }

    //Disable UBWC if it is YUV sensor.
    if ((m_pCapability != NULL) &&
            (m_pCapability->sensor_type.sens_type == CAM_SENSOR_YUV)) {
        return FALSE;
    }

    //Disable UBWC if Eztune is enabled
    // Eztune works on CPP output and cannot understand UBWC buffer.
    memset(value, 0, sizeof(value));
    property_get("persist.camera.eztune.enable", value, "0");
    prop_value = atoi(value);
    if (prop_value) {
        return FALSE;
    }
    return TRUE;
#else
    return FALSE;
#endif
}

/*===========================================================================
 * FUNCTION   : isPostProcScaling
 *
 * DESCRIPTION: is scaling to be done by CPP?
 *
 * PARAMETERS : none
 *
 * RETURN     : TRUE  : If CPP scaling enabled
 *              FALSE : If VFE scaling enabled
 *==========================================================================*/
bool QCameraParameters::isPostProcScaling()
{
    char value[PROPERTY_VALUE_MAX];
    bool cpp_scaling = FALSE;

    if (getRecordingHintValue()) {
        return FALSE;
    }

    property_get("persist.camera.pp_scaling", value, "0");
    cpp_scaling = atoi(value) > 0 ? TRUE : FALSE;

    LOGH("Post proc scaling enabled : %d",
             cpp_scaling);
    return cpp_scaling;
}

/*===========================================================================
 * FUNCTION   : isLLNoiseEnabled
 *
 * DESCRIPTION: Low light noise change
 *
 * PARAMETERS : none
 *
 * RETURN     : TRUE  : If low light noise enabled
 *              FALSE : If low light noise disabled
 *==========================================================================*/
bool QCameraParameters::isLLNoiseEnabled()
{
    char value[PROPERTY_VALUE_MAX];
    bool llnoise = FALSE;

    if (!isWNREnabled()) {
        return FALSE;
    }

    property_get("persist.camera.llnoise", value, "0");
    llnoise = atoi(value) > 0 ? TRUE : FALSE;

    LOGH("Low light noise enabled : %d",
             llnoise);
    return llnoise;
}

/*===========================================================================
 * FUNCTION   : setBufBatchCount
 *
 * DESCRIPTION: Function to configure batch buffer
 *
 * PARAMETERS : int8_t buf_cnt
 *                     Buffer batch count
 *
 * RETURN     :  None
 *==========================================================================*/
void QCameraParameters::setBufBatchCount(int8_t buf_cnt)
{
    mBufBatchCnt = 0;
    char value[PROPERTY_VALUE_MAX];
    int8_t count = 0;

    property_get("persist.camera.batchcount", value, "0");
    count = atoi(value);

    if (!(count != 0 || buf_cnt > CAMERA_MIN_BATCH_COUNT)) {
        LOGH("Buffer batch count = %d", mBufBatchCnt);
        set(KEY_QC_VIDEO_BATCH_SIZE, mBufBatchCnt);
        return;
    }

    while((m_pCapability->max_batch_bufs_supported != 0)
            && (m_pCapability->max_batch_bufs_supported < buf_cnt)) {
        buf_cnt = buf_cnt / 2;
    }

    if (count > 0) {
        mBufBatchCnt = count;
        LOGH("Buffer batch count = %d", mBufBatchCnt);
        set(KEY_QC_VIDEO_BATCH_SIZE, mBufBatchCnt);
        return;
    }

    if (buf_cnt > CAMERA_MIN_BATCH_COUNT) {
        mBufBatchCnt = buf_cnt;
        LOGH("Buffer batch count = %d", mBufBatchCnt);
        set(KEY_QC_VIDEO_BATCH_SIZE, mBufBatchCnt);
        return;
    }
}

/*===========================================================================
 * FUNCTION   : setVideoBatch()
 *
 * DESCRIPTION: Function to batching for video.
 *
 * PARAMETERS : none
 *
 * RETURN     :  None
 *==========================================================================*/
void QCameraParameters::setVideoBatchSize()
{
    char value[PROPERTY_VALUE_MAX];
    int8_t minBatchcnt = 2; //Batching enabled only if batch size if greater than 2;
    int32_t width = 0, height = 0;
    mVideoBatchSize = 0;

    if (getBufBatchCount()) {
        //We don't need HAL to HAL batching if camera batching enabled.
        return;
    }

    getVideoSize(&width, &height);
    if ((width > 1920) || (height > 1080)) {
        //Cannot enable batch mode for video size bigger than 1080p
        return;
    }

    //Batch size "6" is the recommended and gives optimal power saving.
    property_get("persist.camera.video.batchsize", value, "0");
    mVideoBatchSize = atoi(value);

    if (mVideoBatchSize > CAMERA_MAX_CONSUMER_BATCH_BUFFER_SIZE) {
        mVideoBatchSize = CAMERA_MAX_CONSUMER_BATCH_BUFFER_SIZE;
    } else if (mVideoBatchSize <= minBatchcnt) {
        //Batching enabled only if batch size is greater than 2.
        mVideoBatchSize = 0;
    }
    LOGD("mVideoBatchSize = %d", mVideoBatchSize);
    set(KEY_QC_VIDEO_BATCH_SIZE, mVideoBatchSize);
}

/*===========================================================================
 * FUNCTION   : setCustomParams
 *
 * DESCRIPTION: Function to update OEM specific custom parameter
 *
 * PARAMETERS : params: Input Parameter object
 *
 * RETURN     :  error value
 *==========================================================================*/
int32_t QCameraParameters::setCustomParams(__unused const QCameraParameters& params)
{
    int32_t rc = NO_ERROR;

    /* Application specific parameter can be read from "params" and update m_pParamBuf
       We can also update internal OEM custom parameters in this funcion.
       "CAM_CUSTOM_PARM_EXAMPLE" is used as a example */

    /*Get the pointer of shared buffer for custom parameter*/
    custom_parm_buffer_t *customParam =
            (custom_parm_buffer_t *)POINTER_OF_META(CAM_INTF_PARM_CUSTOM, m_pParamBuf);


    /*start updating custom parameter values*/
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(customParam, CAM_CUSTOM_PARM_EXAMPLE, 1)) {
        LOGE("Failed to update CAM_CUSTOM_PARM_DUMMY");
        return BAD_VALUE;
    }

    /*set custom parameter values to main parameter buffer. Update isvalid flag*/
    ADD_GET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_CUSTOM);

    return rc;
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
String8 QCameraParameters::dump()
{
    String8 str("\n");
    char s[128];

    snprintf(s, 128, "Preview Pixel Fmt: %d\n", getPreviewHalPixelFormat());
    str += s;

    snprintf(s, 128, "ZSL Burst Interval: %d\n", getZSLBurstInterval());
    str += s;

    snprintf(s, 128, "ZSL Queue Depth: %d\n", getZSLQueueDepth());
    str += s;

    snprintf(s, 128, "ZSL Back Look Count %d\n", getZSLBackLookCount());
    str += s;

    snprintf(s, 128, "Max Unmatched Frames In Queue: %d\n",
        getMaxUnmatchedFramesInQueue());
    str += s;

    snprintf(s, 128, "Is ZSL Mode: %d\n", isZSLMode());
    str += s;

    snprintf(s, 128, "Is No Display Mode: %d\n", isNoDisplayMode());
    str += s;

    snprintf(s, 128, "Is WNR Enabled: %d\n", isWNREnabled());
    str += s;

    snprintf(s, 128, "isHfrMode: %d\n", isHfrMode());
    str += s;

    snprintf(s, 128, "getNumOfSnapshots: %d\n", getNumOfSnapshots());
    str += s;

    snprintf(s, 128, "getNumOfExtraHDRInBufsIfNeeded: %d\n",
        getNumOfExtraHDRInBufsIfNeeded());
    str += s;

    snprintf(s, 128, "getNumOfExtraHDROutBufsIfNeeded: %d\n",
        getNumOfExtraHDROutBufsIfNeeded());
    str += s;

    snprintf(s, 128, "getRecordingHintValue: %d\n", getRecordingHintValue());
    str += s;

    snprintf(s, 128, "getJpegQuality: %u\n", getJpegQuality());
    str += s;

    snprintf(s, 128, "getJpegRotation: %u\n", getJpegRotation());
    str += s;

    snprintf(s, 128, "isHistogramEnabled: %d\n", isHistogramEnabled());
    str += s;

    snprintf(s, 128, "isFaceDetectionEnabled: %d\n", isFaceDetectionEnabled());
    str += s;

    snprintf(s, 128, "isHDREnabled: %d\n", isHDREnabled());
    str += s;

    snprintf(s, 128, "isAutoHDREnabled: %d\n", isAutoHDREnabled());
    str += s;

    snprintf(s, 128, "isAVTimerEnabled: %d\n", isAVTimerEnabled());
    str += s;

    snprintf(s, 128, "getFocusMode: %d\n", getFocusMode());
    str += s;

    snprintf(s, 128, "isJpegPictureFormat: %d\n", isJpegPictureFormat());
    str += s;

    snprintf(s, 128, "isNV16PictureFormat: %d\n", isNV16PictureFormat());
    str += s;

    snprintf(s, 128, "isNV21PictureFormat: %d\n", isNV21PictureFormat());
    str += s;

    snprintf(s, 128, "isSnapshotFDNeeded: %d\n", isSnapshotFDNeeded());
    str += s;

    snprintf(s, 128, "isHDR1xFrameEnabled: %d\n", isHDR1xFrameEnabled());
    str += s;

    snprintf(s, 128, "isYUVFrameInfoNeeded: %d\n", isYUVFrameInfoNeeded());
    str += s;

    snprintf(s, 128, "isHDR1xExtraBufferNeeded: %d\n",
        isHDR1xExtraBufferNeeded());
    str += s;

    snprintf(s, 128, "isHDROutputCropEnabled: %d\n", isHDROutputCropEnabled());
    str += s;

    snprintf(s, 128, "isPreviewFlipChanged: %d\n", isPreviewFlipChanged());
    str += s;

    snprintf(s, 128, "isVideoFlipChanged: %d\n", isVideoFlipChanged());
    str += s;

    snprintf(s, 128, "isSnapshotFlipChanged: %d\n", isSnapshotFlipChanged());
    str += s;

    snprintf(s, 128, "isHDRThumbnailProcessNeeded: %d\n",
        isHDRThumbnailProcessNeeded());
    str += s;

    snprintf(s, 128, "getAutoFlickerMode: %d\n", getAutoFlickerMode());
    str += s;

    snprintf(s, 128, "getNumOfExtraBuffersForImageProc: %d\n",
        getNumOfExtraBuffersForImageProc());
    str += s;

    snprintf(s, 128, "isUbiFocusEnabled: %d\n", isUbiFocusEnabled());
    str += s;

    snprintf(s, 128, "isChromaFlashEnabled: %d\n", isChromaFlashEnabled());
    str += s;

    snprintf(s, 128, "isOptiZoomEnabled: %d\n", isOptiZoomEnabled());
    str += s;

    snprintf(s, 128, "isStillMoreEnabled: %d\n", isStillMoreEnabled());
    str += s;

    snprintf(s, 128, "getBurstCountForAdvancedCapture: %d\n",
        getBurstCountForAdvancedCapture());
    str += s;

    return str;
}

/*===========================================================================
 * FUNCTION   : getNumOfExtraBuffersForVideo
 *
 * DESCRIPTION: get number of extra buffers needed by image processing
 *
 * PARAMETERS : none
 *
 * RETURN     : number of extra buffers needed by ImageProc;
 *              0 if not ImageProc enabled
 *==========================================================================*/
uint8_t QCameraParameters::getNumOfExtraBuffersForVideo()
{
    uint8_t numOfBufs = 0;

    if (isSeeMoreEnabled() || isHighQualityNoiseReductionMode()) {
        numOfBufs = 1;
    }

    return numOfBufs;
}

/*===========================================================================
 * FUNCTION   : getNumOfExtraBuffersForPreview
 *
 * DESCRIPTION: get number of extra buffers needed by image processing
 *
 * PARAMETERS : none
 *
 * RETURN     : number of extra buffers needed by ImageProc;
 *              0 if not ImageProc enabled
 *==========================================================================*/
uint8_t QCameraParameters::getNumOfExtraBuffersForPreview()
{
    uint8_t numOfBufs = 0;

    if ((isSeeMoreEnabled() || isHighQualityNoiseReductionMode())
            && !isZSLMode() && getRecordingHintValue()) {
        numOfBufs = 1;
    }

    return numOfBufs;
}

/*===========================================================================
 * FUNCTION   : setToneMapMode
 *
 * DESCRIPTION: enable or disable tone map
 *
 * PARAMETERS :
 *   @enable : enable: 1; disable 0
 *   @initCommit: if configuration list needs to be initialized and commited
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setToneMapMode(uint32_t enable, bool initCommit)
{
    int32_t rc = NO_ERROR;
    LOGH("tone map mode %d ", enable);

    if (initCommit) {
        if (initBatchUpdate(m_pParamBuf) < 0) {
            LOGE("Failed to initialize group update table");
            return FAILED_TRANSACTION;
        }
    }

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_TONE_MAP_MODE, enable)) {
        LOGE("Failed to update tone map mode");
        return BAD_VALUE;
    }

    if (initCommit) {
        rc = commitSetBatch();
        if (rc != NO_ERROR) {
            LOGE("Failed to commit tone map mode");
            return rc;
        }
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : getLongshotStages
 *
 * DESCRIPTION: get number of stages for longshot
 *
 * PARAMETERS : none
 *
 * RETURN     : number of stages
 *==========================================================================*/
uint8_t QCameraParameters::getLongshotStages()
{
    uint8_t numStages =
            isLowMemoryDevice() ? CAMERA_MIN_LONGSHOT_STAGES : CAMERA_DEFAULT_LONGSHOT_STAGES;

    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.longshot.stages", prop, "0");
    uint8_t propStages = atoi(prop);
    if (propStages > 0 && propStages <= CAMERA_DEFAULT_LONGSHOT_STAGES) {
        numStages = propStages;
    }
    return numStages;
}

/*===========================================================================
 * FUNCTION   : setCDSMode
 *
 * DESCRIPTION: set CDS mode
 *
 * PARAMETERS :
 *   @cds_mode : cds mode
 *   @initCommit: if configuration list needs to be initialized and commited
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setCDSMode(int32_t cds_mode, bool initCommit)
{
    if (initCommit) {
        if (initBatchUpdate(m_pParamBuf) < 0) {
            LOGE("Failed to initialize group update table");
            return FAILED_TRANSACTION;
        }
    }

    int32_t rc = NO_ERROR;
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_CDS_MODE, cds_mode)) {
        LOGE("Failed to update cds mode");
        return BAD_VALUE;
    }

    if (initCommit) {
        rc = commitSetBatch();
        if (NO_ERROR != rc) {
            LOGE("Failed to set cds mode");
            return rc;
        }
    }

    LOGH("cds mode -> %d", cds_mode);

    return rc;
}

/*===========================================================================
 * FUNCTION   : setLowLightCapture
 *
 * DESCRIPTION: Function to enable low light capture
 *==========================================================================*/
void QCameraParameters::setLowLightCapture()
{
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.llc", prop, "0");
    m_LLCaptureEnabled = (atoi(prop) > 0) ? TRUE : FALSE;

    if (!m_LLCaptureEnabled) {
        m_LowLightLevel = CAM_LOW_LIGHT_OFF;
    }
}

/*===========================================================================
 * FUNCTION   : fdModeInVideo
 *
 * DESCRIPTION: FD in Video change
 *
 * PARAMETERS : none
 *
 * RETURN     : FD Mode in Video
 *              0 : If FD in Video disabled
 *              1 : If FD in Video enabled for Detection, focus
 *              2 : If FD in Video enabled only for focus
 *==========================================================================*/
uint8_t QCameraParameters::fdModeInVideo()
{
    char value[PROPERTY_VALUE_MAX];
    uint8_t fdvideo = 0;

    property_get("persist.camera.fdvideo", value, "0");
    fdvideo = (atoi(value) > 0) ? atoi(value) : 0;

    LOGD("FD mode in Video : %d", fdvideo);
    return fdvideo;
}

/*===========================================================================
 * FUNCTION   : setManualCaptureMode
 *
 * DESCRIPTION: Function to set Manual capture modes
 *
 * PARAMETERS :
 *   @mode : Capture mode configured
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setManualCaptureMode(QCameraManualCaptureModes mode)
{
    int32_t rc = NO_ERROR;
    char value[PROPERTY_VALUE_MAX];
    int8_t count = 0;

    property_get("persist.camera.manual.capture", value, "0");
    count = atoi(value);

    if (count) {
        if (mode == CAM_MANUAL_CAPTURE_TYPE_OFF) {
            m_ManualCaptureMode = CAM_MANUAL_CAPTURE_TYPE_1;
        } else {
            m_ManualCaptureMode = mode;
        }
    } else {
        m_ManualCaptureMode = CAM_MANUAL_CAPTURE_TYPE_OFF;
    }

    if (m_ManualCaptureMode == CAM_MANUAL_CAPTURE_TYPE_2) {
        setOfflineRAW(FALSE);
    } else if (m_ManualCaptureMode >= CAM_MANUAL_CAPTURE_TYPE_3) {
        setOfflineRAW(TRUE);
    } else {
        setOfflineRAW(FALSE);
    }
    setReprocCount();
    LOGH("Manual capture mode - %d", m_ManualCaptureMode);
    return rc;
}

/*===========================================================================
 * FUNCTION   : isReprocScaleEnabled
 *
 * DESCRIPTION: Whether reprocess scale is enabled or not
 *
 * PARAMETERS : none
 *
 * RETURN     : TRUE  : Reprocess scale is enabled
 *              FALSE : Reprocess scale is not enabled
 *==========================================================================*/
bool QCameraParameters::isReprocScaleEnabled()
{
    return m_reprocScaleParam.isScaleEnabled();
}

/*===========================================================================
 * FUNCTION   : isUnderReprocScaling
 *
 * DESCRIPTION: Whether image is under reprocess scaling
 *
 * PARAMETERS : none
 *
 * RETURN     : TRUE  : Image is under reprocess scaling
 *              FALSE : Image is not under reprocess scaling
 *==========================================================================*/
bool QCameraParameters::isUnderReprocScaling()
{
    return m_reprocScaleParam.isUnderScaling();
}

/*===========================================================================
 * FUNCTION   : getPicSizeFromAPK
 *
 * DESCRIPTION: Get picture size set from application.
 *
 * PARAMETERS :
 *   @width   : with set by application
 *   @height  : height set by application
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::getPicSizeFromAPK(int &width, int &height)
{
    return m_reprocScaleParam.getPicSizeFromAPK(width, height);
}

/*===========================================================================
 * FUNCTION   : setDualLedCalibration
 *
 * DESCRIPTION: set dual led calibration
 *
 * PARAMETERS :
 *   @params  : user setting parameters
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setLedCalibration(
        __unused const QCameraParameters& params)
{
    const char *str = params.get(KEY_QC_LED_CALIBRATION_MODES);
    const char *prev_str = get(KEY_QC_LED_CALIBRATION_MODES);

    if (str != NULL) {
        if (prev_str == NULL || strcmp(str, prev_str) != 0) {
            return setLedCalibration(str);
        }
    } else {
        char value[PROPERTY_VALUE_MAX];

        memset(value, 0, sizeof(value));
        property_get("persist.camera.led_calib_mode", value, "0");
        if (strlen(value) > 0) {
            if (prev_str == NULL || strcmp(value, prev_str) != 0) {
                return setLedCalibration(value);
            }
        }
    }
    return NO_ERROR;
}


/*===========================================================================
 * FUNCTION   : setLedCalibration
 *
 * DESCRIPTION: set Led Calibration Mode
 *
 * PARAMETERS :
 *   @calibration_mode : calibration mode string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
 int32_t QCameraParameters::setLedCalibration(const char *calibModeStr)
{

    if (calibModeStr != NULL) {
        int32_t value = lookupAttr(LED_CALIBRATION_MODE_MAP,
                PARAM_MAP_SIZE(LED_CALIBRATION_MODE_MAP), calibModeStr);
        if (value != NAME_NOT_FOUND) {
            m_ledCalibrationMode = static_cast<cam_led_calibration_mode_t>(value);
            LOGD("Setting led calibration mode %d", m_ledCalibrationMode);
            updateParamEntry(KEY_QC_LED_CALIBRATION_MODES, calibModeStr);

            if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf,
                    CAM_INTF_PARM_LED_CALIBRATION, m_ledCalibrationMode)) {
                LOGE("Failed to update led calibration param");
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
    }
    LOGE("Invalid Calibraon Mode value: %s",
            (calibModeStr == NULL) ? "NULL" : calibModeStr);
    return BAD_VALUE;
}


/*===========================================================================
 * FUNCTION   : setinstantAEC
 *
 * DESCRIPTION: set instant AEC value to backend
 *
 * PARAMETERS :
 *   @value : instant aec enabled or not.
 *            0 - disable
 *            1 - Enable and set agressive AEC algo to the backend
 *            2 - Enable and set fast AEC algo to the backend
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setInstantAEC(uint8_t value, bool initCommit)
{
    if (initCommit) {
        if (initBatchUpdate(m_pParamBuf) < 0) {
            LOGE("Failed to initialize group update table");
            return FAILED_TRANSACTION;
        }
    }

    int32_t rc = NO_ERROR;
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf, CAM_INTF_PARM_INSTANT_AEC, value)) {
        LOGE("Failed to instant aec value");
        return BAD_VALUE;
    }

    if (initCommit) {
        rc = commitSetBatch();
        if (NO_ERROR != rc) {
            LOGE("Failed to instant aec value");
            return rc;
        }
    }

    LOGD(" Instant AEC value set to backend %d", value);
    m_bInstantAEC = value;
    return rc;
}

/*===========================================================================
 * FUNCTION   : setAdvancedCaptureMode
 *
 * DESCRIPTION: set advanced capture mode
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::setAdvancedCaptureMode()
{
    uint8_t value = isAdvCamFeaturesEnabled();
    LOGD("updating advanced capture mode value to %d",value);
    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf,
            CAM_INTF_PARM_ADV_CAPTURE_MODE, value)) {
        LOGE("Failed to set advanced capture mode param");
        return BAD_VALUE;
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : getAnalysisInfo
 *
 * DESCRIPTION: Get the Analysis information based on
 *     current mode and feature mask
 *
 * PARAMETERS :
 *   @fdVideoEnabled : Whether fdVideo enabled currently
 *   @videoEnabled   : Whether hal3 or hal1
 *   @featureMask    : Feature mask
 *   @analysis_info  : Analysis info to be filled
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::getAnalysisInfo(
        bool fdVideoEnabled,
        bool hal3,
        cam_feature_mask_t featureMask,
        cam_analysis_info_t *pAnalysisInfo)
{
    return mCommon.getAnalysisInfo(fdVideoEnabled, hal3, featureMask, pAnalysisInfo);
}

/*===========================================================================
 * FUNCTION   : getMetaRawInfo
 *
 * DESCRIPTION: fetch meta raw dimension
 *
 * PARAMETERS :
 *   @dim  : get dimension for meta raw stream
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::getMetaRawInfo()
{
    int32_t rc = NO_ERROR;
    cam_dimension_t meta_stream_size;

    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    ADD_GET_PARAM_ENTRY_TO_BATCH(m_pParamBuf,
            CAM_INTF_META_RAW);

    rc = commitGetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to get extened RAW info");
        return rc;
    }

    READ_PARAM_ENTRY(m_pParamBuf,
            CAM_INTF_META_RAW, meta_stream_size);

    if (meta_stream_size.width == 0 || meta_stream_size.height == 0) {
        LOGE("Error getting RAW size. Setting to Capability value");
        meta_stream_size = m_pCapability->raw_meta_dim[0];
    }
    LOGH("RAW meta size. width =%d height =%d",
      meta_stream_size.width, meta_stream_size.height);

    setRawSize(meta_stream_size);
    m_bMetaRawEnabled = true;
    return rc;
}
/*===========================================================================
 * FUNCTION   : sendStreamConfigForPickRes
 *
 * DESCRIPTION: send Stream config info.
 *
 * PARAMETERS :
 *   @stream_config_info: Stream config information
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
bool QCameraParameters::sendStreamConfigForPickRes
    (cam_stream_size_info_t &stream_config_info) {
    int32_t rc = NO_ERROR;
    if(initBatchUpdate(m_pParamBuf) < 0 ) {
        LOGE("Failed to initialize group update table");
        return BAD_TYPE;
    }

    if (ADD_SET_PARAM_ENTRY_TO_BATCH(m_pParamBuf,
            CAM_INTF_META_STREAM_INFO_FOR_PIC_RES, stream_config_info)) {
        LOGE("%s:Failed to update table");
        return BAD_VALUE;
    }

    rc = commitSetBatch();
    if (rc != NO_ERROR) {
        LOGE("Failed to set stream info parm");
        return rc;
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : updateDtVc
 *
 * DESCRIPTION: Update DT and Vc from capabilities
 *
 * PARAMETERS :
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraParameters::updateDtVc(int32_t *dt, int32_t *vc)
{
    int32_t rc = NO_ERROR;
    char prop[PROPERTY_VALUE_MAX];

    int dt_val = 0;
    int vc_val = 0;

    /* Setting Dt from setprop or capability */
    property_get("persist.camera.dt", prop, "0");
    dt_val = atoi(prop);
    if (dt_val == 0) {
        dt_val = m_pCapability->dt[0];
    }
    *dt = dt_val;

    /*Setting vc from setprop or capability */
    property_get("persist.camera.vc", prop, "-1");
    vc_val = atoi(prop);
    if (vc_val== -1) {
        vc_val = m_pCapability->vc[0];
    }
    *vc = vc_val;

    LOGH("dt=%d vc=%d",*dt, *vc);
    return rc;
}
/*===========================================================================
 * FUNCTION   : isLinkPreviewForLiveShot()
 *
 * DESCRIPTION: Function to check whether link preview for liveshot or not
 *
 * PARAMETERS : none
 *
 * RETURN     : true: Thumbnail is generated from Preview stream
 *              false: Thumbnail is generated from main image
 *==========================================================================*/
bool QCameraParameters::isLinkPreviewForLiveShot()
{

   char prop[PROPERTY_VALUE_MAX];

   memset(prop, 0, sizeof(prop));
   // 0. Thumbnail is generated from main image  (or)
   // 1. Thumbnail is generated from Preview stream
   property_get("persist.camera.linkpreview", prop, "0");
   bool enable = atoi(prop) > 0 ? TRUE : FALSE;

   LOGD("Link preview for thumbnail %d", enable);
   return enable;
}

}; // namespace qcamera
