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
	#print("CWD : ", os.getcwd())
	IMAGETREE=PrjVars["IMAGE_TREE"]
	PKG_MINOR=PrjVars["PKG_MINOR"]
	PKG_MAJOR=PrjVars["PKG_MAJOR"]
	PKG_AUX=PrjVars["PKG_AUX"]
	SPXLIB=PrjVars["SPXLIB"]
	BUILD = PrjVars['BUILD']+"/"+PrjVars['PACKAGE']+"/data/"

	retval = Py_CopyDir("./",IMAGETREE+"/usr/local/lib/")
	if retval != 0:
		return retval

	Py_MkdirClean(SPXLIB+"/libspx_restservice_mfp/")
	Py_CopyFile("./libspx_restservice_mfp.a",SPXLIB+"/libspx_restservice_mfp/");
	Py_CopyFile("./libspx_restservice_mfp.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX,SPXLIB+"/libspx_restservice_mfp/");

	retval = Py_AddLiblinks(IMAGETREE+"/usr/local/lib/","libspx_restservice_mfp.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX)
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
