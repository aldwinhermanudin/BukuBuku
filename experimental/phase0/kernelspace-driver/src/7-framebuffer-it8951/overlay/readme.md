compile and load overlay is by:
    1) to compile run : dtc -@ -Hepapr -I dts -O dtb -o ws2in13b-spi.dtbo ws2in13b-spi.dts
    2) to load dtbo run : sudo dtoverlay -v -d . ws2in13b-spi.dtbo

other alternative to load dtbo is by loading it to /boot/config.txt
    1) copy .dtbo to /boot/overlays/
    2) add the following to the end of /boot/config.txt
        dtoverlay=ws2in13b-spi
    3) reboot. 

