#include "BamWriter.h"

#include <QFileInfo>

BamWriter::BamWriter(const QString& bam_file, const QString& ref_file)
	: bam_file_(QFileInfo(bam_file).absoluteFilePath())
	, fp_(sam_open(bam_file.toLatin1().constData(), "wb"))
{
	if (fp_==nullptr)
	{
		THROW(FileAccessException, "Could not open file for writing: " + bam_file_);
	}

	//set new header for Cram file
	//int fai = cram_set_header(fp_->fp.cram, header_);
	//if(fai < 0)
	//{
	//	THROW(FileAccessException, "Reference genome could not be read from cram header, needed for reading cram file!");
	//}

}

BamWriter::~BamWriter()
{
	sam_close(fp_);
}
