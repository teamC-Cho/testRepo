#!/usr/bin/env python
#-----------------------------------------Import Rules Files -------------------------------------------
import PyRules
from   PyRules import PrjVars
from   PyRules import *
#--------------------------------------- Extra Python modules -------------------------------------------
#-------------------------------------------------------------------------------------------------------
LIBMFP_MAJOR = "1"
LIBMFP_MINOR = "0"
#-------------------------------------------------------------------------------------------------------
#		  	      Rules for Installing to ImageTree
#-------------------------------------------------------------------------------------------------------
def build_install():
	SPXINC=PrjVars["SPXINC"]
	IMAGETREE=PrjVars["IMAGE_TREE"]
	SPXLIB=PrjVars["SPXLIB"]
	PKG_MAJOR=PrjVars["PKG_MAJOR"]
	PKG_MINOR=PrjVars["PKG_MINOR"]
	PKG_AUX=PrjVars["PKG_AUX"]

	retval = Py_Mkdir(SPXINC+"/mfp")
	if retval != 0:
		return retval

	retval = Py_CopyFile("./mfp_ami.h",SPXINC+"/mfp/")
	if retval != 0:
		return retval

	retval = Py_MkdirClean(SPXLIB+"/mfp")
	if retval != 0:
		return retval


	return 0

#-------------------------------------------------------------------------------------------------------
#				Rules for Debug Install

#-------------------------------------------------------------------------------------------------------
def debug_install():
	return 0

#-------------------------------------------------------------------------------------------------------
