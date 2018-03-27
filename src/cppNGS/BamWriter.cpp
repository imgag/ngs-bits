#include "BamWriter.h"

#include <QFileInfo>

BamWriter::BamWriter(const QString& bam_file)
	: bam_file_(QFileInfo(bam_file).absoluteFilePath())
	, fp_(hts_open(bam_file.toLatin1().constData(), "wb"))
{
	if (fp_==nullptr)
	{
		THROW(FileAccessException, "Could not open BAM file for writing: " + bam_file_);
	}
}

BamWriter::~BamWriter()
{
	sam_close(fp_);
}
