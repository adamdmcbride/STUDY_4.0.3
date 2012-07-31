#ifndef __MODEM_LOG_CONTROLER___H___
#define __MODEM_LOG_CONTROLER___H___


extern int do_open();
extern int initSDCard();
extern int do_close();
extern int waitTtyDev();
extern int do_clearLogs();

#define USE_MOUNT_IN_PROGRAM 0

#endif

