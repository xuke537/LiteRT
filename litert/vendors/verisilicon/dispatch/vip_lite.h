/****************************************************************************
*
*    Copyright 2017 - 2025 Vivante Corporation, Santa Clara, California.
*    All Rights Reserved.
*
*    Permission is hereby granted, free of charge, to any person obtaining
*    a copy of this software and associated documentation files (the
*    'Software'), to deal in the Software without restriction, including
*    without limitation the rights to use, copy, modify, merge, publish,
*    distribute, sub license, and/or sell copies of the Software, and to
*    permit persons to whom the Software is furnished to do so, subject
*    to the following conditions:
*
*    The above copyright notice and this permission notice (including the
*    next paragraph) shall be included in all copies or substantial
*    portions of the Software.
*
*    THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
*    IN NO EVENT SHALL VIVANTE AND/OR ITS SUPPLIERS BE LIABLE FOR ANY
*    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
*    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
*    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/

#ifndef _VIP_LITE_H
#define _VIP_LITE_H

#include "vip_lite_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 *\brief The VIP lite API for Convolution Neural Network application on CPU/MCU/DSP type of
    embedded environment.
 *\details This VIP lite APIs is not thread-safe if vpmdENABLE_MULTIPLE_TASK is set to 0,
           user must guarantee to call these APIs in a proper way.
           But defines vpmdENABLE_MULTIPLE_TASK 1, VIPLite can support multiple task
           (multiple thread/process).
           and it's thread-safe.
 *Memory allocation and file io functions used inside driver internal would
    depend on working enviroment.

 *\defgroup group_global   Data Type Definitions and Global APIs,
 *\                        brief Data type definition and global APIs that are used in the VIPLite
 *\defgroup group_buffer   Buffer API,
                           The API to manage input/output buffers
 *\defgroup group_network  Network API
                           The API to manage networks
 */

/* \brief An enumeration property that specifies which power management operation to execute.
  * \ingroup group_global
  * \version 1.2
 */
typedef enum _vip_power_property_e {
    VIP_POWER_PROPERTY_NONE = 0,
    /*!< \brief specify the VIP frequency */
    VIP_POWER_PROPERTY_SET_FREQUENCY = 1,
    /*!< \brief power off VIP hardware */
    VIP_POWER_PROPERTY_OFF = 2,
    /*!< \brief power on VIP hardware */
    VIP_POWER_PROPERTY_ON = 3,
    /*!< \brief stop VIP perform network */
    VIP_POWER_PROPERTY_STOP = 4,
    /*!< \brief start VIP perform network */
    VIP_POWER_PROPERTY_START = 5,
    VIP_POWER_PROPERTY_MAX
} vip_power_property_e;

/* \brief query hardware caps property
  * \ingroup group_global
 */
typedef enum _vip_query_hardware_property_e {
    /*!< \brief the customer ID of this VIP/NPU, the returned value is vip_uint32_t */
    VIP_QUERY_HW_PROP_CID = 0,
    /*!< \brief the number of deivce, the returned value is vip_uint32_t */
    VIP_QUERY_HW_PROP_DEVICE_COUNT = 1,
    /*!< \brief the number of core count for each device, the returned value is
        vip_uint32_t * device_count */
    VIP_QUERY_HW_PROP_CORE_COUNT_EACH_DEVICE = 2,
    VIP_QUERY_HW_PROP_MAX,
} vip_query_hardware_property_e;

/* \brief The list of properties of a network.
  * \ingroup group_network
  * \version 1.0
 */
