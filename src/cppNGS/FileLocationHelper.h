#ifndef FILELOCATIONHELPER_H
#define FILELOCATIONHELPER_H

#include "cppNGS_global.h"
#include "Exceptions.h"

enum class PathType
{
	PROJECT_FOLDER, // project root folder
	SAMPLE_FOLDER, // folder with samples
	BAM, //
	GSVAR, //
	VCF, //
	BAF, //
	CNV_CALLS, // BED files
	CNV_ESTIMATES, // SEG file with copy
	OTHER // everything else
};

class CPPNGSSHARED_EXPORT FileLocationHelper
{
public:
	///Returns a string representation for PathType
	static QString pathTypeToString(PathType type);

	//Returns the file path to the Manta evididence file for a given BAM file.
	static QString getEvidenceFile(const QString& bam_file);

};

#endif // FILELOCATIONHELPER_H
