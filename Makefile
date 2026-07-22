EM		= emcc
EMPP		= em++

# Places
NXP_SRCDIR	= C:/Users/chauv/Documents/emsdk/src
NXP_BUILDDIR	= C:/Users/chauv/Documents/emsdk/build

DSL_DIR	        = C:/cygwin64/home/Moria
DSL_SRCDIR      = C:/cygwin64/home/Moria/embed-master

# C/CPP Flags
CFLAGS		= -I$(NXP_SRCDIR)
CEXTRAFLAGS	= -D NXPEM -I$(DSL_DIR)/libforth -I$(DSL_DIR)/embed-master -I$(DSL_DIR)/libcsv -I$(NXP_SRCDIR)/zhash/src
EMSDK_FLAGS     = -s EXPORTED_RUNTIME_METHODS=ccall,cwrap

DSL_CFLAGS	= -D ENGINE_DSL -D ENGINE_DSL_HOWERJFORTH
DSL_LIBS	= $(DSL_DIR)/libcsv/libcsv_la-libcsv.o $(DSL_DIR)/embed-master/util.o -L$(DSL_DIR)/embed-master -lembed # -lm

# Linker flags
LFLAGS =
LIBS = 
# DSL_LIBS	= $(DSL_DIR)/libcsv/libcsv_la-libcsv.o $(DSL_DIR)/embed-master/util.o -L$(DSL_DIR)/embed-master -lembed # -lm 
DSL_LIBS	=  $(NXP_BUILDDIR)/embed.o $(NXP_BUILDDIR)/util.o $(NXP_BUILDDIR)/image.o

# NXP 40y Architecture
NXP_OBJFILES	= $(NXP_BUILDDIR)/sign.o $(NXP_BUILDDIR)/rule.o $(NXP_BUILDDIR)/hypo.o $(NXP_BUILDDIR)/compound.o $(NXP_BUILDDIR)/engine.o $(NXP_BUILDDIR)/engine_dsl.o $(NXP_BUILDDIR)/loadkb.o $(NXP_BUILDDIR)/nxp_hash.o $(NXP_BUILDDIR)/nxp_evoke.o

ZHASH_OBJFILES	= $(NXP_BUILDDIR)/zhash.o

# Some targets
nxpem: nxpem.c $(NXP_OBJFILES) $(ZHASH_OBJFILES) $(DSL_LIBS)
	$(EM) $^ -o nxpem.html $(CFLAGS) $(DSL_CFLAGS) $(CEXTRAFLAGS) $(EMSDK_FLAGS) $(LFLAGS) $(LIBS) --preload-file satfault.org

$(ZHASH_OBJFILES): $(NXP_SRCDIR)/zhash/src/zhash.c
	$(EM) -c -o $(ZHASH_OBJFILES) $< $(CFLAGS) $(CEXTRAFLAGS)


clean:
	rm $(NXP_BUILDDIR)/*.o

# Generic Rules

$(NXP_BUILDDIR)/%.o: $(NXP_SRCDIR)/%.c
	$(EM) -c -o $@ $< $(CFLAGS) $(CEXTRAFLAGS) $(DSL_CFLAGS) $(EMSDK_FLAGS)

$(NXP_BUILDDIR)/%.o: $(DSL_SRCDIR)/%.c
	$(EM) -c -o $@ $< $(CEXTRAFLAGS) $(EMSDK_FLAGS)


# %.o: %.cpp
# 	$(EMPP) -c -o $(NXP_BUILDDIR)/$@ $< $(CFLAGS) $(CEXTRAFLAGS)

