# Day 2
Day 2 was focusing on figuring out why `experimental/phase0/kernelspace-driver/src/7-framebuffer` segfault when the following functions are called:

	static struct fb_ops it8951_ops = {
		...
		.fb_write		= it8951_fb_write,
		.fb_fillrect	= it8951_fb_fillrect,
		.fb_copyarea	= it8951_fb_copyarea,
		.fb_imageblit	= it8951_fb_imageblit,
	};
specifically it segfaults because of `null pointer de-reference` 	when calling `cfb*` functions, such as `cfb_imageblit`. The following is the complete error log from `dmesg`:

```
[  136.389792] fb_it8951: loading out-of-tree module taints kernel.
[  136.396643] Registering SPI device
[  136.396990] [IT8951]: Succeeded getting vcom from DT 1950
[  136.397001] [IT8951]: Succeeded setting SPI speed from DT 8 MHz
[  136.397005] Succeeded getting data from DT
[  136.397008] it8951: Setting-up SPI device
[  136.397018] it8951: Succeeded to setup SPI device
[  136.397022] it8951: initializing
[  138.646914] it8951: panel 800x600
[  138.646932] it8951: FW version = SWv_0.1.
[  138.646938] it8951: LUT version = M641
[  138.647347] it8951: VCOM = 2600
[  138.685497] it8951: VCOM = 1950
[  138.707220] Console: switching to colour frame buffer device 100x37
[  138.707274] it8951: entering it8951_fb_imageblit()
[  138.707310] Unable to handle kernel NULL pointer dereference at virtual address 00000000
[  138.707328] pgd = 417b8f7c
[  138.707335] [00000000] *pgd=0f07f831, *pte=00000000, *ppte=00000000
[  138.707360] Internal error: Oops: 17 [#1] ARM
[  138.707367] Modules linked in: fb_it8951(O+) cmac bnep hci_uart btbcm serdev bluetooth ecdh_generic 8021q garp stp llc spidev brcmfmac brcmutil sha256_generic cfg80211 raspberrypi_hwmon hwmon snd_bcm2835(C) snd_pcm rfkill snd_timer snd bcm2835_codec(C) bcm2835_v4l2(C) v4l2_mem2mem bcm2835_mmal_vchiq(C) v4l2_common videobuf2_dma_contig videobuf2_vmalloc videobuf2_memops videobuf2_v4l2 videobuf2_common spi_bcm2835 videodev media vc_sm_cma(C) fixed uio_pdrv_genirq uio sysimgblt sysfillrect syscopyarea fb_sys_fops ip_tables x_tables ipv6
[  138.707521] CPU: 0 PID: 1374 Comm: insmod Tainted: G         C O      4.19.118+ #1311
[  138.707526] Hardware name: BCM2835
[  138.707562] PC is at cfb_imageblit+0x1d0/0x980
[  138.707571] LR is at 0xd95a1000
[  138.707578] pc : [<c042991c>]    lr : [<d95a1000>]    psr: 60000013
[  138.707585] sp : cf0af848  ip : 00000000  fp : cf0af89c
[  138.707591] r10: 00000000  r9 : 00000001  r8 : d0b5e800
[  138.707599] r7 : 00000008  r6 : 00000020  r5 : d32ebe40  r4 : d0b5e800
[  138.707606] r3 : 00000000  r2 : 0000000f  r1 : 00000000  r0 : d0b5e800
[  138.707615] Flags: nZCv  IRQs on  FIQs on  Mode SVC_32  ISA ARM  Segment user
[  138.707623] Control: 00c5387d  Table: 0f088008  DAC: 00000055
[  138.707633] Process insmod (pid: 1374, stack limit = 0x471f16ed)
[  138.707641] Stack: (0xcf0af848 to 0xcf0b0000)
[  138.707655] f840:                   c006750c c0066a18 00000000 00000001 00000037 c0a2d028
[  138.707670] f860: d32ebe40 00000000 c0066e2c d970b800 cf0af894 d0b5e800 d32ebe40 d0b5e800
[  138.707685] f880: 00000010 00000001 00000001 d4d76000 cf0af8b4 cf0af8a0 bf613d38 c0429758
[  138.707701] f8a0: d32ebe40 cf0af914 cf0af8f4 cf0af8b8 c04251cc bf613d10 00000010 cf0af8c8
[  138.707715] f8c0: 00000000 d32ebe78 00000000 d4d11000 cf0af914 00000000 00000000 00000008
[  138.707731] f8e0: d0b5e800 c0a2d028 cf0af98c cf0af8f8 c0424bd4 c042505c c00642e0 00000000
[  138.707746] f900: 00000000 00000002 00000010 00000002 c00d5518 0000003b 80000001 d32315e0
[  138.707759] f920: 00000000 00000000 000001d0 00000008 00000010 00000000 0000000f cf0af901
[  138.707775] f940: c07393bc c0089650 cf0af964 cf0af958 c0066474 c00f0ed0 cf0af9ac 093fa966
[  138.707791] f960: cf0af98c d702c400 d0b5e800 00000002 c0424684 0000f072 00000000 00000000
[  138.707806] f980: cf0af9c4 cf0af990 c041e7b0 c0424690 00000000 0000000f c08a4b34 d702c400
[  138.707820] f9a0: c0a8d53c c0a2d028 00000000 00000001 c08a4b34 0000003f cf0af9dc cf0af9c8
[  138.707836] f9c0: c0460cc0 c041e698 d702c400 00000000 cf0afa14 cf0af9e0 c04632bc c0460c8c
[  138.707851] f9e0: c0066e2c c00674c8 cf0afa0c 093fa966 00000000 00000000 c0b6d2b8 00000080
[  138.707866] fa00: 00000001 c08a4b34 cf0afa6c cf0afa18 c0463f38 c0463158 00000025 00000000
[  138.707880] fa20: 0000003e 00000001 00000001 c08a4b34 00000000 00000000 00000005 0000003e
[  138.707895] fa40: cf0afa78 c073d5b4 c08a92b8 00000000 00000000 0000003e 00000001 c0b6d2d4
[  138.707911] fa60: cf0afab4 cf0afa70 c04643b8 c0463c24 c0a8d564 c08a92b8 00000001 c0217d44
[  138.707926] fa80: c0217bb8 00000000 00000000 c0a8a624 c0b6cf14 00000005 00000001 c0b6cf14
[  138.707942] faa0: c0b6af14 d0b5e810 cf0afacc cf0afab8 c0421f6c c0464284 c0a8a624 d0b5e800
[  138.707957] fac0: cf0afb0c cf0afad0 c0422d40 c0421f00 00001000 00000000 c0717f68 c0a8a614
[  138.707972] fae0: 00000000 00000000 cf0afb9c 00000005 ffffffff 00000000 c0b6af14 d0b5e810
[  138.707988] fb00: cf0afb34 cf0afb10 c0044cdc c0422504 ffffffff ffffffff c0a8a538 cf0afb9c
[  138.708003] fb20: 00000005 c0a2db88 cf0afb5c cf0afb38 c0045194 c0044c94 00000000 c00d6618
[  138.708018] fb40: c0a2db8c d0b5e800 00000000 c0a2d028 cf0afb74 cf0afb60 c00451d4 c0045148
[  138.708033] fb60: 00000000 cf0afb70 cf0afb84 cf0afb78 c04151e0 c00451b8 cf0afc0c cf0afb88
[  138.708049] fb80: c041745c c04151c8 c08a4614 00000001 006080c0 d0b5ea18 cf0afaf0 d0b5e800
[  138.708063] fba0: ffffffff 00000000 00000000 00000320 00000258 00000000 00000000 00000000
[  138.708077] fbc0: 00000000 00000000 00000000 00000000 00000000 00000000 00000020 093fa966

Message from syslogd@raspberrypi at Jul 27 03:50:30 ...
 kernel:[  138.707360] Internal error: Oops: 17 [#1] ARM

Message from syslogd@raspberrypi at Jul 27 03:50:30 ...
 kernel:[  138.707633] Process insmod (pid: 1374, stack limit = 0x471f16ed)

Message from syslogd@raspberrypi at Jul 27 03:50:30 ...
 kernel:[  138.707641] Stack: (0xcf0af848 to 0xcf0b0000)
[  138.708092] fbe0: d4052800 d4052800 d0b5eb18 c0a2d028 d0b5e800 001d4c00 00000000 00000002
[  138.708108] fc00: cf0afc54 cf0afc10 bf6137f4 c0417240 00000000 00000002 d4052800 d0b5eb24
[  138.708124] fc20: 0002fc44 093fa966 c049d184 bf61607c d4052800 00000000 c0b6e578 bf61608c
[  138.708139] fc40: 00000013 00000000 cf0afc74 cf0afc58 c04dc1d8 bf6133b0 c04dc150 c0b6e574
[  138.708154] fc60: d4052800 00000000 cf0afca4 cf0afc78 c048d910 c04dc15c 00000000 d4052800
[  138.708170] fc80: bf61608c d4052834 c0a2d028 d09319c0 00000002 d0931a00 cf0afcdc cf0afca8
[  138.708186] fca0: c048db70 c048d6d8 c059ca50 c059a71c cf0afcdc c07036a4 d4052800 bf61608c
[  138.708201] fcc0: d4052834 c0a2d028 d09319c0 00000002 cf0afcfc cf0afce0 c048dd84 c048db10
[  138.708216] fce0: 00000000 bf61608c bf61608c c048dc94 cf0afd2c cf0afd00 c048b7f8 c048dca0
[  138.708233] fd00: cf0afd38 d713c7cc d4018510 093fa966 bf61608c d32ebcc0 c0a97418 00000000
[  138.708249] fd20: cf0afd3c cf0afd30 c048d180 c048b790 cf0afd64 cf0afd40 c048cbdc c048d164
[  138.708264] fd40: bf615758 cf0afd50 bf61608c bf619000 c0a2d028 00000000 cf0afd7c cf0afd68
[  138.708280] fd60: c048e52c c048ca54 bf6160e0 bf619000 cf0afd8c cf0afd80 c04dc0f8 c048e4ac
[  138.708296] fd80: cf0afd9c cf0afd90 bf619028 c04dc0ac cf0afe14 cf0afda0 c000aea8 bf61900c
[  138.708312] fda0: c013487c c0a2d028 d09319c0 00000002 cf0afddc cf0afdc0 c00d5518 c00d6618
[  138.708327] fdc0: 0000f07c d7ea6b70 20000013 c0a2d028 cf0afdfc c0184150 cf0afe14 cf0afde8
[  138.708343] fde0: c0184150 c0190564 d4c91680 093fa966 bf6160e0 bf6160e0 bf6160e0 d4c91680
[  138.708360] fe00: c0a2d028 d09319c0 cf0afe3c cf0afe18 c00943ac c000ae68 cf0aff30 bf6160e0
[  138.708375] fe20: cf0afe3c cf0aff30 bf6160e0 00000002 cf0aff0c cf0afe40 c0093218 c009434c
[  138.708390] fe40: bf6160ec 00007fff bf6160e0 c00909f0 00000000 d959b000 bf6161d4 bf6162b4
[  138.708406] fe60: bf61af10 bf617000 00000000 bf616128 cf0afe94 c08f1fac c01994e0 c019934c
[  138.708420] fe80: 00004748 00000000 00000000 00000000 00000000 00000000 00000000 00000000
[  138.708434] fea0: 6e72656b 00006c65 00000000 00000000 00000000 00000000 00000000 00000000
[  138.708448] fec0: 00000000 00000000 00000000 00000000 00000000 00000000 00000000 093fa966
[  138.708463] fee0: 7fffffff c0a2d028 00000000 00000003 0002d064 7fffffff 00000000 00000000
[  138.708478] ff00: cf0affa4 cf0aff10 c0093bb8 c009178c 7fffffff 00000000 00000003 00000000
[  138.708493] ff20: 00000000 d959b000 00004748 00000000 d959c6b9 d959d540 d959b000 00004748
[  138.708507] ff40: d959f0b8 d959eef8 d959e1a8 00004000 000044c0 00000000 00000000 00000000
[  138.708522] ff60: 00001f10 00000027 00000028 0000001f 0000001c 00000018 00000000 093fa966
[  138.708537] ff80: f90b8e00 be996804 0003fce8 0000017b c00091a4 cf0ae000 00000000 cf0affa8
[  138.708553] ffa0: c0009000 c0093b0c f90b8e00 be996804 00000003 0002d064 00000000 b6f50d64
[  138.708568] ffc0: f90b8e00 be996804 0003fce8 0000017b 00bad818 00000000 00000002 00000000
[  138.708583] ffe0: be996638 be996628 00022cb8 b6c2caf0 60000010 00000003 00000000 00000000
[  138.708655] [<c042991c>] (cfb_imageblit) from [<bf613d38>] (it8951_fb_imageblit+0x34/0x60 [fb_it8951])
[  138.708723] [<bf613d38>] (it8951_fb_imageblit [fb_it8951]) from [<c04251cc>] (soft_cursor+0x17c/0x1e0)
[  138.708745] [<c04251cc>] (soft_cursor) from [<c0424bd4>] (bit_cursor+0x550/0x58c)
[  138.708763] [<c0424bd4>] (bit_cursor) from [<c041e7b0>] (fbcon_cursor+0x124/0x184)
[  138.708783] [<c041e7b0>] (fbcon_cursor) from [<c0460cc0>] (hide_cursor+0x40/0xa4)
[  138.708805] [<c0460cc0>] (hide_cursor) from [<c04632bc>] (redraw_screen+0x170/0x254)
[  138.708824] [<c04632bc>] (redraw_screen) from [<c0463f38>] (do_bind_con_driver+0x320/0x3bc)
[  138.708842] [<c0463f38>] (do_bind_con_driver) from [<c04643b8>] (do_take_over_console+0x140/0x1e0)
[  138.708860] [<c04643b8>] (do_take_over_console) from [<c0421f6c>] (do_fbcon_takeover+0x78/0xe4)
[  138.708879] [<c0421f6c>] (do_fbcon_takeover) from [<c0422d40>] (fbcon_event_notify+0x848/0x8c0)
[  138.708912] [<c0422d40>] (fbcon_event_notify) from [<c0044cdc>] (notifier_call_chain+0x54/0x94)
[  138.708937] [<c0044cdc>] (notifier_call_chain) from [<c0045194>] (__blocking_notifier_call_chain+0x58/0x70)
[  138.708957] [<c0045194>] (__blocking_notifier_call_chain) from [<c00451d4>] (blocking_notifier_call_chain+0x28/0x30)
[  138.708995] [<c00451d4>] (blocking_notifier_call_chain) from [<c04151e0>] (fb_notifier_call_chain+0x24/0x2c)
[  138.709020] [<c04151e0>] (fb_notifier_call_chain) from [<c041745c>] (register_framebuffer+0x228/0x2f8)
[  138.709060] [<c041745c>] (register_framebuffer) from [<bf6137f4>] (it8951_probe+0x450/0x53c [fb_it8951])
[  138.709107] [<bf6137f4>] (it8951_probe [fb_it8951]) from [<c04dc1d8>] (spi_drv_probe+0x88/0xb4)
[  138.709133] [<c04dc1d8>] (spi_drv_probe) from [<c048d910>] (really_probe+0x244/0x2d4)
[  138.709152] [<c048d910>] (really_probe) from [<c048db70>] (driver_probe_device+0x6c/0x190)
[  138.709169] [<c048db70>] (driver_probe_device) from [<c048dd84>] (__driver_attach+0xf0/0xf4)
[  138.709186] [<c048dd84>] (__driver_attach) from [<c048b7f8>] (bus_for_each_dev+0x74/0xc4)
[  138.709203] [<c048b7f8>] (bus_for_each_dev) from [<c048d180>] (driver_attach+0x28/0x30)
[  138.709219] [<c048d180>] (driver_attach) from [<c048cbdc>] (bus_add_driver+0x194/0x21c)
[  138.709237] [<c048cbdc>] (bus_add_driver) from [<c048e52c>] (driver_register+0x8c/0x124)
[  138.709256] [<c048e52c>] (driver_register) from [<c04dc0f8>] (__spi_register_driver+0x58/0x6c)
[  138.709288] [<c04dc0f8>] (__spi_register_driver) from [<bf619028>] (it8951_init+0x28/0x1000 [fb_it8951])
[  138.709336] [<bf619028>] (it8951_init [fb_it8951]) from [<c000aea8>] (do_one_initcall+0x4c/0x190)
[  138.709361] [<c000aea8>] (do_one_initcall) from [<c00943ac>] (do_init_module+0x6c/0x1f0)
[  138.709382] [<c00943ac>] (do_init_module) from [<c0093218>] (load_module+0x1a98/0x2220)
[  138.709400] [<c0093218>] (load_module) from [<c0093bb8>] (sys_finit_module+0xb8/0xcc)
[  138.709417] [<c0093bb8>] (sys_finit_module) from [<c0009000>] (ret_fast_syscall+0x0/0x28)
[  138.709424] Exception stack(0xcf0affa8 to 0xcf0afff0)
[  138.709438] ffa0:                   f90b8e00 be996804 00000003 0002d064 00000000 b6f50d64
[  138.709453] ffc0: f90b8e00 be996804 0003fce8 0000017b 00bad818 00000000 00000002 00000000
[  138.709463] ffe0: be996638 be996628 00022cb8 b6c2caf0
[  138.709480] Code: 151b303c 05921010 05922014 15934010 (07934101) 
[  138.709492] ---[ end trace e1e1eb906f1ddf30 ]---

```

