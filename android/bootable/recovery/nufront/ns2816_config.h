#ifndef NS2816_CONFIG_H
#define NS2816_CONFIG_H

#include "../common.h"


//define whether open or close recovery debug option
//#define DEBUG_ANDROID_RECOVERY    1


#ifdef DEBUG_ANDROID_RECOVERY
    #define DEBUG_RECOVERY(fmt, args...)          ui_print(fmt, ##args)
#else   
    #define DEBUG_RECOVERY(fmt, args...)
#endif


//define error type of ns2816 recovery system
#define RESTORE_IMG_NONE             -1

#endif
