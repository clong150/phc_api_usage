CC      := gcc
CFLAGS  := -Wall -Werror -g -O2 -I.
APPS    := phc_example pps_example

.PHONY: all clean

all: $(APPS)

clean:
	rm -f $(APPS) *.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

