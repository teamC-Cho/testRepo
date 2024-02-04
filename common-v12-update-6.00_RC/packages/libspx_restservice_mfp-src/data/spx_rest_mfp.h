#ifndef __SPXRESTEXT_H__
#define __SPXRESTEXT_H__
#include "raphters.h"
#include "rest_types.h"
#include <stdlib.h>
#include <string.h>
#include "fcgiapp.h"
#include "json-c/json.h"
#include "libipmi_struct.h"
#include "libipmi_IPM.h"
#include "libspx_rest.h"
#include "rest_common.h"

#define ADMIN 0x04
#define OPERATOR 0x03
#define USER 0x02
#define CALLBACK 0x01
#define RESERVED 0x00

CoreFeatures_T g_corefeatures;
CoreMacros_T g_coremacros;

typedef enum REST_MFP_ERROR_CODES
{
	COULD_NOT_ESTABLISH_NULL_IPMI_SESSION = 1000,
	INVALID_API_CALL,
	INVALID_PRIVILEGE_ACCESS = 8000,
	FEATURE_NOT_ENABLED = 8500,
	ERROR_GETTING_BUFFER_OVERFLOW,
	CANT_OPEN_MFP_REPORT
} REST_MFP_ERROR_CODES_T;

map_handler *last_map;
int IPMI_lo_Login(char *UserName, char *Password, IPMI20_SESSION_T *IPMISession, uint8 byPrivLevel, uint8 allownulluser, char *uname, char *ipstr, INT8U *ServIPAddr);
void IPMI_lo_Logout(IPMI20_SESSION_T *IPMISession);
extern struct json_object *json_object_object_get_rest(struct json_object *obj, const char *key);
#endif