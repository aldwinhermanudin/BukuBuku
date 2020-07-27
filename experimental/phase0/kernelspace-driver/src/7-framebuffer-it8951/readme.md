source of this experimental code are below:
  1) [tinydrm_it8951 DRM driver](https://github.com/aldwinhermanudin/tinydrm_it8951)
  2) [Simple Framebuffer driver for non-it8951 e-paper](https://github.com/aldwinhermanudin/hermod)https://imxdev.gitlab.io/tutorial/How_to_add_DT_support_for_a_driver/ 
  [Related documentation](https://blog.react0r.com/2019/09/20/prototype-low-power-low-cost-linux-terminal-device/)
  3) [userspace driver for IT8951](https://github.com/aldwinhermanudin/PaperTTY)
  4) [Another simple non-IT8951 kernel driver](https://github.com/aldwinhermanudin/mangOH/tree/master/experimental/waveshare_eink/linux_kernel_modules)

## what-works
 - framebuffer device is created under /dev/fbX
 - able to display images using fbi
 - sending random data works using cat /dev/urandom> /dev/fb1

## todo
 - [ ] rename 7-framebuffer to 7-it8951
 - [ ] rename simple-fb.c to it8951.c
 - [ ] need to fix kernel segfault when using fbcon from this [tutorial](https://github.com/aldwinhermanudin/tinydrm_it8951/blob/master/README.md)
 - [ ] need to test it8951_fb_fillrect, it8951_fb_copyarea, and it8951_fb_imageblit
 - [ ] clean-up for documentation