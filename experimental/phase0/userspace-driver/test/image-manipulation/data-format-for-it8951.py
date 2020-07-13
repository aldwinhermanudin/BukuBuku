from PIL import Image, ImageChops, ImageDraw, ImageFont, ImageOps

## image = Image.open("04.bmp")
image = Image.open("test.png")
# image = ImageOps.flip(image)
image_width, image_height = image.size
print ("Width: {}, Height: {}". format(image_width, image_height))

## driver_width, driver_height = (800,600)
## output_image = ImageOps.pad(image, (driver_width, driver_height), color=0x0)
## print ("Original image mode: {}". format(output_image.mode))
## output_image.show()

## image_grey = output_image.convert("1")
image_grey = image.convert("L")
frame_buffer = list(image_grey.getdata())
print ("Converted image mode: {}". format(image_grey.mode))

# For now, only 4 bit packing is supported. Theoretically we could
# achieve a transfer speed up by using 2 bit packing for black and white
# images. However, 2bpp doesn't seem to play well with the DU rendering
# mode.
packed_buffer = []

# The driver board assumes all data is read in as 16bit ints. To match
# the endianness every pair of bytes must be swapped.
# for example, 0xabcd where a,b,c,d is 4 different pixel with 4-bit greyscale
# need to be swapped to 0xdcba. where 0xdcba is 1 SPI transfer. this mean every
# 1 SPI transfer = 2 pixel.
# The image is always padded to a multiple of 8, so we can safely 
# go in steps of 4. which means 0,4,8,12..
for i in range(0, len(frame_buffer), 4):
    # Values are in the range 0..255, so we don't need to "and" after we shift
    # converting 8-bit greyscale to 4-bit greyscale by removing first
    # 4 least significant bit.
    packed_buffer += [(frame_buffer[i + 2] >> 4) | (frame_buffer[i + 3] & 0xF0)]
    packed_buffer += [(frame_buffer[i] >> 4) | (frame_buffer[i + 1] & 0xF0)]

print("\n")
# print (frame_buffer)
print("Original Framebuffer")
for num in frame_buffer:
    print("{} ".format(hex(num)[2:]),end="")
print("\n")

# print (packed_buffer)
print("Packed Framebuffer")
for num in packed_buffer:
    print("{} ".format(hex(num)[2:]),end="")
print("\n")
## image_grey.show()