from PIL import Image, ImageChops, ImageDraw, ImageFont, ImageOps

image = Image.open("04.bmp")
# image = ImageOps.flip(image)
image_width, image_height = image.size
print ("Width: {}, Height: {}". format(image_width, image_height))

driver_width, driver_height = (800,600)
output_image = ImageOps.pad(image, (driver_width, driver_height), color=0x0)
print ("Original image mode: {}". format(output_image.mode))
output_image.show()

## image_grey = output_image.convert("1")
image_grey = output_image.convert("L")
print ("Converted image mode: {}". format(image_grey.mode))

image_grey.show()