// Copyright 2026 Google LLC.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ODML_LITERT_LITERT_CC_OPTIONS_LITERT_VERISILICON_OPTIONS_H_
#define ODML_LITERT_LITERT_CC_OPTIONS_LITERT_VERISILICON_OPTIONS_H_

#include <memory>

#include "litert/c/litert_common.h"
#include "litert/c/options/litert_verisilicon_options.h"
#include "litert/cc/litert_expected.h"
#include "litert/cc/litert_macros.h"
#include "litert/cc/options/litert_concrete_options_base.h"

namespace litert::verisilicon {

/// @brief C++ wrapper for Verisilicon NPU dispatch options.
class VerisiliconOptions : public ConcreteOptionsBase {
public:
  VerisiliconOptions() = delete;
  explicit VerisiliconOptions(LrtVerisiliconOptions options)
      : options_(options, LrtDestroyVerisiliconOptions) {}

  VerisiliconOptions(const VerisiliconOptions &) = delete;
  VerisiliconOptions &operator=(const VerisiliconOptions &) = delete;
  VerisiliconOptions(VerisiliconOptions &&) = default;
  VerisiliconOptions &operator=(VerisiliconOptions &&) = default;
  ~VerisiliconOptions() override = default;

  static Expected<VerisiliconOptions> Create() {
    LrtVerisiliconOptions options = nullptr;
    LITERT_RETURN_IF_ERROR(LrtCreateVerisiliconOptions(&options));
    return VerisiliconOptions(options);
  }

  static const char *Discriminator() {
    return LrtVerisiliconOptionsGetIdentifier();
  }

  LrtVerisiliconOptions Get() const { return options_.get(); }
  LrtVerisiliconOptions Release() { return options_.release(); }

  LiteRtStatus
  GetOpaqueOptionsData(const char **identifier, void **payload,
                       void (**payload_deleter)(void *)) const override {
    return LrtGetOpaqueVerisiliconOptionsData(Get(), identifier, payload,
                                              payload_deleter);
  }

  /// @brief Index of the Verisilicon device to use. Defaults to zero.
  Expected<void> SetDeviceIndex(unsigned int device_index) {
    LITERT_RETURN_IF_ERROR(
        LrtVerisiliconOptionsSetDeviceIndex(Get(), device_index));
    return {};
  }

  Expected<unsigned int> GetDeviceIndex() const {
    unsigned int device_index = 0;
    LITERT_RETURN_IF_ERROR(
        LrtVerisiliconOptionsGetDeviceIndex(Get(), &device_index));
    return device_index;
  }

  /// @brief Index of the VIP core to use. Defaults to zero.
  Expected<void> SetCoreIndex(unsigned int core_index) {
    LITERT_RETURN_IF_ERROR(
        LrtVerisiliconOptionsSetCoreIndex(Get(), core_index));
    return {};
  }

  Expected<unsigned int> GetCoreIndex() const {
    unsigned int core_index = 0;
    LITERT_RETURN_IF_ERROR(
        LrtVerisiliconOptionsGetCoreIndex(Get(), &core_index));
    return core_index;
  }

  /// @brief Network timeout in milliseconds. Zero lets VIPLite choose its
  /// default timeout.
  Expected<void> SetTimeOut(unsigned int time_out) {
    LITERT_RETURN_IF_ERROR(LrtVerisiliconOptionsSetTimeOut(Get(), time_out));
    return {};
  }

  Expected<unsigned int> GetTimeOut() const {
    unsigned int time_out = 0;
    LITERT_RETURN_IF_ERROR(LrtVerisiliconOptionsGetTimeOut(Get(), &time_out));
    return time_out;
  }

  /// @brief Zero disables profiling. One executes commands individually and
  /// two additionally emits more detailed profiling logs.
  Expected<void> SetProfileLevel(unsigned int profile_level) {
    LITERT_RETURN_IF_ERROR(
        LrtVerisiliconOptionsSetProfileLevel(Get(), profile_level));
    return {};
  }

  Expected<unsigned int> GetProfileLevel() const {
    unsigned int profile_level = 0;
    LITERT_RETURN_IF_ERROR(
        LrtVerisiliconOptionsGetProfileLevel(Get(), &profile_level));
    return profile_level;
  }

  /// @brief Enables dumping the NBG, inputs, and outputs for debugging.
  Expected<void> SetDumpNbg(bool enable) {
    LITERT_RETURN_IF_ERROR(LrtVerisiliconOptionsSetDumpNBG(Get(), enable));
    return {};
  }

  Expected<bool> GetDumpNbg() const {
    bool enable = false;
    LITERT_RETURN_IF_ERROR(LrtVerisiliconOptionsGetDumpNBG(Get(), &enable));
    return enable;
  }

private:
  std::unique_ptr<LrtVerisiliconOptionsT, void (*)(LrtVerisiliconOptions)>
      options_;
};

} // namespace litert::verisilicon

#endif // ODML_LITERT_LITERT_CC_OPTIONS_LITERT_VERISILICON_OPTIONS_H_
