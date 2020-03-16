#include "Transcript.h"
#include "Exceptions.h"

Transcript::Transcript()
	: coding_start_(0)
	, coding_end_(0)
{
}

void Transcript::setRegions(const BedFile& regions, int coding_start, int coding_end)
{
	regions_ = regions;

	coding_start_ = coding_start;
	coding_end_ = coding_end;

	if (isCoding())
	{
		coding_regions_.clear();

		for (int i=0; i<regions_.count(); ++i)
		{
			const BedLine& line = regions_[i];

			int start = std::max(line.start(), coding_start_);
			int end = std::min(line.end(), coding_end_);

			if (end<coding_start_ || start>coding_end_) continue;

			coding_regions_.append(BedLine(line.chr(), start, end));
		}

		coding_regions_.merge();
	}
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

int Transcript::cDnaToGenomic(int coord)
{
	if (coord<1) THROW(ArgumentException, "Invalid cDNA coordinate " + QString::number(coord) + " given for transcript " + name_ +"!");

	int tmp = coord;
	if(strand_==PLUS)
	{
		for (int i=0; i<coding_regions_.count(); ++i)
		{
			const BedLine& line = coding_regions_[i];
			int start = line.start();
			int end = line.end();
			int length = end - start + 1;

			tmp -= length;
			//qDebug() << start << end << length << tmp;

			if (tmp<=0) return end + tmp;
		}
	}
	else
	{
		for (int i=coding_regions_.count()-1; i>=0; --i)
		{
			const BedLine& line = coding_regions_[i];
			int start = line.start();
			int end = line.end();
			int length = end - start + 1;

			tmp -= length;
			//qDebug() << start << end << length << tmp;

			if (tmp<=0) return start - tmp;
		}
	}

	THROW(ArgumentException, "Invalid cDNA coordinate " + QString::number(coord) + " (bigger than coding region) given for transcript " + name_ +"!");
}

Variant Transcript::hgvsToVariant(QString hgvs_c)
{
	Variant variant;
	//TODO
	return variant;
}

