#include "Transcript.h"
#include "Exceptions.h"

Transcript::Transcript()
{
}

QString Transcript::sourceToString(Transcript::SOURCE source)
{
	switch(source)
	{
		case CCDS:
			return "CCDS";
		case ENSEMBL:
			return "ENSEMBL";
	}

	THROW(ProgrammingException, "Unknown transcript source enum value '" + QString::number(source) + "!");
}

Transcript::SOURCE Transcript::stringToSource(QString source)
{
	source = source.toUpper();
	if (source=="CCDS")
	{
		return CCDS;
	}
	if (source=="ENSEMBL")
	{
		return ENSEMBL;
	}

	THROW(ProgrammingException, "Unknown transcript source string '" + source + "!");
}

QByteArray Transcript::strandToString(Transcript::STRAND strand)
{
	switch(strand)
	{
		case PLUS:
			return "+";
		case MINUS:
			return "-";
	}

	THROW(ProgrammingException, "Unknown transcript strand enum value '" + QString::number(strand) + "!");
}

Transcript::STRAND Transcript::stringToStrand(QByteArray strand)
{
	strand = strand.toUpper();
	if (strand=="+")
	{
		return PLUS;
	}
	if (strand=="-")
	{
		return MINUS;
	}

	THROW(ProgrammingException, "Unknown transcript strand string '" + strand + "!");
}

