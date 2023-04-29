SRC = proj2
CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic -lpthread

proj2 : $(SRC).c
	$(CC) $(CFLAGS) $(SRC).c -o proj2 

clean:
	rm -f *.o $(SRC)