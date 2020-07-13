SRCS=basic-test.cpp
CC=g++
TARGET=basic-test

$(TARGET):$(SRCS)
	$(CC) -Wall $(SRCS) -o $(TARGET) -lbcm2835 -std=c++14 `pkg-config --cflags --libs opencv`
	# $(CC) -Wall $(SRCS) -o $(TARGET) -lbcm2835 -std=c++14
	
clean:
	rm -f $(TARGET)
