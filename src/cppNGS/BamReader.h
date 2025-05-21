#ifndef BAMREADER_H
#define BAMREADER_H

#include "cppNGS_global.h"
#include "Chromosome.h"
#include "Pileup.h"
#include "VariantList.h"
#include "FastaFileIndex.h"
#include "QBitArray"

#include "QHash"

#include "RefGenomeService.h"
#include "htslib/sam.h"
#include "htslib/cram.h"

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

		//Returns if the read is a supplementary alignment, i.e. the read is a supplementary part of the an alignment that could only be perfored when splitting the read in several separate pieces.
		bool isSupplementaryAlignment() const
		{
			return aln_->core.flag & BAM_FSUPPLEMENTARY;
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
		//Returns if the CIGAR is only insertion (or soft-clipped).
		bool cigarIsOnlyInsertion() const;

		//Returns the sequence bases.
		Sequence bases() const;
		//Sets the sequence bases.
		void setBases(const Sequence& bases);
		//Returns the n-th base.
		char base(int n) const
		{
			return seq_nt16_str[bam_seqi(bam_get_seq(aln_), n)];
		}
		//Fills the given vector with integer representations of bases ()
		QVector<int> baseIntegers() const;

		//Returns the sequence qualities - ASCII encoded in Illumina 1.8 format i.e. 0-41 equals '!"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJ'
		QByteArray qualities() const;
		//Sets the sequence qualities - ASCII encoded in Illumina 1.8 format i.e. 0-41 equals '!"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJ'
		void setQualities(const QByteArray& qualities);
		//Returns the quality of the n-th base (integer value).
		int quality(int n)const
		{
			return bam_get_qual(aln_)[n];
		}
		//Fills a bit array representing base qualities - a bit is set if the base quality is >= min_baseq
		//the array is of size al.end() - al.start() +1 and by that includes deletions
		void qualities(QBitArray& qualities, int min_baseq, int len) const;
		//Returns the string data of a tag.
		QByteArray tag(const QByteArray& tag) const;
		//Adds a tag to the alignment.
		void addTag(const QByteArray& tag, char type, const QByteArray& data);
		//Returns the integer data of a tag with type 'i'.
		int tagi(const QByteArray&) const;

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


		/**
		  @brief Returns the base and quality at a chromosomal position (1-based).
		  @note If the base is deleted, '-' with quality 255 is returned. If the base is skipped/soft-clipped, '~' with quality -1 is returned.
		*/
		QPair<char, int> extractBaseByCIGAR(int pos);
		QPair<char, int> extractBaseByCIGAR(int pos, int& actual_pos);

		/**
		  @brief Returns the indels at a chromosomal position (1-based) or a range when using the @p indel_window parameter.
		  @note Insertions: The string starts with '+' and then contains the bases (e.g. '+TT'). The position is the base @em before which insersion is located.
		  @note Deletions: the string starts with '-' and then contains the number of bases (e.g. '-2'). The position is the first deleted base.
		*/
		QList<Sequence> extractIndelsByCIGAR(int pos, int indel_window=0);

	protected:
		bam1_t* aln_;

		//friends
		friend class BamReader;
		friend class BamWriter;
};

//Variant details struct.
struct CPPNGSSHARED_EXPORT VariantDetails
{
	VariantDetails()
		: depth(std::numeric_limits<int>::quiet_NaN())
		, frequency(std::numeric_limits<double>::quiet_NaN())
		, mapq0_frac(std::numeric_limits<double>::quiet_NaN())
		, obs(std::numeric_limits<int>::quiet_NaN())
	{
	}

	int depth;
	double frequency;
	double mapq0_frac;
	int obs;
};

//C++ wrapper for htslib BAM file access
class CPPNGSSHARED_EXPORT BamReader
{
	public:
		//Default constructor
		BamReader(const QString& bam_file);
		//CRAM Constructor with explicit reference genome
		//reference genome is mandatory for CRAM support
        BamReader(const QString& bam_file, QString ref_genome);

		//Destructor
		~BamReader();

		//Returns the BAM header lines
		QByteArrayList headerLines() const;
		//Returns the genome build based on the length of the chr1 (works for human only). Throws an exception if it could not be determined.
		GenomeBuild build() const;
		/**
			@brief Returns true if reads from loaded BAM file are from long-read sequencing
			@warning WARNING: function changes the set region, use before setting a region or re-set your region
			@details Checks the BRCA1 locus for single-end reads
			@param reads	number of reads which are checked
		*/
		bool is_single_end(int reads=100);

		//Set region for alignment retrieval (1-based coordinates).
		void setRegion(const Chromosome& chr, int start, int end);

		//Get next alignment and stores it in @p al.
		bool getNextAlignment(BamAlignment& al)
		{
			int res = (iter_!=nullptr) ? sam_itr_next(fp_, iter_, al.aln_) : sam_read1(fp_, header_, al.aln_);
			if (res<-1)
			{
				THROW(FileAccessException, "Could not read next alignment in BAM/CRAM file " + bam_file_);
			}

			return res>=0;
		}

		//Returns all chromosomes stored in the BAM header.
		const QList<Chromosome>& chromosomes() const;

		//Returns a chromosome based on the chromosome ID. If the chomosome is not found, an ArgumentException is thrown.
		const Chromosome& chromosome(int chr_id) const;

		//Returns the size of a chromosome stored in the BAM header.
		int chromosomeSize(const Chromosome& chr) const;

		//Returns the size sum of all chromosomes stored in the BAM header.
		double genomeSize(bool include_special_chromosomes) const;

		/**
		  @brief Returns the pileup at the given chromosomal position (1-based).
		  @param indel_window The value controls how far up- and down-stream of the given postion, indels are considered to compensate for alignment differences. Indels are not reported when this parameter is set to -1.
		  @param include_not_properly_paired also uses reads which are not properly paired. This flag has to be set when used on long-read data.
		*/
		Pileup getPileup(const Chromosome& chr, int pos, int indel_window = -1, int min_mapq = 1, bool include_not_properly_paired = false, int min_baseq = 13);

		/**
			@bried Returns the depth/frequency for a variant (start, ref, obs in TSV style). If the depth is 0, quiet_NaN is returned as frequency.
			@param include_not_properly_paired also uses reads which are not properly paired. This flag has to be set when used on long-read data.
		*/
		VariantDetails getVariantDetails(const FastaFileIndex& reference, const Variant& variant, bool include_not_properly_paired);

		/**
		  @brief Returns indels for a chromosomal range (1-based) and the depth of the region.
		  @param include_not_properly_paired also uses reads which are not properly paired. This flag has to be set when used on long-read data.
		  @note Insertions are prefixed with '+', deletions with '-'.
		*/
		void getIndels(const FastaFileIndex& reference, const Chromosome& chr, int start, int end, QVector<Sequence>& indels, int& depth, double& mapq0_frac, bool include_not_properly_paired);


	protected:
		QString bam_file_;
		QList<Chromosome> chrs_;
		QHash<Chromosome, int> chrs_sizes_;
		samFile* fp_ = nullptr;
		sam_hdr_t* header_ = nullptr;
		hts_idx_t* index_ = nullptr;
		hts_itr_t* iter_  = nullptr;

		//Releases resources held by the iterator (index is not cleared)
		void clearIterator();
		void checkChromosomeLengths(const QString& ref_genome);
        void init(const QString& bam_file, QString ref_genome = QString());

		//"declared away" methods
		BamReader(const BamReader&) = delete;
		BamReader& operator=(const BamReader&) = delete;

		//friends
		friend class BamWriter;
		friend class BamWriter_Test;
};

#endif // BAMREADER_H
