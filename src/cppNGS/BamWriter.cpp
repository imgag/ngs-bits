#include "BamWriter.h"

#include <QFileInfo>

BamWriter::BamWriter(const QString& bam_file, const QString& ref_file)
	: bam_file_(QFileInfo(bam_file).absoluteFilePath())
{
	//open file pointer
	if(ref_file == "" || ref_file == QString::null)
	{
		fp_ = sam_open(bam_file.toLatin1().constData(), "wb");
	}
	else
	{
		fp_ = sam_open(bam_file.toLatin1().constData(), "wc");
	}

	if (fp_==nullptr)
	{
		THROW(FileAccessException, "Could not open file for writing: " + bam_file_);
	}

	//set reference for CRAM files
	if(fp_->is_cram)
	{
		int fai = hts_set_fai_filename(fp_, ref_file.toLatin1().constData());
		if(fai < 0)
		{
			THROW(FileAccessException, "Error while setting reference genome for cram file!");
		}
	}

}

BamWriter::~BamWriter()
{
	sam_close(fp_);
}
