#ifdef BOARD_USES_HDMI

#ifndef __NS115_HDMI_DEVICE_
#define __NS115_HDMI_DEVICE_

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

int hdmi_init(struct private_module_t* module);
int hdmi_deinit();
int hdmi_post(private_module_t *module, int index);

#endif // __NS115_HDMI_DEVICE_

#endif
