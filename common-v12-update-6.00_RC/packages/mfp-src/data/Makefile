#------- Comment/uncomment the following line to enable/disable debugging---------
DEBUG = n

#---------------------- Change according to your files ----------------------------
SRC = mfp.c	bios.c AddressDecode.c		# Source File
TARGET = mfp 			# Output file

CFLAGS+= -I${SPXINC}/dbgout
CFLAGS += -I${SPXINC}/global
CFLAGS += -I${SPXINC}/unix
CFLAGS += -I${SPXINC}/ipmi
CFLAGS += -I${SPXINC}/libipmi
CFLAGS += -I${SPXINC}/procmanager
CFLAGS += -I${SPXINC}/libhiredis
CFLAGS += -I${SPXINC}/mfp/intel
ifdef CONFIG_SPX_FEATURE_MFP_2_0
CFLAGS += -I${SPXINC}/peci
CFLAGS += -I${SPXINC}/mfperrcollect
else ifdef CONFIG_SPX_FEATURE_MFP_2_1
CFLAGS += -I${SPXINC}/peci
CFLAGS += -I${SPXINC}/mfperrcollect
endif
CFLAGS += -I${SPXINC}/featuredefine

# Add the Corresponding library
LIBS += -L${SPXLIB}/dbgout -ldbgout
LIBS += -L${SPXLIB}/mfp/intel -lmfp
LIBS += -L${SPXLIB}/unix -lunix
LIBS += -L${SPXLIB}/procmanager -lprocmanager
LIBS += -lpthread
LIBS += -L${SPXLIB}/linker -lhiredis
ifdef CONFIG_SPX_FEATURE_MFP_2_0
LIBS += -L${SPXLIB}/peci -lpeci
LIBS += -L${SPXLIB}/mfperrcollect -lmfperrcollect
else ifdef CONFIG_SPX_FEATURE_MFP_2_1
LIBS += -L${SPXLIB}/peci -lpeci
LIBS += -L${SPXLIB}/mfperrcollect -lmfperrcollect
endif
LIBS +=	-L${SPXLIB}/ipmi -lipmi
LIBS += -L${SPXLIB}/ipmiparams -lipmiparams
LIBS += -L${SPXLIB}/featuredefine -lfeaturedefine
LIBS += -L${SPXLIB}/mmap -lmmap
LIBS += -rdynamic
LIBS += -ldl

include ${TOOLDIR}/rules/Rules.make.apps

#----------------------------------------------------------------------------------------