typedef enum _vip_network_property_e {
    /* query network */
    /*!< \brief The number of layers in this network, the returned value is vip_uint32_t */
    VIP_NETWORK_PROP_LAYER_COUNT = 0,
    /*!< \brief The number of input in this network, the returned value is vip_uint32_t*/
    VIP_NETWORK_PROP_INPUT_COUNT = 1,
    /*!< \brief The number of output in this network, the returned value is vip_uint32_t*/
    VIP_NETWORK_PROP_OUTPUT_COUNT = 2,
    /*!< \brief The network name, the returned value is vip_char_t[64] */
    VIP_NETWORK_PROP_NETWORK_NAME = 3,

    /*!< \brief The size of memory pool, the returned value is vip_size_t*/
    VIP_NETWORK_PROP_MEMORY_POOL_SIZE = 6,
    /*!< \brief The network profling data, the returned value is vip_inference_profile_t */
    VIP_NETWORK_PROP_PROFILING = 7,
    /*!< \brief The the number of core for this network, the returned value is vip_uint8_t */
    VIP_NETWORK_PROP_CORE_COUNT = 8,
    /*!< \brief get the information of output of dumped layer. the returned value is vip_nld_output_t */
    VIP_NETWORK_PROP_GET_LAYER_DUMP_OUTPUT = 9,
    /*!< \brief Get the size of executable NBG, the returned value is vip_size_t */
    VIP_NETWORK_PROP_EXE_NBG_SIZE = 10,
    /*!< \brief Fill the executable NBG content in void* memory which specify by application,
        the returned value is NBG file in void* memory */
    VIP_NETWORK_PROP_EXE_NBG_GENERATE = 11,
    /*!< \brief The state of this network, the returned value is vip_network_state_e */
    VIP_NETWORK_PROP_STATE = 12,
    /*!< \brief The size of kinds of video memory size, the returned value is vip_video_memory_info_t */
    VIP_NETWORK_PROP_VIDEO_MEMORY_SIZE = 13,

    /* set network */
    /* set network property should be called before vip_prepare_network */
    /*!< \brief set network to enable change PPU parameters feature for this vip_network.
       the vip_set_network value param used to indicates disable or enable this feature.
       vip_uint8_t *value is 0x1, enable change ppu param for the input of network.
       vip_uint8_t *value is 0x2, enable change ppu param for the output of network.
       vip_uint8_t *value is 0x3, enable change ppu param for both inupt and output of network.
       vip_uint8_t *value is 0, disable change ppu param
       The value is vip_uint8_t */
    VIP_NETWORK_PROP_CHANGE_PPU_PARAM = 64,
    /*!< \brief set memory pool buffer for network. networks can share a memory pool buffer.
         the set value is <tt>\ref vip_buffer<tt>  */
    VIP_NETWORK_PROP_SET_MEMORY_POOL = 65,
    /*!< \brief set device index for network.
       Change the network running device which specified in creating network stage.
       The network can be submitted this vip device if the device can access the same video memory
       with creating network device. the value is vip_uint32_t */
    VIP_NETWORK_PROP_SET_DEVICE_INDEX = 66,
    /*!< \brief set priority of network. 0 ~ 255, 0 indicates the lowest priority.
        the value is vip_uint8_t */
    VIP_NETWORK_PROP_SET_PRIORITY = 67,
    /*!< \brief set time out of network. unit: ms . the value is vip_uint32_t */
    VIP_NETWORK_PROP_SET_TIME_OUT = 68,
    /*!< \brief set a memory for partial of full pre-load coeff data to this memory.
       This memory can't be freed until the network is released. the value is vip_buffer */
    VIP_NETWORK_PROP_SET_COEFF_MEMORY = 69,
    /*!< \brief set core index for network. network start with which core of device.
       the value is vip_int32_t data type */
    VIP_NETWORK_PROP_SET_CORE_INDEX = 70,
    /*!< \brief enable probe mode performance function, should be called before vip_prepare_network.
     * the value is vip_bool_e data type, set 1 to enable NPD */
    VIP_NETWORK_PROP_SET_ENABLE_NPD = 71,
    /*!< \brief enable preload coeff into vipsram. the value is vip_bool_e data type */
    VIP_NETWORK_PROP_SET_VIPSRAM_PRELOAD = 72,
    /*!< \brief set layer ids that need to be layer dumped. the value is vip_nld_layer_id_t */
    VIP_NETWORK_PROP_SET_LAYER_DUMP_ID = 73,
    /*!< \brief set video memory physical address, the value is vip_video_memory_info_t */
    VIP_NETWORK_PROP_SET_VIDEO_MEMORY = 74,
} vip_network_property_e;

/* \brief The list of properties of a network.
  * \ingroup group_network
  * \version 2.1
 */
typedef enum _vip_network_state {
    /*!< \brief None. invalid state */
    VIP_NET_NONE = 0,
    /*!< \brief Indicate the network is created */
    VIP_NET_CREATED = 1,
    /*!< \brief Indicate the network is prepared */
    VIP_NET_PREPARED = 2,
    /*!< \brief Indicate the network is inference complete */
    VIP_NET_COMPLETE = 3,
    /*!< \brief Indicate the network is running */
    VIP_NET_RUNNING = 4,
} vip_network_state_e;

/* \brief The list of properties of a group.
  * \ingroup group_network
  * \version 1.0
 */
typedef enum _vip_group_property_e {
    /* query group */
    /*!< \brief The group profling data, the returned value is vip_inference_profile_t */
    VIP_GROUP_PROP_PROFILING = 0,
    /*!< \brief Get the size of group executable NBG, the returned value is vip_size_t */
    VIP_GROUP_PROP_EXE_NBG_SIZE = 1,
    /*!< \brief Fill the group executable NBG content in void* memory which specify by application,
        the returned value is NBG file in void* memory */
    VIP_GROUP_PROP_EXE_NBG_GENERATE = 2,

    /* set group */
    /* set group property should be called before vip_add_network()
       and all network in group runs on same device */
    /*!< \brief set device index for group. networks in group can be submitted this vip device.
     * This prop should be called before vip_prepare_network */
    VIP_GROUP_PROP_SET_DEVICE_ID = 64, /* will be rejected later */
    VIP_GROUP_PROP_SET_DEVICE_INDEX = 64,
    /*!< \brief set core index for group. networks in group start with which core of current device.
     * This prop should be called before vip_prepare_network */
    VIP_GROUP_PROP_SET_CORE_INDEX = 65,
    /*!< \brief setting inference timeout value for group. unit: ms */
    VIP_GROUP_PROP_SET_TIME_OUT = 68,

} vip_group_property_e;

/* \brief The list of property of an input or output.
  * \ingroup group_buffer
  * \version 1.0
 */
