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
	
	retval = Py_MkdirClean(SPXLIB+"/ipmiamioemmfpfault")
	if retval != 0:
		return retval

	retval = Py_CopyDir("./",IMAGETREE+"/usr/local/lib/")
	if retval != 0:
		return retval

	retval = Py_AddLiblinks(IMAGETREE+"/usr/local/lib/","libipmiamioemmfpfault.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX)
	if retval != 0:
		return retval

	retval = Py_CopyFile("./libipmiamioemmfpfault.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX,SPXLIB+"/ipmiamioemmfpfault/")
	if retval != 0:
		return retval

	retval = Py_AddLiblinks(SPXLIB+"/ipmiamioemmfpfault/","libipmiamioemmfpfault.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX)
	if retval != 0:
		return retval

	retval = Py_Cwd(IMAGETREE+"/usr/local/lib/ipmi")
	if retval != 0:
		return retval

	retval = Py_Execute("rm -rf libipmiamioemmfpfault.so.*")
	if retval != 0:
		return retval

	retval = Py_SoftLink("../libipmiamioemmfpfault.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX, "libipmiamioemmfpfault.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX)
	if retval != 0:
		return retval
		
	retval = Py_AddToFeatures("CONFIG_SPX_FEATURE_MFP_2_1")
	if retval != 0:
		return retval

	return 0

#-------------------------------------------------------------------------------------------------------
#				Rules for Debug Install

#-------------------------------------------------------------------------------------------------------
def debug_install():
	return 0

#-------------------------------------------------------------------------------------------------------
