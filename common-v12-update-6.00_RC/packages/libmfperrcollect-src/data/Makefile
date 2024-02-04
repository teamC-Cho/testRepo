#--------------------------------------------------------------------------------------------------------------------
#               Makefile to build libmfperrcollect
#               Copyright (c) 2021 American Megatrends Inc
#---------------------------------------------------------------------------------------------------------------------

#------- Comment/uncomment the following line to enable/disable debugging---------------
DEBUG = n
#---------------------------------------------------------------------------------------------------------------------
LIBRARY_NAME 	=  libmfperrcollect
#---------------------------------------------------------------------------------------------------------------------
#---------------------- Change according to your files ----------------------------------------------------
SRC     =   cpu.c channel.c

CFLAGS += -I${SPXINC}/unix
CFLAGS += -I${SPXINC}/dbgout
CFLAGS += -I${SPXINC}/global
CFLAGS += -I${SPXINC}/featuredefine
CFLAGS += -I${SPXINC}/peci


# Add the Corresponding library
LIBS+= -L${SPXLIB}/peci -lpeci

LIBS   += -L${SPXLIB}/featuredefine -lfeaturedefine


include ${TOOLDIR}/rules/Rules.make.libs
