#################
#   variables   #
#################

# files
EXECUTABLE  = pthreads0
SOURCES  = pthreads0.c
OBJECTS  = $(SOURCES:.c=.o)

# compilation and linking
CC      = gcc
CFLAGS  = -std=c99 -c 
LDFLAGS = -lpthread
WARN    = -Wall -Wextra -pedantic
COMPILE.c = $(CC) $(CFLAGS) $(CPPFLAGS) $(WARN)
LINK.c    = $(CC)

#################
#     targets   #
#################

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(LINK.c) $^ -o $@ $(LDFLAGS)

$(OBJECTS): %.o: %.c
	$(COMPILE.c) $< -o $@

# phony targets
.PHONY: clean

# remove object files, emacs temporaries
clean:
	rm -f *.o *~ $(EXECUTABLE)

# print-VAR prints the value of the variable VAR
print-%  : ; @echo $* = $($*)

