#ifndef __DISPLAY_REGULATE_H
#define __DISPLAY_REGULATE_H

#define S1280x720p 0
#define S1920x1080p 1

//只在改变hdmi输出分辨率时调用一次
int set_output_display_resolution(int resolution);

//只在改变输入分辨率时调用一次
int init_display_config(unsigned int width, unsigned int hight);

int display_flush_frame(int phy_addY, int phy_addUV);

#endif//__DISPLAY_REGULATE_H