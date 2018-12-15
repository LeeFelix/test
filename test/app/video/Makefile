BIN	 = video
CC	 = ../../prebuilts/toolschain/usr/bin/arm-linux-gcc
CPP	 = ../../prebuilts/toolschain/usr/bin/arm-linux-g++
INCS	 = -I./../../out/system/include/


LIBS	 = -L./../../out/system/lib -lcam_hal \
           -lminigui_ths -lm -lpthread -ldl \
           -lpng -lion -lvpu -lmpp -lrkipc \
           -lswresample      \
           -lavformat -lavcodec -lavutil -lfdk-aac -lz \
           -lsalsa -lfsmanage -ldpp -lrk_backtrace -liep \
           -lrkfb -lrkrga


ifeq ($(RK_PROTOCOL_GB28181),YES)
	LIBS += \
				-lrk_gb28181 -leXosip2 -losip2 -losipparser2 -lmxml
endif

ifeq ($(RK_FWK_MSG),YES)
	LIBS += \
				-lfwk_controller -lfwk_glue -lfwk_msg
endif

TMP_CSRCS = $(wildcard *.c customer_interface/*.c ueventmonitor/*.c)
ETCPATH  = ../../out/system/etc
RESPATH  = ../../out/system/share/minigui/res/images

#Resolution = 320x240 ###Screan Resolution = 852*480; BOARD VERSION=V10/V11
#Resolution = 854x480 ###Screan Resolution = 854*480; BOARD VERSION=V10/V11
#Resolution = 1280x720 ###Screan Resolution = 1280*720; BOARD VERSION=V12
Resolution = 960x540 ###Screan Resolution = 960x540; BOARD HX108-v11 Or LCD v30

define all-cpp-files-under
$(shell find $(1) -name "*."$(2) -and -not -name ".*" )
endef

define all-subdir-cpp-files  
$(call all-cpp-files-under,.,"cpp")
endef

CPPSRCS	 = $(call all-subdir-cpp-files)

#default no
COMPILE_WITH_NO_GUI = no
ifeq ($(COMPILE_WITH_NO_GUI), yes)
	#CSRCS = $(filter-out camera_ui.c, $(TMP_CSRCS))
	CSRCS = $(TMP_CSRCS)
else
    CSRCS = $(filter-out customer_interface/test_interface.c, $(TMP_CSRCS))
endif

COBJS	:= $(CSRCS:.c=.o)
CPPOBJS	:= $(CPPSRCS:.cpp=.o)

DEBUGFLAGS = -funwind-tables -g -rdynamic -O0 -DDEBUG
#DEBUGFLAGS += -DSTRESS_TEST
#DEBUGFLAGS += -DMUL_ENCODERS_DEBUG
OPTIMIZEFLAGS = -O3 -Wl,-gc-sections
CFLAGS	 = -Wno-multichar
CPPFLAGS = -DSUPPORT_ION -std=c++11 -DLINUX  -DENABLE_ASSERT -Wno-multichar -DResolution

CFLAGS += $(DEBUGFLAGS)
CPPFLAGS += $(DEBUGFLAGS)
#CFLAGS += $(OPTIMIZEFLAGS)
#CPPFLAGS += $(OPTIMIZEFLAGS)

ifeq ($(COMPILE_WITH_NO_GUI), yes)
	CFLAGS += -DHAVE_GUI=0 -DHAVE_DISPLAY=0
	CPPFLAGS += -DHAVE_GUI=0 -DHAVE_DISPLAY=0
else
	CFLAGS += -DHAVE_GUI=1 -DUSE_WATERMARK=1 -DHAVE_DISPLAY=1
	CPPFLAGS += -DHAVE_GUI=1 -DUSE_WATERMARK=1 -DHAVE_DISPLAY=1
endif

CPPFLAGS += -D USE_RK_V4L2_HEAD_FILES

ifeq ($(RK_PROTOCOL_GB28181),YES)
	CPPFLAGS += -DPROTOCOL_GB28181
	CFLAGS += -DPROTOCOL_GB28181
endif

ifeq ($(RK_FWK_MSG),YES)
	CPPFLAGS += -DMSG_FWK
	CFLAGS += -DMSG_FWK
endif

#copy the res to device
$(shell cp -r res/ui_$(strip $(Resolution)).h ui_resolution.h )
$(shell cp -r res/res_$(strip $(Resolution))/MiniGUI.cfg $(ETCPATH) )
$(shell cp -r res/res_$(strip $(Resolution))/* $(RESPATH) )
$(shell cp -r res/watermark/* $(RESPATH) )

all : $(BIN)

$(COBJS) : %.o : %.c
	@$(CC) -c $< -o $@ $(INCS) $(CFLAGS) $(LIBS)
$(CPPOBJS) : %.o : %.cpp
	@$(CPP) -c $< -o $@ $(INCS) $(CPPFLAGS) $(LIBS)

$(BIN) : $(COBJS) $(CPPOBJS)
	@$(CPP) -o $(BIN) $(COBJS) $(CPPOBJS) $(LIBS)
	@-rm $(COBJS) $(CPPOBJS)
	@echo "video build success!"
			
clean:
	@rm -f $(BIN) $(COBJS) $(CPPOBJS)
	@echo "video clean success!"
