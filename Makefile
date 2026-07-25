EM		= emcc
EEMPP		= em++
CC              = gcc

# Places
NXP_SRCDIR	= C:/Users/chauv/Documents/emsdk/src
NXP_BUILDDIR	= C:/Users/chauv/Documents/emsdk/build

DSL_DIR	        = C:/Users/chauv/Documents/emsdk
# was C:/cygwin64/home/Moria
DSL_SRCDIR      = $(DSL_DIR)/embed-master
# was C:/cygwin64/home/Moria/embed-master

# C/CPP Flags
CFLAGS		= -I$(NXP_SRCDIR) -I$(DSL_DIR)/libforth -I$(DSL_DIR)/embed-master -I$(NXP_SRCDIR)/zhash/src # -I$(DSL_DIR)/libcsv 
CEXTRAFLAGS	= -D NXPEM 
EMSDK_FLAGS     = -D NXPEM_MEMCPY # -s EXPORTED_RUNTIME_METHODS=ccall,cwrap

DSL_CFLAGS	= -D ENGINE_DSL -D ENGINE_DSL_HOWERJFORTH
# DSL_LIBS	= $(DSL_DIR)/libcsv/libcsv_la-libcsv.o $(DSL_DIR)/embed-master/util.o -L$(DSL_DIR)/embed-master -lembed # -lm

# Linker flags
LFLAGS =
LIBS = 
# DSL_LIBS	= $(DSL_DIR)/libcsv/libcsv_la-libcsv.o $(DSL_DIR)/embed-master/util.o -L$(DSL_DIR)/embed-master -lembed # -lm 
DSL_LIBS	= $(NXP_BUILDDIR)/embed.o $(NXP_BUILDDIR)/util.o $(NXP_BUILDDIR)/image.o
DSL_CLANG_LIBS	= $(NXP_BUILDDIR)/embed.clang.o $(NXP_BUILDDIR)/util.clang.o $(NXP_BUILDDIR)/image.clang.o

# NXP 40y Architecture
NXP_OBJFILES	= $(NXP_BUILDDIR)/sign.o $(NXP_BUILDDIR)/rule.o $(NXP_BUILDDIR)/hypo.o $(NXP_BUILDDIR)/compound.o $(NXP_BUILDDIR)/engine.o $(NXP_BUILDDIR)/engine_dsl.o $(NXP_BUILDDIR)/loadkb.o $(NXP_BUILDDIR)/nxp_hash.o $(NXP_BUILDDIR)/nxp_evoke.o

ZHASH_OBJFILES	= $(NXP_BUILDDIR)/zhash.o

NXP_CLANG_OBJFILES	= $(NXP_BUILDDIR)/sign.clang.o $(NXP_BUILDDIR)/rule.clang.o $(NXP_BUILDDIR)/hypo.clang.o $(NXP_BUILDDIR)/compound.clang.o $(NXP_BUILDDIR)/engine.clang.o  $(NXP_BUILDDIR)/loadkb.clang.o $(NXP_BUILDDIR)/nxp_hash.clang.o $(NXP_BUILDDIR)/nxp_evoke.clang.o $(NXP_BUILDDIR)/engine_dsl.clang.o

ZHASH_CLANG_OBJFILES	= $(NXP_BUILDDIR)/zhash.clang.o


# Some targets
nxpem: nxpem.c $(NXP_OBJFILES) $(ZHASH_OBJFILES) $(DSL_LIBS)
	$(EM) $^ -o nxpem.html $(CFLAGS) $(DSL_CFLAGS) $(CEXTRAFLAGS) $(EMSDK_FLAGS) $(LFLAGS) $(LIBS) --preload-file satfault.org


nxpem_main: nxpem_main.c $(NXP_CLANG_OBJFILES) $(ZHASH_CLANG_OBJFILES) $(DSL_CLANG_LIBS)
	$(CC) $^ -o nxpem_main.exe  $(CFLAGS) $(DSL_CFLAGS) $(LFLAGS) $(LIBS)

preproc: src/engine_dsl.c
	$(CC) $^ -o test.i  -E -P $(CFLAGS) $(DSL_CFLAGS) $(LFLAGS) $(LIBS)


$(ZHASH_OBJFILES): $(NXP_SRCDIR)/zhash/src/zhash.c
	$(EM) -c -o $(ZHASH_OBJFILES) $< $(CFLAGS) $(CEXTRAFLAGS)

$(ZHASH_CLANG_OBJFILES): $(NXP_SRCDIR)/zhash/src/zhash.c
	$(CC) -c -o $(ZHASH_CLANG_OBJFILES) $< $(CFLAGS) 


clean:
	rm $(NXP_BUILDDIR)/*.o

clean_clang:
	rm $(NXP_BUILDDIR)/*.clang.o


# Generic Rules

$(NXP_BUILDDIR)/%.clang.o: $(NXP_SRCDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS) $(CEXTRAFLAGS) $(DSL_CFLAGS)

$(NXP_BUILDDIR)/%.clang.o: $(DSL_SRCDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS) $(CEXTRAFLAGS)


$(NXP_BUILDDIR)/%.o: $(NXP_SRCDIR)/%.c
	$(EM) -c -o $@ $< $(CFLAGS) $(CEXTRAFLAGS) $(DSL_CFLAGS) $(EMSDK_FLAGS)

$(NXP_BUILDDIR)/%.o: $(DSL_SRCDIR)/%.c
	$(EM) -c -o $@ $< $(CEXTRAFLAGS) $(EMSDK_FLAGS)



# %.o: %.cpp
# 	$(EMPP) -c -o $(NXP_BUILDDIR)/$@ $< $(CFLAGS) $(CEXTRAFLAGS)

