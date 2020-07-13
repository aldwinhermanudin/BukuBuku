CC=g++
TARGET=open-image data-format-for-it8951


open-image: open-image.cpp
	$(CC) -Wall -ggdb open-image.cpp -o open-image -std=c++14 `pkg-config --cflags --libs opencv`

data-format-for-it8951: data-format-for-it8951.cpp
	$(CC) -Wall -ggdb data-format-for-it8951.cpp -o data-format-for-it8951 -std=c++14 `pkg-config --cflags --libs opencv`

all: open-image data-format-for-it8951

clean:
	rm -f $(TARGET)
