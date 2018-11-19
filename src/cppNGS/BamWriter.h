#ifndef BAMWRITER_H
#define BAMWRITER_H

#include "cppNGS_global.h"
#include "BamReader.h"
#include "Exceptions.h"

//C++ wrapper for htslib BAM file access
class CPPNGSSHARED_EXPORT BamWriter
{
	public:
		//Default constructor
		BamWriter(const QString& bam_file);
		//Destructor
		~BamWriter();

		//Write a BAM header from another BAM file
		void writeHeader(const BamReader& reader)
		{
			if (bam_hdr_write(fp_->fp.bgzf, reader.header_)==-1)
			{
				THROW(FileAccessException, "Could not write header to BAM file " + bam_file_);
			}
		}

		//Write an alignment from another BAM file
		void writeAlignment(const BamAlignment& al)
		{
			if(bam_write1(fp_->fp.bgzf, al.aln_)==-1)
			{
				THROW(FileAccessException, "Could not write alignment " + al.name() + " to BAM file " + bam_file_);
			}
		}

	protected:
		QString bam_file_;
		samFile* fp_ = nullptr;

		//"declared away" methods
		BamWriter(const BamWriter&) = delete;
		BamWriter& operator=(const BamWriter&) = delete;
};

#endif // BAMWRITER_H
