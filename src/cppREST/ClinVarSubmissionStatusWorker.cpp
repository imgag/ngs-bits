#include "ClinVarSubmissionStatusWorker.h"
#include "NGSD.h"

ClinVarSubmissionStatusWorker::ClinVarSubmissionStatusWorker()
{
}

void ClinVarSubmissionStatusWorker::run()
{
	Log::info("Starting ClinVar submission status worker");
	try
	{
		QPair<int,int> var_counts = NGSD().updateClinvarSubmissionStatus(false);
		Log::info("The submission status of " + QString::number(var_counts.first) + " published varaints has been checked, " + QString::number(var_counts.second) + " NGSD entries were updated." );
	}
	catch (DatabaseException& e)
	{
		Log::error("A database error has been detected while updating a ClinVar submission status: " + e.message());
	}
	catch (Exception& e)
	{
		Log::error("An error has been detected while updating a ClinVar submission status: " + e.message());
	}
}
