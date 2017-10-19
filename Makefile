CC = gcc
CFLAGS = -Wall -g
all: libsdt-activator.so main

main: main.c hello_provider.o hello_provider.h
	$(CC) $(CFLAGS) -o $@ main.c -ldl hello_provider.o lttng-elf.o

hello_provider.o: hello_provider.d
	dtrace -s $< -o $@ -G

hello_provider.h: hello_provider.d
	dtrace -s $< -o $@ -h
sdt-activator.o: sdt-activator.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

lttng-elf.o: lttng-elf.c lttng-elf.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

libsdt-activator.so: sdt-activator.o lttng-elf.o
	$(CC) -shared $(CFLAGS) -Wl,-soname,libsdt-activator.so -o $@ -ldl $^

.PHONY: clean
clean:
	rm -f *.o *.so main hello_provider.h
