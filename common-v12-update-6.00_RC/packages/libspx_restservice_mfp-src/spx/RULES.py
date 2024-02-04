#!/usr/bin/env python
#-----------------------------------------Import Rules Files -------------------------------------------
import PyRules
from   PyRules import PrjVars
from   PyRules import *
#-------------------------------------------------------------------------------------------------------
LIBSPXREST_BUILD = PrjVars["BUILD"]+"/"+PrjVars["PACKAGE"]+"/data/"

#-------------------------------------------------------------------------------------------------------
#			Rules for Extracting Source
#-------------------------------------------------------------------------------------------------------
def extract_source():
	return 0

#-------------------------------------------------------------------------------------------------------
#			Rules for Clean Source Directory
#-------------------------------------------------------------------------------------------------------
def clean_source():
	retval = Py_Cwd(PrjVars["BUILD"]+"/"+PrjVars["PACKAGE"]+"/data")
	if retval != 0:
		return retval

	retval = Py_RunMake("clean")
	if retval != 0:
		return retval

	return 0

#-------------------------------------------------------------------------------------------------------
#     Rules for Building Source
#-------------------------------------------------------------------------------------------------------
def build_source():
	retval = Py_Cwd(PrjVars["BUILD"]+"/"+PrjVars["PACKAGE"]+"/data")
	if retval != 0:
		return retval

	retval = Py_RunMake("")
	if retval != 0:
		return retval

	return 0

#-------------------------------------------------------------------------------------------------------
#			 Rules for Creating (Packing) Binary Package
#-------------------------------------------------------------------------------------------------------
# Rules to create libspx_restservice_mfp package
def build_package_libspx_restservice_mfp():
	LIBSPXREST_TEMP = PrjVars["TEMPDIR"]+"/"+PrjVars["PACKAGE"]+"/tmp/"
	retval = Py_MkdirClean(LIBSPXREST_TEMP)
	if retval != 0:
		return retval

	Py_AddMacro("PACKAGE_NAME",PrjVars["PACKAGE"])
	retval=Py_SetValue("PACKAGE_NAME",PrjVars["PACKAGE"])
	if retval != 0:
		return retval

	retval = Py_CopyFile(LIBSPXREST_BUILD+"/libspx_restservice_mfp.so."+PrjVars["PKG_MAJOR"]+"."+PrjVars["PKG_MINOR"]+"."+PrjVars["PKG_AUX"],LIBSPXREST_TEMP)
	if retval != 0:
		return retval

	return Py_PackSPX("./",LIBSPXREST_TEMP)

# Rules to create libspx_restservice_mfp_dev package
def build_package_libspx_restservice_mfp_dev():
	LIBSPXREST_TEMP = PrjVars["TEMPDIR"]+"/"+PrjVars["PACKAGE"]+"/tmp/"
	retval = Py_MkdirClean(LIBSPXREST_TEMP)
	if retval != 0:
		return retval

	Py_CopyFile(LIBSPXREST_BUILD+"/libspx_restservice_mfp.a",LIBSPXREST_TEMP)
	Py_CopyFile(LIBSPXREST_BUILD+"/libspx_restservice_mfp.so."+PrjVars["PKG_MAJOR"]+"."+PrjVars["PKG_MINOR"]+"."+PrjVars["PKG_AUX"],LIBSPXREST_TEMP)

	return Py_PackSPX("./",LIBSPXREST_TEMP)
