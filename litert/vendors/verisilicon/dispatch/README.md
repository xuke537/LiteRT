# Verisilicon dispatch plug-in

This directory implements the LiteRT dispatch plug-in for Verisilicon NPUs. The
plug-in dynamically loads `libNBGlinker.so` from a VIPLite SDK, creates a network
from an NBG payload embedded in an Acuity-generated LiteRT model, and executes
that network on the selected Verisilicon device and VIP core.

## Model generation

Generate a LiteRT model with Acuity toolkit using an NBG target
compatible with the deployment device. The generated `.tflite` file must contain
a LiteRT `DISPATCH_OP` with embedded Verisilicon NPU NBG bytecode. No separate
Verisilicon NPU compiler plug-in is needed at runtime.

### Compile model
You can use Acuity toolkit to compile a model to LiteRT compiled model.

from acuitylib.vsi_nn import VSInn

nn = VSInn()
net = nn.load_onnx('xxx.onnx')
nn.export_tflite(net, output_path='xxx', gen_litert=True, optimize='VIP9000NANOSI_PID0X1000005D', pack_nbg=True,
                  viv_sdk="your vsi_sdk path")
Note:

Your vsi_sdk must follow the same directory structure as "acuity/acuitynet/acuitylib/vsi_sdk/prebuilt-sdk/x86_64_linux/*".
The folder structure should be /your_path/vsi_sdk/prebuilt-sdk/x86_64_linux, meaning the root of vsi_sdk is /your_path/vsi_sdk.
Then set the export parameter : viv_sdk="/your_path/vsi_sdk".
gen_litert=True & pack_nbg=True must be set.
optimize='VIP9000NANOSI_PID0X1000005D', please replace 'VIP9000NANOSI_PID0X1000005D' with your own NPU_TARGET_NAME

## Runtime options

- `device_index`: Verisilicon NPU device index for multi-device systems.
- `core_index`: VIP-core index for multi-core devices(eg.VIP9400).
- `time_out`: VIPLite network timeout in milliseconds; zero uses the SDK default.
- `profile_level`: `0` disabled, `1` step-by-step execution, `2` verbose logs.
- `dump_nbg`: dump the NBG, inputs, and outputs.

The equivalent `run_model` flags are `--verisilicon_device_index`,
`--verisilicon_core_index`, `--verisilicon_time_out`,
`--verisilicon_profile_level`, and `--verisilicon_dump_nbg`.

## Bazel build

From the repository root:

```sh
bazel build //litert/vendors/verisilicon/dispatch:dispatch_api_so
bazel build //litert/tools:run_model
```

The output plug-in is named `libLiteRtDispatch_Verisilicon.so`.

## CMake build

For a host build that enables only Verisilicon dispatch, run from `litert/`:

```sh
cmake --preset default -DLITERT_ENABLE_ALL_VENDORS=OFF -DLITERT_ENABLE_VERISILICON=ON
cmake --build cmake_build --target dispatch_api_Verisilicon_so run_model -j8
```

The plug-in is written to `cmake_build/vendors/`. The vendored declarations in
this directory match the supplied VIPLite SDK; the implementation still
loads the SDK runtime dynamically and does not link a vendor library at build
time.

## Run an Acuity-generated model

Place `libLiteRtDispatch_Verisilicon.so` in the VIPLite SDK `drivers` directory
(or another directory that also contains `libNBGlinker.so` and its dependent
Verisilicon NPU driver libraries), then run:

```sh
export VIPLITE_SDK=/path/to/viplite_sdk/
cp /path_to/libLiteRtDispatch_Verisilicon.so $VIPLITE_SDK/drivers
export LD_LIBRARY_PATH="$VIPLITE_SDK/drivers:$LD_LIBRARY_PATH"

litert/cmake_build/tools/run_model \
  --graph=/path/to/acuity_generated.tflite \
  --accelerator=npu \
  --dispatch_library_dir="$VIPLITE_SDK/drivers"
```

For deterministic input data, pass `--input_dir` with `.raw` files named after
the model signature inputs. Outputs are written by `--output_dir` using the
model signature output names.
