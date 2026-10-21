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

#include "litert/tools/flags/vendors/verisilicon_flags.h"

#include "absl/flags/flag.h" // from @com_google_absl
#include "litert/cc/litert_expected.h"
#include "litert/cc/litert_macros.h"
#include "litert/cc/litert_options.h"
#include "litert/cc/options/litert_verisilicon_options.h"
#include "litert/tools/flags/options_parser_registry.h"

ABSL_FLAG(unsigned int, verisilicon_device_index, 0,
          "Index of the Verisilicon NPU device to use.");

ABSL_FLAG(unsigned int, verisilicon_core_index, 0,
          "Index of the Verisilicon VIP core to use.");

ABSL_FLAG(unsigned int, verisilicon_time_out, 0,
          "Verisilicon network timeout in milliseconds; zero uses the SDK "
          "default.");

ABSL_FLAG(unsigned int, verisilicon_profile_level, 0,
          "Verisilicon profiling level: 0=off, 1=step-by-step, 2=verbose.");

ABSL_FLAG(bool, verisilicon_dump_nbg, false,
          "Dump the Verisilicon NBG, model inputs, and model outputs.");

namespace litert::verisilicon {

Expected<void> UpdateVerisiliconOptionsFromFlags(VerisiliconOptions &options) {
  LITERT_RETURN_IF_ERROR(
      options.SetDeviceIndex(absl::GetFlag(FLAGS_verisilicon_device_index)));
  LITERT_RETURN_IF_ERROR(
      options.SetCoreIndex(absl::GetFlag(FLAGS_verisilicon_core_index)));
  LITERT_RETURN_IF_ERROR(
      options.SetTimeOut(absl::GetFlag(FLAGS_verisilicon_time_out)));
  LITERT_RETURN_IF_ERROR(
      options.SetProfileLevel(absl::GetFlag(FLAGS_verisilicon_profile_level)));
  LITERT_RETURN_IF_ERROR(
      options.SetDumpNbg(absl::GetFlag(FLAGS_verisilicon_dump_nbg)));
  return {};
}

LITERT_REGISTER_OPTIONS_PARSER([](Options &options) -> Expected<void> {
  LITERT_ASSIGN_OR_RETURN(auto &verisilicon_options,
                          options.GetOptions<VerisiliconOptions>());
  return UpdateVerisiliconOptionsFromFlags(verisilicon_options);
});

} // namespace litert::verisilicon
