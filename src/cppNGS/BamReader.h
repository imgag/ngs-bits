#ifndef BAMREADER_H
#define BAMREADER_H

#include "cppNGS_global.h"
#include "Chromosome.h"

#include "QHash"

#include <htslib/sam.h>

//Representation of a CIGAR operation
struct CPPNGSSHARED_EXPORT CigarOp
{
	int Type;
	int Length;

	//Converts the CIGAR type to a char
	char typeAsChar() const
	{
		return BAM_CIGAR_STR[Type];
	}
};

//Representation of a BAM alignment
class CPPNGSSHARED_EXPORT BamAlignment
{
	public:
		//Default constructor
		BamAlignment();
		//Destructor
		~BamAlignment();
		//Copy constructor (makes a deep copy of the alignment > slow)
		BamAlignment(const BamAlignment& rhs);
		//Assignment operator
		BamAlignment& operator=(const BamAlignment& rhs)
		{
			aln_ = bam_dup1(rhs.aln_);
			return *this;
		}

		//Returns the read name
		QByteArray name() const
		{
			return QByteArray::fromStdString(bam_get_qname(aln_));
		}

		int chromosomeID() const
		{
			return aln_->core.tid;
		}

		//Returns the start position of the alignment (1-based).
		int start() const
		{
			return aln_->core.pos + 1;
		}
		//Sets the start position of the alignment (1-based).
		void setStart(int start)
		{
			aln_->core.pos = start - 1;
		}

		//Returns the end position of the alignment (1-based).
		int end() const
		{
			return bam_endpos(aln_);
		}

		//Returns the read length.
		int length() const
		{
			return aln_->core.l_qseq;
		}

		//Returns the insert size of the read pair.
		int insertSize() const
		{
			return aln_->core.isize;
		}
		//Sets the insert size
		void setInsertSize(int insert_size)
		{
			aln_->core.isize = insert_size;
		}

		//Returns the mapping quality of the read
		int mappingQuality() const
		{
			return aln_->core.qual;
		}
		//Sets the mapping quality of the read
		void setMappingQuality(int mapping_quality)
		{
			aln_->core.qual = mapping_quality;
		}

		//Returns if the read is paired in sequencing, no matter if it is mapped in a pair.
		bool isPaired() const
		{
			return aln_->core.flag & BAM_FPAIRED;
		}

		//Returns if the read is properly paired in mapping (same chromosome and within the normal insert size distribution).
		bool isProperPair() const
		{
			return aln_->core.flag & BAM_FPROPER_PAIR;
		}

		//Returns if the read is a secondary alignment, i.e. it is usually ignored.
		bool isSecondaryAlignment() const
		{
			return aln_->core.flag & BAM_FSECONDARY;
		}
		//Sets if the read is a secondary alignment.
		void setIsSecondaryAlignment(bool state)
		{
			if (isSecondaryAlignment()!=state)
			{
				aln_->core.flag ^= BAM_FSECONDARY;
			}
		}

		//Returns if the read is unmapped, i.e. it is usually ignored.
		bool isUnmapped() const
		{
			return aln_->core.flag & BAM_FUNMAP;
		}
		//Sets if the read is unmapped.
		void setIsUnmapped(bool state)
		{
			if (isUnmapped()!=state)
			{
				aln_->core.flag ^= BAM_FUNMAP;
			}
		}

		//Returns if the read is a PCR or optical duplicate.
		bool isDuplicate() const
		{
			return aln_->core.flag & BAM_FDUP;
		}

		//Returns if the read is Read 1 (of a read pair)
		bool isRead1() const
		{
			return aln_->core.flag & BAM_FREAD1;
		}
		//Returns if the read is Read 2 (of a read pair)
		bool isRead2() const
		{
			return aln_->core.flag & BAM_FREAD2;
		}

		//Returns if the read is mapped to the reverse strand
		bool isReverseStrand() const
		{
			return aln_->core.flag & BAM_FREVERSE;
		}

		//Returns the CIGAR data.
		QList<CigarOp> cigarData() const;
		//Sets the CIGAR data.
		void setCigarData(const QList<CigarOp>& cigar);
		//Returns the CIGAR data as a string.
		QByteArray cigarDataAsString(bool expand=false) const;

		//Returns the sequence bases.
		QByteArray bases() const;
		//Sets the sequence bases.
		void setBases(const QByteArray& bases);
		//Returns the quality of the n-th base.
		char base(int n) const
		{
			return seq_nt16_str[bam_seqi(bam_get_seq(aln_), n)];
		}

		//Returns the sequence qualities - ASCII encoded in Illumina 1.8 format i.e. 0-41 equals '!"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJ'
		QByteArray qualities() const;
		//Sets the sequence qualities - ASCII encoded in Illumina 1.8 format i.e. 0-41 equals '!"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJ'
		void setQualities(const QByteArray& qualities);
		//Returns the quality of the n-th base (integer value).
		int quality(int n)const
		{
			return bam_get_qual(aln_)[n];
		}

		//Returns the string data of a tag.
		QByteArray tag(const QByteArray& tag) const;
		//Adds a tag to the alignment.
		void addTag(const QByteArray& tag, char type, const QByteArray& data);

		/**************************** MATE functions ****************************/

		//returnt the mate start position
		int mateStart() const
		{
			return aln_->core.mpos;
		}
		void setMateStart(int start)
		{
			aln_->core.mpos = start -1;
		}
		//Returns the mate chromosome ID
		int mateChrosomeID() const
		{
			return aln_->core.mtid;
		}
		//Returns if the mate is mapped to the reverse strand
		bool isMateReverseStrand() const
		{
			return aln_->core.flag & BAM_FMREVERSE;
		}
		//Returns if the mate is unmapped, i.e. it is usually ignored.
		bool isMateUnmapped() const
		{
			return aln_->core.flag & BAM_FMUNMAP;
		}

	protected:
		bam1_t* aln_;

		//friends
		friend class BamReader;
		friend class BamWriter;
};

//C++ wrapper for htslib BAM file access
class CPPNGSSHARED_EXPORT BamReader
{
	public:
		//Default constructor
		BamReader(const QString& bam_file);
		//Destructor
		~BamReader();

		//Returns the BAM header lines
		QByteArrayList headerLines() const;

		//Set region for alignment retrieval (1-based coordinates).
		void setRegion(const Chromosome& chr, int start, int end);

		//Get next alignment and stores it in @p al.
		bool getNextAlignment(BamAlignment& al);

		//Returns all chromosomes stored in the BAM header.
		const QList<Chromosome>& chromosomes() const;

		//Returns a chromosome based on the chromosome ID. If the chomosome is not found, an ArgumentException is thrown.
		const Chromosome& chromosome(int chr_id) const;

		//Returns the size of a chromosome stored in the BAM header.
		int chromosomeSize(const Chromosome& chr) const;

		//Returns the size sum of all chromosomes stored in the BAM header.
		double genomeSize(bool nonspecial_only) const;

	protected:
		QString bam_file_;
		QList<Chromosome> chrs_;
		QHash<Chromosome, int> chrs_sizes_;
		samFile* fp_ = nullptr;
		bam_hdr_t* header_ = nullptr;
		hts_idx_t* index_ = nullptr;
		hts_itr_t* iter_  = nullptr;

		//Releases resources held by the iterator (index is not cleared)
		void clearIterator();

		//"declared away" methods
		BamReader(const BamReader&) = delete;
		BamReader& operator=(const BamReader&) = delete;

		//friends
		friend class BamWriter;
};

#endif // BAMREADER_H
