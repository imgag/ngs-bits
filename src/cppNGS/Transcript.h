#ifndef TRANSCRIPT_H
#define TRANSCRIPT_H

#include "cppNGS_global.h"
#include "BedFile.h"
#include "VariantList.h"

///Representation of a gene transcript.
class CPPNGSSHARED_EXPORT Transcript
{
public:
	///Default construtor - creates an invalid transcript
	Transcript();

	bool isValid() const
	{
		return strand_!=INVALID && !(name_.isEmpty());
	}

    const QByteArray& name() const
	{
		return name_;
	}
    void setName(const QByteArray& name)
	{
		name_ = name;
	}

	enum SOURCE
	{
		CCDS,
		ENSEMBL
	};
	SOURCE source() const
	{
		return source_;
	}
	void setSource(SOURCE source)
	{
		source_ = source;
	}

	enum STRAND
	{
		INVALID,
		PLUS,
		MINUS
	};
	STRAND strand() const
	{
		return strand_;
	}
	void setStrand(STRAND strand)
	{
		strand_ = strand;
	}

	const BedFile& regions() const
	{
		return regions_;
	}
	//Sets the regions. If coding start and end is given, the coding regions and UTRs are calculated as well.
	void setRegions(const BedFile& regions, int coding_start=0, int coding_end=0);

	//Returns if the transcript is coding
	bool isCoding() const
	{
		return coding_start_!=0 && coding_end_!=0;
	}
	//Returns the start of the coding region (0 for non-coding transcripts)
	int codingStart() const
	{
		return coding_start_;
	}
	//Returns the end of the coding region (0 for non-coding transcripts)
	int codingEnd() const
	{
		return coding_end_;
	}
	//Returns the coding regions (empty for non-coding transcripts)
	const BedFile& codingRegions() const
	{
		return coding_regions_;
	}
	//Returns the 3' UTR regions, i.e. the after the end codon (empty for non-coding transcripts)
	const BedFile& utr3prime() const
	{
		return utr_3prime_;
	}
	//Returns the 5' UTR regions, i.e. the before the start codon (empty for non-coding transcripts)
	const BedFile& utr5prime() const
	{
		return utr_5prime_;
	}

	//Returns the exon number of a region. Error codes: -1 if no exon overlaps, -2 if several exons overlap
	int exonNumber(int start, int end) const;

	///Converts source enum to string value.
	static QString sourceToString(SOURCE source);
	///Converts string to source enum.
	static SOURCE stringToSource(QString source);

	///Converts strand enum to string value.
    static QByteArray strandToString(STRAND strand);
	///Converts string to strand enum.
    static STRAND stringToStrand(QByteArray strand);

	///Converts a cDNA coordinate to genomic coordinates. Throws an exception if the coordinate is not valid.
	int cDnaToGenomic(int coord) const;
	///Converts a non-coding DNA coordinate to genomic coordinates. Throws an exception if the coordinate is not valid.
	int nDnaToGenomic(int coord) const;

	///Converts a HGVS cDNA change to a variant in GSvar format.
	Variant hgvsToVariant(QString hgvs_c, const FastaFileIndex& genome_idx);

protected:
    QByteArray name_;
	SOURCE source_;
	STRAND strand_;
	BedFile regions_;
	int coding_start_;
	int coding_end_;
	BedFile coding_regions_;
	BedFile utr_3prime_;
	BedFile utr_5prime_;

	///Auxilary function: parses a HGVS.c position (single position, not a range!)
	void hgvsParsePosition(const QString& position, bool non_coding, int& pos, int& offset) const;
	///Auxilary function: corrects 5'UTR offset when UTR is split into several regions
	void correct5PrimeUtrOffset(int& offset) const;
	///Auxilary function: corrects 3'UTR offset when UTR is split into several regions
	void correct3PrimeUtrOffset(int& offset) const;
	///Returns the genomic end position of the 5' UTR, i.e. the last base before the start codon
	int utr5primeEnd() const;
	///Returns the genomic start position of the 3' UTR, i.e. the first base after the stop codon
	int utr3primeStart() const;
};

#endif // TRANSCRIPT_H
