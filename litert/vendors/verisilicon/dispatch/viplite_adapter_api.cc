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

#include "litert/vendors/verisilicon/dispatch/viplite_adapter_api.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "absl/strings/str_cat.h" // from @com_google_absl
#include "litert/c/litert_common.h"
#include "litert/cc/litert_expected.h"
#include "litert/cc/litert_macros.h"

namespace litert::verisilicon {
namespace {

constexpr char kVipliteLibraryName[] = "libNBGlinker.so";
constexpr uint32_t kMinimumVipliteVersion = 0x00020000U;

#define LOAD_VIPLITE_SYMBOL(symbol_function, symbol_field)                     \
  do {                                                                         \
    if (auto symbol = dlib_.LookupSymbol<void *>(#symbol_function);            \
        symbol.HasValue()) {                                                   \
      api_->symbol_field =                                                     \
          reinterpret_cast<decltype(&symbol_function)>(symbol.Value());        \
    } else {                                                                   \
      LITERT_LOG(LITERT_WARNING, "Failed to load VIPLite symbol %s",           \
                 #symbol_function);                                            \
    }                                                                          \
  } while (false)

bool AllRequiredSymbolsLoaded(const VipliteAdapterApi::Api &api) {
  return api.get_version != nullptr && api.init != nullptr &&
         api.deinit != nullptr && api.query_hardware != nullptr &&
         api.create_buffer != nullptr && api.destroy_buffer != nullptr &&
         api.map_buffer != nullptr && api.unmap_buffer != nullptr &&
         api.get_buffer_size != nullptr && api.flush_buffer != nullptr &&
         api.create_network != nullptr && api.destroy_network != nullptr &&
         api.prepare_network != nullptr && api.finish_network != nullptr &&
         api.set_network != nullptr && api.query_network != nullptr &&
         api.run_network != nullptr && api.trigger_network != nullptr &&
         api.wait_network != nullptr && api.cancel_network != nullptr &&
         api.query_input != nullptr && api.query_output != nullptr &&
         api.set_input != nullptr && api.set_output != nullptr &&
         api.create_group != nullptr && api.destroy_group != nullptr &&
         api.set_group != nullptr && api.query_group != nullptr &&
         api.add_network != nullptr && api.run_group != nullptr &&
         api.trigger_group != nullptr && api.wait_group != nullptr &&
         api.power_management != nullptr;
}

} // namespace

VipliteAdapterApi::VipliteAdapterApi() : api_(new Api) {}

Expected<VipliteAdapterApi::Ptr>
VipliteAdapterApi::Create(std::optional<std::string> shared_library_dir) {
  auto adapter = std::unique_ptr<VipliteAdapterApi>(new VipliteAdapterApi());
  LITERT_RETURN_IF_ERROR(adapter->LoadSymbols(shared_library_dir));

  adapter->runtime_version_ = adapter->api_->get_version();
  if (adapter->runtime_version_ < kMinimumVipliteVersion) {
    return Unexpected(kLiteRtStatusErrorRuntimeFailure,
                      absl::StrCat("Unsupported VIPLite version ",
                                   (adapter->runtime_version_ >> 16) & 0xFFU,
                                   ".",
                                   (adapter->runtime_version_ >> 8) & 0xFFU,
                                   ".", adapter->runtime_version_ & 0xFFU,
                                   "; version 2.0.0 or newer is required"));
  }

  LITERT_LOG(LITERT_INFO, "Loaded VIPLite version %u.%u.%u",
             (adapter->runtime_version_ >> 16) & 0xFFU,
             (adapter->runtime_version_ >> 8) & 0xFFU,
             adapter->runtime_version_ & 0xFFU);
  return adapter;
}

Expected<void> VipliteAdapterApi::LoadSymbols(
    const std::optional<std::string> &shared_library_dir) {
  const std::string library_path =
      shared_library_dir.has_value()
          ? absl::StrCat(*shared_library_dir, "/", kVipliteLibraryName)
          : kVipliteLibraryName;

  auto library = SharedLibrary::Load(library_path, RtldFlags::Default());
  if (!library.HasValue()) {
    LITERT_LOG(LITERT_ERROR, "Failed to load VIPLite runtime from %s: %s",
               library_path.c_str(),
               std::string(library.Error().Message()).c_str());
    return Unexpected(library.Error().Status(), library.Error().Message());
  }
  dlib_ = std::move(library.Value());
  LITERT_LOG(LITERT_INFO, "Loaded Verisilicon VIPLite runtime from %s",
             library_path.c_str());

  LOAD_VIPLITE_SYMBOL(vip_get_version, get_version);
  LOAD_VIPLITE_SYMBOL(vip_init, init);
  LOAD_VIPLITE_SYMBOL(vip_destroy, deinit);
  LOAD_VIPLITE_SYMBOL(vip_query_hardware, query_hardware);
  LOAD_VIPLITE_SYMBOL(vip_create_buffer, create_buffer);
  LOAD_VIPLITE_SYMBOL(vip_destroy_buffer, destroy_buffer);
  LOAD_VIPLITE_SYMBOL(vip_map_buffer, map_buffer);
  LOAD_VIPLITE_SYMBOL(vip_unmap_buffer, unmap_buffer);
  LOAD_VIPLITE_SYMBOL(vip_get_buffer_size, get_buffer_size);
  LOAD_VIPLITE_SYMBOL(vip_flush_buffer, flush_buffer);
  LOAD_VIPLITE_SYMBOL(vip_create_network, create_network);
  LOAD_VIPLITE_SYMBOL(vip_destroy_network, destroy_network);
  LOAD_VIPLITE_SYMBOL(vip_prepare_network, prepare_network);
  LOAD_VIPLITE_SYMBOL(vip_finish_network, finish_network);
  LOAD_VIPLITE_SYMBOL(vip_set_network, set_network);
  LOAD_VIPLITE_SYMBOL(vip_query_network, query_network);
  LOAD_VIPLITE_SYMBOL(vip_run_network, run_network);
  LOAD_VIPLITE_SYMBOL(vip_trigger_network, trigger_network);
  LOAD_VIPLITE_SYMBOL(vip_wait_network, wait_network);
  LOAD_VIPLITE_SYMBOL(vip_cancel_network, cancel_network);
  LOAD_VIPLITE_SYMBOL(vip_query_input, query_input);
  LOAD_VIPLITE_SYMBOL(vip_query_output, query_output);
  LOAD_VIPLITE_SYMBOL(vip_set_input, set_input);
  LOAD_VIPLITE_SYMBOL(vip_set_output, set_output);
  LOAD_VIPLITE_SYMBOL(vip_create_group, create_group);
  LOAD_VIPLITE_SYMBOL(vip_destroy_group, destroy_group);
  LOAD_VIPLITE_SYMBOL(vip_set_group, set_group);
  LOAD_VIPLITE_SYMBOL(vip_query_group, query_group);
  LOAD_VIPLITE_SYMBOL(vip_add_network, add_network);
  LOAD_VIPLITE_SYMBOL(vip_run_group, run_group);
  LOAD_VIPLITE_SYMBOL(vip_trigger_group, trigger_group);
  LOAD_VIPLITE_SYMBOL(vip_wait_group, wait_group);
  LOAD_VIPLITE_SYMBOL(vip_power_management, power_management);

  if (!AllRequiredSymbolsLoaded(*api_)) {
    return Unexpected(
        kLiteRtStatusErrorDynamicLoading,
        "The VIPLite runtime is missing one or more required symbols");
  }
  return {};
}

} // namespace litert::verisilicon
