#include "BamWriter.h"
#include "Helper.h"

#include <QFileInfo>

BamWriter::BamWriter(const QString& bam_file, const QString& ref_file)
	: bam_file_(Helper::canonicalPath(bam_file))
{
	if(bam_file_.endsWith(".bam"))
	{
		fp_ = sam_open(bam_file_.toUtf8().constData(), "wb");
	}
	else if(bam_file_.endsWith(".cram"))
	{
		fp_ = sam_open(bam_file_.toUtf8().constData(), "wc");

		//set reference for CRAM files
		if(ref_file=="")
		{
			THROW(FileAccessException, "No reference genome provided for writing CRAM file: " + bam_file_ + ".");
		}
		int fai = hts_set_fai_filename(fp_, ref_file.toUtf8().constData());
		if(fai < 0)
		{
			THROW(FileAccessException, "Error while setting reference genome for CRAM file " + bam_file_);
		}
	}
	else
	{
		THROW(FileAccessException, "Could not write file: " + bam_file_ + ". File extension has to be '.bam' or '.cram'.");
	}

	if (fp_==nullptr)
	{
		THROW(FileAccessException, "Could not open file for writing: " + bam_file_);
	}

	//apply optimizations
	hts_set_threads(fp_, 1); //one extra thread for compression
}

BamWriter::~BamWriter()
{
	close();
}

void BamWriter::close()
{
	if (!fp_closed_) sam_close(fp_);
	fp_closed_ = true;
}
