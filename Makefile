NAME    := turndown
BIN_DIR := /usr/local/bin

all: build

build: $(NAME).c
	gcc -o $(NAME) -lao $<
	printf "turndown.wav" >> $(NAME)
	cat turndown.wav >> $(NAME)

test: build
	./$(NAME)

install: $(NAME)
	install -m 0755 $(NAME) $(BIN_DIR)

uninstall:
	rm -f $(BIN_DIR)/$(NAME)

clean:
	rm -f $(NAME)