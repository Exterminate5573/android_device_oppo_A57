# Copyright (C) 2021 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#
# This Makefile appends the public key extracted from stock ROM,
# which is required to boot custom images on this device, since
# OPPO accidentally shipped an engineering bootloader that allowed
# for custom images on an earlier version.
#

BOOT_PATCH := $(DEVICE_PATH)/boot

$(INSTALLED_BOOTIMAGE_TARGET): $(MKBOOTIMG) $(INTERNAL_BOOTIMAGE_FILES) $(INSTALLED_KERNEL_TARGET)
	$(call pretty, "Target boot image: $@")
	$(hide) $(MKBOOTIMG) --kernel $(call bootimage-to-kernel,$@) $(INTERNAL_BOOTIMAGE_ARGS) $(INTERNAL_MKBOOTIMG_VERSION_ARGS) $(BOARD_MKBOOTIMG_ARGS) --output $@
	$(hide) cat $(BOOT_PATCH)/boot.patch >> $@
	$(hide) $(call assert-max-image-size, $@, $(call get-bootimage-partition-size,$@,boot))
	@echo "Made boot image: $@"

$(INSTALLED_RECOVERYIMAGE_TARGET): $(recoveryimage-deps) $(RECOVERYIMAGE_EXTRA_DEPS) $(INSTALLED_BOOTIMAGE_TARGET)
	$(call build-recoveryimage-target, $@, $(recovery_kernel))
	$(hide) cat $(BOOT_PATCH)/recovery.patch >> $@
	@echo "Made recovery image: $@"
