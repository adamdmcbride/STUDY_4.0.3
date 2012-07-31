/**
 * @addtogroup app_ConfigMgr
 * @{
 */

/***************************************************************************
 *                                                                         *
 *                   SiRF Technology, a CSR plc Company                    *
 *                           GPS Software                                  *
 *                                                                         *
 *    Copyright (c) 2007-2011 by SiRF Technology, a CSR plc Company        *
 *                        Inc. All rights reserved.                        *
 *                                                                         *
 *    This Software is protected by United States copyright laws and       *
 *    international treaties.  You may not reverse engineer, decompile     *
 *    or disassemble this Software.                                        *
 *                                                                         *
 *    WARNING:                                                             *
 *    This Software contains SiRF Technology Inc.’s confidential and       *
 *    proprietary information. UNAUTHORIZED COPYING, USE, DISTRIBUTION,    *
 *    PUBLICATION, TRANSFER, SALE, RENTAL OR DISCLOSURE IS PROHIBITED      *
 *    AND MAY RESULT IN SERIOUS LEGAL CONSEQUENCES.  Do not copy this      *
 *    Software without SiRF Technology, Inc.’s  express written            *
 *    permission.   Use of any portion of the contents of this Software    *
 *    is subject to and restricted by your signed written agreement with   *
 *    SiRF Technology, Inc.                                                *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *  MODULE:         LSMDemo                                                *
 *                                                                         *
 *  FILENAME:       TagManager.c                                           *
 *                                                                         *
 *  DESCRIPTION:                                                           *
 *                                                                         *
 *  NOTES:                                                                 *
 *                                                                         *
 ***************************************************************************/
#ifndef OS_UCOSII 
#include <memory.h>
#endif /* OS_UCOSII */
#include "TestAppConfigMgr.h" 
#include "TagManager.h"
#include "string_sif.h"
#include "XMLReader.h"
#include "RegistryReader.h"
#include <stdlib.h>

/* Default AGPS Port i.e. 7275, It needs to be converted in bytes for initialization. */
#define DEFAULT_AGPS_PORT {(0x00FF&7275),(7275>>8)} 

