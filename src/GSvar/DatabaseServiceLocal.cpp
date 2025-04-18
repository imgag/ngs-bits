#include "DatabaseServiceLocal.h"
#include "Settings.h"

DatabaseServiceLocal::DatabaseServiceLocal()
    : enabled_(NGSD::isAvailable())
{
}

bool DatabaseServiceLocal::enabled() const
{
    return enabled_;
}

QString DatabaseServiceLocal::checkPassword(const QString user_name, const QString password) const
{
    checkEnabled(__PRETTY_FUNCTION__);
    return NGSD().checkPassword(user_name, password);
}

BedFile DatabaseServiceLocal::processingSystemRegions(int sys_id, bool ignore_if_missing) const
{
    checkEnabled(__PRETTY_FUNCTION__);
    return NGSD().processingSystemRegions(sys_id, ignore_if_missing);
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

FileInfo DatabaseServiceLocal::analysisJobLatestLogInfo(const int& job_id) const
{
    checkEnabled(__PRETTY_FUNCTION__);
    return NGSD().analysisJobLatestLogInfo(job_id);
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

FileLocation DatabaseServiceLocal::analysisJobLogFile(const int& job_id) const
{
    checkEnabled(__PRETTY_FUNCTION__);
    NGSD db;
	AnalysisJob job = db.analysisInfo(job_id, true);
	QString id = db.processedSampleName(db.processedSampleId(job.samples[0].name));
	QString log = analysisJobLatestLogInfo(job_id).file_name;

	//prepend folder
	QString folder = db.analysisJobFolder(job_id);
	log = folder + log;

	return FileLocation(id, PathType::OTHER, log, QFile::exists(log));
}

QList<MultiSampleAnalysisInfo> DatabaseServiceLocal::getMultiSampleAnalysisInfo(QStringList& analyses) const
{
    checkEnabled(__PRETTY_FUNCTION__);
    NGSD db;
	QList<MultiSampleAnalysisInfo> out;

	foreach(QString gsvar, analyses)
	{
		VariantList vl;
		vl.loadHeaderOnly(gsvar);

		MultiSampleAnalysisInfo analysis_info;
		analysis_info.analysis_file = gsvar;
		analysis_info.analysis_name = vl.analysisName();

		foreach(const SampleInfo& info, vl.getSampleHeader())
		{
			analysis_info.ps_sample_name_list.append(info.name);
			analysis_info.ps_sample_id_list.append(db.processedSampleId(info.name));
		}
		out.append(analysis_info);
	}
	return out;
}

QStringList DatabaseServiceLocal::getRnaFusionPics(const QString& rna_id) const
{
    checkEnabled(__PRETTY_FUNCTION__);
    return Helper::findFiles(processedSamplePath(NGSD().processedSampleId(rna_id), PathType::FUSIONS_PIC_DIR).filename, "*.png", false);
}

QStringList DatabaseServiceLocal::getRnaExpressionPlots(const QString& rna_id) const
{
    checkEnabled(__PRETTY_FUNCTION__);
    return Helper::findFiles(processedSamplePath(NGSD().processedSampleId(rna_id), PathType::SAMPLE_FOLDER).filename, rna_id + "_expr.*.png", false);
}
