# Compiler variables
CC = gcc
CFLAGS = -g -Wall -Wextra -std=c17 -D_POSIX_C_SOURCE=200809L
#CFLAGS += -fsanitize=thread
LDFLAGS = -lncurses

# Directory variables
OBJ_DIR = obj
BIN_DIR = bin
INCLUDE_DIR = include
CLIENT_DIR = src/client
SERVER_DIR = src/server

# executable 
TARGET = Pacmanist

#client
CLIENT = client

#server
SERVER = server


#Client objects
OBJS_CLIENT = client_main.o debug.o api.o display.o

#Server objects
OBJS_SERVER = server_main.o board.o game.o parser.o

# Dependencies
display.o = display.h
board.o = board.h
parser.o = parser.h
api.o = api.h protocol.h
game.o = game.h

# Object files path
vpath %.o $(OBJ_DIR)
vpath %.c $(CLIENT_DIR) $(INCLUDE_DIR)
vpath %.c $(SERVER_DIR) $(INCLUDE_DIR)

# Make targets
all: client server

client: $(BIN_DIR)/$(CLIENT)

$(BIN_DIR)/$(CLIENT): $(OBJS_CLIENT) | folders
	$(CC) $(CFLAGS) $(addprefix $(OBJ_DIR)/,$(OBJS_CLIENT)) -o $@ $(LDFLAGS)

# dont include LDFLAGS in the end, to allow compilation on macos
%.o: %.c $($@) | folders
	$(CC) -I $(INCLUDE_DIR) $(CFLAGS) -o $(OBJ_DIR)/$@ -c $<

server: $(BIN_DIR)/$(SERVER)

$(BIN_DIR)/$(SERVER): $(OBJS_SERVER) | folders
	$(CC) $(CFLAGS) $(addprefix $(OBJ_DIR)/,$(OBJS_SERVER)) -o $@ $(LDFLAGS)

# Create folders
folders:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(BIN_DIR)

# Clean object files and executable
clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(BIN_DIR)/$(TARGET)
	rm -f $(BIN_DIR)/$(CLIENT)
	rm -f $(BIN_DIR)/$(SERVER)

# indentify targets that do not create files
.PHONY: all clean run folders
