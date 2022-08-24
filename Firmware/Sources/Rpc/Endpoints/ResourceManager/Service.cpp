#include <etl/algorithm.h>
#include <etl/type_traits.h>
#include <printf/printf.h>

#include <string.h>

#include "stm32mp1xx.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include "../../MessageHandler.h"
#include "../../Rpc.h"
#include "Handler.h"
#include "Service.h"

using namespace Rpc::ResourceManager;

/* The rpmsg_srm_message_t and related structures must be aligned with the ones
 * used by the remote processor */
typedef struct __clock_config_t {
    uint32_t index;
    uint8_t name[16];
    uint32_t rate;
} clock_config_t;

typedef struct __regu_config_t {
    uint32_t index;
    uint8_t name[16];
    uint32_t enable;
    uint32_t curr_voltage_mv; /* GetConfig */
    uint32_t min_voltage_mv; /* SetConfig */
    uint32_t max_voltage_mv; /* SetConfig */
} regu_config_t;

typedef struct __rpmsg_srm_message_t {
    uint32_t message_type;
    uint8_t device_id[32];
    uint32_t rsc_type;
    union {
        clock_config_t clock_config;
        regu_config_t regu_config;
    };
} rpmsg_srm_message_t;



#define ETZPC_NO_INDEX 0xff

/**
 * @brief Device configuration
 *
 * The STM32MP15x-specific device mappings for resource IDs.
 */
