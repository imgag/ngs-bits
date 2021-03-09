#include "FileLocationHelper.h"
#include "QString"
#include <QDir>
#include <QFileInfo>

QString FileLocationHelper::pathTypeToString(PathType type)
{
	switch(type)
	{
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
		case PathType::COPY_NUMBER_CALLS:
			return "COPY_NUMBER_CALLS";
		case PathType::COPY_NUMBER_RAW_DATA:
			return "COPY_NUMBER_RAW_DATA";
		case PathType::MANTA_EVIDENCE:
			return "MANTA_EVIDENCE";
		case PathType::OTHER:
			return "OTHER";
		default:
		 return "Invalid PathType";
	}
}

PathType FileLocationHelper::stringToPathType(QString in)
{
	if (in.toUpper() == "PROJECT_FOLDER") return PathType::PROJECT_FOLDER;
	if (in.toUpper() == "SAMPLE_FOLDER") return PathType::SAMPLE_FOLDER;
	if (in.toUpper() == "BAM") return PathType::BAM;
	if (in.toUpper() == "GSVAR") return PathType::GSVAR;
	if (in.toUpper() == "VCF") return PathType::VCF;
	if (in.toUpper() == "BAF") return PathType::BAF;
	if (in.toUpper() == "COPY_NUMBER_CALLS") return PathType::COPY_NUMBER_CALLS;
	if (in.toUpper() == "COPY_NUMBER_RAW_DATA") return PathType::COPY_NUMBER_RAW_DATA;
	if (in.toUpper() == "MANTA_EVIDENCE") return PathType::MANTA_EVIDENCE;

	return PathType::OTHER;
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

QStringList FileLocationHelper::getFileLocationsAsStringList(const QList<FileLocation>& file_locations)
{
	QStringList output {};
	for (int i = 0; i < file_locations.count(); i++)
	{
		output.append(file_locations[i].filename);
	}
	return output;
}
