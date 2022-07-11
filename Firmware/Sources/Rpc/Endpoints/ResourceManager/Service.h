#ifndef RPC_ENDPOINTS_RESOURCEMANAGER_SERVICE_H
#define RPC_ENDPOINTS_RESOURCEMANAGER_SERVICE_H

#include <stdint.h>
#include <stddef.h>

#include <etl/span.h>
#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/variant.h>

#include "Rtos/Rtos.h"

namespace Rpc {
void Init();
}


namespace Rpc::ResourceManager {
class Handler;

enum ResourceId: uint32_t {
    RESMGR_ID_ADC1                            ,
    RESMGR_ID_ADC2                            ,
    RESMGR_ID_CEC                             ,
    RESMGR_ID_CRC                             ,
    RESMGR_ID_CRC1 = RESMGR_ID_CRC            ,
    RESMGR_ID_CRC2                            ,
    RESMGR_ID_CRYP1                           ,
    RESMGR_ID_CRYP2                           ,
    RESMGR_ID_DAC1                            ,
    RESMGR_ID_DBGMCU                          ,
    RESMGR_ID_DCMI                            ,
    RESMGR_ID_DFSDM1                          ,
    RESMGR_ID_DLYB_QUADSPI                    ,
    RESMGR_ID_DLYB_SDMMC1                     ,
    RESMGR_ID_DLYB_SDMMC2                     ,
    RESMGR_ID_DLYB_SDMMC3                     ,
    RESMGR_ID_DMA1                            ,
    RESMGR_ID_DMA2                            ,
    RESMGR_ID_DMAMUX1                         ,
    RESMGR_ID_DSI                             ,
    RESMGR_ID_ETH                             ,
    RESMGR_ID_EXTI                             ,
    RESMGR_ID_FDCAN_CCU                       ,
    RESMGR_ID_FDCAN1                          ,
    RESMGR_ID_FDCAN2                          ,
    RESMGR_ID_FMC                             ,
    RESMGR_ID_GPIOA                           ,
    RESMGR_ID_GPIOB                           ,
    RESMGR_ID_GPIOC                           ,
    RESMGR_ID_GPIOD                           ,
    RESMGR_ID_GPIOE                           ,
    RESMGR_ID_GPIOF                           ,
    RESMGR_ID_GPIOG                           ,
    RESMGR_ID_GPIOH                           ,
    RESMGR_ID_GPIOI                           ,
    RESMGR_ID_GPIOJ                           ,
    RESMGR_ID_GPIOK                           ,
    RESMGR_ID_GPIOZ                           ,
    RESMGR_ID_GPU                             ,
    RESMGR_ID_HASH1                           ,
    RESMGR_ID_HASH2                           ,
    RESMGR_ID_HSEM                            ,
    RESMGR_ID_I2C1                            ,
    RESMGR_ID_I2C2                            ,
    RESMGR_ID_I2C3                            ,
    RESMGR_ID_I2C4                            ,
    RESMGR_ID_I2C5                            ,
    RESMGR_ID_I2C6                            ,
    RESMGR_ID_IPCC                            ,
    RESMGR_ID_IWDG1                           ,
    RESMGR_ID_IWDG2                           ,
    RESMGR_ID_LPTIM1                          ,
    RESMGR_ID_LPTIM2                          ,
    RESMGR_ID_LPTIM3                          ,
    RESMGR_ID_LPTIM4                          ,
    RESMGR_ID_LPTIM5                          ,
    RESMGR_ID_LTDC                            ,
    RESMGR_ID_MDIOS                           ,
    RESMGR_ID_MDMA                            ,
    RESMGR_ID_QUADSPI                         ,
    RESMGR_ID_RNG                             ,
    RESMGR_ID_RNG1 = RESMGR_ID_RNG            ,
    RESMGR_ID_RNG2                            ,
    RESMGR_ID_RTC                             ,
    RESMGR_ID_SAI1                            ,
    RESMGR_ID_SAI2                            ,
    RESMGR_ID_SAI3                            ,
    RESMGR_ID_SAI4                            ,
    RESMGR_ID_SDMMC1                          ,
    RESMGR_ID_SDMMC2                          ,
    RESMGR_ID_SDMMC3                          ,
    RESMGR_ID_SPDIFRX                         ,
    RESMGR_ID_SPI1                            ,
    RESMGR_ID_SPI2                            ,
    RESMGR_ID_SPI3                            ,
    RESMGR_ID_SPI4                            ,
    RESMGR_ID_SPI5                            ,
    RESMGR_ID_SPI6                            ,
    RESMGR_ID_SYSCFG                          ,
    RESMGR_ID_TIM1                            ,
    RESMGR_ID_TIM12                           ,
    RESMGR_ID_TIM13                           ,
    RESMGR_ID_TIM14                           ,
    RESMGR_ID_TIM15                           ,
    RESMGR_ID_TIM16                           ,
    RESMGR_ID_TIM17                           ,
    RESMGR_ID_TIM2                            ,
    RESMGR_ID_TIM3                            ,
    RESMGR_ID_TIM4                            ,
    RESMGR_ID_TIM5                            ,
    RESMGR_ID_TIM6                            ,
    RESMGR_ID_TIM7                            ,
    RESMGR_ID_TIM8                            ,
    RESMGR_ID_DTS                             ,
    RESMGR_ID_UART4                           ,
    RESMGR_ID_UART5                           ,
    RESMGR_ID_UART7                           ,
    RESMGR_ID_UART8                           ,
    RESMGR_ID_USART1                          ,
    RESMGR_ID_USART2                          ,
    RESMGR_ID_USART3                          ,
    RESMGR_ID_USART6                          ,
    RESMGR_ID_USB1HSFSP1                      ,
    RESMGR_ID_USB1HSFSP2                      ,
    RESMGR_ID_USB1_OTG_HS                     ,
    RESMGR_ID_USBPHYC                         ,
    RESMGR_ID_VREFBUF                         ,
    RESMGR_ID_WWDG1                           ,
    RESMGR_ID_RESMGR_TABLE                    ,
};

/**
 * @brief Resource manager service
 *
 * Interface to the resource manager service running on the Linux side as a kernel module.
 */
class Service {
    friend class Handler;
    friend void Rpc::Init();

