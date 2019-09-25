#ifndef TRANSCRIPT_H
#define TRANSCRIPT_H

#include "cppNGS_global.h"
#include "BedFile.h"

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
	void setRegions(const BedFile& regions)
	{
		regions_ = regions;
	}

	///Converts source enum to string value.
	static QString sourceToString(SOURCE source);
	///Converts string to source enum.
	static SOURCE stringToSource(QString source);

	///Converts strand enum to string value.
    static QByteArray strandToString(STRAND strand);
	///Converts string to strand enum.
    static STRAND stringToStrand(QByteArray strand);

protected:
    QByteArray name_;
	SOURCE source_;
	STRAND strand_;
	BedFile regions_;
};

#endif // TRANSCRIPT_H