typedef enum _vip_buffer_property_e {
    /*!< \brief The quantization format, the returned value is <tt>\ref
            vip_buffer_quantize_format_e </tt> */
    VIP_BUFFER_PROP_QUANT_FORMAT = 0,
    /*!< \brief The number of dimension for this input, the returned value is vip_uint32_t */
    VIP_BUFFER_PROP_NUM_OF_DIMENSION = 1,
    /*!< \brief The size of each dimension for this input,
                the returned value is vip_uint32_t * num_of_dim */
    VIP_BUFFER_PROP_SIZES_OF_DIMENSION = 2,
    /*!< \brief The data format for this input,
                the returned value is <tt>\ref vip_buffer_format_e</tt> */
    VIP_BUFFER_PROP_DATA_FORMAT = 3,
    /*!< \brief The position of fixed point for dynamic fixed point,
                the returned value is vip_uint8_t */
    VIP_BUFFER_PROP_FIXED_POINT_POS = 4,
    /*!< \brief The scale value for TF quantization format, the returned value is vip_float_t */
    VIP_BUFFER_PROP_TF_SCALE = 5,
    /*!< \brief The zero point for TF quantization format, the returned value is vip_uint32_t */
    VIP_BUFFER_PROP_TF_ZERO_POINT = 6,
    /*!< \brief The name for network's inputs and outputs, the returned value is vip_char_t[64] */
    VIP_BUFFER_PROP_NAME = 7,
} vip_buffer_property_e;

/* \brief The list of property of operation vip_buffer type.
  * \ingroup group_buffer
  * \version 1.3
 */
typedef enum _vip_buffer_operation_type_e {
    /*!< \brief None operation </tt> */
    VIP_BUFFER_OPER_TYPE_NONE = 0,
    /*!< \brief Flush the vip buffer </tt> */
    VIP_BUFFER_OPER_TYPE_FLUSH = 1,
    /*!< \brief invalidate the vip buffer </tt> */
    VIP_BUFFER_OPER_TYPE_INVALIDATE = 2,
    VIP_BUFFER_OPER_TYPE_MAX,
} vip_buffer_operation_type_e;

/*! \brief The list of type of vip_buffer.
  * \ingroup group_buffer
  * \version 2.2
 */
typedef enum _vip_buffer_create_prop_e {
    /*!< \brief None operation. invalid prop */
    VIP_BUFFER_CREATE_PROP_NONE = 0,
    /*!< \brief The vip-buffer is a tensor.
         Application should fild the tensor parameters when create buffer */
    VIP_BUFFER_CREATE_PROP_IS_TENSOR = 0x01,
    /*!< \brief The memory of vip-buffer is 32bit address */
    VIP_BUFFER_CREATE_PROP_32BIT_ADDR = 0x02,
    /*!< \brief not npu mmu mapping */
    VIP_BUFFER_CREATE_PROP_NO_MMU_MAP = 0x04,
} vip_buffer_create_prop_e;

/*! \brief The list of type of vip_buffer.
  * \ingroup group_buffer
  * \version 2.2
 */
typedef enum _vip_buffer_create_type_e {
    /*!< \brief None operation. invalid type */
    VIP_BUFFER_CREATE_NONE = 0,
    /*!< \brief create buffer and alloc memory from viplite driver.
        The memory of vip-buffer is allocated from the video memory management of VIPLite */
    VIP_BUFFER_CREATE_ALLOC_MEM = 1,
    /*!< \brief create a buffer from user contiguous or scatter non-contiguous CPU/NPU physical address.
       the vip_buffer created by this API doesn't support flush CPU cache in VIPLite.
       So the physical memory should be a non-cache buffer or flush CPU on Host control.
       physical can come from Host(CPU physical) or Device(NPU physical). */
    VIP_BUFFER_CREATE_FROM_PHY = 2,
    /*!< \brief create a buffer from handle(CPU logical address).
       The vip_buffer can be used to input, output, memory pool and so on.
       NOTE: driver will operation CPU cache when call vip_flush_buffer API.
       After write data into this buffer, APP should call vip_flush_buffer(VIP_BUFFER_OPER_TYPE_FLUSH)
       before CPU read date from this buffer. APP should call vip_flush_buffer(VIP_BUFFER_OPER_TYPE_INVALIDATE) */
    VIP_BUFFER_CREATE_FROM_USER_MEM = 3,
    /*!< \brief create a vip buffer from user fd(file descriptor).
       only  support create buffer from dma-buf on Linux.
       the vip_buffer created by this APi doesn't support flush CPU cache in driver.
       So the dma-buf should be a non-cache buffer or flush CPU on Host control. */
    VIP_BUFFER_CREATE_FROM_FD = 4,
    /*!< \brief create new buffer from a source_buffer */
    VIP_BUFFER_CREATE_FROM_BUFFER = 5,
    /*!< \brief create new buffer, and reserved virtual address of NPU */
    VIP_BUFFER_CREATE_LOCK_VA = 6,
    /*!< \brief create new buffer, and fill phyiscal address for src_buffer+offset
         The src_buffer should be created from LOCK_VA */
    VIP_BUFFER_CREATE_FILL_PA = 7,
    VIP_BUFFER_CREATE_MAX
} vip_buffer_create_type_e;

