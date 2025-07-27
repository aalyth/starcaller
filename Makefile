# MAKEFLAGS += -j

TARGET := star

INCDIR := include
OBJDIR := output
SRCDIR := src

SOURCES := $(shell find $(SRCDIR) -type f -name '*.c') main.c
OBJECTS := $(SOURCES:%=$(OBJDIR)/%.o)

CC := clang
STD := c17

WARNING_FLAGS := -pedantic -Werror -Wall -Wextra -Wpedantic -Wformat=2 \
	-Wformat-security -Wformat-signedness -Wnull-dereference \
	-Wstack-protector -Walloca -Wvla -Warray-bounds \
	-Wimplicit-fallthrough -Wstrict-prototypes -Wold-style-definition \
	-Wmissing-prototypes -Wmissing-declarations -Wredundant-decls \
	-Wnested-externs -Wpointer-arith -Wbad-function-cast -Wc++-compat \
	-Wcast-qual -Wwrite-strings -Wconversion -Wsign-conversion \
	-Wfloat-conversion -Wshadow -Wmissing-variable-declarations \
	-Wswitch-default -Wswitch-enum -Wstrict-overflow=5

SECURITY_FLAGS := -fstack-protector-strong -fPIE -D_FORTIFY_SOURCE=2 -Wno-macro-redefined
DEBUG_FLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer 
PERFORMANCE_FLAGS := -O3 -march=native -mtune=native

CFLAGS := -I$(INCDIR) -std=$(STD) $(WARNING_FLAGS) $(SECURITY_FLAGS) $(DEBUG_FLAGS)
LDFLAGS := -fsanitize=address,undefined

$(shell mkdir -p $(OBJDIR))
$(shell mkdir -p $(dir $(OBJECTS)))

.PHONY: all clean debug release info

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo "Build complete: $(TARGET)"

$(OBJDIR)/%.c.o: %.c
	@echo "Compiling $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning build artifacts..."
	@rm -f $(TARGET)
	@rm -rf $(OBJDIR)
	@echo "Clean complete"

debug: CFLAGS := $(filter-out -O3,$(CFLAGS)) -O0 -DDEBUG
debug: $(TARGET)

release: CFLAGS := $(filter-out -g -fsanitize=address,undefined -fno-omit-frame-pointer,$(CFLAGS)) -DNDEBUG
release: LDFLAGS := $(filter-out -fsanitize=address,undefined,$(LDFLAGS))
release: $(TARGET)

info:
	@echo "Target: $(TARGET)"
	@echo "Sources: $(SOURCES)"
	@echo "Objects: $(OBJECTS)"
	@echo "CC: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"

-include $(OBJECTS:.o=.d)

$(OBJDIR)/%.c.d: %.c
	@mkdir -p $(dir $@)
	@$(CC) -MM -MT $(@:.d=.o) $(CFLAGS) $< > $@
