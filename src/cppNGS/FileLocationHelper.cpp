#include "FileLocationHelper.h"
#include "QString"
#include <QDir>
#include <QFileInfo>

QString FileLocationHelper::pathTypeToString(PathType type)
{
	switch(type) {
		case PathType::PROJECT_FOLDER:
			return "PROJECT_FOLDER";
		case PathType::SAMPLE_FOLDER:
			return "SAMPLE_FOLDER";
		case PathType::BAM:
			return "BAM";
		case PathType::GSVAR:
			return "GSVAR";
		case PathType::VCF:
			return "VCF";
		case PathType::BAF:
			return "BAF";
		case PathType::CNV_CALLS:
			return "CNV_CALLS";
		case PathType::CNV_ESTIMATES:
			return "CNV_ESTIMATES";
		case PathType::OTHER:
			return "other";
	  default:
		 return "Invalid PathType";
   }
}


QString FileLocationHelper::getEvidenceFile(const QString& bam_file)
{
	if (!bam_file.endsWith(".bam", Qt::CaseInsensitive))
	{
		THROW(ArgumentException, "Invalid BAM file path \"" + bam_file + "\"!");
	}
	QFileInfo bam_file_info(bam_file);
	QDir evidence_dir(bam_file_info.absolutePath() + "/manta_evid/");
	QString ps_name = bam_file_info.fileName().left(bam_file_info.fileName().length() - 4);
	return evidence_dir.absoluteFilePath(ps_name + "_manta_evidence.bam");
}
