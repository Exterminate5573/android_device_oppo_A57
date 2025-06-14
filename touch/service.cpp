/*
 * Copyright (C) 2019 The LineageOS Project
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

#define LOG_TAG "vendor.lineage.touch@1.0-service.A57"

#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>

#include "KeyDisabler.h"
#include "KeySwapper.h"
#include "TouchscreenGesture.h"

using ::android::OK;
using ::android::sp;

using ::vendor::lineage::touch::V1_0::IKeyDisabler;
using ::vendor::lineage::touch::V1_0::IKeySwapper;
using ::vendor::lineage::touch::V1_0::ITouchscreenGesture;
using ::vendor::lineage::touch::V1_0::implementation::KeyDisabler;
using ::vendor::lineage::touch::V1_0::implementation::KeySwapper;
using ::vendor::lineage::touch::V1_0::implementation::TouchscreenGesture;

int main() {
    sp<IKeyDisabler> keyDisablerService = new KeyDisabler();
    sp<IKeySwapper> keySwapperService = new KeySwapper();
    sp<ITouchscreenGesture> gestureService = new TouchscreenGesture();

    android::hardware::configureRpcThreadpool(1, true /*callerWillJoin*/);

    if (keyDisablerService->registerAsService() != OK) {
        LOG(ERROR) << "Cannot register key disabler HAL service.";
        return 1;
    }

    if (keySwapperService->registerAsService() != OK) {
        LOG(ERROR) << "Cannot register key swapper HAL service.";
        return 1;
    }

    if (gestureService->registerAsService() != OK) {
        LOG(ERROR) << "Cannot register touchscreen gesture HAL service.";
        return 1;
    }

    LOG(INFO) << "Touchscreen HAL service ready.";

    android::hardware::joinRpcThreadpool();

    LOG(ERROR) << "Touchscreen HAL service failed to join thread pool.";
    return 1;
}
