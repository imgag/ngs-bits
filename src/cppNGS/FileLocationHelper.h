#ifndef FILELOCATIONHELPER_H
#define FILELOCATIONHELPER_H

#include "cppNGS_global.h"
#include "Exceptions.h"
#include "FileLocation.h"

class CPPNGSSHARED_EXPORT FileLocationHelper
{
public:
	///Returns a string representation for PathType
//	static QString pathTypeToString(PathType type);

	///Converts a string into PathType value
//	static PathType stringToPathType(QString in);

	///Returns the file path to the Manta evididence file for a given BAM file.
	static QString getEvidenceFile(const QString& bam_file);

	///Returns a list of URLs or file paths based on the provided list of the FileLocation type
	static QStringList getFileLocationsAsStringList(const QList<FileLocation>& file_locations);
};

#endif // FILELOCATIONHELPER_H
