/*
** Copyright (c) 2012-2017, The Linux Foundation. All rights reserved.
** Not a Contribution. Apache license notifications and license are
** retained for attribution purposes only.
**
** Copyright 2008, The Android Open Source Project
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef ANDROID_HARDWARE_QCAMERA_PARAMETERS_H
#define ANDROID_HARDWARE_QCAMERA_PARAMETERS_H

// To remove
#include <cutils/properties.h>

// System dependencies
//#include <camera/CameraParameters.h>
#include <utils/Errors.h>

// Camera dependencies
#include "hardware/camera.h"
#include "QCameraMem.h"
#include "QCameraParametersIntf.h"
#include "QCameraThermalAdapter.h"
#include "QCameraCommon.h"


extern "C" {
#include "mm_jpeg_interface.h"
}

using namespace android;

namespace qcamera {

//EXIF globals
static const char ExifAsciiPrefix[] = { 0x41, 0x53, 0x43, 0x49, 0x49, 0x0, 0x0, 0x0 };          // "ASCII\0\0\0"
static const char ExifUndefinedPrefix[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // "\0\0\0\0\0\0\0\0"

#define FOCAL_LENGTH_DECIMAL_PRECISION   100

#define CAMERA_MIN_BATCH_COUNT           4

#define QCAMERA_MAX_EXP_TIME_LEVEL1      100
#define QCAMERA_MAX_EXP_TIME_LEVEL2      500
#define QCAMERA_MAX_EXP_TIME_LEVEL3      1000
#define QCAMERA_MAX_EXP_TIME_LEVEL4      10000

class QCameraParameters: private CameraParameters
{

private:

    class QCameraReprocScaleParam{
    public:

        QCameraReprocScaleParam();
        ~QCameraReprocScaleParam();

        void setScaleEnable(bool enabled);
        int32_t setScaleSizeTbl(size_t scale_cnt,
                cam_dimension_t *scale_tbl, size_t org_cnt,
                cam_dimension_t *org_tbl);
        int32_t setValidatePicSize(int &width, int &height);

        bool isScaleEnabled();
        bool isUnderScaling();

        size_t getScaleSizeTblCnt();
        cam_dimension_t *getScaledSizeTbl();
        size_t getTotalSizeTblCnt();
        cam_dimension_t *getTotalSizeTbl();
        int32_t getPicSizeFromAPK(int &width, int &height);
        int32_t getPicSizeSetted(int &width, int &height);

    private:
        bool isScalePicSize(int width, int height);
        bool isValidatePicSize(int width, int height);
        int32_t setSensorSupportedPicSize();
        size_t checkScaleSizeTable(size_t scale_cnt, cam_dimension_t *scale_tbl,
                size_t org_cnt, cam_dimension_t *org_tbl);

        bool mScaleEnabled;
        bool mIsUnderScaling;   //if in scale status

        // picture size cnt that need scale operation
        size_t mNeedScaleCnt;
        cam_dimension_t mNeedScaledSizeTbl[MAX_SCALE_SIZES_CNT];

        // sensor supported size cnt and table
        size_t mSensorSizeTblCnt;
        cam_dimension_t *mSensorSizeTbl;

        // Total size cnt (sensor supported + need scale cnt)
        size_t mTotalSizeTblCnt;
        cam_dimension_t mTotalSizeTbl[MAX_SIZES_CNT];

        cam_dimension_t mPicSizeFromAPK;   // dimension that APK is expected
        cam_dimension_t mPicSizeSetted;    // dimension that config vfe
    };

    // Supported PREVIEW/RECORDING SIZES IN HIGH FRAME RATE recording, sizes in pixels.
    // Example value: "800x480,432x320". Read only.
    static const char KEY_QC_SUPPORTED_HFR_SIZES[];
    // The mode of preview frame rate.
    // Example value: "frame-rate-auto, frame-rate-fixed".
    static const char KEY_QC_PREVIEW_FRAME_RATE_MODE[];
    static const char KEY_QC_SUPPORTED_PREVIEW_FRAME_RATE_MODES[];
    static const char KEY_QC_PREVIEW_FRAME_RATE_AUTO_MODE[];
    static const char KEY_QC_PREVIEW_FRAME_RATE_FIXED_MODE[];
    static const char KEY_QC_SUPPORTED_SKIN_TONE_ENHANCEMENT_MODES[] ;

    // Supported live snapshot sizes
    static const char KEY_QC_SUPPORTED_LIVESNAPSHOT_SIZES[];

    // Supported Raw formats
    static const char KEY_QC_SUPPORTED_RAW_FORMATS[];
    static const char KEY_QC_RAW_FORMAT[];

    //Touch Af/AEC settings.
    static const char KEY_QC_TOUCH_AF_AEC[];
    static const char KEY_QC_SUPPORTED_TOUCH_AF_AEC[];
    //Touch Index for AEC.
    static const char KEY_QC_TOUCH_INDEX_AEC[];
    //Touch Index for AF.
    static const char KEY_QC_TOUCH_INDEX_AF[];
    // Current auto scene detection mode.
    // Example value: "off" or "on" constants. Read/write.
    static const char KEY_QC_SCENE_DETECT[];
    // Supported auto scene detection settings.
    // Example value: "off,on". Read only.
    static const char KEY_QC_SUPPORTED_SCENE_DETECT[];
    static const char KEY_QC_SELECTABLE_ZONE_AF[];

    static const char KEY_QC_ISO_MODE[];
    static const char KEY_QC_CONTINUOUS_ISO[];
    static const char KEY_QC_MIN_ISO[];
    static const char KEY_QC_MAX_ISO[];
    static const char KEY_QC_SUPPORTED_ISO_MODES[];
    static const char KEY_QC_EXPOSURE_TIME[];
    static const char KEY_QC_MIN_EXPOSURE_TIME[];
    static const char KEY_QC_MAX_EXPOSURE_TIME[];
    static const char KEY_QC_LENSSHADE[] ;
    static const char KEY_QC_SUPPORTED_LENSSHADE_MODES[] ;
    static const char KEY_QC_AUTO_EXPOSURE[];
    static const char KEY_QC_SUPPORTED_AUTO_EXPOSURE[];

    static const char KEY_QC_GPS_LATITUDE_REF[];
    static const char KEY_QC_GPS_LONGITUDE_REF[];
    static const char KEY_QC_GPS_ALTITUDE_REF[];
    static const char KEY_QC_GPS_STATUS[];
    static const char KEY_QC_MEMORY_COLOR_ENHANCEMENT[];
    static const char KEY_QC_SUPPORTED_MEM_COLOR_ENHANCE_MODES[];
    static const char KEY_QC_DIS[];
    static const char KEY_QC_OIS[];
    static const char KEY_QC_SUPPORTED_DIS_MODES[];
    static const char KEY_QC_SUPPORTED_OIS_MODES[];

    static const char KEY_QC_ZSL[];
    static const char KEY_QC_SUPPORTED_ZSL_MODES[];
    static const char KEY_QC_ZSL_BURST_INTERVAL[];
    static const char KEY_QC_ZSL_BURST_LOOKBACK[];
    static const char KEY_QC_ZSL_QUEUE_DEPTH[];

    static const char KEY_QC_CAMERA_MODE[];
    static const char KEY_QC_ORIENTATION[];

    static const char KEY_QC_VIDEO_HIGH_FRAME_RATE[];
    static const char KEY_QC_VIDEO_HIGH_SPEED_RECORDING[];
    static const char KEY_QC_SUPPORTED_VIDEO_HIGH_FRAME_RATE_MODES[];
    static const char KEY_QC_HIGH_DYNAMIC_RANGE_IMAGING[];
    static const char KEY_QC_SUPPORTED_HDR_IMAGING_MODES[];
    static const char KEY_QC_AE_BRACKET_HDR[];
    static const char KEY_QC_SUPPORTED_AE_BRACKET_MODES[];
    static const char KEY_QC_CAPTURE_BURST_EXPOSURE[];
    static const char KEY_QC_NUM_SNAPSHOT_PER_SHUTTER[];
    static const char KEY_QC_NUM_RETRO_BURST_PER_SHUTTER[];
    static const char KEY_QC_SNAPSHOT_BURST_LED_ON_PERIOD[];
    static const char KEY_QC_SNAPSHOT_BURST_NUM[];
    static const char KEY_QC_NO_DISPLAY_MODE[];
    static const char KEY_QC_RAW_PICUTRE_SIZE[];
    static const char KEY_QC_TINTLESS_ENABLE[];
    static const char KEY_QC_SCENE_SELECTION[];
    static const char KEY_QC_CDS_MODE[];
    static const char KEY_QC_VIDEO_CDS_MODE[];
    static const char KEY_QC_SUPPORTED_CDS_MODES[];
    static const char KEY_QC_SUPPORTED_VIDEO_CDS_MODES[];
    static const char KEY_QC_TNR_MODE[];
    static const char KEY_QC_VIDEO_TNR_MODE[];
    static const char KEY_QC_SUPPORTED_TNR_MODES[];
    static const char KEY_QC_SUPPORTED_VIDEO_TNR_MODES[];

    static const char KEY_INTERNAL_PERVIEW_RESTART[];
    static const char KEY_QC_WB_MANUAL_CCT[];
    static const char KEY_QC_MIN_WB_CCT[];
    static const char KEY_QC_MAX_WB_CCT[];
    static const char KEY_QC_MANUAL_WB_GAINS[];
    static const char KEY_QC_MIN_WB_GAIN[];
    static const char KEY_QC_MAX_WB_GAIN[];
    static const char WHITE_BALANCE_MANUAL[];
    static const char FOCUS_MODE_MANUAL_POSITION[];
    static const char KEY_QC_LONG_SHOT[];
    static const char KEY_QC_INITIAL_EXPOSURE_INDEX[];
    static const char KEY_QC_INSTANT_AEC[];
    static const char KEY_QC_INSTANT_CAPTURE[];
    static const char KEY_QC_INSTANT_AEC_SUPPORTED_MODES[];
    static const char KEY_QC_INSTANT_CAPTURE_SUPPORTED_MODES[];
    static const char KEY_QC_LED_CALIBRATION_MODES[];

    static const char KEY_QC_MANUAL_FOCUS_POSITION[];
    static const char KEY_QC_MANUAL_FOCUS_POS_TYPE[];
    static const char KEY_QC_MIN_FOCUS_POS_INDEX[];
    static const char KEY_QC_MAX_FOCUS_POS_INDEX[];
    static const char KEY_QC_MIN_FOCUS_POS_DAC[];
    static const char KEY_QC_MAX_FOCUS_POS_DAC[];
    static const char KEY_QC_MIN_FOCUS_POS_RATIO[];
    static const char KEY_QC_MAX_FOCUS_POS_RATIO[];
    static const char KEY_QC_MIN_FOCUS_POS_DIOPTER[];
    static const char KEY_QC_MAX_FOCUS_POS_DIOPTER[];
    static const char KEY_QC_FOCUS_POSITION_SCALE[];
    static const char KEY_QC_FOCUS_POSITION_DIOPTER[];

    static const char KEY_QC_SUPPORTED_MANUAL_FOCUS_MODES[];
    static const char KEY_QC_SUPPORTED_MANUAL_EXPOSURE_MODES[];
    static const char KEY_QC_SUPPORTED_MANUAL_WB_MODES[];
    static const char KEY_QC_FOCUS_SCALE_MODE[];
    static const char KEY_QC_FOCUS_DIOPTER_MODE[];
    static const char KEY_QC_ISO_PRIORITY[];
    static const char KEY_QC_EXP_TIME_PRIORITY[];
    static const char KEY_QC_USER_SETTING[];
    static const char KEY_QC_WB_CCT_MODE[];
    static const char KEY_QC_WB_GAIN_MODE[];
    static const char KEY_QC_MANUAL_WB_TYPE[];
    static const char KEY_QC_MANUAL_WB_VALUE[];
    static const char KEY_QC_CURRENT_EXPOSURE_TIME[];
    static const char KEY_QC_CURRENT_ISO[];
    static const char KEY_QC_CACHE_VIDEO_BUFFERS[];

    // DENOISE
    static const char KEY_QC_DENOISE[];
    static const char KEY_QC_SUPPORTED_DENOISE[];

    //Selectable zone AF.
    static const char KEY_QC_FOCUS_ALGO[];
    static const char KEY_QC_SUPPORTED_FOCUS_ALGOS[];

    //Face Detection
    static const char KEY_QC_FACE_DETECTION[];
    static const char KEY_QC_SUPPORTED_FACE_DETECTION[];

    //Face Recognition
    static const char KEY_QC_FACE_RECOGNITION[];
    static const char KEY_QC_SUPPORTED_FACE_RECOGNITION[];

    //Indicates number of faces requested by the application.
    //This value will be rejected if the requested faces
    //greater than supported by hardware.
    //Write only.
    static const char KEY_QC_MAX_NUM_REQUESTED_FACES[];

    //preview flip
    static const char KEY_QC_PREVIEW_FLIP[];
    //video flip
    static const char KEY_QC_VIDEO_FLIP[];
    //snapshot picture flip
    static const char KEY_QC_SNAPSHOT_PICTURE_FLIP[];

    static const char KEY_QC_SUPPORTED_FLIP_MODES[];

    //Face Detection, Facial processing requirement
    static const char KEY_QC_SNAPSHOT_FD_DATA[];

    //Auto HDR enable
    static const char KEY_QC_AUTO_HDR_ENABLE[];
    // video rotation
    static const char KEY_QC_VIDEO_ROTATION[];
    static const char KEY_QC_SUPPORTED_VIDEO_ROTATION_VALUES[];

    //Redeye Reduction
    static const char KEY_QC_REDEYE_REDUCTION[];
    static const char KEY_QC_SUPPORTED_REDEYE_REDUCTION[];
    static const char EFFECT_EMBOSS[];
    static const char EFFECT_SKETCH[];
    static const char EFFECT_NEON[];
    static const char EFFECT_BEAUTY[];

    //AF Bracketing
    static const char KEY_QC_AF_BRACKET[];
    static const char KEY_QC_SUPPORTED_AF_BRACKET_MODES[];

    //Refocus
    static const char KEY_QC_RE_FOCUS[];
    static const char KEY_QC_SUPPORTED_RE_FOCUS_MODES[];

    //Chroma Flash
    static const char KEY_QC_CHROMA_FLASH[];
    static const char KEY_QC_SUPPORTED_CHROMA_FLASH_MODES[];

    //Opti Zoom
    static const char KEY_QC_OPTI_ZOOM[];
    static const char KEY_QC_SUPPORTED_OPTI_ZOOM_MODES[];

    // Auto HDR supported
    static const char KEY_QC_AUTO_HDR_SUPPORTED[];

    // HDR modes
    static const char KEY_QC_HDR_MODE[];
    static const char KEY_QC_SUPPORTED_KEY_QC_HDR_MODES[];

    //True Portrait
    static const char KEY_QC_TRUE_PORTRAIT[];
    static const char KEY_QC_SUPPORTED_TRUE_PORTRAIT_MODES[];

    //See more
    static const char KEY_QC_SEE_MORE[];
    static const char KEY_QC_SUPPORTED_SEE_MORE_MODES[];

    //Still more
    static const char KEY_QC_STILL_MORE[];
    static const char KEY_QC_SUPPORTED_STILL_MORE_MODES[];

    //Noise reduction mode
    static const char KEY_QC_NOISE_REDUCTION_MODE[];
    static const char KEY_QC_NOISE_REDUCTION_MODE_VALUES[];

    //Longshot
    static const char KEY_QC_LONGSHOT_SUPPORTED[];

    //ZSL+HDR
    static const char KEY_QC_ZSL_HDR_SUPPORTED[];

    // Values for Touch AF/AEC
    static const char TOUCH_AF_AEC_OFF[];
    static const char TOUCH_AF_AEC_ON[];

    // Values for Scene mode
    static const char SCENE_MODE_ASD[];
    static const char SCENE_MODE_BACKLIGHT[];
    static const char SCENE_MODE_FLOWERS[];
    static const char SCENE_MODE_AR[];
    static const char SCENE_MODE_HDR[];
    static const char PIXEL_FORMAT_YUV420SP_ADRENO[]; // ADRENO
    static const char PIXEL_FORMAT_YV12[]; // NV12
    static const char PIXEL_FORMAT_NV12[]; //NV12
    static const char QC_PIXEL_FORMAT_NV12_VENUS[]; //NV12 VENUS

    // Values for raw picture format
    static const char QC_PIXEL_FORMAT_YUV_RAW_8BIT_YUYV[];
    static const char QC_PIXEL_FORMAT_YUV_RAW_8BIT_YVYU[];
    static const char QC_PIXEL_FORMAT_YUV_RAW_8BIT_UYVY[];
    static const char QC_PIXEL_FORMAT_YUV_RAW_8BIT_VYUY[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_8GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_8GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_8RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_8BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_10GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_10GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_10RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_10BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_12GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_12GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_12RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_12BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_14GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_14GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_14RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_14BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_8GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_8GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_8RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_8BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_10GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_10GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_10RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_10BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_12GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_12GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_12RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_12BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_14GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_14GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_14RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_14BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_8GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_8GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_8RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_8BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_10GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_10GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_10RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_10BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_12GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_12GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_12RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_12BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_14GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_14GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_14RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_14BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_8GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_8GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_8RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_8BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_10GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_10GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_10RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_10BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_12GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_12GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_12RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_12BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_14GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_14GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_14RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_14BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN8_8GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN8_8GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN8_8RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN8_8BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_8GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_8GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_8RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_8BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_10GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_10GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_10RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_10BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_12GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_12GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_12RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_12BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_14GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_14GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_14RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_14BGGR[];

    // ISO values
    static const char ISO_AUTO[];
    static const char ISO_HJR[];
    static const char ISO_100[];
    static const char ISO_200[];
    static const char ISO_400[];
    static const char ISO_800[];
    static const char ISO_1600[];
    static const char ISO_3200[];
    static const char ISO_MANUAL[];

    // Values for auto exposure settings.
    static const char AUTO_EXPOSURE_FRAME_AVG[];
    static const char AUTO_EXPOSURE_CENTER_WEIGHTED[];
    static const char AUTO_EXPOSURE_SPOT_METERING[];
    static const char AUTO_EXPOSURE_SMART_METERING[];
    static const char AUTO_EXPOSURE_USER_METERING[];
    static const char AUTO_EXPOSURE_SPOT_METERING_ADV[];
    static const char AUTO_EXPOSURE_CENTER_WEIGHTED_ADV[];

    // Values for instant AEC modes
    static const char KEY_QC_INSTANT_AEC_DISABLE[];
    static const char KEY_QC_INSTANT_AEC_AGGRESSIVE_AEC[];
    static const char KEY_QC_INSTANT_AEC_FAST_AEC[];

    // Values for instant capture modes
    static const char KEY_QC_INSTANT_CAPTURE_DISABLE[];
    static const char KEY_QC_INSTANT_CAPTURE_AGGRESSIVE_AEC[];
    static const char KEY_QC_INSTANT_CAPTURE_FAST_AEC[];

    // Values for led calibration modes
    static const char KEY_QC_LED_CALIBRATION_OFF[];
    static const char KEY_QC_LED_CALIBRATION_DUAL[];
    static const char KEY_QC_LED_CALIBRATION_SINGLE[];

    static const char KEY_QC_SHARPNESS[];
    static const char KEY_QC_MIN_SHARPNESS[];
    static const char KEY_QC_MAX_SHARPNESS[];
    static const char KEY_QC_SHARPNESS_STEP[];
    static const char KEY_QC_CONTRAST[];
    static const char KEY_QC_MIN_CONTRAST[];
    static const char KEY_QC_MAX_CONTRAST[];
    static const char KEY_QC_CONTRAST_STEP[];
    static const char KEY_QC_SATURATION[];
    static const char KEY_QC_MIN_SATURATION[];
    static const char KEY_QC_MAX_SATURATION[];
    static const char KEY_QC_SATURATION_STEP[];
    static const char KEY_QC_BRIGHTNESS[];
    static const char KEY_QC_MIN_BRIGHTNESS[];
    static const char KEY_QC_MAX_BRIGHTNESS[];
    static const char KEY_QC_BRIGHTNESS_STEP[];
    static const char KEY_QC_SCE_FACTOR[];
    static const char KEY_QC_MIN_SCE_FACTOR[];
    static const char KEY_QC_MAX_SCE_FACTOR[];
    static const char KEY_QC_SCE_FACTOR_STEP[];

    static const char KEY_QC_HISTOGRAM[] ;
    static const char KEY_QC_SUPPORTED_HISTOGRAM_MODES[] ;
    static const char KEY_QC_SUPPORTED_HDR_NEED_1X[];
    static const char KEY_QC_HDR_NEED_1X[];
    static const char KEY_QC_VIDEO_HDR[];
    static const char KEY_QC_VT_ENABLE[];
    static const char KEY_QC_SUPPORTED_VIDEO_HDR_MODES[];
    static const char KEY_QC_SENSOR_HDR[];
    static const char KEY_QC_SUPPORTED_SENSOR_HDR_MODES[];
    static const char KEY_QC_RDI_MODE[];
    static const char KEY_QC_SUPPORTED_RDI_MODES[];
    static const char KEY_QC_SECURE_MODE[];
    static const char KEY_QC_SUPPORTED_SECURE_MODES[];

    // Values for SKIN TONE ENHANCEMENT
    static const char SKIN_TONE_ENHANCEMENT_ENABLE[] ;
    static const char SKIN_TONE_ENHANCEMENT_DISABLE[] ;

    // Values for Denoise
    static const char DENOISE_OFF[] ;
    static const char DENOISE_ON[] ;

    // Values for auto exposure settings.
    static const char FOCUS_ALGO_AUTO[];
    static const char FOCUS_ALGO_SPOT_METERING[];
    static const char FOCUS_ALGO_CENTER_WEIGHTED[];
    static const char FOCUS_ALGO_FRAME_AVERAGE[];

    // Values for AE Bracketing settings.
    static const char AE_BRACKET_OFF[];
    static const char AE_BRACKET[];

    // Values for AF Bracketing settings.
    static const char AF_BRACKET_OFF[];
    static const char AF_BRACKET_ON[];

    // Values for Refocus settings.
    static const char RE_FOCUS_OFF[];
    static const char RE_FOCUS_ON[];

    // Values for Chroma Flash settings.
    static const char CHROMA_FLASH_OFF[];
    static const char CHROMA_FLASH_ON[];

    // Values for Opti Zoom settings.
    static const char OPTI_ZOOM_OFF[];
    static const char OPTI_ZOOM_ON[];

    // Values for Still More settings.
    static const char STILL_MORE_OFF[];
    static const char STILL_MORE_ON[];

    // Values for HDR mode settings.
    static const char HDR_MODE_SENSOR[];
    static const char HDR_MODE_MULTI_FRAME[];

    // Values for True Portrait settings.
    static const char TRUE_PORTRAIT_OFF[];
    static const char TRUE_PORTRAIT_ON[];

    // Values for HFR settings.
    static const char VIDEO_HFR_OFF[];
    static const char VIDEO_HFR_2X[];
    static const char VIDEO_HFR_3X[];
    static const char VIDEO_HFR_4X[];
    static const char VIDEO_HFR_5X[];
    static const char VIDEO_HFR_6X[];
    static const char VIDEO_HFR_7X[];
    static const char VIDEO_HFR_8X[];
    static const char VIDEO_HFR_9X[];

    // Values for feature on/off settings.
    static const char VALUE_OFF[];
    static const char VALUE_ON[];

    // Values for feature enable/disable settings.
    static const char VALUE_ENABLE[];
    static const char VALUE_DISABLE[];

    // Values for feature true/false settings.
    static const char VALUE_FALSE[];
    static const char VALUE_TRUE[];

    //Values for flip settings
    static const char FLIP_MODE_OFF[];
    static const char FLIP_MODE_V[];
    static const char FLIP_MODE_H[];
    static const char FLIP_MODE_VH[];

    //Values for CDS Mode
    static const char CDS_MODE_OFF[];
    static const char CDS_MODE_ON[];
    static const char CDS_MODE_AUTO[];

    static const char VALUE_FAST[];
    static const char VALUE_HIGH_QUALITY[];

    static const char KEY_SELECTED_AUTO_SCENE[];

    // Values for Video rotation
    static const char VIDEO_ROTATION_0[];
    static const char VIDEO_ROTATION_90[];
    static const char VIDEO_ROTATION_180[];
    static const char VIDEO_ROTATION_270[];

#ifdef TARGET_TS_MAKEUP
    static const char KEY_TS_MAKEUP[];
    static const char KEY_TS_MAKEUP_WHITEN[];
    static const char KEY_TS_MAKEUP_CLEAN[];
#endif
    //param key for HFR batch size
    static const char KEY_QC_VIDEO_BATCH_SIZE[];
    enum {
        CAMERA_ORIENTATION_UNKNOWN = 0,
        CAMERA_ORIENTATION_PORTRAIT = 1,
        CAMERA_ORIENTATION_LANDSCAPE = 2,
    };

    template <typename valueType> struct QCameraMap {
        const char *const desc;
        valueType val;
    };

public:
    QCameraParameters();
    QCameraParameters(const String8 &params);
    ~QCameraParameters();

    int32_t allocate();
    int32_t init(cam_capability_t *,
                 mm_camera_vtbl_t *,
                 QCameraAdjustFPS *);
    void deinit();
    int32_t initDefaultParameters();
    int32_t updateParameters(const String8& params, bool &needRestart);
    int32_t commitParameters();

    char* getParameters();
    void getPreviewFpsRange(int *min_fps, int *max_fps) const {
            CameraParameters::getPreviewFpsRange(min_fps, max_fps);
    }
#ifdef TARGET_TS_MAKEUP
    bool getTsMakeupInfo(int &whiteLevel, int &cleanLevel) const;
#endif

    int getPreviewHalPixelFormat();
    int32_t getStreamRotation(cam_stream_type_t streamType,
                               cam_pp_feature_config_t &featureConfig,
                               cam_dimension_t &dim);
    int32_t getStreamFormat(cam_stream_type_t streamType,
                             cam_format_t &format);
  int32_t getStreamSubFormat(cam_stream_type_t streamType,
             cam_sub_format_type_t &sub_format);

    int32_t getStreamDimension(cam_stream_type_t streamType,
            cam_dimension_t &dim);
    void getThumbnailSize(int *width, int *height) const;


    uint8_t getZSLBurstInterval();
    uint8_t getZSLQueueDepth();
    uint8_t getZSLBackLookCount();
    uint8_t getMaxUnmatchedFramesInQueue();
    bool isZSLMode() {return m_bZslMode;};
    bool isRdiMode() {return m_bRdiMode;};
    bool isSecureMode() {return m_bSecureMode;};
    bool isNoDisplayMode() {return m_bNoDisplayMode;};
    bool isWNREnabled() {return m_bWNROn;};
    bool isTNRSnapshotEnabled() {return m_bTNRSnapshotOn;};
    int32_t getCDSMode() {return mCds_mode;};
    bool isLTMForSeeMoreEnabled() {return m_bLtmForSeeMoreEnabled;};
    bool isHfrMode() {return m_bHfrMode;};
    void getHfrFps(cam_fps_range_t &pFpsRange) { pFpsRange = m_hfrFpsRange;};
    uint8_t getNumOfSnapshots();
    uint8_t getNumOfRetroSnapshots();
    uint8_t getNumOfExtraHDRInBufsIfNeeded();
    uint8_t getNumOfExtraHDROutBufsIfNeeded();

    bool getRecordingHintValue() {return m_bRecordingHint;}; // return local copy of video hint
    uint32_t getJpegQuality();
    uint32_t getRotation();
    uint32_t getDeviceRotation();
    uint32_t getJpegExifRotation();
    bool useJpegExifRotation();
    int32_t getEffectValue();
    bool isInstantAECEnabled() {return m_bInstantAEC;};
    bool isInstantCaptureEnabled() {return m_bInstantCapture;};
    uint8_t getAecFrameBoundValue() {return mAecFrameBound;};
    uint8_t getAecSkipDisplayFrameBound() {return mAecSkipDisplayFrameBound;};

    int32_t getExifDateTime(String8 &dateTime, String8 &subsecTime);
    int32_t getExifFocalLength(rat_t *focalLenght);
    uint16_t getExifIsoSpeed();
    int32_t getExifGpsProcessingMethod(char *gpsProcessingMethod, uint32_t &count);
    int32_t getExifLatitude(rat_t *latitude, char *latRef);
    int32_t getExifLongitude(rat_t *longitude, char *lonRef);
    int32_t getExifAltitude(rat_t *altitude, char *altRef);
    int32_t getExifGpsDateTimeStamp(char *gpsDateStamp, uint32_t bufLen, rat_t *gpsTimeStamp);
    bool isVideoBuffersCached();
    int32_t updateFocusDistances(cam_focus_distances_info_t *focusDistances);

    bool isAEBracketEnabled();
    int32_t setAEBracketing();
    bool isFpsDebugEnabled() {return m_bDebugFps;};
    bool isHistogramEnabled() {return m_bHistogramEnabled;};
    bool isSceneSelectionEnabled() {return m_bSceneSelection;};
    int32_t setSelectedScene(cam_scene_mode_type scene);
    cam_scene_mode_type getSelectedScene();
    bool isFaceDetectionEnabled() {return ((m_nFaceProcMask &
            (CAM_FACE_PROCESS_MASK_DETECTION | CAM_FACE_PROCESS_MASK_FOCUS)) != 0);};
    int32_t setFaceDetectionOption(bool enabled);
    int32_t setHistogram(bool enabled);
    int32_t setFaceDetection(bool enabled, bool initCommit);
    int32_t setFrameSkip(enum msm_vfe_frame_skip_pattern pattern);
    qcamera_thermal_mode getThermalMode() {return m_ThermalMode;};
    int32_t updateRecordingHintValue(int32_t value);
    int32_t setHDRAEBracket(cam_exp_bracketing_t hdrBracket);
    bool isHDREnabled();
    bool isAutoHDREnabled();
    int32_t stopAEBracket();
    int32_t updateRAW(cam_dimension_t max_dim);
    bool isAVTimerEnabled();
    bool isDISEnabled();
    int32_t setISType();
    cam_is_type_t getVideoISType();
    cam_is_type_t getPreviewISType();
    uint8_t getMobicatMask();

    cam_focus_mode_type getFocusMode() const {return mFocusMode;};
    int32_t setNumOfSnapshot();
    int32_t adjustPreviewFpsRange(cam_fps_range_t *fpsRange);
    bool isJpegPictureFormat() {return (mPictureFormat == CAM_FORMAT_JPEG);};
    bool isNV16PictureFormat() {return (mPictureFormat == CAM_FORMAT_YUV_422_NV16);};
    bool isNV21PictureFormat() {return (mPictureFormat == CAM_FORMAT_YUV_420_NV21);};
    cam_denoise_process_type_t getDenoiseProcessPlate(cam_intf_parm_type_t type);
    int32_t getMaxPicSize(cam_dimension_t &dim) { dim = m_maxPicSize; return NO_ERROR; };
    int getFlipMode(cam_stream_type_t streamType);
    bool isSnapshotFDNeeded();

    bool isHDR1xFrameEnabled() {return m_bHDR1xFrameEnabled;}
    bool isSupportedSensorHdrSize(const QCameraParameters& params);
    bool isYUVFrameInfoNeeded();
    const char*getFrameFmtString(cam_format_t fmt);
    bool isHDR1xExtraBufferNeeded() {return m_bHDR1xExtraBufferNeeded;}
    bool isHDROutputCropEnabled() {return m_bHDROutputCropEnabled;}

    bool isPreviewFlipChanged() { return m_bPreviewFlipChanged; };
    bool isVideoFlipChanged() { return m_bVideoFlipChanged; };
    bool isSnapshotFlipChanged() { return m_bSnapshotFlipChanged; };
    void setHDRSceneEnable(bool bflag);
    int32_t updateAWBParams(cam_awb_params_t &awb_params);

    const char *getASDStateString(cam_auto_scene_t scene);
    bool isHDRThumbnailProcessNeeded() { return m_bHDRThumbnailProcessNeeded; };
    void setMinPpMask(cam_feature_mask_t min_pp_mask) { m_nMinRequiredPpMask = min_pp_mask; };
    bool setStreamConfigure(bool isCapture, bool previewAsPostview, bool resetConfig);
    int32_t addOnlineRotation(uint32_t rotation, uint32_t streamId, int32_t device_rotation);
    uint8_t getNumOfExtraBuffersForImageProc();
    uint8_t getNumOfExtraBuffersForVideo();
    uint8_t getNumOfExtraBuffersForPreview();
    uint32_t getExifBufIndex(uint32_t captureIndex);
    bool needThumbnailReprocess(cam_feature_mask_t *pFeatureMask);
    inline bool isUbiFocusEnabled() {return m_bAFBracketingOn && !m_bReFocusOn;};
    inline bool isChromaFlashEnabled() {return m_bChromaFlashOn;};
    inline bool isHighQualityNoiseReductionMode() {return m_bHighQualityNoiseReductionMode;};
    inline bool isTruePortraitEnabled() {return m_bTruePortraitOn;};
    inline size_t getTPMaxMetaSize() {
        return m_pCapability->true_portrait_settings_need.meta_max_size;};
    inline bool isSeeMoreEnabled() {return m_bSeeMoreOn;};
    inline bool isStillMoreEnabled() {return m_bStillMoreOn;};
    bool isOptiZoomEnabled();

    int32_t commitAFBracket(cam_af_bracketing_t afBracket);
    int32_t set3ALock(bool lock3A);
    int32_t setAndCommitZoom(int zoom_level);
    uint8_t getBurstCountForAdvancedCapture();
    uint32_t getNumberInBufsForSingleShot();
    uint32_t getNumberOutBufsForSingleShot();
    int32_t setLongshotEnable(bool enable);
    String8 dump();
    inline bool isUbiRefocus() {return m_bReFocusOn &&
            (m_pCapability->refocus_af_bracketing_need.output_count > 1);};
    inline uint32_t getRefocusMaxMetaSize() {
            return m_pCapability->refocus_af_bracketing_need.meta_max_size;};
    inline uint8_t getRefocusOutputCount() {
            return m_pCapability->refocus_af_bracketing_need.output_count;};
    inline bool generateThumbFromMain() {return isUbiFocusEnabled() ||
            isChromaFlashEnabled() || isOptiZoomEnabled() || isUbiRefocus()
            || isHDREnabled() || isStillMoreEnabled() || isTruePortraitEnabled(); }
    void updateCurrentFocusPosition(cam_focus_pos_info_t &cur_pos_info);
    void updateAEInfo(cam_3a_params_t &ae_params);
    bool isAdvCamFeaturesEnabled() {return isUbiFocusEnabled() ||
            isChromaFlashEnabled() || m_bOptiZoomOn || isHDREnabled() ||
            isAEBracketEnabled() || isStillMoreEnabled() || isUbiRefocus();}
    int32_t setAecLock(const char *aecStr);
    int32_t updateDebugLevel();
    bool is4k2kVideoResolution();
    bool isUBWCEnabled();

    int getBrightness();
    int32_t updateOisValue(bool oisValue);
    int32_t setIntEvent(cam_int_evt_params_t params);
    bool getofflineRAW() {return mOfflineRAW;}
    bool getQuadraCfa();
    int32_t updatePpFeatureMask(cam_stream_type_t stream_type);
    int32_t getStreamPpMask(cam_stream_type_t stream_type, cam_feature_mask_t &pp_mask);
    int32_t getSharpness() {return m_nSharpness;};
    int32_t getEffect() {return mParmEffect;};
    int32_t updateFlashMode(cam_flash_mode_t flash_mode);
    int32_t configureAEBracketing(cam_capture_frame_config_t &frame_config);
    int32_t configureHDRBracketing(cam_capture_frame_config_t &frame_config);
    int32_t configFrameCapture(bool commitSettings);
    int32_t resetFrameCapture(bool commitSettings, bool lowLightEnabled);
    cam_still_more_t getStillMoreSettings() {return m_stillmore_config;};
    void setStillMoreSettings(cam_still_more_t stillmore_config)
            {m_stillmore_config = stillmore_config;};
    cam_still_more_t getStillMoreCapability()
            {return m_pCapability->stillmore_settings_need;};
    cam_dyn_img_data_t getDynamicImgData() { return m_DynamicImgData; }
    void setDynamicImgData(cam_dyn_img_data_t d) { m_DynamicImgData = d; }

    int32_t getParmZoomLevel(){return mParmZoomLevel;};
    int8_t  getReprocCount(){return mTotalPPCount;};
    bool isMultiPassReprocessing();
    int8_t  getCurPPCount(){return mCurPPCount;};
    void    setReprocCount();
    bool    isPostProcScaling();
    bool    isLLNoiseEnabled();
    void    setCurPPCount(int8_t count) {mCurPPCount = count;};
    int32_t setQuadraCfaMode(uint32_t value, bool initCommit);
    int32_t setToneMapMode(uint32_t value, bool initCommit);
    void setTintless(bool enable);
    uint8_t getLongshotStages();
    int8_t  getBufBatchCount() {return mBufBatchCnt;};
    int8_t  getVideoBatchSize() {return mVideoBatchSize;};

    int32_t setManualCaptureMode(
            QCameraManualCaptureModes value = CAM_MANUAL_CAPTURE_TYPE_OFF);
    QCameraManualCaptureModes getManualCaptureMode()
            {return m_ManualCaptureMode;};
    int64_t getExposureTime() {return m_expTime;};

    cam_capture_frame_config_t getCaptureFrameConfig()
            { return m_captureFrameConfig; };
    void setJpegRotation(int rotation);
    uint32_t getJpegRotation() { return mJpegRotation;};

    void setLowLightLevel(cam_low_light_mode_t value)
            { m_LowLightLevel = value; };
    cam_low_light_mode_t getLowLightLevel() {return m_LowLightLevel;};
    bool getLowLightCapture() { return m_LLCaptureEnabled; };

    /* Dual camera specific */
    bool getDcrf() { return m_bDcrfEnabled; }
    int32_t setRelatedCamSyncInfo(
            cam_sync_related_sensors_event_info_t* info);
    const cam_sync_related_sensors_event_info_t*
            getRelatedCamSyncInfo(void);
    int32_t setFrameSyncEnabled(bool enable);
    bool isFrameSyncEnabled(void);
    int32_t getRelatedCamCalibration(
            cam_related_system_calibration_data_t* calib);
    int32_t bundleRelatedCameras(bool sync, uint32_t sessionid);
    uint8_t fdModeInVideo();
    bool isOEMFeatEnabled() { return m_bOEMFeatEnabled; }

    int32_t setZslMode(bool value);
    int32_t updateZSLModeValue(bool value);

    bool isReprocScaleEnabled();
    bool isUnderReprocScaling();
    int32_t getPicSizeFromAPK(int &width, int &height);

    int32_t checkFeatureConcurrency();
    int32_t setInstantAEC(uint8_t enable, bool initCommit);

    int32_t getAnalysisInfo(
        bool fdVideoEnabled,
        bool hal3,
        cam_feature_mask_t featureMask,
        cam_analysis_info_t *pAnalysisInfo);

    int32_t getMetaRawInfo();
    bool sendStreamConfigForPickRes(cam_stream_size_info_t &stream_config_info);
    int32_t updateDtVc(int32_t *dt, int32_t *vc);
    bool isLinkPreviewForLiveShot();