/*! \brief the parameters for vip_buffer
 * \ingroup group_buffer
*/
typedef struct _vip_buffer_param {
    /*!< \brief The number of dimensions specified in *sizes*/
    vip_uint32_t num_of_dims;
    /*!< \brief The pointer to an array of dimension */
    vip_size_t sizes[8];
    /*!< \brief Data format for the tensor, see <tt>\ref vip_buffer_format_e </tt> */
    vip_enum data_format;
    /*!< \brief Quantized format see <tt>\ref vip_buffer_quantize_format_e </tt>. */
    vip_enum quant_format;
    /*<! \brief The union of quantization information */
    union {
        struct {
            /*!< \brief Specifies the fixed point position when the input element type is int16,
                        if 0 calculations are performed in integer math */
            vip_int32_t fixed_point_pos;
        } dfp;

        struct {
            /*!< \brief Scale value for the quantized value */
            vip_float_t scale;
            /*!< \brief  A 32 bit integer, in range [0, 255] */
            vip_int32_t zero_point;
        } affine;
    } quant_data;
} vip_buffer_param_t;

typedef struct _vip_network* vip_network;
typedef struct _vip_buffer* vip_buffer;
typedef struct _vip_group* vip_group;

typedef struct _vip_buffer_create_params {
    /*<! \brief The index of the device which own this VIP buffer */
    vip_uint32_t device_index;
    /*!< \brief The type of creating the buffer, see vip_buffer_create_type_e */
    vip_buffer_create_type_e type;
    /* The source of creating vip-buffer, choose by vip_buffer_create_type_e */
    union {
        struct {
            /*The total size of memory should be allocated for the vip-buffer*/
            vip_size_t size;
            /*The alignment size of both base address and total size for the buffer of memory */
            vip_uint32_t align;
        } alloc_mem;
        struct {
            /*The type of this physical memory */
            vip_buffer_from_phy_type_e memory_type;
            /*The number of physical table element.
              Physical_num is 1 when create buffer from contiguous phyiscal.*/
            vip_uint32_t physical_num;
            /*Physical address table of VIP. should be wraped for VIP hardware.
            If memory_type is VIP_BUFFER_FROM_PHY_TYPE_HOST, the physical_table is CPU physical.
            If memory_type is VIP_BUFFER_FROM_PHY_TYPE_DEVICE, the physical_table is NPU physical.*/
            vip_address_t* physical_table;
            /*The size of physical memory for each physical_table element.*/
            vip_size_t* size_table;
        } from_physical;
        struct {
            /* The type of this user memory */
            vip_buffer_from_user_mem_type_e memory_type;
            /*The total size of the buffer.
              The size should be aligned to 64byte(cache line) for easy flash CPU cache*/
            vip_size_t size;
            /*The CPU logical address of the vip-buffer */
            vip_ptr logical_addr;
        } from_handle;
        struct {
            /*The type of the fd */
            vip_buffer_from_fd_type_e memory_type;
            /*The total size of the buffer*/
            vip_size_t size;
            /*fd(file descriptor) value*/
            vip_uint32_t fd_value;
        } from_fd;
        struct {
            /*The new buffer is created from the src_buffer */
            vip_buffer src_buffer;
        } from_buffer;
        struct {
            /*The new buffer is created for the src_buffer, the src_buffer should be created from LOCK_VA */
            vip_buffer src_buffer;
            /* The offset of src_buffer be filled physical address */
            vip_size_t offset;
            /*The total size of memory should be allocated for the vip-buffer*/
            vip_size_t size;
        } fill_pa;
    } src;

    /* The property of vip-buffer*/
    vip_buffer_create_prop_e prop;
    union {
        struct {
            /*The tensor information, demension and quant parameters of this buffer */
            vip_buffer_param_t tensor_param;
        } param;
    };
} vip_buffer_create_params_t;

typedef enum _vip_net_create_prop_e {
    VIP_NET_CREATE_PROP_NONE = 0x0,
    /*!< \brief Create network source type is NBG file.
        Should choose one nbg_type for vip_net_create_nbg_type_e. And fill vip_create_network_param_t.nbg */
    VIP_NET_CREATE_PROP_FROM_NBG = 0x1,
    /*!< \brief Create network source type is another network.
       Should choose one net_type for vip_net_create_net_type_e. And fill vip_create_network_param_t.net*/
    VIP_NET_CREATE_PROP_FROM_NETWORK = 0x2,
    /*!< \brief Indicate do not alloc video memory during creating network, do it during preparing network */
    VIP_NET_CREATE_PROP_LAZY_ALLOC = 0x4,
} vip_net_create_prop_e;

typedef enum _vip_net_create_nbg_flash_type_e {
    /*!< \brief invalid none */
    VIP_NET_CREATE_NBG_FLASH_MEM_NONE = 0,
    /*!< \brief NBG file on flash logical memory and create network from this flash logical address */
    VIP_NET_CREATE_NBG_FLASH_MEM_LOGICAL = 1,
    /*!< \brief NBG file on flash physical memory and create network from this flash physical address */
    VIP_NET_CREATE_NBG_FLASH_MEM_PHYSICAL = 2,
} vip_net_create_nbg_flash_type_e;

