#!/usr/bin/env python
#-----------------------------------------Import Rules Files -------------------------------------------
import PyRules
from   PyRules import PrjVars
from   PyRules import *
#--------------------------------------- Extra Python modules -------------------------------------------
#-------------------------------------------------------------------------------------------------------


#-------------------------------------------------------------------------------------------------------
#		  	      Rules for Installing to ImageTree
#-------------------------------------------------------------------------------------------------------
def build_install():
	IMAGETREE=PrjVars["IMAGE_TREE"]
	SPXLIB=PrjVars["SPXLIB"]
	PKG_MAJOR=PrjVars["PKG_MAJOR"]
	PKG_MINOR=PrjVars["PKG_MINOR"]
	PKG_AUX=PrjVars["PKG_AUX"]

	retval = Py_MkdirClean(SPXLIB+"/ipmioemmfp")
	if retval != 0:
		return retval

	retval = Py_CopyDir("./",IMAGETREE+"/usr/local/lib/")
	if retval != 0:
		return retval

	retval = Py_AddLiblinks(IMAGETREE+"/usr/local/lib/","libipmioemmfp.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX)
	if retval != 0:
		return retval

	retval = Py_CopyFile("./libipmioemmfp.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX,SPXLIB+"/ipmioemmfp/")
	if retval != 0:
		return retval

	retval = Py_AddLiblinks(SPXLIB+"/ipmioemmfp/","libipmioemmfp.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX)
	if retval != 0:
		return retval

	retval = Py_Cwd(IMAGETREE+"/usr/local/lib/ipmi")
	if retval != 0:
		return retval

	retval = Py_Execute("rm -rf libipmioemmfp.so.*")
	if retval != 0:
		return retval

	retval = Py_SoftLink("../libipmioemmfp.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX, "libipmioemmfp.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX)
	if retval != 0:
		return retval

	retval = Py_AddToFeatures("CONFIG_SPX_FEATURE_MFP=MFP_SUPPORT")
	if retval != 0:
		return retval

	return 0

#-------------------------------------------------------------------------------------------------------
#				Rules for Debug Install

#-------------------------------------------------------------------------------------------------------
def debug_install():
	return 0

#-------------------------------------------------------------------------------------------------------
