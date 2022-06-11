# MakeFile
# crearted by ikerli
# 2022-06-11

PATH_BIN = ../bin
PATH_SERVICE = server/service
PATH_INTERFACE = server/interface
PATH_PB = pb
PATH_TINYRPC = ../../tinyrpc


# libtinypc
TINYRPC_INC = $(PATH_TINYRPC)
TINYRPC_LIB = $(PATH_TINYRPC)/lib/libtinyrpc.a


TARGET = $(PATH_BIN)/charon

CXX := g++

CXX_FLAGS := -g -O3 -std=c++11 -Wall -Wno-deprecated -Wno-unused-but-set-variable

CXX_FLAGS += -I $(PATH_TINYRPC) -I../ -I ./$(PATH_PB) -I ./$(PATH_SERVICE) -I ./$(PATH_INTERFACE)

LIBS += /usr/lib/libprotobuf.a /usr/lib/libtinyxml.a

LIBS += $(TINYRPC_LIB)

PB_OBJS = $(patsubst $(PATH_PB)/%.cc, $(PATH_PB)/%.o, $(wildcard $(PATH_PB)/*.cc))
SERVICE_OBJS = $(patsubst $(PATH_SERVICE)/%.cc, $(PATH_SERVICE)/%.o, $(wildcard $(PATH_SERVICE)/*.cc))
INTERFACE_OBJS = $(patsubst $(PATH_INTERFACE)/%.cc, $(PATH_INTERFACE)/%.o, $(wildcard $(PATH_INTERFACE)/*.cc))

OUT : $(TARGET)

$(TARGET) : $(PB_OBJS) $(SERVICE_OBJS) $(INTERFACE_OBJS)
	$(CXX) $(CXX_FLAGS) -o $@ $(LIBS) -ldl -lpthread

$(PATH_PB)/%.o : $(PATH_PB)/%.cc
	$(CXX) $(CXX_FLAGS) -c $< -o $@

$(PATH_SERVICE)/%.o : $(PATH_SERVICE)/%.cc
	$(CXX) $(CXX_FLAGS) -c $< -o $@

$(PATH_INTERFACE)/%.o : $(PATH_INTERFACE)/%.cc
	$(CXX) $(CXX_FLAGS) -c $< -o $@


clean :
	rm -f $(PB_OBJS) $(SERVICE_OBJS) $(INTERFACE_OBJS) $(TARGET)