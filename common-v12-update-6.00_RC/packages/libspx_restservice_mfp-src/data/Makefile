#-----------------------------------------------------------------------------
DEBUG = n
#-----------------------------------------------------------------------------
LIBRARY_NAME	= libspx_restservice_mfp
#------------------------------------------------------------------------------
USE_WSMAN = 1234
SRC  	=	settings_mfp_management.c spx_rest_mfp.c
CFLAGS	+=	-DCFG_PROJ_USE_WSMAN
#Cflags
CFLAGS 	+= 	-I.
CFLAGS  +=  -I${SPXINC}/
CFLAGS  +=  -I${SPXINC}/libspx_restservice
CFLAGS  +=	-I${SPXINC}/fastcgi
CFLAGS  +=	-I${SPXINC}/raphters
CFLAGS  +=	-I${SPXINC}/qdecoder
CFLAGS 	+= 	-I${SPXINC}/global
CFLAGS 	+=	-I${TARGETDIR}/usr/include
CFLAGS 	+= 	-I${SPXINC}/session
CFLAGS 	+= 	-I${SPXINC}/ipmi
CFLAGS 	+= 	-I${SPXINC}/libipmi
CFLAGS 	+= 	-I${SPXINC}/dbgout
CFLAGS  +=  -I${SPXINC}/featuredefine
CFLAGS	+=	-I${SPXINC}/racsessioninfo
CFLAGS 	+= 	-I${SPXINC}/libipmioemmfp
CFLAGS 	+= 	-I${SPXINC}/mfp
CFLAGS 	+= 	-I${SPXINC}/mfp/intel

LIBS	+=	-L${SPXLIB}/libspx_restservice -lspx_restservice
LIBS	+=	-L${SPXLIB}/fastcgi -lfcgi
LIBS	+=	-L${SPXLIB}/raphters -lraphters
LIBS	+=	-L${SPXLIB}/qdecoder -lqdecoder
LIBS    +=  -L${SPXLIB}/session -lsession
LIBS	+=	-L${SPXLIB}/ipmi -lipmi
LIBS	+=	-L${SPXLIB}/ipmiparams -lipmiparams
LIBS	+=	-L${SPXLIB}/dbgout -ldbgout
LIBS    +=  -L${SPXLIB}/featuredefine -lfeaturedefine
LIBS	+=	-L${SPXLIB}/racsessioninfo -lracsessioninfo
##For managing session for wsman
LIBS    +=  -L${SPXLIB}/rtlog -lrtlog
LIBS    +=  -L${SPXLIB}/ipmioemmfp -lipmioemmfp

ifeq ($(CONFIG_SPX_FEATURE_TARGET_JESSIE),YES)
LIBS    +=  ${TARGETDIR}/lib/${MULTI_ARCH_LIB}/libjson-c.so.2.0.0
endif
ifeq ($(CONFIG_SPX_FEATURE_TARGET_BUSTER),YES)
LIBS    +=  ${TARGETDIR}/usr/lib/${MULTI_ARCH_LIB}/libjson-c.so
endif
include ${TOOLDIR}/rules/Rules.make.libs
#------------------------------------------------------------------------------
