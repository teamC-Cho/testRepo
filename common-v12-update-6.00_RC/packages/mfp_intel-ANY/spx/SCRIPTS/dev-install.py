#!/usr/bin/env python
#-----------------------------------------Import Rules Files -------------------------------------------
import PyRules
from   PyRules import PrjVars
from   PyRules import *
#--------------------------------------- Extra Python modules -------------------------------------------
#-------------------------------------------------------------------------------------------------------

#LIBMFP_MAJOR = "1"
#LIBMFP_MINOR = "0"

import os.path

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

	if "CONFIG_SPX_FEATURE_MFP_2_0" in PrjVars:
		retval = Py_AddToFeatures("CONFIG_SPX_FEATURE_MFP_2_0")
		if retval != 0:
			return retval
		LIBMFP_MAJOR = "2"
		LIBMFP_MINOR = "0"
	elif "CONFIG_SPX_FEATURE_MFP_2_1" in PrjVars:
		retval = Py_AddToFeatures("CONFIG_SPX_FEATURE_MFP_2_1")
		if retval != 0:
			return retval
		LIBMFP_MAJOR = "2"
		LIBMFP_MINOR = "1"
		
	elif "CONFIG_SPX_FEATURE_MFP_1_0" in PrjVars:
		retval = Py_AddToFeatures("CONFIG_SPX_FEATURE_MFP_1_0")
		if retval != 0:
			return retval
		LIBMFP_MAJOR = "1"
		LIBMFP_MINOR = "0"
	else:
		print("MFP Version is not defined")
		return -1

	retval = Py_Mkdir(SPXINC+"/mfp/intel")
	if retval != 0:
		return retval

	retval = Py_CopyFile("./mfp.h",SPXINC+"/mfp/intel")
	if retval != 0:
		return retval

	retval = Py_MkdirClean(SPXLIB+"/mfp/intel")
	if retval != 0:
		return retval

	if os.path.isfile("./libmfp.so."+LIBMFP_MAJOR+"."+LIBMFP_MINOR) is True:	
		retval = Py_Rename("./libmfp.so."+LIBMFP_MAJOR+"."+LIBMFP_MINOR, "./libmfp.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX)
		if retval != 0:
			return retval

	retval = Py_CopyFile("./libmfp.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX,IMAGETREE+"/usr/local/lib/")
	if retval != 0:
		return retval

	retval = Py_AddLiblinks(IMAGETREE+"/usr/local/lib/","libmfp.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX)
	if retval != 0:
		return retval

	retval = Py_CopyFile("./libmfp.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX,SPXLIB+"/mfp/intel")
	if retval != 0:
		return retval

	retval = Py_AddLiblinks(SPXLIB+"/mfp/intel","libmfp.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX)
	if retval != 0:
		return retval

	return 0

#-------------------------------------------------------------------------------------------------------
#				Rules for Debug Install

#-------------------------------------------------------------------------------------------------------
def debug_install():
	return 0

#-------------------------------------------------------------------------------------------------------
