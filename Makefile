CXX        = g++
SRCDIR     = source
INCLUDEDIR = include
BUILDDIR   = build

ifeq ($(BARBARIZED),true)
	SRCDIR     := barbarized/$(SRCDIR)
	INCLUDEDIR := barbarized/$(INCLUDEDIR)
endif

override CFLAGS += -Wall -std=c++20 -I$(INCLUDEDIR) -Wno-bitwise-instead-of-logical -Wno-unused-private-field -Wno-misleading-indentation -Wno-unused-but-set-variable

ifeq ($(OS),Windows_NT)
	BINARY = Hyper.exe
	override LDFLAGS += -lsqlite3 -lgmpxx -lgmp -lluajit-5.1 -lglfw3 -lglew32 -lopengl32 -lglu32
else
	BINARY = Hyper

	UNAME := $(shell uname -s)

	ifeq ($(UNAME),Linux)
		override LDFLAGS += -lsqlite3 -lgmpxx -lgmp -lluajit-5.1 -lglfw -lGLEW -lGL -lGLU
	endif

	ifeq ($(UNAME),FreeBSD)
		override LDFLAGS += -lsqlite3 -lgmpxx -lgmp -lluajit-5.1 -lglfw -lGLEW -lGL -lGLU
	endif

	ifeq ($(UNAME),Darwin)
		override LDFLAGS += -lsqlite3 -lgmpxx -lgmp -lluajit-5.1 -lglfw -lGLEW -framework CoreVideo -framework OpenGL -framework IOKit -framework Cocoa -framework Carbon
	endif
endif

DEPS    = Lua
MODULES = Hyper Config Shader Geometry Sheet Physics Game
HEADERS = Math/Gaussian Math/Fuchsian Hyper/Fundamentals \
          Math/Basic Math/Gyrovector Math/Moebius Math/AutD Math/Euclidean \
          Meta/Basic Meta/Enumerable Meta/List Meta/Literal Meta/Tuple

add = $(addprefix $(2)/,$(addsuffix $(1),$(3)))

CXXS = $(call add,.cxx,$(SRCDIR),$(DEPS) $(MODULES))
HXXS = $(call add,.hxx,$(INCLUDEDIR),$(HEADERS) $(DEPS)) $(call add,.hxx,$(INCLUDEDIR)/Hyper,$(MODULES))
OBJS = $(call add,.o,$(BUILDDIR),$(DEPS) $(MODULES))

all: $(BUILDDIR) $(BINARY)

$(BINARY): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $(BINARY)

$(call add,.o,$(BUILDDIR),$(MODULES)): $(BUILDDIR)/%.o: $(SRCDIR)/%.cxx $(HXXS)
	$(CXX) -c $(CFLAGS) $< -o $@

$(call add,.o,$(BUILDDIR),$(DEPS)): $(BUILDDIR)/%.o: $(SRCDIR)/%.cxx $(HXXS)
	$(CXX) -c $(CFLAGS) $< -o $@

run: $(BINARY)
	./$(BINARY) games/devtest/init.lua

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -f $(BINARY) $(OBJS)
	rm -rf barbarized

barbarize:
	python3 barbarize.py $(CXXS) $(HXXS)
