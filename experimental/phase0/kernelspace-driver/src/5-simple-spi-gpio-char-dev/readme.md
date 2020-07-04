for some reason, if the module is not loaded on boot, on the first insmod-rmmod, it will segfault on the rmmod, this issues roots from experimental #1 
source of this experimental code are below:
  1) https://github.com/repk/epd/blob/master/epd_g1.c
  2) https://imxdev.gitlab.io/tutorial/How_to_add_DT_support_for_a_driver/

## notes
- this experiment have all the necessary components for experiment #6
- the character device driver still uses the legacy API, refer to experiment #1 for the more current example