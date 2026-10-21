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

#ifndef ODML_LITERT_LITERT_VENDORS_VERISILICON_DISPATCH_LITERT_DISPATCH_INVOCATION_CONTEXT_H_
#define ODML_LITERT_LITERT_VENDORS_VERISILICON_DISPATCH_LITERT_DISPATCH_INVOCATION_CONTEXT_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "litert/c/litert_common.h"
#include "litert/cc/litert_expected.h"
#include "litert/vendors/c/litert_dispatch.h"
#include "litert/vendors/verisilicon/dispatch/litert_dispatch_device_context.h"
#include "litert/vendors/verisilicon/dispatch/viplite_adapter_api.h"

namespace litert::verisilicon {

/// @brief RAII wrapper around one VIPLite network created from embedded NBG.
class VipliteNetworkT {
public:
  VipliteNetworkT(const VipliteAdapterApi &viplite_adapter_api,
                  const void *bytecode, size_t bytecode_size);
  ~VipliteNetworkT();

  static Expected<std::unique_ptr<VipliteNetworkT>> CreateFromByteCode(
      const VipliteAdapterApi &viplite_adapter_api, const void *bytecode,
      size_t bytecode_size, int expected_num_inputs, int expected_num_outputs,
      const LiteRtDispatchDeviceContextT::VpmNetworkParam &network_params);

  size_t InputCount() const { return input_count_; }
  size_t OutputCount() const { return output_count_; }

  Expected<void> SetInput(size_t index, vip_buffer buffer);
  Expected<void> SetOutput(size_t index, vip_buffer buffer);
  Expected<void> Run();

private:
  Expected<void>
  Setup(const LiteRtDispatchDeviceContextT::VpmNetworkParam &network_params);
  Expected<void> Query(vip_enum property, void *value);
  Expected<void> Set(vip_enum property, void *value);

  const VipliteAdapterApi &viplite_adapter_api_;
  const void *bytecode_;
  size_t bytecode_size_;
  vip_network network_ = nullptr;
  uint32_t device_index_ = 0;
  uint32_t core_index_ = 0;
  size_t input_count_ = 0;
  size_t output_count_ = 0;
};

} // namespace litert::verisilicon

class LiteRtDispatchInvocationContextT {
public:
  using Ptr = std::unique_ptr<LiteRtDispatchInvocationContextT>;

  static litert::Expected<Ptr>
  Create(const LiteRtRuntimeContext *runtime_context,
         litert::verisilicon::VipliteAdapterApi &viplite_adapter_api,
         LiteRtDispatchDeviceContext device_context,
         LiteRtDispatchExecutableType exec_type,
         const LiteRtMemBuffer *exec_bytecode_buffer, const char *function_name,
         int num_inputs, int num_outputs);

  litert::Expected<LiteRtTensorBufferRequirements>
  GetInputRequirements(const LiteRtRankedTensorType &tensor_type) const;
  litert::Expected<LiteRtTensorBufferRequirements>
  GetOutputRequirements(const LiteRtRankedTensorType &tensor_type) const;

  litert::Expected<void>
  AttachInput(int graph_input_index,
              LiteRtTensorBufferHandle tensor_buffer_handle);
  litert::Expected<void>
  AttachOutput(int graph_output_index,
               LiteRtTensorBufferHandle tensor_buffer_handle);
  litert::Expected<void>
  DetachInput(int graph_input_index,
              LiteRtTensorBufferHandle tensor_buffer_handle);
  litert::Expected<void>
  DetachOutput(int graph_output_index,
               LiteRtTensorBufferHandle tensor_buffer_handle);
  litert::Expected<void> Invoke();

  void SetSchedulingInfo(const LiteRtSchedulingInfo *scheduling_info) {
    scheduling_info_ = scheduling_info == nullptr
                           ? std::nullopt
                           : std::make_optional(*scheduling_info);
  }

private:
  LiteRtDispatchInvocationContextT(
      const LiteRtRuntimeContext *runtime_context,
      const litert::verisilicon::VipliteAdapterApi &viplite_adapter_api,
      LiteRtDispatchDeviceContext device_context,
      std::unique_ptr<litert::verisilicon::VipliteNetworkT> model,
      int num_inputs, int num_outputs);

  litert::Expected<LiteRtDispatchDeviceContextT::VipliteMemoryInfo>
  GetMemoryInfo(int index, LiteRtTensorBufferHandle tensor_buffer_handle) const;

  const LiteRtRuntimeContext *runtime_context_;
  const litert::verisilicon::VipliteAdapterApi &viplite_adapter_api_;
  LiteRtDispatchDeviceContext device_context_;
  std::unique_ptr<litert::verisilicon::VipliteNetworkT> model_;
  std::vector<LiteRtTensorBufferHandle> input_buffer_handles_;
  std::vector<LiteRtTensorBufferHandle> output_buffer_handles_;
  std::optional<LiteRtSchedulingInfo> scheduling_info_;
};

#endif // ODML_LITERT_LITERT_VENDORS_VERISILICON_DISPATCH_LITERT_DISPATCH_INVOCATION_CONTEXT_H_
