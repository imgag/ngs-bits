#ifndef TRANSCRIPT_H
#define TRANSCRIPT_H

#include "cppNGS_global.h"
#include "BedFile.h"
#include "VariantList.h"

///Representation of a gene transcript.
class CPPNGSSHARED_EXPORT Transcript
{
public:
	Transcript();

	bool isValid() const
	{
		return !(name_.isEmpty());
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
	void setRegions(const BedFile& regions, int coding_start=0, int coding_end=0);

	//Returns if the transcript is coding
	bool isCoding() const
	{
		return coding_start_!=0 && coding_end_!=0;
	}
	//Returns the start of the coding region
	int codingStart() const
	{
		return coding_start_;
	}
	//Returns the end of the coding region (including stop codon)
	int codingEnd() const
	{
		return coding_end_;
	}
	//Returns the coding regions (empty for non-coding transcripts)
	const BedFile& codingRegions() const
	{
		return coding_regions_;
	}

	///Converts source enum to string value.
	static QString sourceToString(SOURCE source);
	///Converts string to source enum.
	static SOURCE stringToSource(QString source);

	///Converts strand enum to string value.
    static QByteArray strandToString(STRAND strand);
	///Converts string to strand enum.
    static STRAND stringToStrand(QByteArray strand);

	///Converts a cDNA coordinate to genomic coordinates. Throws an exception if the cDNA-coordinate is not valid.
	int cDnaToGenomic(int cdna_cordinate);

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

	void hgvsParsePosition(const QString& position, int& pos, int& offset);
};

#endif // TRANSCRIPT_H