const etl::array<Service::DeviceConfig, Service::kNumDeviceConfigs> Service::gDeviceConfig{{
/* Devices under ETZPC control */
    { RESMGR_ID_USART1,       USART1_BASE,    0x03 },
    { RESMGR_ID_SPI6,         SPI6_BASE,      0x04 },
    { RESMGR_ID_I2C4,         I2C4_BASE,      0x05 },
    { RESMGR_ID_RNG1,         RNG1_BASE,      0x07 },
    { RESMGR_ID_HASH1,        HASH1_BASE,     0x08 },
#if defined (CRYP1)
    { RESMGR_ID_CRYP1,        CRYP1_BASE,     0x09 },
#endif
    { RESMGR_ID_I2C6,         I2C6_BASE,      0x0C },
    { RESMGR_ID_TIM2,         TIM2_BASE,      0x10 },
    { RESMGR_ID_TIM3,         TIM3_BASE,      0x11 },
    { RESMGR_ID_TIM4,         TIM4_BASE,      0x12 },
    { RESMGR_ID_TIM5,         TIM5_BASE,      0x13 },
    { RESMGR_ID_TIM6,         TIM6_BASE,      0x14 },
    { RESMGR_ID_TIM7,         TIM7_BASE,      0x15 },
    { RESMGR_ID_TIM12,        TIM12_BASE,     0x16 },
    { RESMGR_ID_TIM13,        TIM13_BASE,     0x17 },
    { RESMGR_ID_TIM14,        TIM14_BASE,     0x18 },
    { RESMGR_ID_LPTIM1,       LPTIM1_BASE,    0x19 },
    { RESMGR_ID_SPI2,         SPI2_BASE,      0x1B },
    { RESMGR_ID_SPI3,         SPI3_BASE,      0x1C },
    { RESMGR_ID_SPDIFRX,      SPDIFRX_BASE,   0x1D },
    { RESMGR_ID_USART2,       USART2_BASE,    0x1E },
    { RESMGR_ID_USART3,       USART3_BASE,    0x1F },
    { RESMGR_ID_UART4,        UART4_BASE,     0x20 },
    { RESMGR_ID_UART5,        UART5_BASE,     0x21 },
    { RESMGR_ID_I2C1,         I2C1_BASE,      0x22 },
    { RESMGR_ID_I2C2,         I2C2_BASE,      0x23 },
    { RESMGR_ID_I2C3,         I2C3_BASE,      0x24 },
    { RESMGR_ID_I2C5,         I2C5_BASE,      0x25 },
    { RESMGR_ID_CEC,          CEC_BASE,       0x26 },
    { RESMGR_ID_DAC1,         DAC1_BASE,      0x27 },
    { RESMGR_ID_UART7,        UART7_BASE,     0x28 },
    { RESMGR_ID_UART8,        UART8_BASE,     0x29 },
    { RESMGR_ID_TIM1,         TIM1_BASE,      0x30 },
    { RESMGR_ID_TIM8,         TIM8_BASE,      0x31 },
    { RESMGR_ID_USART6,       USART6_BASE,    0x33 },
    { RESMGR_ID_SPI1,         SPI1_BASE,      0x34 },
    { RESMGR_ID_SPI4,         SPI4_BASE,      0x35 },
    { RESMGR_ID_TIM15,        TIM15_BASE,     0x36 },
    { RESMGR_ID_TIM16,        TIM16_BASE,     0x37 },
    { RESMGR_ID_TIM17,        TIM17_BASE,     0x38 },
    { RESMGR_ID_SPI5,         SPI5_BASE,      0x39 },
    { RESMGR_ID_SAI1,         SAI1_BASE,      0x3A },
    { RESMGR_ID_SAI2,         SAI2_BASE,      0x3B },
    { RESMGR_ID_SAI3,         SAI3_BASE,      0x3C },
    { RESMGR_ID_DFSDM1,       DFSDM1_BASE,    0x3D },
#if defined (FDCAN1)
    { RESMGR_ID_FDCAN1,       FDCAN1_BASE,    0x3E }, /* same decprot for all FDCAN */
#endif
#if defined (FDCAN2)
    { RESMGR_ID_FDCAN2,       FDCAN2_BASE,    0x3E }, /* same decprot for all FDCAN */
#endif
#if defined (FDCAN_CCU)
    { RESMGR_ID_FDCAN_CCU,    FDCAN_CCU_BASE, 0x3E }, /* same decprot for all FDCAN */
#endif
    { RESMGR_ID_LPTIM2,       LPTIM2_BASE,    0x40 },
    { RESMGR_ID_LPTIM3,       LPTIM3_BASE,    0x41 },
    { RESMGR_ID_LPTIM4,       LPTIM4_BASE,    0x42 },
    { RESMGR_ID_LPTIM5,       LPTIM5_BASE,    0x43 },
    { RESMGR_ID_SAI4,         SAI4_BASE,      0x44 },
    { RESMGR_ID_VREFBUF,      VREFBUF_BASE,   0x45 },
    { RESMGR_ID_DCMI,         DCMI_BASE,      0x46 },
    { RESMGR_ID_CRC2,         CRC2_BASE,      0x47 },
    { RESMGR_ID_ADC1,         ADC1_BASE,      0x48 }, /* same decprot for both ADC */
    { RESMGR_ID_ADC2,         ADC2_BASE,      0x48 }, /* same decprot for both ADC */
    { RESMGR_ID_HASH2,        HASH2_BASE,     0x49 },
    { RESMGR_ID_RNG2,         RNG2_BASE,      0x4A },
#if defined (CRYP2)
    { RESMGR_ID_CRYP2,        CRYP2_BASE,     0x4B },
#endif
    { RESMGR_ID_USB1_OTG_HS,  USBOTG_BASE,    0x55 },
    { RESMGR_ID_SDMMC3,       SDMMC3_BASE,    0x56 },
    { RESMGR_ID_DLYB_SDMMC3,  DLYB_SDMMC3_BASE,   0x57 },
    { RESMGR_ID_DMA1,         DMA1_BASE,      0x58 },
    { RESMGR_ID_DMA2,         DMA2_BASE,      0x59 },
    { RESMGR_ID_DMAMUX1,      DMAMUX1_BASE,   0x5A },
    { RESMGR_ID_FMC,          FMC_R_BASE,     0x5B },
    { RESMGR_ID_QUADSPI,      QSPI_R_BASE,    0x5C },
    { RESMGR_ID_DLYB_QUADSPI, DLYB_QSPI_BASE,     0x5D },
    { RESMGR_ID_ETH,          ETH_BASE,       0x5E },
    /* Devices NOT under ETZPC control */
    { RESMGR_ID_CRC1,         CRC1_BASE,      DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_DLYB_SDMMC1,  DLYB_SDMMC1_BASE,   DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_DLYB_SDMMC2,  DLYB_SDMMC2_BASE,   DeviceConfig::kNoEtpzcIndex },
#if defined (DSI)
    { RESMGR_ID_DSI,          DSI_BASE,       DeviceConfig::kNoEtpzcIndex },
#endif
#if defined (GPU)
    { RESMGR_ID_GPU,          GPU_BASE,       DeviceConfig::kNoEtpzcIndex },
#endif
    { RESMGR_ID_IPCC,         IPCC_BASE,      DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_IWDG1,        IWDG1_BASE,     DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_IWDG2,        IWDG2_BASE,     DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_LTDC,         LTDC_BASE,      DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_RTC,          RTC_BASE,       DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_SDMMC1,       SDMMC1_BASE,    DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_SDMMC2,       SDMMC2_BASE,    DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_USB1HSFSP1,   USB1HSFSP1_BASE, DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_USB1HSFSP2,   USB1HSFSP2_BASE, DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_USBPHYC,      USBPHYC_BASE,   DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_DBGMCU,       DBGMCU_BASE,    DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_HSEM,         HSEM_BASE,      DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_MDIOS,        MDIOS_BASE,     DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_MDMA,         MDMA_BASE,      DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_SYSCFG,       SYSCFG_BASE,    DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_DTS,          DTS_BASE,       DeviceConfig::kNoEtpzcIndex },
    { RESMGR_ID_WWDG1,        WWDG1_BASE,     DeviceConfig::kNoEtpzcIndex },
}};



