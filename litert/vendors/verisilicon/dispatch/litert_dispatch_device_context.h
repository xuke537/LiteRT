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

#ifndef ODML_LITERT_LITERT_VENDORS_VERISILICON_DISPATCH_LITERT_DISPATCH_DEVICE_CONTEXT_H_
#define ODML_LITERT_LITERT_VENDORS_VERISILICON_DISPATCH_LITERT_DISPATCH_DEVICE_CONTEXT_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "litert/c/litert_common.h"
#include "litert/c/litert_options.h"
#include "litert/c/litert_tensor_buffer_types.h"
#include "litert/cc/litert_expected.h"
#include "litert/vendors/c/litert_dispatch.h"
#include "litert/vendors/verisilicon/dispatch/viplite_adapter_api.h"

class LiteRtDispatchDeviceContextT {
public:
  using Ptr = std::unique_ptr<LiteRtDispatchDeviceContextT>;

  struct VipliteMemoryInfo {
    vip_buffer buffer = nullptr;
    size_t size = 0;
    size_t offset = 0;
    void *host_addr = nullptr;
    vip_buffer_create_type_e create_type = VIP_BUFFER_CREATE_ALLOC_MEM;
  };

  struct VpmNetworkParam {
    unsigned int time_out = 0;
    unsigned int device_index = 0;
    unsigned int core_index = 0;
    unsigned int cnnprofile_level = 0;
    bool dump_nbg = false;
  };

#if LITERT_HAS_DMABUF_SUPPORT
  static constexpr std::array<LiteRtTensorBufferType, 2>
      kSupportedTensorBufferTypes = {
          kLiteRtTensorBufferTypeHostMemory,
          kLiteRtTensorBufferTypeDmaBuf,
  };
#else
  static constexpr std::array<LiteRtTensorBufferType, 1>
      kSupportedTensorBufferTypes = {
          kLiteRtTensorBufferTypeHostMemory,
  };
#endif // LITERT_HAS_DMABUF_SUPPORT

  static litert::Expected<Ptr>
  Create(const LiteRtRuntimeContext *runtime_context,
         const litert::verisilicon::VipliteAdapterApi &viplite_adapter_api,
         LiteRtOptions options);

  ~LiteRtDispatchDeviceContextT();

  const LiteRtRuntimeContext *runtime_context() const {
    return runtime_context_;
  }

  litert::Expected<VpmNetworkParam> GetVpmNetworkParam() const {
    return vpm_param_;
  }

  litert::Expected<LiteRtTensorBufferHandle>
  RegisterTensorBuffer(LiteRtTensorBuffer tensor_buffer);

  litert::Expected<void>
  UnregisterTensorBuffer(LiteRtTensorBufferHandle tensor_buffer_handle) {
    return viplite_memory_registry_.Unregister(tensor_buffer_handle);
  }

  litert::Expected<VipliteMemoryInfo>
  GetVipliteMemoryInfo(LiteRtTensorBufferHandle tensor_buffer_handle) {
    return viplite_memory_registry_.Find(tensor_buffer_handle);
  }

private:
  class VipliteMemoryRegistry {
  public:
    explicit VipliteMemoryRegistry(
        const litert::verisilicon::VipliteAdapterApi &viplite_adapter_api)
        : viplite_adapter_api_(viplite_adapter_api) {}
    ~VipliteMemoryRegistry();

    LiteRtTensorBufferHandle Register(const VipliteMemoryInfo &memory_info);
    litert::Expected<void>
    Unregister(LiteRtTensorBufferHandle tensor_buffer_handle);
    litert::Expected<VipliteMemoryInfo>
    Find(LiteRtTensorBufferHandle tensor_buffer_handle) const;

  private:
    const litert::verisilicon::VipliteAdapterApi &viplite_adapter_api_;
    std::vector<VipliteMemoryInfo> records_;
  };

  LiteRtDispatchDeviceContextT(
      const LiteRtRuntimeContext *runtime_context,
      const litert::verisilicon::VipliteAdapterApi &viplite_adapter_api,
      LiteRtOptions options);

  litert::Expected<void> Init();

  const LiteRtRuntimeContext *runtime_context_;
  const litert::verisilicon::VipliteAdapterApi &viplite_adapter_api_;
  LiteRtOptions lrt_options_;
  VipliteMemoryRegistry viplite_memory_registry_;
  VpmNetworkParam vpm_param_;
};

#endif // ODML_LITERT_LITERT_VENDORS_VERISILICON_DISPATCH_LITERT_DISPATCH_DEVICE_CONTEXT_H_
