USE_OPTIMIZED_DSEXAMPLE?=0
CUDA_VER?=10.2
ifeq ($(CUDA_VER),)
  $(error "CUDA_VER is not set")
endif
TARGET_DEVICE = $(shell gcc -dumpmachine | cut -f1 -d -)
CXX:= g++

ifeq ($(USE_OPTIMIZED_DSEXAMPLE),1)
  SRCS:= gstdsexample_optimized.cpp
else
  SRCS:= gstdsexample.cpp
endif

INCS:= $(wildcard *.h)
LIB:=libnvdsgst_dsexample.so

NVDS_VERSION:=5.0

DEP:=dsexample_lib/libdsexample.a
DEP_FILES:=$(wildcard dsexample_lib/dsexample_lib.* )
DEP_FILES-=$(DEP)

CFLAGS+= -fPIC -DDS_VERSION=\"5.0.0\" \
	 -I /usr/local/cuda-$(CUDA_VER)/include \
	 -I ../../includes

GST_INSTALL_DIR?=/opt/nvidia/deepstream/deepstream-$(NVDS_VERSION)/lib/gst-plugins/
LIB_INSTALL_DIR?=/opt/nvidia/deepstream/deepstream-$(NVDS_VERSION)/lib/

LIBS := -shared -Wl,-no-undefined \
	-L dsexample_lib -ldsexample \
	-L/usr/local/cuda-$(CUDA_VER)/lib64/ -lcudart -ldl \
	-lnppc -lnppig -lnpps -lnppicc -lnppidei \
	-L$(LIB_INSTALL_DIR) -lnvdsgst_helper -lnvdsgst_meta -lnvds_meta -lnvbufsurface -lnvbufsurftransform\
	-Wl,-rpath,$(LIB_INSTALL_DIR)

OBJS:= $(SRCS:.cpp=.o)
ifeq ($(TARGET_DEVICE),aarch64)
	PKGS:= gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0 opencv4
else
	PKGS:= gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0 opencv
endif

CFLAGS+=$(shell pkg-config --cflags $(PKGS))
LIBS+=$(shell pkg-config --libs $(PKGS))

all: $(LIB)

%.o: %.cpp $(INCS) Makefile
	@echo $(CFLAGS)
	$(CXX) -c -o $@ $(CFLAGS) $<

$(LIB): $(OBJS) $(DEP) Makefile
	@echo $(CFLAGS)
	$(CXX) -o $@ $(OBJS) $(LIBS)

$(DEP): $(DEP_FILES)
	$(MAKE) -C dsexample_lib/

install: $(LIB)
	cp -rv $(LIB) $(GST_INSTALL_DIR)

clean:
	rm -rf $(OBJS) $(LIB)
