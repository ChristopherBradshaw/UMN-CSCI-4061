CFLAGS += -Wall
LDFLAGS += -lcrypto

SRC := md5_test.c

MD5_SRC := md5sum.c
MD5_OBJ := $(MD5_SRC:.c=.o)

TARGET := $(SRC:.c=)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(MD5_OBJ) md5sum.h
	$(CC) $(CFLAGS) -o $@ $< $(SRC) $(LDFLAGS)

clean:
	-$(RM) $(TARGET) $(MD5_OBJ)
