/*
 * Copyright (C) 2014, 2017-2018 The  Linux Foundation. All rights reserved.
 * Not a contribution
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2018 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "LightsService"

#include "Light.h"
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <fstream>

const static std::string kLcdBacklightPath = "/sys/class/leds/lcd-backlight/brightness";
const static std::string kLcdMaxBacklightPath = "/sys/class/leds/lcd-backlight/max_brightness";
const static std::string kWhiteLedPath = "/sys/class/leds/white/brightness";
const static std::string kWhiteBlinkPath = "/sys/class/leds/white/device/blink";
const static std::string kWhiteFreqPath = "/sys/class/leds/white/device/grpfreq";
const static std::string kWhitePwmPath = "/sys/class/leds/white/device/grppwm";

namespace android {
namespace hardware {
namespace light {
namespace V2_0 {
namespace implementation {

/*
 * Write value to path and close file.
 */
template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);
    file << value;
}

template <typename T>
static T get(const std::string& path, const T& def) {
    std::ifstream file(path);
    T result;

    file >> result;
    return file.fail() ? def : result;
}

static int rgbToBrightness(const LightState& state) {
    int color = state.color & 0x00ffffff;
    return ((77 * ((color >> 16) & 0x00ff))
            + (150 * ((color >> 8) & 0x00ff))
            + (29 * (color & 0x00ff))) >> 8;
}

Light::Light() {
    mLights.emplace(Type::ATTENTION, std::bind(&Light::handleWhiteLed, this, std::placeholders::_1, 0));
    mLights.emplace(Type::BACKLIGHT,
            [](const LightState& state) { set(kLcdBacklightPath, rgbToBrightness(state)); });
    mLights.emplace(Type::BATTERY, std::bind(&Light::handleWhiteLed, this, std::placeholders::_1, 2));
    mLights.emplace(Type::NOTIFICATIONS, std::bind(&Light::handleWhiteLed, this, std::placeholders::_1, 1));
}

void Light::handleWhiteLed(const LightState& state, size_t index) {
    mLightStates.at(index) = state;

    LightState stateToUse = mLightStates.front();
    for (const auto& lightState : mLightStates) {
        if (lightState.color & 0xffffff) {
            stateToUse = lightState;
            break;
        }
    }

    int brightness = rgbToBrightness(stateToUse);

    set(kWhiteLedPath, brightness);

    int onMs = stateToUse.flashMode == Flash::TIMED ? stateToUse.flashOnMs : 0;
    int offMs = stateToUse.flashMode == Flash::TIMED ? stateToUse.flashOffMs : 0;
    bool blink = onMs > 0 && offMs > 0;

    if (blink) {
        int totalMs = onMs + offMs;

        // the LED appears to blink about once per second if freq is 20
        // 1000ms / 20 = 50
        int freq = totalMs / 50;
        // pwm specifies the ratio of ON versus OFF
        // pwm = 0 -> always off
        // pwm = 255 => always on
        int pwm = (onMs * 255) / totalMs;

        if (pwm > 0 && pwm < 16) {
            pwm = 16;
        }

        set(kWhiteFreqPath, freq);
        set(kWhitePwmPath, pwm);
    }
    set(kWhiteBlinkPath, blink ? 1 : 0);

    LOG(DEBUG) << base::StringPrintf(
        "handleWhiteLed: mode=%d, color=%08X, onMs=%d, offMs=%d",
        static_cast<std::underlying_type<Flash>::type>(stateToUse.flashMode), stateToUse.color,
        onMs, offMs);
}

Return<Status> Light::setLight(Type type, const LightState& state) {
    auto it = mLights.find(type);

    if (it == mLights.end()) {
        return Status::LIGHT_NOT_SUPPORTED;
    }

    /*
     * Lock global mutex until light state is updated.
     */
    std::lock_guard<std::mutex> lock(mLock);

    it->second(state);

    return Status::SUCCESS;
}

Return<void> Light::getSupportedTypes(getSupportedTypes_cb _hidl_cb) {
    std::vector<Type> types;

    for (auto const& light : mLights) {
        types.push_back(light.first);
    }

    _hidl_cb(types);

    return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace light
}  // namespace hardware
}  // namespace android
