# 找出所有 .c 檔案
SRCS = $(wildcard *.c)

# 把 .c 轉成 .out執行檔（比如 7+3008.c --> 7_3008）
TARGETS = $(SRCS:.c=)

# 編譯器設定
CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lm

# 預設目標：build所有
all: $(TARGETS)

# 每個.c怎麼編成自己的執行檔
%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# 清除所有執行檔
clean:
	rm -f $(TARGETS)