/**
 * @brief Initialize the resource manager service
 *
 * @param handler Pointer to the underlying message handler
 */
Service::Service(Handler *handler) : handler(handler) {
    this->reqLock = xSemaphoreCreateMutex();
    REQUIRE(this->reqLock, "%s failed", "xSemaphoreCreateMutex");
}

/**
 * @brief Clean up resources allocated by the service
 */
Service::~Service() {
    vSemaphoreDelete(this->reqLock);
}

/**
 * @brief Set resource configuration
 *
 * @param resId Identifier for this resource (or kResourceIdNone to use resName instead)
 * @param resName Resource name (optional)
 * @param requestedConfig Configuration to apply to this resource
 * @param outActualConfig Actual configuration applied to the device
 * @param timeout How long to wait for a response to the request
 *
 * @return 0 on success or an error code
 */
int Service::setConfigInternal(const uint32_t resId, const etl::string_view resName,
        const ResourceConfig requestedConfig, ResourceConfig &outActualConfig,
        const TickType_t timeout) {
    int err{0};
    BaseType_t ok;
    rpmsg_srm_message_t msg{};
    ResourceType resType;
    etl::span<uint8_t> rawResponse;

    // format the message name
    if(resId != kResourceIdNone) {
        // convert ID to address
        uint32_t addr = GetDeviceAddress(resId);
        snprintf(reinterpret_cast<char *>(msg.device_id), sizeof(msg.device_id), "%x", addr);
    } else {
        // use the specified name directly
        if(resName.empty()) {
            return -1;
        }
        strncpy(reinterpret_cast<char *>(msg.device_id), resName.data(), sizeof(msg.device_id));
    }

    // figure out message type
    resType = etl::holds_alternative<ClockConfig>(requestedConfig) ? ResourceType::Clock :
        ResourceType::Regulator;

    // fill out the rest of the message based on the desired configuration type
    etl::visit([&msg](auto&& arg) {
        using T = etl::decay_t<decltype(arg)>;

        if constexpr(etl::is_same_v<T, ClockConfig>) {
            msg.clock_config.index = arg.index;
            msg.clock_config.rate = arg.rate;
            //strncpy(reinterpret_cast<char *>(msg.clock_config.name), arg.name.c_str(),
            //        sizeof(msg.clock_config.name));
            etl::copy_s(arg.name.begin(), arg.name.end(), etl::begin(msg.clock_config.name),
                    etl::end(msg.clock_config.name));
        } else if constexpr(etl::is_same_v<T, RegulatorConfig>) {
            msg.regu_config.index = arg.index;
            msg.regu_config.enable = arg.enable;
            msg.regu_config.min_voltage_mv = arg.minRequestedVoltage;
            msg.regu_config.max_voltage_mv = arg.maxRequestedVoltage;
            //strncpy(reinterpret_cast<char *>(msg.regu_config.name), arg.name.c_str(),
            //        sizeof(msg.clock_config.name));
            etl::copy_s(arg.name.begin(), arg.name.end(), etl::begin(msg.regu_config.name),
                    etl::end(msg.regu_config.name));
        }
    }, requestedConfig);

    // fill out message header
    switch(resType) {
        case ResourceType::Clock:
            msg.rsc_type = kRpmsgResourceClock;
            break;
        case ResourceType::Regulator:
            msg.rsc_type = kRpmsgResourceRegulator;
            break;
    }
    msg.message_type = kRpmsgMsgSetConfig;

    // send message and wait for response
    ok = xSemaphoreTake(this->reqLock, timeout);
    if(ok != pdTRUE) {
        return -1;
    }

    err = this->handler->sendRequestAndBlock({reinterpret_cast<uint8_t *>(&msg), sizeof(msg)},
            rawResponse, timeout);
    if(err) {
        goto beach;
    }

    // decode response
    if(rawResponse.size() < sizeof(rpmsg_srm_message_t)) {
        Logger::Warning("srm message too small (%u)", rawResponse.size());
        return -2;
    }

    err = DecodeResponse(rawResponse, resType, outActualConfig);

beach:;
    // release lock and return
    xSemaphoreGive(this->reqLock);
    return err;
}