typedef enum _vip_net_create_nbg_type_e {
    /*!< \brief invalid none */
    VIP_NET_CREATE_NBG_FROM_NONE = 0,
    /*!< \brief Create network from NBG file path */
    VIP_NET_CREATE_NBG_FROM_FILE = 1,
    /*!< \brief Create network from memory address, NBG has been loaded in this memory before */
    VIP_NET_CREATE_NBG_FROM_MEMORY = 2,
    /*!< \brief Create network from flash device or user memory.
      The *data param of vip_create_network means are:
       1. If the NPU's MMU is enabled, the *data means that the CPU's logical address which access the memory.
       2. If the NPU's MMU is disabled, the *data means that the CPU's phyiscal address which access the memory.
       This is for DDR-less project.
       1. Load NBG from flash device. The NBG file should be placed to flash device before running VIPLite.
          Pass the NBG size and the location of NBG in flash device to this API.
       2. The NBG file pre-load into user memory which alloc via malloc function, or contiguous physical.
          Advantage: coeff data is not copied again, save more memory than create_network_from_memory type.
          Need enable VIP's MMU when works on Linux.
    */
    VIP_NET_CREATE_NBG_FROM_FLASH = 3,
} vip_net_create_nbg_type_e;

typedef enum _vip_net_create_net_type_e {
    /*!< \brief invalid none */
    VIP_NET_CREATE_NET_NONE = 0,
    /*!< \brief New network share coeff with net.src_network */
    VIP_NET_CREATE_NET_SHARE_COEFF = 1,
} vip_net_create_net_type_e;

typedef struct _vip_create_network_params {
    /*<! \brief The index of device on which the network is created. */
    vip_uint32_t device_index;
    /*! \brief the property of creating network, can multi-property or together */
    vip_net_create_prop_e prop;
    /*!< \brief create network source is NBG file */
    struct {
        vip_net_create_nbg_type_e type;
        union {
            /*!< \brief create network from nbg memory */
            struct {
                void* nbg_memory;
                vip_size_t nbg_size;
            } memory;
            /*!< \brief create network from nbg flash */
            struct {
                vip_net_create_nbg_flash_type_e flash_type;
                union {
                    void* flash_logical;
                    vip_address_t flash_physical;
                };
                vip_size_t nbg_size;
            } flash;
            /*!< \brief create network from nbg file path */
            struct {
                /* the path of NBG file in file system */
                void* nbg_path;
                /* the offset of NBG data in file */
                vip_size_t offset;
            } file;
        };
    } nbg;
    /*!< \brief create network source is network */
    struct {
        vip_net_create_net_type_e type;
        /*!< \brief create network from network, source network */
        vip_network src_network;
    } net;
} vip_create_network_param_t;

typedef struct _vip_power_frequency {
    /*
        The VIP core clock scale percent. fscale_percent value should be 1~~100.
        100 means that full clock frequency.
        1 means that minimum clock frequency.
    */
    vip_uint8_t fscale_percent;
} vip_power_frequency_t;

/*! \brief network performance parameter
 * \ingroup group_network
*/
typedef struct _vip_inference_profile {
    /* the time of inference the network, unit us, microsecond */
    vip_uint32_t inference_time;
    /* the VIP's cycle of inference the network */
    vip_uint64_t total_cycle;
} vip_inference_profile_t;

/*! \brief change PPU parameters
 * \ingroup group_network
*/
typedef struct _vip_ppu_param {
    /* work-dim should equal to PPU application work-dim seeting when generating NBG file */
    vip_uint32_t work_dim;
    vip_uint32_t global_offset[3];
    vip_uint32_t global_scale[3];
    vip_uint32_t local_size[3];
    vip_uint32_t global_size[3];
} vip_ppu_param_t;

/*! \brief the ids(layer id) of layer dump
 * \ingroup group_network
*/
typedef struct _vip_layer_dump_id {
    /* the count of layer output dump. Dump all layer output if layer_count is -1*/
    vip_int32_t layer_count;
    /* the id of layer output dump */
    vip_int32_t* layer_id;
} vip_nld_layer_id_t;

/*! \brief informations for layer dumped one output
 * \ingroup group_network
*/
typedef struct _vip_nld_output_info {
    /* the name of this layer output */
    vip_char_t layer_name[64];
    /* the id of layer dump. the layer_id is one id in layer_id[] array set via vip_nld_layer_id_t */
    vip_uint32_t layer_id;
    /* if a layer has multiple outputs, this is the index of output in a layer */
    vip_uint32_t layer_output_index;
    /* unique id for this layer. unique id is genereated by Acuity tool */
    vip_uint32_t uid;
    /* the memory parameter for the output */
    vip_buffer_param_t param;
    /* used size of output buffer */
    vip_uint32_t size;
    /* memory pointer for the output buffer */
    void* memory;
} vip_nld_output_info_t;

/*! \brief the output data for network layer dump
 * \ingroup group_network
*/
typedef struct _vip_nld_output {
    /* the count of output */
    vip_uint32_t count;
    /* output information data */
    vip_nld_output_info_t* info;
} vip_nld_output_t;

typedef struct _vip_video_memory_info {
    /* Indicate the type of physical adderss */
    vip_buffer_from_phy_type_e type;
    struct {
        vip_address_t physical;
        vip_size_t size;
    } command;
    struct {
        vip_address_t physical;
        vip_size_t size;
    } coeff;
    struct {
        vip_address_t physical;
        vip_size_t size;
    } mem_pool;
    struct {
        vip_address_t physical;
        vip_size_t size;
    } other;
} vip_video_memory_info_t;

/***** API Prototypes. *****/

