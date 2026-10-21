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

#include "litert/vendors/verisilicon/dispatch/litert_dispatch_device_context.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/str_format.h" // from @com_google_absl
#include "litert/c/internal/litert_runtime_context.h"
#include "litert/c/options/litert_verisilicon_options.h"
#include "litert/cc/internal/litert_context_wrapper.h"
#include "litert/cc/internal/litert_options_wrapper.h"
#include "litert/cc/litert_expected.h"
#include "litert/cc/litert_macros.h"

namespace {

using litert::Error;
using litert::Expected;
using litert::Unexpected;

void *AddOffset(void *address, size_t offset) {
  return static_cast<uint8_t *>(address) + offset;
}

} // namespace

LiteRtDispatchDeviceContextT::LiteRtDispatchDeviceContextT(
    const LiteRtRuntimeContext *runtime_context,
    const litert::verisilicon::VipliteAdapterApi &viplite_adapter_api,
    LiteRtOptions options)
    : runtime_context_(runtime_context),
      viplite_adapter_api_(viplite_adapter_api),
      viplite_memory_registry_(viplite_adapter_api), lrt_options_(options) {}

LiteRtDispatchDeviceContextT::~LiteRtDispatchDeviceContextT() = default;

Expected<LiteRtDispatchDeviceContextT::Ptr>
LiteRtDispatchDeviceContextT::Create(
    const LiteRtRuntimeContext *runtime_context,
    const litert::verisilicon::VipliteAdapterApi &viplite_adapter_api,
    LiteRtOptions options) {
  if (runtime_context == nullptr) {
    return Unexpected(kLiteRtStatusErrorInvalidArgument,
                      "Runtime context is null");
  }
  auto context = Ptr(new LiteRtDispatchDeviceContextT(
      runtime_context, viplite_adapter_api, options));
  LITERT_RETURN_IF_ERROR(context->Init());
  return context;
}

Expected<void> LiteRtDispatchDeviceContextT::Init() {
  LrtVerisiliconOptions verisilicon_options = nullptr;
  if (lrt_options_ != nullptr) {
    litert::internal::OptionsWrapper options_wrapper(
        litert::internal::ContextWrapper(runtime_context_), lrt_options_);
    auto payload = options_wrapper.FindOpaqueOptionsData(
        LrtVerisiliconOptionsGetIdentifier());
    if (payload.HasValue()) {
      LITERT_RETURN_IF_ERROR(LrtCreateVerisiliconOptionsFromToml(
          static_cast<const char *>(payload.Value()), &verisilicon_options));
    }
  }

  if (verisilicon_options != nullptr) {
    using VerisiliconOptionsGuard =
        std::unique_ptr<LrtVerisiliconOptionsT,
                        void (*)(LrtVerisiliconOptions)>;
    VerisiliconOptionsGuard options_guard(verisilicon_options,
                                          LrtDestroyVerisiliconOptions);
    using Getter = LiteRtStatus (*)(LrtVerisiliconOptions, unsigned int *);
    auto read_unsigned = [&](Getter getter,
                             unsigned int &value) -> Expected<void> {
      LITERT_RETURN_IF_ERROR(getter(options_guard.get(), &value));
      return {};
    };
    LITERT_RETURN_IF_ERROR(read_unsigned(LrtVerisiliconOptionsGetDeviceIndex,
                                         vpm_param_.device_index));
    LITERT_RETURN_IF_ERROR(read_unsigned(LrtVerisiliconOptionsGetCoreIndex,
                                         vpm_param_.core_index));
    LITERT_RETURN_IF_ERROR(
        read_unsigned(LrtVerisiliconOptionsGetTimeOut, vpm_param_.time_out));
    LITERT_RETURN_IF_ERROR(read_unsigned(LrtVerisiliconOptionsGetProfileLevel,
                                         vpm_param_.cnnprofile_level));
    LITERT_RETURN_IF_ERROR(LrtVerisiliconOptionsGetDumpNBG(
        options_guard.get(), &vpm_param_.dump_nbg));
  }

  if (viplite_adapter_api_.api().init() != VIP_SUCCESS) {
    return Unexpected(kLiteRtStatusErrorRuntimeFailure,
                      "Failed to initialize the VIPLite runtime");
  }

  uint32_t device_count = 0;
  if (viplite_adapter_api_.api().query_hardware(VIP_QUERY_HW_PROP_DEVICE_COUNT,
                                                sizeof(device_count),
                                                &device_count) != VIP_SUCCESS ||
      device_count == 0) {
    return Unexpected(kLiteRtStatusErrorRuntimeFailure,
                      "No Verisilicon NPU device is available");
  }

  if (vpm_param_.device_index >= device_count) {
    LITERT_LOG(LITERT_WARNING,
               "Requested Verisilicon device %u, but only %u devices exist; "
               "using device 0",
               vpm_param_.device_index, device_count);
    vpm_param_.device_index = 0;
  }

  std::vector<uint32_t> core_counts(device_count, 0);
  if (viplite_adapter_api_.api().query_hardware(
          VIP_QUERY_HW_PROP_CORE_COUNT_EACH_DEVICE,
          sizeof(uint32_t) * core_counts.size(),
          core_counts.data()) != VIP_SUCCESS) {
    return Unexpected(kLiteRtStatusErrorRuntimeFailure,
                      "Failed to query Verisilicon VIP core counts");
  }

  const uint32_t core_count = core_counts[vpm_param_.device_index];
  if (vpm_param_.core_index >= core_count) {
    LITERT_LOG(LITERT_WARNING,
               "Requested Verisilicon core %u, but device %u has %u cores; "
               "using core 0",
               vpm_param_.core_index, vpm_param_.device_index, core_count);
    vpm_param_.core_index = 0;
  }

  LITERT_LOG(LITERT_INFO,
             "Using Verisilicon device %u, core %u, timeout %ums, profile "
             "level %u",
             vpm_param_.device_index, vpm_param_.core_index,
             vpm_param_.time_out, vpm_param_.cnnprofile_level);
  return {};
}

