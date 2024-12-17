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
		//writes CRAM file if a reference genome is given and file ends with '.cram'
		//writes BAM file if file ends with '.bam'
		BamWriter(const QString& bam_file, const QString& ref_file = QString());
		//Destructor
		~BamWriter();
		//close file (not necessary in most cases, as the file is closed in the constructor anyway)
		void close();

		//Write a BAM header from another BAM file
		void writeHeader(const BamReader& reader)
		{
			if (sam_hdr_write(fp_, reader.header_)==-1)
			{
				THROW(FileAccessException, "Could not write header to file " + bam_file_);
			}
			header_ = reader.header_;
		}

		//Write an alignment from another BAM file
		void writeAlignment(const BamAlignment& al)
		{
			if(sam_write1(fp_, header_, al.aln_)==-1)
			{
				THROW(FileAccessException, "Could not write alignment " + al.name() + " to file " + bam_file_);
			}
		}

	protected:
		QString bam_file_;
		samFile* fp_ = nullptr;
		sam_hdr_t* header_ = nullptr;
		bool fp_closed_ = false;

		//"declared away" methods
		BamWriter(const BamWriter&) = delete;
		BamWriter& operator=(const BamWriter&) = delete;
};

#endif // BAMWRITER_H
