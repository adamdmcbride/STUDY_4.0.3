#ifdef BOARD_USES_HDMI

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include "gralloc_priv.h"
#include "gralloc_helper.h"

#include "hdmi_device.h"

#ifdef USE_GC_CONVERTER
#include "gc_converter.h"
#endif

#include <hardware_legacy/uevent.h>
#include <poll.h>
#include <stdlib.h>

void handle_hdmi_hotplug(private_module_t *module)
{
	if (module->hdmi_fd == 0 || module->hdmi_fd == -1)
		return;

	if (module->hdmi_state == 1) { // plugged
		struct fb_fix_screeninfo hdmi_finfo;
		if (ioctl(module->hdmi_fd, FBIOGET_FSCREENINFO, &hdmi_finfo) == -1) {
			LOGE("failed to get fb info (%d)", errno);
			return;
		}
		struct fb_var_screeninfo hdmi_info;
		if (ioctl(module->hdmi_fd, FBIOGET_VSCREENINFO, &hdmi_info) == -1) {
			LOGE("failed to get fb info (%d)", errno);
			return;
		}
		module->hdmi_finfo = hdmi_finfo;
		module->hdmi_info = hdmi_info;

		LOGD("%s mHdmiPhysAddr=0x%x, mHdmiXRes=0x%x, mHdmiYRes=0x%x, mHdmiBitsPerPixel=0x%x", \
					__FUNCTION__, module->hdmi_finfo.smem_start, module->hdmi_info.xres, module->hdmi_info.yres, module->hdmi_info.bits_per_pixel);

#ifdef USE_GC_CONVERTER
        gcconverter_blit_ui(module, 0);
#endif
        hdmi_post(module, 0);
#ifdef USE_GC_CONVERTER
        gcconverter_blit_ui(module, 1);
#endif
        hdmi_post(module, 1);
	}
}

static void *ns115_hdmi_thread(void *data)
{
	private_module_t *module = (private_module_t *)data;
	static char uevent_desc[4096];
	struct pollfd poll_fd;
	int timeout = -1;
	int err;
	char *s = NULL;
	int state = 0;

	uevent_init();

	poll_fd.fd = uevent_get_fd();
	poll_fd.events = POLLIN;
	memset(uevent_desc, 0, sizeof(uevent_desc));
	do {
		err = poll(&poll_fd, 1, timeout);

		if (poll_fd.revents & POLLIN) {
			/* keep last 2 zeroes to ensure double 0 termination */
			uevent_next_event(uevent_desc, sizeof(uevent_desc) - 2);
			if (strcmp(uevent_desc, "change@/devices/virtual/switch/hdmi"))
				continue;

			s = uevent_desc;
			s += strlen(s) + 1;

			while(*s) {
				if (!strncmp(s, "SWITCH_STATE=", strlen("SWITCH_STATE="))) {
					state = atoi(s + strlen("SWITCH_STATE="));
					module->hdmi_state = state == 1;
					handle_hdmi_hotplug(module);
				}

				s += strlen(s) + 1;
			}
		}
	} while (1);

	return NULL;
}

int hdmi_init(struct private_module_t* module)
{
	int hdmi_fd = -1;
	int i = 0;
	char name[64];

	char const * const device_template[] =
	{
		"/dev/graphics/fb%u",
		"/dev/fb%u",
		NULL
	};

	while ( (hdmi_fd == -1 || hdmi_fd == 0) && device_template[i]) {
		snprintf(name, 64, device_template[i], 1);
		hdmi_fd = open(name, O_RDWR, 0);
		i++;
	}

	if (hdmi_fd <= 0) {
		LOGE("%s: open(%s) fail\n", __func__, name);
		module->hdmi_fd = -1;
		return -1;
	}
	module->hdmi_fd = hdmi_fd;
	module->hdmi_state = 0;

	/* read switch state */
	int sw_fd = open("/sys/class/switch/hdmi/state", O_RDONLY);
	if (sw_fd >= 0) {
		char value;
		if (read(sw_fd, &value, 1) == 1)
			module->hdmi_state = value == '1';
		close(sw_fd);
	}
	handle_hdmi_hotplug(module);

	if (pthread_create(&module->hdmi_thread, NULL, ns115_hdmi_thread, module))
	{
		LOGE("failed to create HDMI listening thread (%d): %m", errno);
		return -1;
	}

	return 0;
}

int hdmi_deinit()
{
	return 0;
}

int hdmi_post(private_module_t *module, int index)
{
	if (!module || module->hdmi_state == 0 || module->hdmi_fd <= 0)
		return 0;

	if (index < 0 || index > 1)
		index = 0;

	LOGV("hdmi_post(index=0x%x)", index);

	module->hdmi_info.activate = FB_ACTIVATE_VBL;
	module->hdmi_info.yoffset = index * module->hdmi_info.yres;

	if (ioctl(module->hdmi_fd, FBIOPAN_DISPLAY, &module->hdmi_info) == -1) {
		LOGE("FBIOPAN_DISPLAY failed");
		return -1;
	}

	if(ioctl(module->hdmi_fd, FBIO_WAITFORVSYNC, 0) < 0) {
		LOGE("FBIO_WAITFORVSYNC failed");
	}
	return 0;
}
#endif