/* 
   According to the ANSI standard, initialization of a union must be 
   directed to the first element. An attempt to initialize some other 
   union member may be interpreted by the compiler as a syntactically 
   incorrect initialization of the first element. 
   So typecasting of integers required.
*/
static config_info_t theConfigTable[] =
{
   /* name                      type          value (default) */
   { TAG_GPS_PORT            , TYPE_STR ,    "\\\\.\\com1"                       },
   { TAG_RESET_PORT          , TYPE_STR ,    "\\\\.\\com3"                       },
   { TAG_AUX_PORT            , TYPE_STR ,    "\\\\.\\com2"                       },
   { TAG_SERVER_PORT         , TYPE_INT ,    80                                  },
   { TAG_MODEM_PORT          , TYPE_STR ,    "Modem"                             },
   { TAG_BEARER              , TYPE_STR ,    "Bearer"                            },
   { TAG_APN                 , TYPE_STR ,    "Apn"                               }, 
   { TAG_LOGIN               , TYPE_STR ,    "Login"                             },
   { TAG_PASSWORD            , TYPE_STR ,    "Password"                          },
   { TAG_MODEM               , TYPE_STR ,    "Modem"                             },
   { TAG_PHONE_NUMBER        , TYPE_STR ,    "0123456789"                        },
   { TAG_SESSIONS            , TYPE_INT ,    1                                   },
   { TAG_AGPS_ADDRESS        , TYPE_STR ,    "sls1.sirf.com"                     },
   { TAG_AGPS_PORT           , TYPE_INT ,    DEFAULT_AGPS_PORT                   },
   { TAG_AGPS_SECURITY       , TYPE_INT ,    NO                                  },
   { TAG_RESET_TYPE          , TYPE_INT ,    COLD                                },
   { TAG_AIDING_TYPE         , TYPE_INT ,    NET_AID                             },
   { TAG_POS_TECH_MSB        , TYPE_INT ,    YES                                 },
   { TAG_POS_TECH_MSA        , TYPE_INT ,    NO                                  },
   { TAG_POS_TECH_AUTO       , TYPE_INT ,    NO                                  },
   { TAG_POS_TECH_ECID       , TYPE_INT ,    NO                                  },
   { TAG_PREF_METHOD         , TYPE_INT ,    PREF_MSB                            },
   { TAG_FIX_TYPE            , TYPE_INT ,    SINGLE                              },
   { TAG_FIXES               , TYPE_INT ,    1                                   },
   { TAG_INTERVAL            , TYPE_INT ,    1                                   },
   { TAG_ASSIST_PROTO        , TYPE_INT ,    AP_SUPL_RRLP                        },
   { TAG_APX_LOC             , TYPE_INT ,    0                                   },
   { TAG_APX_LAT             , TYPE_STR ,    "0.0000000000"                      },
   { TAG_APX_LON             , TYPE_STR ,    "0.0000000000"                      },
   { TAG_APX_ALT             , TYPE_INT ,    0                                   },
   { TAG_APX_HERR            , TYPE_INT ,    30000                               },
   { TAG_APX_VERR            , TYPE_INT ,    1000                                },
   { TAG_QOP                 , TYPE_INT ,    ABSENT                              },
   { TAG_HOR_ACCURACY        , TYPE_INT ,    0                                   },
   { TAG_VER_ACCURACY        , TYPE_INT ,    0                                   },
   { TAG_MAX_RESP_TIME       , TYPE_INT ,    0                                   },
   { TAG_MAX_LOC_AGE         , TYPE_INT ,    0                                   },
   { TAG_CGEE                , TYPE_INT ,    ABSENT                              },
   { TAG_SGEE                , TYPE_INT ,    ABSENT                              },
   { TAG_EEFILE              , TYPE_INT ,    NO                                  },
   { TAG_LOG_TYPE            , TYPE_INT ,    NO                                  },
   { TAG_LOG_BRIEF           , TYPE_STR ,    "brieflog.txt                    "  },
   { TAG_LOG_DETAIL          , TYPE_STR ,    "detaillog.txt                   "  },
   { TAG_LOG_AGPS            , TYPE_STR ,    "agpslog.txt                     "  },
   { TAG_LOG_SLC             , TYPE_STR ,    "slclog.txt                      "  },
   { TAG_LOG_SN              , TYPE_STR ,    "sn4log.gps                      "  },
   { TAG_NAV_BIT_AID         , TYPE_INT ,    NO                                  },
   { TAG_SGEE_SERVER_ADR1    , TYPE_STR ,    "instantfix.csr.com              "  },
   { TAG_SGEE_SERVER_ADR2    , TYPE_STR ,    "instantfix.csr.com              "  },
   { TAG_SGEE_SERVER_PORT    , TYPE_INT ,    80                                  },
   { TAG_SGEE_SERVER_FILE    , TYPE_STR ,    "/diff/packedDifference.f2p7enc.ee" },
   { TAG_SGEE_SERVER_AUTH    , TYPE_STR ,    "c2lyZjpTaXJmU2dlZTIwMDU"           },
   { TAG_SGEE_URID_FLAG      , TYPE_INT ,    0                                   },
   { TAG_SGEE_URID_DEVICEID  , TYPE_STR ,    "00000000000000000"                 },
   { TAG_SGEE_URID_OEMSUBID  , TYPE_STR ,    "0000000000000"                     },
   { TAG_NETWORK_STATUS      , TYPE_INT ,    NO                                  },
   { TAG_SIRF_AWARE          , TYPE_INT ,    NO                                  },
   { TAG_LISTENER_NMEA       , TYPE_INT ,    DISABLE                             },
   { TAG_LISTENER_SSB        , TYPE_INT ,    DISABLE                             },
   { TAG_LISTENER_SERIAL     , TYPE_INT ,    DISABLE                             },
   { TAG_LISTENER_DEBUG      , TYPE_INT ,    ENABLE                              },
   {0,0,0}
};

tSIRF_RESULT config_table_init(tSIRF_VOID)
{
   return SIRF_SUCCESS;
}

tSIRF_RESULT config_table_deinit(tSIRF_VOID)
{
   return SIRF_SUCCESS;
}

tSIRF_RESULT config_table_update(eConfigSourceType aType)
{
   tSIRF_UINT8 index = 0;
   tSIRF_RESULT ret = SIRF_FAILURE;
   for (index = 0; theConfigTable[index].name != 0; index++)
   {
      switch(aType)
      {
      case eCONFIG_XML:
         ret = ReadXMLEntry(&(theConfigTable[index]));
         break;
      case eCONFIG_REGISTRY:
         ret = ReadRegistryEntry(&(theConfigTable[index]));
         break;
      default:
         ret = SIRF_FAILURE;
         break;
      }
      if (SIRF_SUCCESS != ret)
      {
         return SIRF_FAILURE;
      }
   } /* for */
   return SIRF_SUCCESS;
}

config_info_t* GetEntryValue(const tSIRF_CHAR* aName)
{
   tSIRF_UINT8 index;

   if(0 == aName) return 0;
   for (index = 0;  (0 != theConfigTable[index].name) ; index++)
   {
      if(0 == strcmp(aName, theConfigTable[index].name))
      {
         return &(theConfigTable[index]);
      }
   }
   return 0;
}

/**
 * @}
 * End of file.
 */