/*! \brief  Get VIPLite driver version.
 * \return <tt>\ref vip_uint32_t </tt>
 * \ingroup group_global
 */
VIP_API
vip_uint32_t vip_get_version(void);

/*! \brief  Initial VIP Hardware, VIP lite software environment and power on VIP hardware.
 * \details when vpmdENABLE_MULTIPLE_TASK set to 0,
            This function should be only called once before using VIP hardware if.
            when vpmdENABLE_MULTIPLE_TASK set to 1,
            vip_init can be called multiple times, but should paired with vip_destroy.
            vip_init should be called in every process.
            only need call vip_init once in multi-thread.
 * VIP lite driver would construct some global variable for this call.Also
 * it will reset VIP and initialize VIP hardware to a ready state to accept jobs.
 * \return <tt>\ref vip_status_e </tt>
 * \ingroup group_global
 * \version 1.0
 */
VIP_API
vip_status_e vip_init(void);

/*! \brief Terminate VIP lite driver and shut down VIP hardware.
 * \details This function should be the last function called by application.
            vip_destroy should paired with vip_init called.
 * After it, no VIP lite API should be called except <tt>\ref vip_init </tt>
 * \return <tt>\ref vip_status_e </tt>
 * \ingroup group_global
 * \version 1.0
 * \notes vip_destroy should be called in the same thread as vip_init.
 */
VIP_API
vip_status_e vip_destroy(void);

/*! \brief Queries hardware caps information. This function shold be called after calling vip_init.
 *\param property, the query property enum.
 *\param size, the size of value buffer.
 *\param value, the value buffer of returns.
 * \ingroup group_global
*/
VIP_API
vip_status_e vip_query_hardware(IN vip_query_hardware_property_e property, IN vip_uint32_t size,
                                OUT void* value);

/*! \brief Create a input or output buffer with specified parameters.
 *\details The buffer object always takes [w, h, c, n] order,
           there is no padding/hole between lines/slices/batches.
 *\param [in] create_param The pointer to <tt>\ref vip_buffer_create_params_t </tt> structure.
 *\param [in] size_of_param The size of create_param pointer.
 *\param [out] buffer  An opaque handle for the new buffer object if the request is executed successfully.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_buffer
 *\version 1.0
 */
VIP_API
vip_status_e vip_create_buffer(IN vip_buffer_create_params_t* create_param, IN vip_uint32_t size_of_param,
                               OUT vip_buffer* buffer);

/*! \brief  Destroy a buffer object which was created before.
 *\param [in] buffer The opaque handle of buffer to be destroyed.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_buffer
 *\version 1.0
*/
VIP_API
vip_status_e vip_destroy_buffer(IN vip_buffer buffer);

/*! \brief Map a buffer to get the CPU accessible address for read or write
 *\param [in] buffer The handle of buffer to be mapped.
 *\return A pointer that application can use to read or write the buffer data.
 *\ingroup group_buffer
 *\version 1.0
*/
VIP_API
void* vip_map_buffer(IN vip_buffer buffer);

/*! \brief Unmap a buffer which was mapped before.
 *\param [in] buffer The handle of buffer to be unmapped.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_buffer
 *\version 1.0
*/
VIP_API
vip_status_e vip_unmap_buffer(IN vip_buffer buffer);

/*! \brief Get the size of bytes allocated for the buffer.
 *\param [in] buffer The handle of buffer to be queried.
 *\return <tt>\ref the size of bytes </tt>
 *\ingroup group_buffer
 *\version 1.0
*/
VIP_API
vip_uint32_t vip_get_buffer_size(IN vip_buffer buffer);

/*! \brief operation the vip buffer CPU chace. flush, invalidate cache.
  You should call vip_flush_buffer to flush buffer for input.
  and invalidate buffer for network's output if these memories with CPU cache.
*\param buffer The vip buffer object.
*\param the type of this operation. see vip_buffer_operation_type_e.
*\ingroup group_buffer
*/
VIP_API
vip_status_e vip_flush_buffer(IN vip_buffer buffer, IN vip_buffer_operation_type_e type);

/*! \brief Create a network object from the given parameters.
 *\details The binary is generated by the binary graph generator and it's a blob binary.
 *\VIP lite Driver could interprete it to create a network object.
 *\param [in] param, The pointer to <tt>\ref vip_create_network_params_t </tt> structure.
                     Network is created from a nbg file or another network.
 *\param [in] param_size, The size of param pointer.
 *\param [out] network An opaque handle to the new network object if the request is executed successfully
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 2.2
 */
VIP_API
vip_status_e vip_create_network(IN vip_create_network_param_t* param, IN vip_uint32_t param_size,
                                OUT vip_network* network);

