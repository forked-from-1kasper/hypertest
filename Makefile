CXX       ?= g++
SRCDIR     = source
INCLUDEDIR = include
BUILDDIR  ?= build

ifeq ($(BARBARIZED),true)
	SRCDIR     := barbarized/$(SRCDIR)
	INCLUDEDIR := barbarized/$(INCLUDEDIR)
endif

override CFLAGS += -Wall -std=c++20 -I$(INCLUDEDIR)
override CFLAGS += -Wno-bitwise-instead-of-logical -Wno-unused-private-field -Wno-misleading-indentation -Wno-unused-but-set-variable

ifeq ($(OS),Windows_NT)
	BINARY ?= Hyper.exe
	override LDFLAGS += -lgmpxx -lgmp -lluajit-5.1 -lglfw3 -lglew32 -lopengl32 -lglu32
else
	BINARY ?= Hyper

	UNAME := $(shell uname -s)

	ifeq ($(UNAME),Linux)
		override LDFLAGS += -lgmpxx -lgmp -lluajit-5.1 -lglfw -lGLEW -lGL -lGLU
	endif

	ifeq ($(UNAME),Darwin)
		override LDFLAGS += -lgmpxx -lgmp -lluajit-5.1 -lglfw -lGLEW -framework CoreVideo -framework OpenGL -framework IOKit -framework Cocoa -framework Carbon
	endif
endif

DEPS     = PicoPNG Lua
MODULES  = Hyper Config Shader Geometry Sheet Physics Game
HEADERS  = Hyper/Gaussian Hyper/Fuchsian Hyper/Fundamentals
HEADERS += Hyper/Gyrovector Hyper/Moebius Hyper/AutD Hyper/EuclideanDomain
HEADERS += Meta/Enumerable Meta/List Meta/Literal Meta/Tuple

add = $(addprefix $(2)/,$(addsuffix $(1),$(3)))

CPPS = $(call add,.cpp,$(SRCDIR),$(DEPS) $(MODULES))
HPPS = $(call add,.hpp,$(INCLUDEDIR),$(HEADERS) $(DEPS)) $(call add,.hpp,$(INCLUDEDIR)/Hyper,$(MODULES))
OBJS = $(call add,.o,$(BUILDDIR),$(DEPS) $(MODULES))

all: $(BUILDDIR) $(BINARY)

lua:
	git submodule init
	git submodule update
	make -C Fennel LUA=luajit

$(BINARY): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $(BINARY)

$(call add,.o,$(BUILDDIR),$(MODULES)): $(BUILDDIR)/%.o: $(SRCDIR)/%.cpp $(HPPS)
	$(CXX) -c $(CFLAGS) $< -o $@

$(call add,.o,$(BUILDDIR),$(DEPS)): $(BUILDDIR)/%.o: $(SRCDIR)/%.cpp $(HPPS)
	$(CXX) -c $(CFLAGS) $< -o $@

run: $(BINARY)
	./$(BINARY) games/devtest/init.lua

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -f $(BINARY) $(OBJS)
	rm -rf barbarized

barbarize:
	python3 barbarize.py $(CPPS) $(HPPS)