private:
    int32_t setPreviewSize(const QCameraParameters& );
    int32_t setVideoSize(const QCameraParameters& );
    int32_t setPictureSize(const QCameraParameters& );
    int32_t setLiveSnapshotSize(const QCameraParameters& );
    int32_t setPreviewFormat(const QCameraParameters& );
    int32_t setPictureFormat(const QCameraParameters& );
    int32_t setOrientation(const QCameraParameters& );
    int32_t setJpegThumbnailSize(const QCameraParameters& );
    int32_t setJpegQuality(const QCameraParameters& );
    int32_t setPreviewFpsRange(const QCameraParameters& );
    int32_t setPreviewFrameRate(const QCameraParameters& );
    int32_t setAutoExposure(const QCameraParameters& );
    int32_t setEffect(const QCameraParameters& );
    int32_t setBrightness(const QCameraParameters& );
    int32_t setFocusMode(const QCameraParameters& );
    int32_t setFocusPosition(const QCameraParameters& );
    int32_t setSharpness(const QCameraParameters& );
    int32_t setSaturation(const QCameraParameters& );
    int32_t setContrast(const QCameraParameters& );
    int32_t setSkinToneEnhancement(const QCameraParameters& );
    int32_t setSceneDetect(const QCameraParameters& );
    int32_t setVideoHDR(const QCameraParameters& );
    int32_t setVtEnable(const QCameraParameters& );
    int32_t setZoom(const QCameraParameters& );
    int32_t setISOValue(const QCameraParameters& );
    int32_t setContinuousISO(const QCameraParameters& );
    int32_t setExposureTime(const QCameraParameters& );
    int32_t setRotation(const QCameraParameters& );
    int32_t setVideoRotation(const QCameraParameters& );
    int32_t setFlash(const QCameraParameters& );
    int32_t setAecLock(const QCameraParameters& );
    int32_t setAwbLock(const QCameraParameters& );
    int32_t setMCEValue(const QCameraParameters& );
    int32_t setDISValue(const QCameraParameters& params);
    int32_t setLensShadeValue(const QCameraParameters& );
    int32_t setExposureCompensation(const QCameraParameters& );
    int32_t setWhiteBalance(const QCameraParameters& );
    int32_t setManualWhiteBalance(const QCameraParameters& );
    int32_t setAntibanding(const QCameraParameters& );
    int32_t setFocusAreas(const QCameraParameters& );
    int32_t setMeteringAreas(const QCameraParameters& );
    int32_t setSceneMode(const QCameraParameters& );
    int32_t setSelectableZoneAf(const QCameraParameters& );
    int32_t setAEBracket(const QCameraParameters& );
    int32_t setAFBracket(const QCameraParameters& );
    int32_t setReFocus(const QCameraParameters& );
    int32_t setChromaFlash(const QCameraParameters& );
    int32_t setOptiZoom(const QCameraParameters& );
    int32_t setHDRMode(const QCameraParameters& );
    int32_t setHDRNeed1x(const QCameraParameters& );
    int32_t setTruePortrait(const QCameraParameters& );
    int32_t setSeeMore(const QCameraParameters& );
    int32_t setStillMore(const QCameraParameters& );
