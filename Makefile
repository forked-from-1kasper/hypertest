SRCDIR     = source
INCLUDEDIR = include
BUILDDIR   = build
BINARY     = Hyper

CXX     = g++
CFLAGS  = -Wall -std=c++2a -Iinclude/
CFLAGS += -Wno-misleading-indentation -Wno-unused-but-set-variable

ifeq ($(OS),Windows_NT)
	BINARY = Hyper.exe
	LDFLAGS = -lglfw3 -lglew32 -lopengl32 -lglu32
else
	BINARY = Hyper

	UNAME := $(shell uname -s)

	ifeq ($(UNAME),Linux)
		LDFLAGS = -lglfw -lGLEW -lGL -lGLU
	endif

	ifeq ($(UNAME),Darwin)
		LDFLAGS = -lglfw -lGLEW -framework CoreVideo -framework OpenGL -framework IOKit -framework Cocoa -framework Carbon
	endif
endif

DEPS     = PicoPNG
MODULES  = Hyper Shader Geometry Sheet
HEADERS  = Hyper/Fuchsian Hyper/Fundamentals
HEADERS += Hyper/Grid Hyper/Gyrovector
HEADERS += Hyper/Moebius Hyper/Tesselation
HEADERS += Enumerable List Literal Tuple

add = $(addprefix $(2)/,$(addsuffix $(1),$(3)))

CPPS = $(call add,.cpp,$(SRCDIR),$(MODULES) $(DEPS))
HPPS = $(call add,.hpp,$(INCLUDEDIR),$(HEADERS))
OBJS = $(call add,.o,$(BUILDDIR),$(MODULES) $(DEPS))

all: $(BUILDDIR) $(BINARY)

$(BINARY): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $(BINARY)

$(call add,.o,$(BUILDDIR),$(MODULES)): $(BUILDDIR)/%.o: $(SRCDIR)/%.cpp $(INCLUDEDIR)/Hyper/%.hpp $(HPPS)
	$(CXX) -c $(CFLAGS) $< -o $@

$(call add,.o,$(BUILDDIR),$(DEPS)): $(BUILDDIR)/%.o: $(SRCDIR)/%.cpp $(INCLUDEDIR)/%.hpp $(HPPS)
	$(CXX) -c $(CFLAGS) $< -o $@

run: $(BINARY)
	./$(BINARY)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -f $(BINARY) $(OBJS)

barbarize:
	python3 barbarize.py $(CPPS) $(HPPS)
