CC     = gcc
CFLAGS = -Ofast -Wall -Wextra

SRCS   = main.c board.c attacks.c random.c hash.c moves.c eval.c tt.c search.c uci.c perft.c bitboard.c
OBJDIR = release
OBJS   = $(addprefix $(OBJDIR)/, $(SRCS:.c=.o))
TARGET = bbc.exe

# ---------- default build ----------
all: $(OBJDIR) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

# ---------- debug build ----------
DBGDIR = debug
DBGOBJS = $(addprefix $(DBGDIR)/, $(SRCS:.c=.o))

debug: $(DBGDIR) bbc_debug.exe

bbc_debug.exe: $(DBGOBJS)
	$(CC) -O0 -g -Wall -Wextra -o $@ $^

$(DBGDIR)/%.o: %.c | $(DBGDIR)
	$(CC) -O0 -g -Wall -Wextra -c $< -o $@

$(DBGDIR):
	mkdir -p $(DBGDIR)

# ---------- legacy single-file targets ----------
legacy:
	$(CC) -Ofast bbc.c -o bbc_legacy.exe

dll:
	$(CC) -shared -fPIC -o bbc.dll bbc.c

ANN:
	$(CC) -Ofast ANN.c -o ANN.exe

# ---------- utility ----------
clean:
	rm -rf $(OBJDIR) $(DBGDIR) $(TARGET) bbc_debug.exe bbc_legacy.exe bbc_new.exe

.PHONY: all debug legacy dll ANN clean
