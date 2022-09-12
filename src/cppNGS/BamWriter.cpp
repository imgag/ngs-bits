#include "BamWriter.h"
#include "Helper.h"

#include <QFileInfo>

BamWriter::BamWriter(const QString& bam_file, const QString& ref_file)
	: bam_file_(Helper::canonicalPath(bam_file))
{
	//open file pointer
	if(bam_file.endsWith(".bam"))
	{
		fp_ = sam_open(bam_file.toUtf8().constData(), "wb");
	}
	else if(bam_file.endsWith(".cram"))
	{
		if(ref_file != "" && ref_file != QString())
		{
			fp_ = sam_open(bam_file.toUtf8().constData(), "wc");
		}
		else
		{
			THROW(FileAccessException, "No reference genome used in BamWriter for file: " + bam_file_ + ".");
		}
	}
	else
	{
		THROW(FileAccessException, "Could not write file: " + bam_file_ + ". For writing BAM files use the file extension \'.bam\'; for writing CRAM files use the extension \'.cram\' with a reference genome.");
	}

	if (fp_==nullptr)
	{
		THROW(FileAccessException, "Could not open file for writing: " + bam_file_);
	}

	//set reference for CRAM files
	if(fp_->is_cram)
	{
		int fai = hts_set_fai_filename(fp_, ref_file.toUtf8().constData());
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
