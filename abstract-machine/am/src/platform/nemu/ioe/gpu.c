#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init() {
  // int i;
  // int w = 400;
  // int h = 300;
  // uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  // for (i = 0; i < w * h; i++)
  //   fb[i] = i;
  // outl(SYNC_ADDR, 1);
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  int screen_size = inl(VGACTL_ADDR);
  int screen_w = screen_size >> 16;
  int screen_h = screen_size & 0xffff;
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = screen_w, .height = screen_h,
    .vmemsz = screen_w * screen_h * 32
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
  int w = ctl->w;
  int h = ctl->h;
  int x = ctl->x;
  int y = ctl->y;
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  uint32_t *pixels = ctl->pixels;
  int screen_w = inl(VGACTL_ADDR) >> 16;
  for (int i = x; i < x + w; i ++) {
    for (int j = y; j < y + h; j ++) {
      fb[screen_w*j + i] = pixels[(j-y)*w + (i-x)];
    }
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
