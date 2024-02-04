#!/usr/bin/env python
#-----------------------------------------Import Rules Files -------------------------------------------
import PyRules
from   PyRules import PrjVars
from   PyRules import *
#-------------------------------------------------------------------------------------------------------
MFP_BUILD = PrjVars["BUILD"]+"/"+PrjVars["PACKAGE"]+"/data/"
LIBMFP_MAJOR = "1"
LIBMFP_MINOR = "0"
#-------------------------------------------------------------------------------------------------------
#			Rules for Extracting Source
#-------------------------------------------------------------------------------------------------------
def extract_source():
	return 0

#-------------------------------------------------------------------------------------------------------
#			Rules for Clean Source Directory
#-------------------------------------------------------------------------------------------------------
def clean_source():
	retval = Py_Cwd(MFP_BUILD)
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
	retval = Py_Cwd(MFP_BUILD)
	if retval != 0:
		return retval

	retval = Py_RunMake("")
	if retval != 0:
		return retval

	return 0

#-------------------------------------------------------------------------------------------------------
#			 Rules for Creating (Packing) Binary Packge
#-------------------------------------------------------------------------------------------------------
# Rules to create mfp package
def build_package_mfp():
	MFP_TEMP = PrjVars["TEMPDIR"]+"/"+PrjVars["PACKAGE"]+"/tmp/"
	retval = Py_MkdirClean(MFP_TEMP)
	if retval != 0:
		return retval

	retval = Py_CopyFile(MFP_BUILD+"/mfp",MFP_TEMP)
	if retval != 0:
		return retval

	retval = Py_CopyFile(MFP_BUILD+"/mfp.sh",MFP_TEMP)
	if retval != 0:
		return retval

	retval = Py_CopyFile(MFP_BUILD+"/mfp_report",MFP_TEMP)
	if retval != 0:
		return retval

	return Py_PackSPX("./",MFP_TEMP)
	
# Rules to create mfp_dev package
def build_package_mfp_dev():
	TEMPDIR = PrjVars["TEMPDIR"]
	PACKAGE = PrjVars["PACKAGE"]
	BUILD = PrjVars["BUILD"]

	retval = Py_MkdirClean(TEMPDIR+"/"+PACKAGE+"/tmp")
	if retval != 0:
		return retval


	retval = Py_CopyFile(MFP_BUILD+"/mfp_ami.h",TEMPDIR+"/"+PACKAGE+"/tmp")
	if retval != 0:
		return retval

	return Py_PackSPX("./",TEMPDIR+"/"+PACKAGE+"/tmp")