/*! \brief Destroy a network object
 *\details Release all resources allocated for this network.
 *\param [in] network The opaque handle to the network to be destroyed
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_destroy_network(IN vip_network network);

/*! \brief Prepare a network to run on VIP.
 *\details This function only need to be called once to prepare a network and
           make it ready to execute on VIP hardware.
 * It would do all heavy-duty work, including allocate internal memory resource for this network,
   deploy all operation's resource
 * to internal memory pool, allocate/generate command buffer for this network,
   patch command buffer for the resource in the internal memory
 * allocations. If this function is called more than once, driver will silently ignore it.
                If this function is executed successfully, this network is prepared.
 *\param [in] network The opaque handle to the network which need to be prepared.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_prepare_network(IN vip_network network);

/*! \brief Finish using this network to do inference.
 *\details This function is paired with <tt>\ref vip_prepare_network </tt>.
           It's suggested to be called once after <tt>\ref vip_prepare_network </tt> called.
 * If it's called more than that, it will be silently ignored.
   If the network is not prepared but finished is called, it's silently ignored too.
 * This function would release all internal memory allocations which are allocated when
   the network is prepared. Since the preparation of network takes much time,
 * it is suggested that if the network will be still used later, application should not
   finish the network unless there is no much system resource remained for other
 * networks. The network object is still alive unitl it's destroyed by <tt>\ref vip_destroy_network </tt>.
 *\param [in] network The opaque handle to the network which will be finished.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_finish_network(IN vip_network network);

/*! \brief Configure network property. configure network. this API should be called before
           calling vip_prepare_network.
 *\details Configure network's layer inputs/outputs information
 *\param [in] network A property <tt>\ref vip_network_property_e </tt> to be configuied.
 *\return <tt>\ref vip_status_e </tt>
 */
VIP_API
vip_status_e vip_set_network(IN vip_network network, IN vip_enum property, IN void* value);

/*! \brief Query a property of the network object.
 *\details User can use this API to get any properties from a network.
 *\param [in] network The opaque handle to the network to be queried
 *\param [in] property A property <tt>\ref vip_network_property_e </tt> to be queried.
 *\param [out] value A pointer to memory to store the return value,
               different property could return different type/size of value.
 * please see comment of <tt>\ref vip_network_property_e </tt> for detail.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_query_network(IN vip_network network, IN vip_enum property, OUT void* value);

/*! \brief. Kick off the network execution and send command buffer of this network to VIP hardware.
 *\details This function can be called multiple times.
           Every time it's called it would do inference with current attached
 * input buffers and output buffers. It would return until VIP finish the execution.
         If the network is not ready to execute
 * for some reason like not be prepared by <tt>\ref vip_prepare_network </tt>,
   it would fail with status reported.
 *\param [in] network The opaque handle to the network to be executed.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_run_network(IN vip_network network);

/*! \brief. Kick off the network execution and send command buffer of this network to VIP hardware.
*\details This function is similar to <tt>\ref vip_run_network </tt> except that it returns
          immediately without waiting for HW to complete the commands.
*\param [in] network The opaque handle to the network to be executed.
*\return <tt>\ref vip_status_e </tt>
*\ingroup group_network
*\version 1.0
*/
VIP_API
vip_status_e vip_trigger_network(IN vip_network network);

/*! \brief. Explicitly wait for HW to finish executing the submitted commands.
*\details This function waits for HW to complete the commands.
          This should be called once CPU needs to access the network currently being run.
*\return <tt>\ref vip_status_e </tt>
*\ingroup group_network
*\version 1.0
*/
VIP_API
vip_status_e vip_wait_network(IN vip_network network);

/*! \brief. Cancle network running on vip hardware after network is commited.
*\details This function is cancel network running on vip hardware.
*\return <tt>\ref vip_status_e </tt>
*\ingroup group_network
*\version 1.0
*/
VIP_API
vip_status_e vip_cancel_network(IN vip_network network);

/*! \brief Query a property of a specific input of a given network.
 *\details The specified input/property/network must be valid,
           otherwise VIP_ERROR_INVALID_ARGUMENTS will be returned.
 *\param [in] network The opaque handle to the network to be queried
 *\param [in] index Specify which input to be queried in case there are multiple inputs in the network
 *\param [in] property Specify which property application wants to know, see <tt>\ref vip_buffer_property_e </tt>
 *\param [out] value Returned value, the details type/size, please refer to the comment of
                <tt>\ref vip_input_property_e </tt>
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_query_input(IN vip_network network, IN vip_uint32_t index, IN vip_enum property,
                             OUT void* value);

/*! \brief Query a property of a specific output of a given network.
 *\details The specified output/property/network must be valid,
           otherwise VIP_ERROR_INVALID_ARGUMENTS will be returned.
 *\param [in] network The opaque handle to the network to be queried
 *\param [in] index Specify which output to be queried in case there are multiple outputs in the network
 *\param [in] property Specify which property application wants to know, see <tt>\ref vip_buffer_property_e </tt>
 *\param [out] value Returned value, the details type/size, please refer to the comment of
                <tt>\ref vip_input_property_e </tt>
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_query_output(IN vip_network network, IN vip_uint32_t index, IN vip_enum property,
                              OUT void* value);

/*! \brief Attach an input buffer to the specified index of the network.
 *\details All the inputs of the network need to be attached to a valid input buffer before running a network,
   otherwise
 * VIP_ERROR_MISSING_INPUT_OUTPUT will be returned when calling <tt> \ref vip_run_network </tt>.
   When attaching an input buffer
 * to the network, driver would patch the network command buffer to fill in this input buffer address.
   This function could be called
 * multiple times to let application update the input buffers before next network execution.
   The network must be prepared by <tt>\ref vip_prepare_network </tt> before
 * attaching an input.
 *\param [in] network The opaque handle to a network which we want to attach an input buffer
 *\param [in] index The index specify which input in the network will be set
 *\param [in] input The opaque handle to a buffer which will be attached to the network.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_set_input(IN vip_network network, IN vip_uint32_t index, IN vip_buffer input);

/*! \brief Attach an output buffer to the specified index of the network.
 *\details All the outputs of the network need to be attached to a
        valid output buffer before running a network, otherwise
 * VIP_ERROR_MISSING_INPUT_OUTPUT will be returned when calling <tt> \ref vip_run_network </tt>.
    When attaching an output buffer
 * to the network, driver would patch the network command buffer to fill in this output buffer address.
    This function could be called
 * multiple times to let application update the output buffers before next network execution.
   The network must be prepared by <tt>\ref vip_prepare_network </tt> before
 * attaching an output.
 *\param [in] network The opaque handle to a network which we want to attach an output buffer
 *\param [in] index The index specify which output in the network will be set
 *\param [in] output The opaque handle to a buffer which will be attached to the network.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_set_output(IN vip_network network, IN vip_uint32_t index, IN vip_buffer output);

/*! \brief. Create a vip_group object to run multiple tasks(network or node)
            and without interrupt between each task.
*\return <tt>\ref vip_status_e </tt>
*\param count The maximum number of tasks supports by this group.
*\param group Return vip_group object be created.
*\version 1.0
*/
VIP_API
vip_status_e vip_create_group(IN vip_uint32_t count, OUT vip_group* group);

