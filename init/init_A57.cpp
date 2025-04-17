/*
 * Copyright (C) 2021 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libinit_dalvik_heap.h>
#include <libinit_variant.h>

#include "vendor_init.h"

#include <android-base/file.h>
#include <android-base/logging.h>

using android::base::ReadFileToString;

static const variant_info_t a57_info = {
    .brand = "OPPO",
    .device = "A57",
    .marketname = "OPPO A57",
    .model = "OPPO A57",
    .build_fingerprint = "OPPO/A57/A57:6.0.1/MMB29M/1527754036:user/release-keys",
    .build_description = "msm8937_64-user 6.0.1 MMB29M eng.root.20191205.095236 dev-keys",
    .build_ota = "A57_11.A.32_0320_201912050917",
};

static const variant_info_t cph1701_info = {
    .brand = "OPPO",
    .device = "CPH1701",
    .marketname = "OPPO A57",
    .model = "CPH1701",
    .build_fingerprint = "Android/msm8937_64/msm8937_64:6.0.1/MMB29M/root10091402:user/release-keys",
    .build_description = "msm8937_64-user 6.0.1 MMB29M eng.root.20181009.140111 release-keys",
    .build_ota = "CPH1701EX_11.A.36_INT_036_201810091339",
};

static const variant_info_t cph1701fw_info = {
    .brand = "OPPO",
    .device = "CPH1701fw",
    .marketname = "OPPO A57",
    .model = "CPH1701fw",
    .build_fingerprint = "Android/msm8937_64/msm8937_64:6.0.1/MMB29M/root10091402:user/release-keys",
    .build_description = "msm8937_64-user 6.0.1 MMB29M eng.root.20181009.140111 release-keys",
    .build_ota = "CPH1701EX_11.A.36_INT_036_201810091339",
};

static void determine_device() {
    std::string kPrjVersion, kOperatorName;
    int prjVersion, operatorName = 0;

    // Determine project version
    ReadFileToString("/proc/oppoVersion/prjVersion", &kPrjVersion);
    prjVersion = std::stoi(kPrjVersion);

    // Determine operator name
    ReadFileToString("/proc/oppoVersion/operatorName", &kOperatorName);
    operatorName = std::stoi(kOperatorName);

    // Match operator name
    if (prjVersion == 16061) {
        switch (operatorName) {
            /* CN V1 */
            case 8:
            /* CN V2 */
            case 10:
            case 11:
            /* CN V3 */
            case 3:
            case 5:
                set_variant_props(a57_info);
                break;

            /* Global */
            case 102:
            case 106:
            case 110:
            case 111:
            case 112:
            case 113:
            case 114:
            {
                std::string reserve_exp1;
                if (ReadFileToString("/dev/block/bootdevice/by-name/reserve_exp1", &reserve_exp1)) {
                    if (!strncmp(reserve_exp1.c_str(), "00010001", 8)) {
                        set_variant_props(cph1701fw_info);
                        return;
                    }
                }
                set_variant_props(cph1701_info);
                break;
            }
            default:
                LOG(ERROR) << "Unknown operator variant";
                break;
        }
    } else {
        LOG(ERROR) << "Unknown device variant";
    }
}

void vendor_load_properties() {
    determine_device();
    set_dalvik_heap();
}
