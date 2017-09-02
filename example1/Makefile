CC=gcc-7
CFLAGS=-g -c -Wall -Wextra -std=c++1z -O4 -DDEBUG
NAME=example1
LDFLAGS=-lstdc++
OBJ=obj
SRC=src
TARGET=target

all: $(NAME)

$(NAME): main.o
	$(CC) $(OBJ)/*.o -o $(TARGET)/$(NAME) $(LDFLAGS)

%.o: $(SRC)/%.cpp
	$(CC) $(CFLAGS) $< -o $(OBJ)/$@

clean:
	rm -rf $(OBJ)
	mkdir $(OBJ)

run:
	./$(TARGET)/$(NAME)

leakTest:
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./$(TARGET)/$(NAME)
