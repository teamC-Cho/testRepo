#--------------------------------------------------------------------------------------------------------------------
#                 Makefile to build IPMI AMI OEM NVMe Commands
#                 Copyright (c) 2021 American Megatrends Inc 
#---------------------------------------------------------------------------------------------------------------------

#------- Comment/uncomment the following line to enable/disable debugging---------------
DEBUG = n
#---------------------------------------------------------------------------------------------------------------------
LIBRARY_NAME 	= libipmiamioemmfpfault
#---------------------------------------------------------------------------------------------------------------------
#---------------------- Change according to your files ----------------------------------------------------
SRC   =  MFPMemFault.c MFPMemFaultCmds.c MFPMemFaultMetaInfo.c

CFLAGS += -I${SPXINC}/unix
CFLAGS += -I${SPXINC}/dbgout
CFLAGS += -I${SPXINC}/global
CFLAGS += -I${SPXINC}/ipmi
CFLAGS += -I${SPXINC}/pdk
CFLAGS += -I${SPXINC}/featuredefine
CFLAGS += -I${SPXINC}/network
CFLAGS += -I${SPXINC}/hostname
CFLAGS += -I${SPXINC}/ipmihalapi
CFLAGS += -I${SPXINC}/ncml
CFLAGS += -I${SPXINC}/mfp
CFLAGS += -I${SPXINC}/mfp/intel
CFLAGS += -I${SPXINC}/featuredefine

LIBS += -L${SPXLIB}/featuredefine -lfeaturedefine

include ${TOOLDIR}/rules/Rules.make.libs
