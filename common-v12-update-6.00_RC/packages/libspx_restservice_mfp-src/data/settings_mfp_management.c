#include "spx_rest_mfp.h"
#include "mfp.h"
#include "mfp_ami.h"
#define MFP_REPORT "/conf/mfp_report"

START_AUTHORIZED_COLLECTION(getMfpInformation, GET, "/maintenance/mfp-information", 1, matches, true)
{
	if (0)
	{
		matches = matches;
	}
	ENFORCE_COLLECTION_FEATURECHECK(IsFeatureEnabled("CONFIG_SPX_FEATURE_MFP"));
	FILE *fpRep;
	ami_mfp_evaluate_result mfpResult;
	fpRep = fopen(MFP_REPORT, "rb"); //rb due to binary file
	if (fpRep == NULL)
	{
		TCRIT("Unable to open file \n\n");
		THROW_COLLECTION_ERROR(STATUS_500, "Unable  to open file", CANT_OPEN_MFP_REPORT);
	}
	while (fread(&mfpResult, sizeof(ami_mfp_evaluate_result), 1, fpRep) == 1)
	{
		COLLECTION_NEW_MODEL();
		MODEL_ADD_INTEGER("dim_id", mfpResult.dimmID);
		MODEL_ADD_INTEGER("socket", mfpResult.result.loc.socket);
		MODEL_ADD_INTEGER("imc", mfpResult.result.loc.imc);
		MODEL_ADD_INTEGER("channel", mfpResult.result.loc.channel);
		MODEL_ADD_INTEGER("slot", mfpResult.result.loc.dimm);
		MODEL_ADD_INTEGER("score", mfpResult.result.score);
		COLLECTION_ADD_MODEL();
	}
	fclose(fpRep);
	COLLECTION_OUTPUT();
}
END_AUTHORIZED_COLLECTION
int settings_mfp_management()
{
	add_handler(getMfpInformation);
	return 0;
}