Expected<LiteRtTensorBufferHandle>
LiteRtDispatchDeviceContextT::RegisterTensorBuffer(
    LiteRtTensorBuffer tensor_buffer) {
  if (tensor_buffer == nullptr) {
    return Unexpected(kLiteRtStatusErrorInvalidArgument,
                      "Tensor buffer is null");
  }

  LiteRtTensorBufferType tensor_buffer_type;
  LITERT_RETURN_IF_ERROR(runtime_context_->get_tensor_buffer_type(
      tensor_buffer, &tensor_buffer_type));
  if (tensor_buffer_type != kLiteRtTensorBufferTypeHostMemory &&
      tensor_buffer_type != kLiteRtTensorBufferTypeDmaBuf) {
    return Error(kLiteRtStatusErrorUnsupported,
                 absl::StrFormat("Unsupported tensor-buffer type %d",
                                 static_cast<int>(tensor_buffer_type)));
  }

  size_t tensor_buffer_size = 0;
  LITERT_RETURN_IF_ERROR(runtime_context_->get_tensor_buffer_size(
      tensor_buffer, &tensor_buffer_size));

  size_t tensor_buffer_offset = 0;
  LITERT_RETURN_IF_ERROR(runtime_context_->get_tensor_buffer_offset(
      tensor_buffer, &tensor_buffer_offset));

  LiteRtRankedTensorType tensor_type;
  LITERT_RETURN_IF_ERROR(runtime_context_->get_tensor_buffer_tensor_type(
      tensor_buffer, &tensor_type));
  if (tensor_type.layout.has_strides) {
    return Unexpected(kLiteRtStatusErrorUnsupported,
                      "Strided tensors are not supported by Verisilicon");
  }

  vip_buffer_create_params_t params{};
  vip_buffer viplite_buffer = nullptr;

  switch (tensor_buffer_type) {
  case kLiteRtTensorBufferTypeHostMemory: {
    void *base_address = nullptr;
    LITERT_RETURN_IF_ERROR(runtime_context_->get_tensor_buffer_host_memory(
        tensor_buffer, &base_address));
    void *data_address = AddOffset(base_address, tensor_buffer_offset);
    const uintptr_t address = reinterpret_cast<uintptr_t>(data_address);
    params.device_index = vpm_param_.device_index;

    bool use_user_memory =
        (address % litert::verisilicon::kVipliteAddressAlignment) == 0 &&
        (tensor_buffer_size %
         litert::verisilicon::kVipliteCacheLineAlignment) == 0;
    if (use_user_memory) {
      params.type = VIP_BUFFER_CREATE_FROM_USER_MEM;
      params.src.from_handle.memory_type = VIP_BUFFER_FROM_USER_MEM_TYPE_HOST;
      params.src.from_handle.logical_addr = data_address;
      params.src.from_handle.size = tensor_buffer_size;
      if (viplite_adapter_api_.api().create_buffer(
              &params, sizeof(params), &viplite_buffer) != VIP_SUCCESS) {
        use_user_memory = false;
        viplite_buffer = nullptr;
      }
    }

    if (!use_user_memory) {
      params = {};
      params.device_index = vpm_param_.device_index;
      params.type = VIP_BUFFER_CREATE_ALLOC_MEM;
      params.src.alloc_mem.size =
          ((tensor_buffer_size +
            litert::verisilicon::kVipliteCacheLineAlignment - 1) /
           litert::verisilicon::kVipliteCacheLineAlignment) *
          litert::verisilicon::kVipliteCacheLineAlignment;
      params.src.alloc_mem.align =
          litert::verisilicon::kVipliteCacheLineAlignment;
      if (viplite_adapter_api_.api().create_buffer(
              &params, sizeof(params), &viplite_buffer) != VIP_SUCCESS) {
        return Unexpected(kLiteRtStatusErrorRuntimeFailure,
                          "Failed to allocate a VIPLite tensor buffer");
      }

      void *mapped_address =
          viplite_adapter_api_.api().map_buffer(viplite_buffer);
      if (mapped_address == nullptr) {
        viplite_adapter_api_.api().destroy_buffer(viplite_buffer);
        return Unexpected(kLiteRtStatusErrorRuntimeFailure,
                          "Failed to map a staged VIPLite tensor buffer");
      }
      std::memcpy(mapped_address, data_address, tensor_buffer_size);
      viplite_adapter_api_.api().unmap_buffer(viplite_buffer);
    }

    if (viplite_adapter_api_.api().flush_buffer(
            viplite_buffer, VIP_BUFFER_OPER_TYPE_FLUSH) != VIP_SUCCESS) {
      viplite_adapter_api_.api().destroy_buffer(viplite_buffer);
      return Unexpected(kLiteRtStatusErrorRuntimeFailure,
                        "Failed to flush a VIPLite tensor buffer");
    }

    VipliteMemoryInfo memory_info;
    memory_info.buffer = viplite_buffer;
    memory_info.size = tensor_buffer_size;
    memory_info.offset = tensor_buffer_offset;
    memory_info.host_addr = data_address;
    memory_info.create_type = params.type;
    return viplite_memory_registry_.Register(memory_info);
  }
  case kLiteRtTensorBufferTypeDmaBuf: {
#if LITERT_HAS_DMABUF_SUPPORT
    void *host_address = nullptr;
    int fd = -1;
    LITERT_RETURN_IF_ERROR(runtime_context_->get_tensor_buffer_dma_buf_buffer(
        tensor_buffer, &host_address, &fd));
    if (tensor_buffer_offset != 0) {
      return Unexpected(
          kLiteRtStatusErrorUnsupported,
          "Nonzero DMA-BUF offsets are not supported by the VIPLite API");
    }
    params.device_index = vpm_param_.device_index;
    params.type = VIP_BUFFER_CREATE_FROM_FD;
    params.src.from_fd.memory_type = VIP_BUFFER_FROM_FD_TYPE_DMA_BUF;
    params.src.from_fd.size = tensor_buffer_size;
    params.src.from_fd.fd_value = static_cast<uint32_t>(fd);
    if (viplite_adapter_api_.api().create_buffer(
            &params, sizeof(params), &viplite_buffer) != VIP_SUCCESS) {
      return Unexpected(kLiteRtStatusErrorRuntimeFailure,
                        "Failed to import a DMA-BUF into VIPLite");
    }

    VipliteMemoryInfo memory_info;
    memory_info.buffer = viplite_buffer;
    memory_info.size = tensor_buffer_size;
    memory_info.offset = tensor_buffer_offset;
    memory_info.host_addr = host_address;
    memory_info.create_type = params.type;
    return viplite_memory_registry_.Register(memory_info);
#else
    return Unexpected(kLiteRtStatusErrorUnsupported,
                      "DMA-BUF support is unavailable on this platform");
#endif // LITERT_HAS_DMABUF_SUPPORT
  }
  default:
    return Unexpected(kLiteRtStatusErrorUnsupported,
                      "Unsupported tensor-buffer type");
  }
}

