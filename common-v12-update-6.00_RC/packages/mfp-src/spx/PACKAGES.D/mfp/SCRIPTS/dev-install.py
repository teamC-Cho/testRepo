#!/usr/bin/env python
#-----------------------------------------Import Rules Files -------------------------------------------
import PyRules
from   PyRules import PrjVars
from   PyRules import *
#--------------------------------------- Extra Python modules -------------------------------------------
#-------------------------------------------------------------------------------------------------------
LIBMFP_MAJOR = "1"
LIBMFP_MINOR = "0"
MFP_BUILD = PrjVars["BUILD"]+"/"+PrjVars["PACKAGE"]+"/data/"
#-------------------------------------------------------------------------------------------------------
#		  	      Rules for Installing to ImageTree
#-------------------------------------------------------------------------------------------------------
def build_install():
	IMAGETREE=PrjVars["IMAGE_TREE"]	
	retval = Py_CopyFile("./mfp", IMAGETREE+"/usr/local/bin")
	if retval != 0:
		return retval
	retval = Py_InitScript("./mfp.sh")
	if retval != 0:
		return retval
	retval = Py_CopyConf("./mfp_report")
	if retval != 0:
		return retval
	return 0

#-------------------------------------------------------------------------------------------------------
#				Rules for Debug Install

#-------------------------------------------------------------------------------------------------------
def debug_install():
	return 0

#-------------------------------------------------------------------------------------------------------
