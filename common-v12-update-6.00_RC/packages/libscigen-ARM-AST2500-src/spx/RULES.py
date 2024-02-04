#!/usr/bin/env python
#-----------------------------------------Import Rules Files -------------------------------------------
import PyRules
from   PyRules import PrjVars
from   PyRules import *
#-------------------------------------------------------------------------------------------------------


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
#			Rules for Building Source
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
#			 Rules for Creating (Packing) Binary Packge
#-------------------------------------------------------------------------------------------------------
# Rules to create libscigen package
def build_package_libscigen():
	TEMPDIR = PrjVars["TEMPDIR"]
	PACKAGE = PrjVars["PACKAGE"]
	BUILD = PrjVars["BUILD"]
	
	retval = Py_MkdirClean(TEMPDIR+"/"+PACKAGE+"/tmp")
	if retval != 0:
		return retval

	retval = Py_CopyFile(BUILD+"/"+PACKAGE+"/data/libscigen.so."+PrjVars["PKG_MAJOR"]+"."+PrjVars["PKG_MINOR"]+"."+PrjVars["PKG_AUX"],TEMPDIR+"/"+PACKAGE+"/tmp")
	if retval != 0:
		return retval
	
	retval = Py_PackSPX("./",TEMPDIR+"/"+PACKAGE+"/tmp")
	if retval !=0:
		return retval

	return 0

	