LiteRtDispatchDeviceContextT::VipliteMemoryRegistry::~VipliteMemoryRegistry() {
  for (auto &record : records_) {
    if (record.buffer != nullptr) {
      viplite_adapter_api_.api().destroy_buffer(record.buffer);
      record.buffer = nullptr;
    }
  }
}

LiteRtTensorBufferHandle
LiteRtDispatchDeviceContextT::VipliteMemoryRegistry::Register(
    const VipliteMemoryInfo &memory_info) {
  for (size_t index = 0; index < records_.size(); ++index) {
    if (records_[index].buffer == nullptr) {
      records_[index] = memory_info;
      return static_cast<LiteRtTensorBufferHandle>(index);
    }
  }
  records_.push_back(memory_info);
  return static_cast<LiteRtTensorBufferHandle>(records_.size() - 1);
}

Expected<void> LiteRtDispatchDeviceContextT::VipliteMemoryRegistry::Unregister(
    LiteRtTensorBufferHandle tensor_buffer_handle) {
  const size_t index = static_cast<size_t>(tensor_buffer_handle);
  if (index >= records_.size() || records_[index].buffer == nullptr) {
    return Unexpected(kLiteRtStatusErrorInvalidArgument,
                      "Invalid Verisilicon tensor-buffer handle");
  }
  viplite_adapter_api_.api().destroy_buffer(records_[index].buffer);
  records_[index].buffer = nullptr;
  return {};
}

Expected<LiteRtDispatchDeviceContextT::VipliteMemoryInfo>
LiteRtDispatchDeviceContextT::VipliteMemoryRegistry::Find(
    LiteRtTensorBufferHandle tensor_buffer_handle) const {
  const size_t index = static_cast<size_t>(tensor_buffer_handle);
  if (index >= records_.size() || records_[index].buffer == nullptr) {
    return Unexpected(kLiteRtStatusErrorInvalidArgument,
                      "Invalid Verisilicon tensor-buffer handle");
  }
  return records_[index];
}
