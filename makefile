
ifdef SystemRoot
	RM = del /Q
	CP = copy
	FixPath = $(subst /,\,$1)
	PLATFLAGS = -lkernel32
	LINKPATHS = -Lsdl-win/lib
	COMPATHS = -Isdl-win/include
	PLATPOST = $(CP) $(call FixPath,sdl-win/bin/SDL.dll bin)
else
	RM = rm -f
	CP = cp
	FixPath = $1
	PLATFLAGS = 
	LINKPATHS = 
	COMPATHS = 
	PLATPOST = 
endif

SRCDIR=src
LIBDIR=lib
EXTDIR=ext
OUTDIR=bin
OBJDIR=obj

CC=gcc
CFLAGS=-D_DEBUG -g

_DEPS = ss_main.h
DEPS = $(patsubst %,$(SRCDIR)/%,$(_DEPS))

_OBJ = ss_main.o ss_script.o ss_sdl.o
OBJ = $(patsubst %,$(OBJDIR)/%,$(_OBJ))


$(OUTDIR)/sgs-sdl: $(OBJ)
	$(MAKE) -C sgscript
	gcc -Wall -o $@ $(OBJ) $(LINKPATHS) -Lsgscript/lib -lmingw32 -lOpenGL32 -lSDLmain -lSDL -lsgscript -mwindows $(PLATFLAGS)
	$(PLATPOST)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(COMPATHS) $(CFLAGS)

.PHONY: clean
clean:
	$(RM) $(call FixPath,$(OBJDIR)/*.o $(OUTDIR)/sgs*)
