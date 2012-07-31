#ifdef USE_GC_CONVERTER

#ifndef __NS115_GC_CONVERTER_
#define __NS115_GC_CONVERTER_

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

int gcconverter_init(struct private_module_t* module);
int gcconverter_deinit();
int gcconverter_blit_ui(struct private_module_t *module, int index);
int gcconverter_start_blit_video(uint32_t srcPhysAddr, uint32_t srcWidth, uint32_t srcHeight, uint32_t srcFormat,
								int srcCropLeft, int srcCropTop, int srcCropRight, int srcCropBottom,
								uint32_t *pdstPhysAddr, uint32_t *pdstWidth, uint32_t *pdstHeight, uint32_t *pdstFormat);
int gcconverter_toggle_video_mode(int state);

#endif // __NS115_GC_CONVERTER_

#endif
