#include "FileLocationHelper.h"
#include "QString"

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
