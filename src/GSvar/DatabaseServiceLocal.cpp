#include "DatabaseServiceLocal.h"
#include "Settings.h"

DatabaseServiceLocal::DatabaseServiceLocal()
	: enabled_(Settings::boolean("NGSD_enabled", true))
{
}

bool DatabaseServiceLocal::enabled() const
{
	return enabled_;
}

BedFile DatabaseServiceLocal::processingSystemRegions(int sys_id, bool ignore_if_missing) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	return NGSD().processingSystemRegions(sys_id, ignore_if_missing);
}

BedFile DatabaseServiceLocal::processingSystemAmplicons(int sys_id, bool ignore_if_missing) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	return NGSD().processingSystemAmplicons(sys_id, ignore_if_missing);
}

GeneSet DatabaseServiceLocal::processingSystemGenes(int sys_id, bool ignore_if_missing) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	return NGSD().processingSystemGenes(sys_id, ignore_if_missing);
}

QStringList DatabaseServiceLocal::secondaryAnalyses(QString processed_sample_name, QString analysis_type) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	QStringList output;

	QStringList analyses = NGSD().secondaryAnalyses(processed_sample_name, analysis_type);
	foreach(QString file, analyses)
	{
		if (QFile::exists(file))
		{
			output << file;
		}
	}

	return output;
}

FileLocation DatabaseServiceLocal::processedSamplePath(const QString& processed_sample_id, PathType type) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	QString id = NGSD().processedSampleName(processed_sample_id);
	QString filename = NGSD().processedSamplePath(processed_sample_id, type);
	return FileLocation(id, type, filename, QFile::exists(filename));
}

FileLocation DatabaseServiceLocal::analysisJobGSvarFile(const int& job_id) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	NGSD db;
	AnalysisJob job = db.analysisInfo(job_id, true);
	QString id = db.processedSampleName(db.processedSampleId(job.samples[0].name));
	QString filename = db.analysisJobGSvarFile(job_id);
	return FileLocation(id, PathType::GSVAR, filename, QFile::exists(filename));
}