    public:
        /// Types of resources managaeable through this service
        enum class ResourceType {
            Clock                       = 0x00,
            Regulator                   = 0x01,
        };
        /// Configuration information for a clock
        struct ClockConfig {
            uint32_t index;
            etl::string<16> name;
            uint32_t rate;
        };
        /// Configuration information for a regulator
        struct RegulatorConfig {
            uint32_t index;
            etl::string<16> name;
            uint32_t enable;
            /// Currently applied voltage (mV, get)
            uint32_t currentVoltage;
            /// Minimum requested voltage (mV, set)
            uint32_t minRequestedVoltage;
            /// Maximum requested voltage (mV, set)
            uint32_t maxRequestedVoltage;
        };

        /// Aggregate type of all configuration types
        using ResourceConfig = etl::variant<ClockConfig, RegulatorConfig>;

        /// Resource id for a peripheral identified by name
        constexpr static const uint32_t kResourceIdNone{0xffffffff};

        /**
         * @brief Set the configuration of a peripheral's clock or regulator
         *
         * @tparam T Configuration object type (either ClockConfig or RegulatorConfig)
         *
         * @param resId Identifier for this resource (or kResourceIdNone to use resName instead)
         * @param resName Resource name (optional)
         * @param requestedConfig Configuration to apply to this resource
         * @param outActualConfig Actual configuration applied to the device
         * @param timeout How long to wait for a response to the request
         *
         * @return 0 on success or an error code
         */
        template<typename T>
        int setConfig(const uint32_t resId, const etl::string_view resName,
                const T &requestedConfig, T &outActualConfig,
                const TickType_t timeout = portMAX_DELAY) {
            ResourceConfig tmpOut;
            int err = setConfigInternal(resId, resName, requestedConfig, tmpOut, timeout);
            if(err) {
                return err;
            }
            outActualConfig = etl::get<T>(tmpOut);
            return err;
        }


    private:
        /// Info about a device (for mapping ids -> address/ETPZC configs)
        struct DeviceConfig {
            uint32_t id;
            uintptr_t address;
            uint8_t etpzcIndex;

            /// Indicates no ETPZC index available
            constexpr static const uint8_t kNoEtpzcIndex{0xff};
        };

        /// Number of devices we have configuration for
        constexpr static const size_t kNumDeviceConfigs{86};

        Service(Handler *);
        ~Service();

        int setConfigInternal(const uint32_t resId, const etl::string_view resName,
                const ResourceConfig requestedConfig, ResourceConfig &outActualConfig,
                const TickType_t timeout = portMAX_DELAY);

        static int DecodeResponse(etl::span<const uint8_t> rawResponse,
                const ResourceType resType, ResourceConfig &outActualConfig);

        /**
         * @brief Find the base address for a resource device with the given id
         *
         * @param id Resource id to search for
         *
         * @return Base address of the device
         */
        inline uintptr_t GetDeviceAddress(const uint32_t id) {
            for(const auto &record : gDeviceConfig) {
                if(record.id == id) {
                    return record.address;
                }
            }

            // not found
            return 0;
        }

    private:
        /// rpmsg message id for "get config" request
        constexpr static const uint8_t kRpmsgMsgGetConfig{0x00};
        /// rpmsg message id for "set config" request
        constexpr static const uint8_t kRpmsgMsgSetConfig{0x01};
        /// rpmsg message id for an error message
        constexpr static const uint8_t kRpmsgMsgError{0xff};

        /// rpmsg resource id for a clock
        constexpr static const uint8_t kRpmsgResourceClock{0x00};
        /// rpmsg resource id for a voltage regulator
        constexpr static const uint8_t kRpmsgResourceRegulator{0x01};
        constexpr static const uint8_t kRpmsgResourceError{0xff};

        /// Device configuration state
        static const etl::array<DeviceConfig, kNumDeviceConfigs> gDeviceConfig;

        /// Message handler (used to send requests)
        Handler *handler;
        /// Mutex to ensure only one caller at a time can make requests
        SemaphoreHandle_t reqLock{nullptr};
};
}

#endif
