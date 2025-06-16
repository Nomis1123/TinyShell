CFLAGS = -g -Wall -Wextra -Werror -fsanitize=address,leak,object-size,bounds-strict,undefined -fsanitize-address-use-after-scope

all: mysh

mysh: mysh.o builtins.o variables.o io_helpers.o
	gcc ${CFLAGS} -o $@ $^

mysh.o: mysh.c builtins.h variables.h io_helpers.h
	gcc ${CFLAGS} -c $<

builtins.o: builtins.c builtins.h
	gcc ${CFLAGS} -c $<

variables.o: variables.c variables.h
	gcc ${CFLAGS} -c $<

io_helpers.o: io_helpers.c io_helpers.h variables.h
	gcc ${CFLAGS} -c $<

clean:
	rm -f *.o mysh