#ifdef TARGET_TS_MAKEUP
    int32_t setTsMakeup(const QCameraParameters& );
#endif
    int32_t setNoiseReductionMode(const QCameraParameters& );
    int32_t setRedeyeReduction(const QCameraParameters& );
    int32_t setGpsLocation(const QCameraParameters& );
    int32_t setRecordingHint(const QCameraParameters& );
    int32_t setNoDisplayMode(const QCameraParameters& );
    int32_t setWaveletDenoise(const QCameraParameters& );
    int32_t setTemporalDenoise(const QCameraParameters&);
    int32_t setZslMode(const QCameraParameters& );
    int32_t setZslAttributes(const QCameraParameters& );
    int32_t setAutoHDR(const QCameraParameters& params);
    int32_t setCameraMode(const QCameraParameters& );
    int32_t setSceneSelectionMode(const QCameraParameters& params);
    int32_t setFaceRecognition(const QCameraParameters& );
    int32_t setFlip(const QCameraParameters& );
    int32_t setRetroActiveBurstNum(const QCameraParameters& params);
    int32_t setBurstLEDOnPeriod(const QCameraParameters& params);
    int32_t setSnapshotFDReq(const QCameraParameters& );
    int32_t setStatsDebugMask();
    int32_t setPAAF();
    int32_t setTintlessValue(const QCameraParameters& params);
    int32_t setCDSMode(const QCameraParameters& params);
    int32_t setInitialExposureIndex(const QCameraParameters& params);
    int32_t setInstantCapture(const QCameraParameters& params);
    int32_t setInstantAEC(const QCameraParameters& params);
    int32_t setMobicat(const QCameraParameters& params);
    int32_t setRdiMode(const QCameraParameters& );
    int32_t setSecureMode(const QCameraParameters& );
    int32_t setCacheVideoBuffers(const QCameraParameters& params);
    int32_t setCustomParams(const QCameraParameters& params);
    int32_t setAutoExposure(const char *autoExp);
    int32_t setPreviewFpsRange(int min_fps,int max_fps,
            int vid_min_fps,int vid_max_fps);
    int32_t setEffect(const char *effect);
    int32_t setBrightness(int brightness);
    int32_t setFocusMode(const char *focusMode);
    int32_t setFocusPosition(const char *typeStr, const char *posStr);
    int32_t setSharpness(int sharpness);
    int32_t setSaturation(int saturation);
    int32_t setContrast(int contrast);
    int32_t setSkinToneEnhancement(int sceFactor);
    int32_t setSceneDetect(const char *scendDetect);
    int32_t setVideoHDR(const char *videoHDR);
    int32_t setSensorSnapshotHDR(const char *snapshotHDR);
    int32_t setVtEnable(const char *vtEnable);
    int32_t setZoom(int zoom_level);
    int32_t setISOValue(const char *isoValue);
    int32_t setContinuousISO(const char *isoValue);
    int32_t setExposureTime(const char *expTimeStr);
    int32_t setFlash(const char *flashStr);
    int32_t setAwbLock(const char *awbStr);
    int32_t setMCEValue(const char *mceStr);
    int32_t setDISValue(const char *disStr);
    int32_t setHighFrameRate(const int32_t hfrMode);
    int32_t setLensShadeValue(const char *lensShadeStr);
    int32_t setExposureCompensation(int expComp);
    int32_t setWhiteBalance(const char *wbStr);
    int32_t setWBManualCCT(const char *cctStr);
    int32_t setManualWBGains(const char *gainStr);
    int32_t setAntibanding(const char *antiBandingStr);
    int32_t setFocusAreas(const char *focusAreasStr);
    int32_t setMeteringAreas(const char *meteringAreasStr);
    int32_t setSceneMode(const char *sceneModeStr);
    int32_t setSelectableZoneAf(const char *selZoneAFStr);
    int32_t setAEBracket(const char *aecBracketStr);
    int32_t setAFBracket(const char *afBracketStr);
    int32_t setReFocus(const char *reFocusStr);
    int32_t setChromaFlash(const char *chromaFlashStr);
    int32_t setOptiZoom(const char *optiZoomStr);
    int32_t setHDRMode(const char *optiZoomStr);
    int32_t setHDRNeed1x(const char *optiZoomStr);
    int32_t setTruePortrait(const char *truePortraitStr);
    int32_t setSeeMore(const char *SeeMoreStr);
    int32_t setStillMore(const char *StillMoreStr);
    int32_t setNoiseReductionMode(const char *noiseReductionModeStr);
    int32_t setRedeyeReduction(const char *redeyeStr);
    int32_t setWaveletDenoise(const char *wnrStr);
    int32_t setFaceRecognition(const char *faceRecog, uint32_t maxFaces);
    int32_t setTintlessValue(const char *tintStr);
    bool UpdateHFRFrameRate(const QCameraParameters& params);
    int32_t setRdiMode(const char *str);
    int32_t setSecureMode(const char *str);
    int32_t setLongshotParam(const QCameraParameters& params);
    int32_t parseGains(const char *gainStr, double &r_gain,
            double &g_gain, double &b_gain);
    int32_t setCacheVideoBuffers(const char *cacheVideoBufStr);
    int32_t setCDSMode(int32_t cds_mode, bool initCommit);
    int32_t setEztune();
    void setLowLightCapture();
    int setRecordingHintValue(int32_t value); // set local copy of video hint and send to server
                                              // no change in parameters value
    int32_t updateFlash(bool commitSettings);
    int32_t setRawSize(cam_dimension_t &dim);
    int32_t setMaxPicSize(cam_dimension_t &dim) { m_maxPicSize = dim; return NO_ERROR; };
    void setBufBatchCount(int8_t buf_cnt);
    void setVideoBatchSize();
    void setDcrf();
    int32_t setStreamPpMask(cam_stream_type_t stream_type, cam_feature_mask_t pp_mask);
    void setOfflineRAW(bool value = 0);
    int32_t setQuadraCfa(const QCameraParameters& params);
    int32_t configureFlash(cam_capture_frame_config_t &frame_config);
    int32_t configureLowLight(cam_capture_frame_config_t &frame_config);
    int32_t configureManualCapture(cam_capture_frame_config_t &frame_config);

    bool isTNRPreviewEnabled() {return m_bTNRPreviewOn;};
    bool isTNRVideoEnabled() {return m_bTNRVideoOn;};
    bool getFaceDetectionOption() { return  m_bFaceDetectionOn;}
    void getLiveSnapshotSize(cam_dimension_t &dim);
    int32_t getRawSize(cam_dimension_t &dim) {dim = m_rawSize; return NO_ERROR;};
    int getAutoFlickerMode();
    bool sendStreamConfigInfo(cam_stream_size_info_t &stream_config_info);
    inline bool isLowMemoryDevice() {return m_bIsLowMemoryDevice;};
    bool isPreviewSeeMoreRequired();
    bool isEztuneEnabled() { return m_bEztuneEnabled; };
    int32_t getZoomLevel(){return mZoomLevel;};
    int32_t parse_pair(const char *str, int *first, int *second,
                       char delim, char **endptr);
    void parseSizesList(const char *sizesStr, Vector<Size> &sizes);
    int32_t parseNDimVector(const char *str, int *num, int N, char delim);
    int32_t parseCameraAreaString(const char *str, int max_num_areas,
                                  cam_area_t *pAreas, int& num_areas_found);
    bool validateCameraAreas(cam_area_t *areas, int num_areas);
    int parseGPSCoordinate(const char *coord_str, rat_t *coord);
    int32_t getRational(rat_t *rat, int num, int denom);
    String8 createSizesString(const cam_dimension_t *sizes, size_t len);
    String8 createHfrValuesString(const cam_hfr_info_t *values, size_t len,
            const QCameraMap<cam_hfr_mode_t> *map, size_t map_len);
    String8 createHfrSizesString(const cam_hfr_info_t *values, size_t len);
    String8 createFpsRangeString(const cam_fps_range_t *fps,
            size_t len, int &default_fps_index);
    String8 createFpsString(cam_fps_range_t &fps);
    String8 createZoomRatioValuesString(uint32_t *zoomRatios, size_t length);
    int32_t setLedCalibration(const QCameraParameters& params);
    int32_t setLedCalibration(const char *calibration_mode);
    int32_t setAdvancedCaptureMode();

    // ops for batch set/get params with server
    int32_t initBatchUpdate(parm_buffer_t *p_table);
    int32_t commitSetBatch();
    int32_t commitGetBatch();

    // ops to tempororily update parameter entries and commit
    int32_t updateParamEntry(const char *key, const char *value);
    int32_t commitParamChanges();
    void updateViewAngles();

    // Map from strings to values
    static const cam_dimension_t THUMBNAIL_SIZES_MAP[];
    static const QCameraMap<cam_auto_exposure_mode_type> AUTO_EXPOSURE_MAP[];
    static const QCameraMap<cam_aec_convergence_type> INSTANT_CAPTURE_MODES_MAP[];
    static const QCameraMap<cam_aec_convergence_type> INSTANT_AEC_MODES_MAP[];
    static const QCameraMap<cam_led_calibration_mode_t> LED_CALIBRATION_MODE_MAP[];
    static const QCameraMap<cam_format_t> PREVIEW_FORMATS_MAP[];
    static const QCameraMap<cam_format_t> PICTURE_TYPES_MAP[];
    static const QCameraMap<cam_focus_mode_type> FOCUS_MODES_MAP[];
    static const QCameraMap<cam_effect_mode_type> EFFECT_MODES_MAP[];
    static const QCameraMap<cam_scene_mode_type> SCENE_MODES_MAP[];
    static const QCameraMap<cam_flash_mode_t> FLASH_MODES_MAP[];
    static const QCameraMap<cam_focus_algorithm_type> FOCUS_ALGO_MAP[];
    static const QCameraMap<cam_wb_mode_type> WHITE_BALANCE_MODES_MAP[];
    static const QCameraMap<cam_antibanding_mode_type> ANTIBANDING_MODES_MAP[];
    static const QCameraMap<cam_iso_mode_type> ISO_MODES_MAP[];
    static const QCameraMap<cam_hfr_mode_t> HFR_MODES_MAP[];
    static const QCameraMap<cam_bracket_mode> BRACKETING_MODES_MAP[];
    static const QCameraMap<int> ON_OFF_MODES_MAP[];
    static const QCameraMap<int> ENABLE_DISABLE_MODES_MAP[];
    static const QCameraMap<int> DENOISE_ON_OFF_MODES_MAP[];
    static const QCameraMap<int> TRUE_FALSE_MODES_MAP[];
    static const QCameraMap<int> TOUCH_AF_AEC_MODES_MAP[];
    static const QCameraMap<cam_flip_t> FLIP_MODES_MAP[];
    static const QCameraMap<int> AF_BRACKETING_MODES_MAP[];
    static const QCameraMap<int> RE_FOCUS_MODES_MAP[];
    static const QCameraMap<int> CHROMA_FLASH_MODES_MAP[];
    static const QCameraMap<int> OPTI_ZOOM_MODES_MAP[];
    static const QCameraMap<int> TRUE_PORTRAIT_MODES_MAP[];
    static const QCameraMap<cam_cds_mode_type_t> CDS_MODES_MAP[];
    static const QCameraMap<int> HDR_MODES_MAP[];
    static const QCameraMap<int> VIDEO_ROTATION_MODES_MAP[];
    static const QCameraMap<int> SEE_MORE_MODES_MAP[];
    static const QCameraMap<int> STILL_MORE_MODES_MAP[];
    static const QCameraMap<int> NOISE_REDUCTION_MODES_MAP[];

    QCameraReprocScaleParam m_reprocScaleParam;
    QCameraCommon           mCommon;

    cam_capability_t *m_pCapability;
    mm_camera_vtbl_t *m_pCamOpsTbl;
    QCameraHeapMemory *m_pParamHeap;
    parm_buffer_t     *m_pParamBuf;  // ptr to param buf in m_pParamHeap
    /* heap for mapping dual cam event info */
    QCameraHeapMemory *m_pRelCamSyncHeap;
    /* ptr to sync buffer in m_pRelCamSyncHeap */
    cam_sync_related_sensors_event_info_t *m_pRelCamSyncBuf;
    cam_sync_related_sensors_event_info_t m_relCamSyncInfo;
    bool m_bFrameSyncEnabled;
    cam_is_type_t mIsTypeVideo;
    cam_is_type_t mIsTypePreview;

    bool m_bZslMode;                // if ZSL is enabled
    bool m_bZslMode_new;
    bool m_bForceZslMode;
    bool m_bRecordingHint;          // local copy of recording hint
    bool m_bRecordingHint_new;
    bool m_bHistogramEnabled;       // if histogram is enabled
    bool m_bLongshotEnabled;       // if longshot is enabled
    uint32_t m_nFaceProcMask;       // face process mask
    bool m_bFaceDetectionOn;        //  if face Detection turned on by user
    bool m_bDebugFps;               // if FPS need to be logged
    cam_focus_mode_type mFocusMode;
    cam_format_t mPreviewFormat;
    cam_format_t mAppPreviewFormat;
    int32_t mPictureFormat;         // could be CAMERA_PICTURE_TYPE_JPEG or cam_format_t
    bool m_bNeedRestart;            // if preview needs restart after parameters updated
    bool m_bNoDisplayMode;
    bool m_bWNROn;
    bool m_bTNRPreviewOn;
    bool m_bTNRVideoOn;
    bool m_bTNRSnapshotOn;
    bool m_bInited;
    int m_nRetroBurstNum;
    int m_nBurstLEDOnPeriod;
    cam_exp_bracketing_t m_AEBracketingClient;
    bool m_bUpdateEffects;          // Cause reapplying of effects
    bool m_bSceneTransitionAuto;    // Indicate that scene has changed to Auto
    bool m_bPreviewFlipChanged;        // if flip setting for preview changed
    bool m_bVideoFlipChanged;          // if flip setting for video changed
    bool m_bSnapshotFlipChanged;       // if flip setting for snapshot changed
    bool m_bFixedFrameRateSet;      // Indicates that a fixed frame rate is set
    qcamera_thermal_mode m_ThermalMode; // adjust fps vs adjust frameskip
    cam_dimension_t m_LiveSnapshotSize; // live snapshot size
    cam_dimension_t m_rawSize; // Raw size
    cam_dimension_t m_maxPicSize;
    bool m_bHDREnabled;             // if HDR is enabled
    bool m_bLocalHDREnabled;   // This flag tells whether HDR enabled or not regarless of APP mode
    bool m_bAVTimerEnabled;    //if AVTimer is enabled
    bool m_bDISEnabled;
    bool m_bOISEnabled;
    cam_still_more_t m_stillmore_config;
    bool m_bMetaRawEnabled;

    uint8_t m_MobiMask;
    QCameraAdjustFPS *m_AdjustFPS;
    bool m_bHDR1xFrameEnabled;          // if frame with exposure compensation 0 during HDR is enabled
    bool m_HDRSceneEnabled; // Auto HDR indication
    bool m_bHDRThumbnailProcessNeeded;        // if thumbnail need to be processed for HDR
    bool m_bHDR1xExtraBufferNeeded;     // if extra frame with exposure compensation 0 during HDR is needed
    bool m_bHDROutputCropEnabled;     // if HDR output frame need to be scaled to user resolution
    DefaultKeyedVector<String8,String8> m_tempMap; // map for temororily store parameters to be set
    cam_fps_range_t m_default_fps_range;
    bool m_bAFBracketingOn;
    bool m_bReFocusOn;
    bool m_bChromaFlashOn;
    bool m_bOptiZoomOn;
    bool m_bSceneSelection;
    Mutex m_SceneSelectLock;
    cam_scene_mode_type m_SelectedScene;
    bool m_bSeeMoreOn;
    bool m_bStillMoreOn;
    bool m_bHighQualityNoiseReductionMode;
    cam_fps_range_t m_hfrFpsRange;
    bool m_bHfrMode;
    bool m_bSensorHDREnabled;             // if HDR is enabled
    bool m_bRdiMode;                // if RDI mode
    bool m_bSecureMode;
    bool m_bAeBracketingEnabled;
    int32_t mFlashValue;
    int32_t mFlashDaemonValue;
    int32_t mHfrMode;
    bool m_bHDRModeSensor;
    bool mOfflineRAW;
    bool m_bTruePortraitOn;
    cam_feature_mask_t m_nMinRequiredPpMask;
    cam_feature_mask_t mStreamPpMask[CAM_STREAM_TYPE_MAX];
    int32_t m_nSharpness;
    int8_t mTotalPPCount;
    int8_t mCurPPCount;
    int32_t mZoomLevel;
    int32_t mParmZoomLevel;
    bool m_bIsLowMemoryDevice;
    int32_t mCds_mode;
    int32_t mParmEffect;
    cam_capture_frame_config_t m_captureFrameConfig;
    int8_t mBufBatchCnt;
    bool m_bEztuneEnabled;
    bool m_bDcrfEnabled;
    uint32_t mRotation;
    uint32_t mJpegRotation;
    int8_t mVideoBatchSize;
    bool m_LLCaptureEnabled;
    cam_low_light_mode_t m_LowLightLevel;
    bool m_bLtmForSeeMoreEnabled;
    int64_t m_expTime;
    bool m_bOEMFeatEnabled;
    int32_t m_isoValue;
    QCameraManualCaptureModes m_ManualCaptureMode;
    cam_dyn_img_data_t m_DynamicImgData;
    cam_led_calibration_mode_t m_ledCalibrationMode;
    // Param to trigger instant AEC.
    bool m_bInstantAEC;
    // Param to trigger instant capture.
    bool m_bInstantCapture;
    // Number of frames, camera interface will wait for getting the instant capture frame.
    uint8_t mAecFrameBound;
    // Number of preview frames, that HAL will hold without displaying, for instant AEC mode.
    uint8_t mAecSkipDisplayFrameBound;
    bool m_bQuadraCfa;
};

}; // namespace qcamera

#endif
