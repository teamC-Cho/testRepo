#!/usr/bin/env python
#-----------------------------------------Import Rules Files -------------------------------------------
import PyRules 
from   PyRules import PrjVars
from   PyRules import *
#--------------------------------------- Extra Pyhon modules -------------------------------------------
##-------------------------------------------------------------------------------------------------------


#-------------------------------------------------------------------------------------------------------
#		  	      Rules for Installing to ImageTree
#-------------------------------------------------------------------------------------------------------
def build_install():
	SPXINC=PrjVars["SPXINC"]
	retval = Py_MkdirClean(SPXINC+"/mfperrcollect")
	if retval != 0:
		return retval

	retval = Py_CopyFile("./cpu.h",SPXINC+"/mfperrcollect/")
	if retval != 0:
		return retval
	retval = Py_CopyFile("./channel.h",SPXINC+"/mfperrcollect/")
	if retval != 0:
		return retval
	retval = Py_CopyFile("./mfp_registers_structs.h",SPXINC+"/mfperrcollect/")
	if retval != 0:
		return retval
	retval = Py_CopyFile("./mfp_structs.h",SPXINC+"/mfperrcollect/")
	if retval != 0:
		return retval
	return 0

#-------------------------------------------------------------------------------------------------------
#				Rules for Debug Install
#-------------------------------------------------------------------------------------------------------
def debug_install():
	return 0