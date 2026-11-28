#ifndef VFD_H
#define VFD_H

#include <Arduino.h>

// These symbols are generated from the files specified in platformio.ini under 'board_build.embed_files'
// See https://docs.platformio.org/en/latest/platforms/espressif32.html#embedding-binary-data for more info

extern const byte vfd_0_start[] asm("_binary_images_ClockWidget_vfd_vfd_0_jpg_start");
extern const byte vfd_0_end[] asm("_binary_images_ClockWidget_vfd_vfd_0_jpg_end");
extern const byte vfd_1_start[] asm("_binary_images_ClockWidget_vfd_vfd_1_jpg_start");
extern const byte vfd_1_end[] asm("_binary_images_ClockWidget_vfd_vfd_1_jpg_end");
extern const byte vfd_2_start[] asm("_binary_images_ClockWidget_vfd_vfd_2_jpg_start");
extern const byte vfd_2_end[] asm("_binary_images_ClockWidget_vfd_vfd_2_jpg_end");
extern const byte vfd_3_start[] asm("_binary_images_ClockWidget_vfd_vfd_3_jpg_start");
extern const byte vfd_3_end[] asm("_binary_images_ClockWidget_vfd_vfd_3_jpg_end");
extern const byte vfd_4_start[] asm("_binary_images_ClockWidget_vfd_vfd_4_jpg_start");
extern const byte vfd_4_end[] asm("_binary_images_ClockWidget_vfd_vfd_4_jpg_end");
extern const byte vfd_5_start[] asm("_binary_images_ClockWidget_vfd_vfd_5_jpg_start");
extern const byte vfd_5_end[] asm("_binary_images_ClockWidget_vfd_vfd_5_jpg_end");
extern const byte vfd_6_start[] asm("_binary_images_ClockWidget_vfd_vfd_6_jpg_start");
extern const byte vfd_6_end[] asm("_binary_images_ClockWidget_vfd_vfd_6_jpg_end");
extern const byte vfd_7_start[] asm("_binary_images_ClockWidget_vfd_vfd_7_jpg_start");
extern const byte vfd_7_end[] asm("_binary_images_ClockWidget_vfd_vfd_7_jpg_end");
extern const byte vfd_8_start[] asm("_binary_images_ClockWidget_vfd_vfd_8_jpg_start");
extern const byte vfd_8_end[] asm("_binary_images_ClockWidget_vfd_vfd_8_jpg_end");
extern const byte vfd_9_start[] asm("_binary_images_ClockWidget_vfd_vfd_9_jpg_start");
extern const byte vfd_9_end[] asm("_binary_images_ClockWidget_vfd_vfd_9_jpg_end");
extern const byte vfd_colon_off_start[] asm("_binary_images_ClockWidget_vfd_vfd_colon_off_jpg_start");
extern const byte vfd_colon_off_end[] asm("_binary_images_ClockWidget_vfd_vfd_colon_off_jpg_end");
extern const byte vfd_colon_on_start[] asm("_binary_images_ClockWidget_vfd_vfd_colon_on_jpg_start");
extern const byte vfd_colon_on_end[] asm("_binary_images_ClockWidget_vfd_vfd_colon_on_jpg_end");
#if USE_CLOCK_VFD_BLANK
extern const byte vfd_blank_start[] asm("_binary_images_ClockWidget_vfd_vfd_blank_jpg_start");
extern const byte vfd_blank_end[] asm("_binary_images_ClockWidget_vfd_vfd_blank_jpg_end");
#endif

#endif
