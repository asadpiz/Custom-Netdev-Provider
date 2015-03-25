CC=gcc
LDFLAGS=-L$(XF_LPATH1) -L$(XF_LPATH2) -lFocalpointSDK -lpthread -lm #-lssl -lcrypto -lpcap -lrt
#Intel
XF_SPATH1=/home/farrukh/fulcrum/include
XF_SPATH2=/home/farrukh/fulcrum/include/alos/linux
XF_SPATH3=/home/farrukh/fulcrum/include/std/intel/
XF_SPATH4=/home/farrukh/fulcrum/include/platforms/seacliff/
XF_SPATH5=/home/farrukh/fulcrum/include/alos/
XF_SPATH6=/home/farrukh/fulcrum/build/
XF_LPATH1=/intel/lib
XF_LPATH2=/usr/lib64
export C_INCLUDE_PATH=$(XF_SPATH1):$(XF_SPATH2):$(XF_SPATH3):$(XF_SPATH4):$(XF_SPATH5):$(XF_SPATH6)
export LD_LIBRARY_PATH=$(XF_LPATH1):$(XF_LPATH2)
#check and remove warnings with Wno-
CFLAG=-I. -I$(C_INCLUDE_PATH) -g -ldl\
-Wno-error -Wall -W -Wextra \
-Wno-sign-compare -Wno-conversion -Wno-bad-function-cast\
-Wshadow -Wmissing-declarations -Wstrict-prototypes -Wmissing-prototypes -Wformat -Wunused-parameter -Wold-style-definition -Wcast-align -Wstrict-aliasing -Wparentheses -Wsequence-point -Wdeclaration-after-statement -Wundef -Wmissing-field-initializers -Wpointer-arith -Wformat-security -Wswitch-enum -Wnested-externs -Wredundant-decls -Wdisabled-optimization -Woverride-init -pedantic #-std=c99 -O3

all: dheap list mtt hal hypercuts nova

nova: xf_nova_test.c xf_stdinc.c xf_hypercuts.c xf_dheap.c xf_list.c xf_mtt.c xf_acl.c
	${CC} ${CFLAG} xf_nova_test.c xf_stdinc.c xf_hypercuts.c xf_dheap.c xf_list.c xf_mtt.c xf_hal.c xf_acl.c -o nova $(LDFLAGS)

hypercuts: xf_hypercuts_test.c xf_stdinc.c xf_hypercuts.c xf_dheap.c xf_list.c
	${CC} ${CFLAG} xf_hypercuts_test.c xf_stdinc.c xf_hypercuts.c xf_dheap.c xf_list.c -o hypercuts $(LDFLAGS)

dheap: xf_dheap_test.c xf_stdinc.c xf_dheap.c
	$(CC) ${CFLAG} xf_dheap_test.c xf_stdinc.c xf_dheap.c -o dheap $(LDFLAGS)

list: xf_list_test.c xf_stdinc.c xf_list.c
	$(CC) ${CFLAG} xf_list_test.c xf_stdinc.c xf_list.c -o list $(LDFLAGS)

mtt: xf_mtt_test.c xf_mtt.c xf_dheap.c xf_list.c xf_stdinc.c xf_hypercuts.c  
	$(CC) $(CFLAG) xf_mtt_test.c xf_mtt.c xf_acl.c xf_hal.c xf_hypercuts.c xf_dheap.c xf_list.c xf_stdinc.c -o mtt $(LDFLAGS)

hal: xf_hal_test.c xf_hal.c xf_mtt.c xf_dheap.c xf_list.c xf_stdinc.c xf_hypercuts.c
	$(CC) $(CFLAG) xf_hal_test.c xf_hal.c xf_mtt.c xf_acl.c xf_dheap.c xf_list.c xf_stdinc.c xf_hypercuts.c -o hal $(LDFLAGS)

clean: reset
	rm -rf dheap list hal mtt hypercuts nova
reset:
	rm -f TCAM
