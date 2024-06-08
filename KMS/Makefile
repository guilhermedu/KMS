OBJDIR=./obj
IDIR=../include
LDIR=../lib
IDIR2=../eigen/Eigen
CC=g++ -std=c++17 -Wall
CFLAGS=-I $(IDIR) -I $(IDIR2) -Wall -g -w 
LIBS=-lmysqlcppconn

all: kms
DEPS = etsi_qkd_004.h ETSI004_south.h ip_tunnel_ms_windows_20200819.h json.hpp load_ascii_2024.h message_handler.h ms_windows_console_output_common_20200819.h netxpto_20200819.h save_db.h peer_comm.h KeySync_Block.h ETSI004_north.h load_db.h

DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = kms.o etsi_qkd_004.o ETSI004_south.o ip_tunnel_ms_windows_20200819.o load_ascii_2024.o message_handler.o ms_windows_console_output_common_20200819.o netxpto_20200819.o save_db.o peer_comm.o KeySync_Block.o ETSI004_north.o load_db.o

OBJ = $(patsubst %,$(OBJDIR)/%,$(_OBJ))

$(OBJDIR)/%.o: $(LDIR)/%.cpp $(_DEPS) 
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/kms.o: KMS.cpp cv_qokd_ldpc_multi_machine_sdf.h KMS.h
	$(CC) $(CFLAGS) -c $< -o $@

kms: $(OBJ) 
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(OBJ): | $(OBJDIR)
$(OBJDIR):
	mkdir $(OBJDIR)

.PHONY: clean

clean:
	rm -rf $(OBJDIR) kms