### Things I found out:
 - in function `it8951_fb_imageblit(struct  fb_info *info, const  struct  fb_image *image)`
	 - both `info` and `image` is initialized and not null 
	 - there are few pointers under image that have not been checked that might be null.
 - have tried recompile the kernel and adding the driver as a built-in and it's still segfault
 - might need to do some deeper debugging in the kernel to figure out at which line the `cfb*` function segfaults
	 - to enable this might need to recompile the kernel and enable debug mode
	 - recompiling the kernel should follow this [documentation](https://www.raspberrypi.org/documentation/linux/kernel/building.md)
 - there might something that I initialized incorrectly when setting up the framebuffer objects

### Things I have done:
 - renaming `7-framebuffer` to `7-framebuffer-it8951`
 - renaming `simple-fb.c` to `fb-it8951.c`
 - cleaning-up `fb-it8951.c`
 - use `cfb*` functions instead of `sys*` ( in the future, it's better to use `sys*` )

## Next Development Step
At this point, it's better to continue working on the `tinydrm_it8951` driver and skip `fb-it8951` exploration. Definitely, would love to get back to this in the future. 

  - update `readme.md` for `experimental/phase0/kernelspace-driver/readme.md`, `experimental/phase0/kernelspace-driver/resource`, and `experimental/phase0/kernelspace-driver/src/7-framebuffer-it8951/readme.md`
  - directory structure clean-up as mentioned in `day-1.md`
  - `Next Development Step` from `day-1.md`

## Notes
Here are some useful command when testing framebuffer:
  - Display image to a framebuffer
    - `sudo fbi -T 2 -d /dev/fb1 samples/test_pano.jpg`
  - Show framebuffer information
    - `fbset -fb /dev/fb0 -i`