/*! \brief. Destroy group object which created by vip_create_group.
*\return <tt>\ref vip_status_e </tt>
*\param group vip_group object/
*\version 1.0
*/
VIP_API
vip_status_e vip_destroy_group(IN vip_group group);

/*
@brief set group property. configure group. this API should be called before calling vip_run_group.
@param group The group object which created by vip_create_group().
@param property The property be set. see vip_group_property_e.
@param value The set data.
*/
VIP_API
vip_status_e vip_set_group(IN vip_group group, IN vip_enum property, IN void* value);

/*! \brief Query a property of the group object.
 *\param [in] group The group object which created by vip_create_group().
 *\param [in] property A property <tt>\ref vip_group_property_e </tt> to be queried.
 *\param [out] value A pointer to memory to store the return value,
               different property could return different type/size of value.
 * please see comment of <tt>\ref vip_group_property_e </tt> for detail.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_query_group(IN vip_group group, IN vip_enum property, OUT void* value);

/*! \brief. add a vip_network object into group.
*\return <tt>\ref vip_status_e </tt>
*\param group vip_group object, network be added into group.
*\param network vip_network added into group.
*\version 1.0
*/
VIP_API
vip_status_e vip_add_network(IN vip_group group, IN vip_network network);

/*! \brief. run tasks in group. only issue a interrupt after tasks complete.
            These tasks is added by vip_add_network.
            The order of executuion of tasks is call vip_add_network.
*\return <tt>\ref vip_status_e </tt>
*\param group vip_group object
*\param the number of task will be run.
        eg: num is 4, the 0, 1, 2, 3 taks index in group will be run(inference).
*\version 1.0
*/
VIP_API
vip_status_e vip_run_group(IN vip_group group, IN vip_uint32_t num);

/*! \brief. Run tasks in group,these tasks is added by vip_add_network.
            The order of executuion of tasks is call vip_add_network.
*\details This function is similar to <tt>\ref vip_run_group </tt> except that it returns
          immediately without waiting for HW to complete the commands.
*\return <tt>\ref vip_status_e </tt>
*\param group vip_group object
*\param the number of task will be run.
        eg: num is 4, the 0, 1, 2, 3 taks index in group will be run(inference).
*\ingroup group_network
*\version 1.0
*/
VIP_API
vip_status_e vip_trigger_group(IN vip_group group, IN vip_uint32_t num);

/*! \brief. Explicitly wait for HW to finish executing the submitted task in group.
*\details This function waits for HW to complete the submitted commands in group.
          This should be called once CPU needs to access the group currently being run.
*\return <tt>\ref vip_status_e </tt>
*\ingroup group_network
*\version 1.0
*/
VIP_API
vip_status_e vip_wait_group(IN vip_group group);

/*! \brief. give user applications more control over power management for VIP cores.
*\details. control VIP core frequency and power status by property. see vip_power_property_e.
*\param ID of the managed device. device_index is 0 if VIP is single core.
*\param perperty Control VIP core frequency and power status by property. see vip_power_property_e.
*\param value The value for vip_power_property_e property.
       Please see vip_power_frequency_t if property is setting to VIP_POWER_PROPERTY_SET_FREQUENCY.
*\return <tt>\ref vip_status_e </tt>
*\ingroup group_network
*\version 1.0
*/
VIP_API
vip_status_e vip_power_management(IN vip_uint32_t device_index, IN vip_power_property_e property,
                                  IN void* value);

/*! \brief. change PPU engine parameters.
            change local size, global size, global offset and global scale.
*\return <tt>\ref vip_status_e </tt>
*\param network The network object should be changed.
*\param param PPU parameters
*\param index The index of PPU node, not used. please set to zero.
*\ingroup group_network
*\version 1.0
*/
VIP_API
vip_status_e vip_set_ppu_param(IN vip_network network, IN vip_ppu_param_t* param, IN vip_uint32_t index);

#ifdef __cplusplus
}
#endif

#endif
