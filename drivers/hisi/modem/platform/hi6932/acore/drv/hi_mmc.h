#ifndef __HI_MMC_H__
#define __HI_MMC_H__ 
#include "product_config.h"
#define HI6932_MMC 
#if (!defined (BSP_CONFIG_BOARD_TELEMATIC)) && (!defined(BSP_CONFIG_BOARD_E5885Ls_93a))
#define SD_GPIO_DETECT 
#endif
#define MMC_CLOCK_SOURCE (700000000)
#if (FEATURE_ON == MBB_COMMON)
#define MMC0_CAPS (MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ \
                          | MMC_CAP_ERASE | MMC_CAP_SD_HIGHSPEED\
                          | MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 \
                          | MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104)
#else
#define MMC0_CAPS (MMC_CAP_DRIVER_TYPE_A | MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_UHS_SDR104)
#endif
#ifdef BSP_CONFIG_BOARD_TELEMATIC
#define MMC0_CAPS (MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ | MMC_CAP_SD_HIGHSPEED | MMC_CAP_NONREMOVABLE)
#endif
#if defined(BSP_CONFIG_BOARD_E5885Ls_93a)
#define MMC0_CAPS (MMC_CAP_DRIVER_TYPE_A | MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED)
#endif
#define MMC1_CAPS 0
#define MMC_BIU_ENUM_CLOCK (17500000)
#define EMMC_REG_BASE 0x910fc000
#endif
