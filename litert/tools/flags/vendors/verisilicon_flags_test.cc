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

#include <gtest/gtest.h>

#include "absl/flags/flag.h"        // from @com_google_absl
#include "absl/flags/marshalling.h" // from @com_google_absl
#include "litert/cc/litert_expected.h"
#include "litert/cc/options/litert_verisilicon_options.h"

namespace litert::verisilicon {
namespace {

TEST(UpdateVerisiliconOptionsFromFlagsTest, AppliesAllValues) {
  absl::SetFlag(&FLAGS_verisilicon_device_index, 2U);
  absl::SetFlag(&FLAGS_verisilicon_core_index, 1U);
  absl::SetFlag(&FLAGS_verisilicon_time_out, 1000U);
  absl::SetFlag(&FLAGS_verisilicon_profile_level, 2U);
  absl::SetFlag(&FLAGS_verisilicon_dump_nbg, true);

  auto options_result = VerisiliconOptions::Create();
  ASSERT_TRUE(options_result.HasValue());
  auto update_result =
      UpdateVerisiliconOptionsFromFlags(options_result.Value());
  ASSERT_TRUE(update_result.HasValue());

  auto device_index = options_result.Value().GetDeviceIndex();
  auto core_index = options_result.Value().GetCoreIndex();
  auto time_out = options_result.Value().GetTimeOut();
  auto profile_level = options_result.Value().GetProfileLevel();
  auto dump_nbg = options_result.Value().GetDumpNbg();
  ASSERT_TRUE(device_index.HasValue());
  ASSERT_TRUE(core_index.HasValue());
  ASSERT_TRUE(time_out.HasValue());
  ASSERT_TRUE(profile_level.HasValue());
  ASSERT_TRUE(dump_nbg.HasValue());
  EXPECT_EQ(device_index.Value(), 2U);
  EXPECT_EQ(core_index.Value(), 1U);
  EXPECT_EQ(time_out.Value(), 1000U);
  EXPECT_EQ(profile_level.Value(), 2U);
  EXPECT_TRUE(dump_nbg.Value());
}

} // namespace
} // namespace litert::verisilicon
