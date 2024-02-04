#----------------------------------------------------------------------
#                         Makefile to build libipmipdk
#                 Copyright (c) 2008 American Megatrends Inc 
#-----------------------------------------------------------------------

#------- Comment/uncomment the following line to enable/disable debugging------
DEBUG = n
#------------------------------------------------------------------------------

#---------------------- Change according to your files ------------------------
LIBRARY_NAME = libscigen
SRC		=  scigen.c

CFLAGS += -I${SPXINC}/global
CFLAGS += -I${SPXINC}/mmap
CFLAGS += -I${SPXINC}/unix
CFLAGS += -I${SPXINC}/dbgout

include ${TOOLDIR}/rules/Rules.make.libs
#------------------------------------------------------------------------------
