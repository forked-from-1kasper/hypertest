SRCDIR     = source
INCLUDEDIR = include
BUILDDIR   = build
BINARY     = Hyper

CXX     = g++
CFLAGS  = -Wall -std=c++2a -I$(INCLUDEDIR)
CFLAGS += -Wno-bitwise-instead-of-logical -Wno-unused-private-field -Wno-misleading-indentation -Wno-unused-but-set-variable

ifeq ($(OS),Windows_NT)
	BINARY = Hyper.exe
	LDFLAGS = -lluajit-5.1 -lglfw3 -lglew32 -lopengl32 -lglu32
else
	BINARY = Hyper

	UNAME := $(shell uname -s)

	ifeq ($(UNAME),Linux)
		LDFLAGS = -lluajit-5.1 -lglfw -lGLEW -lGL -lGLU
	endif

	ifeq ($(UNAME),Darwin)
		LDFLAGS = -lluajit-5.1 -lglfw -lGLEW -framework CoreVideo -framework OpenGL -framework IOKit -framework Cocoa -framework Carbon
	endif
endif

DEPS     = PicoPNG Lua
MODULES  = Hyper Config Shader Geometry Sheet Physics
HEADERS  = Hyper/Fuchsian Hyper/Fundamentals
HEADERS += Hyper/Gyrovector Hyper/Moebius
HEADERS += Enumerable List Literal Tuple

add = $(addprefix $(2)/,$(addsuffix $(1),$(3)))

CPPS = $(call add,.cpp,$(SRCDIR),$(DEPS) $(MODULES))
HPPS = $(call add,.hpp,$(INCLUDEDIR),$(HEADERS) $(DEPS)) $(call add,.hpp,$(INCLUDEDIR)/Hyper,$(MODULES))
OBJS = $(call add,.o,$(BUILDDIR),$(DEPS) $(MODULES))

all: $(BUILDDIR) $(BINARY)

$(BINARY): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $(BINARY)

$(call add,.o,$(BUILDDIR),$(MODULES)): $(BUILDDIR)/%.o: $(SRCDIR)/%.cpp $(HPPS)
	$(CXX) -c $(CFLAGS) $< -o $@

$(call add,.o,$(BUILDDIR),$(DEPS)): $(BUILDDIR)/%.o: $(SRCDIR)/%.cpp $(HPPS)
	$(CXX) -c $(CFLAGS) $< -o $@

run: $(BINARY)
	./$(BINARY)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -f $(BINARY) $(OBJS)

barbarize:
	python3 barbarize.py $(CPPS) $(HPPS)
