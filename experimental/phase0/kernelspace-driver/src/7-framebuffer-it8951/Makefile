obj-m+=fb-it8951.o


all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

rpi_overlay:
	dtc -@ -I dts -O dtb -o /boot/overlays/it8951.dtbo overlay/it8951.dts

load_rpi_overlay:
	dtoverlay it8951

unload_rpi_overlay:
	dtoverlay -R it8951

unload_module:
	rmmod fb-it8951.ko

load_module:
	insmod fb-it8951.ko