/**
 * @brief Decode a response from rpmsg endpoint
 *
 * @param rawResponse Raw packet received from the remote
 * @param resType Resource type the request _should_ pertain to
 * @param outActualConfig Object to receive the actual configuration applied
 *
 * @return 0 on success or a negative error code
 *
 * @remark This expects that `rawResponse` is large enough to hold a complete message.
 */
int Service::DecodeResponse(etl::span<const uint8_t> rawResponse,
        const ResourceType resType, ResourceConfig &outActualConfig) {
    // get packet header and validate the message type and resource type
    auto res = reinterpret_cast<const rpmsg_srm_message_t *>(rawResponse.data());

    if(res->message_type != kRpmsgMsgGetConfig && res->message_type != kRpmsgMsgSetConfig) {
        return -100;
    }
    switch(resType) {
        case ResourceType::Clock:
            if(res->rsc_type != kRpmsgResourceClock) {
                return -101;
            }
            outActualConfig = ClockConfig{};
            break;
        case ResourceType::Regulator:
            if(res->rsc_type != kRpmsgResourceRegulator) {
                return -101;
            }
            outActualConfig = RegulatorConfig{};
            break;
    }

    // output the actual configuration value
    switch(resType) {
        case ResourceType::Clock: {
            auto ptr = etl::get_if<ClockConfig>(&outActualConfig);
            ptr->index = res->clock_config.index;
            ptr->rate = res->clock_config.rate;
            ptr->name = reinterpret_cast<const char *>(res->clock_config.name);
            break;
        }

        case ResourceType::Regulator:
            auto ptr = etl::get_if<RegulatorConfig>(&outActualConfig);
            ptr->index = res->regu_config.index;
            ptr->enable = res->regu_config.enable;
            ptr->currentVoltage = res->regu_config.curr_voltage_mv;
            ptr->name = reinterpret_cast<const char *>(res->regu_config.name);
            break;
    }

    return 0;
}

