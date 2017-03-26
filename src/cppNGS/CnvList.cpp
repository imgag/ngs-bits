#include "CnvList.h"
#include "Exceptions.h"
#include "Helper.h"

CopyNumberVariant::CopyNumberVariant()
	: chr_()
	, start_(0)
	, end_(0)
	, regions_()
	, cns_()
	, z_scores_()
	, genes_()
{
}

CopyNumberVariant::CopyNumberVariant(const Chromosome& chr, int start, int end, QStringList regions, QList<int> cns, QList<double> z_scores, QStringList genes)
	: chr_(chr)
	, start_(start)
	, end_(end)
	, regions_(regions)
	, cns_(cns)
	, z_scores_(z_scores)
	, genes_(genes)
{
	if (regions_.count()!=cns_.count() || regions_.count()!=z_scores_.count())
	{
		THROW(ProgrammingException, "Error while constructing copy-number variant. Region count mismatch for copy-numbers and/or z-scores!");
	}
}

CnvList::CnvList()
	: comments_()
	, variants_()
{
}

void CnvList::clear()
{
	comments_.clear();
	variants_.clear();
}

void CnvList::load(QString filename)
{
	//clear previous content
	clear();

	//parse input file
	QSharedPointer<QFile> file = Helper::openFileForReading(filename);
	QString sample;
	while (!file->atEnd())
	{
		QString line = file->readLine().trimmed();
		if (line.isEmpty()) continue;

		if (line.startsWith("#"))
		{
			if (line.startsWith("##"))
			{
				comments_.append(line.mid(2, line.length()-2));
			}
		}
		else
		{
			QStringList parts = line.split("\t");
			if (parts.count()<9) THROW(FileParseException, "Invalid CnvHunter file line: " + line);

			//sample
			if (!sample.isEmpty() && sample!=parts[3])
			{
				THROW(FileParseException, "Multi-sample CNV files are currently not supported! Found data for " + sample + "'  and '" + parts[3] + "'!");
			}
			sample = parts[3];

			//copy-numbers
			QList<int> cns;
			QStringList tmp = parts[6].split(",");
			foreach(QString t, tmp)
			{
				cns.append(t.toInt());
			}

			//z-scores
			QList<double> zs;
			tmp = parts[7].split(",");
			foreach(QString t, tmp)
			{
				zs.append(Helper::toDouble(t, "z-score value", line));
			}

			//genes
			tmp.clear();
			if (parts.count()>=10) tmp = parts[9].split(",");

			variants_.append(CopyNumberVariant(parts[0], parts[1].toInt(), parts[2].toInt(), parts[8].split(","), cns, zs, tmp));
		}
	}
}

