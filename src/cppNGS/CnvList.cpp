#include "CnvList.h"
#include "Exceptions.h"
#include "Helper.h"
#include "TSVFileStream.h"
#include "BasicStatistics.h"

CopyNumberVariant::CopyNumberVariant()
	: chr_()
	, start_(0)
	, end_(0)
	, num_regs_()
	, genes_()
	, annotations_()
{
}

CopyNumberVariant::CopyNumberVariant(const Chromosome& chr, int start, int end, int num_regs, GeneSet genes, QByteArrayList annotations)
	: chr_(chr)
	, start_(start)
	, end_(end)
	, num_regs_(num_regs)
	, genes_(genes)
	, annotations_(annotations)
{
}

CnvList::CnvList()
	: type_()
	, comments_()
	, annotation_headers_()
	, variants_()
{
}

void CnvList::clear()
{
	type_.clear();
	comments_.clear();
	variants_.clear();
	annotation_headers_.clear();
}

void CnvList::load(QString filename)
{
	//clear previous content
	clear();

	//parse header
	TSVFileStream file(filename);
	QByteArray type_prefix = "##ANALYSISTYPE=";
	foreach(QByteArray line, file.comments())
	{
		if (line.startsWith(type_prefix))
		{
			type_ = line.mid(type_prefix.length()).trimmed();
		}
		else
		{
			comments_ << line;
		}
	}
	if (type().isEmpty())
	{
		THROW(FileParseException, "CNV file '" + filename + "' does not contain an ##ANALYSISTYPE header line. It is probably outdated, please re-run CNV calling!");
	}

	//handle column indices
	QVector<int> annotation_indices = BasicStatistics::range(file.columns(), 0, 1);
	int i_chr = file.colIndex("chr", true);
	annotation_indices.removeAll(i_chr);
	int i_start = file.colIndex("start", true);
	annotation_indices.removeAll(i_start);
	int i_end = file.colIndex("end", true);
	annotation_indices.removeAll(i_end);
	int i_genes = file.colIndex("genes", false);
	annotation_indices.removeAll(i_genes);
	int i_region_count = file.colIndex("region_count", false);
	annotation_indices.removeAll(i_region_count);

	if (type()=="CNVHUNTER_GERMLINE_SINGLE")
	{
		int i_sample = file.colIndex("sample", true);
		annotation_indices.removeAll(i_sample);
		int i_size = file.colIndex("size", true);
		annotation_indices.removeAll(i_size);
	}

	//parse annotation headers
	foreach(int index, annotation_indices)
	{
		annotation_headers_ << file.header()[index];
	}

	//parse input file
	while (!file.atEnd())
	{
		QByteArrayList parts = file.readLine();

		//regions
		int region_count = -1;
		if (i_region_count!=-1)
		{
			region_count = parts[i_region_count].toInt();
		}
		else
		{
			THROW(FileParseException, "No column with region/exon count found!");
		}

		//genes
		GeneSet genes;
		if (i_genes!=-1)
		{
			genes << GeneSet::createFromText(parts[i_genes], ',');
		}
		else
		{
			THROW(FileParseException, "No column with genes found!");
		}

		//parse annotation headers
		QByteArrayList annos;
		foreach(int index, annotation_indices)
		{
			annos << parts[index];
		}

		variants_.append(CopyNumberVariant(parts[i_chr], parts[i_start].toInt(), parts[i_end].toInt(), region_count, genes, annos));
	}
}

int CnvList::annotationIndexByName(const QByteArray& name, bool throw_on_error) const
{
	QList<int> matches;
	for(int i=0; i<annotation_headers_.count(); ++i)
	{
		if (annotation_headers_[i] == name )
		{
			matches.append(i);
		}
	}

	//Error handling
	if (matches.count()<1)
	{
		if (throw_on_error)
		{
			THROW(ArgumentException, "Could not find annotation column '" + name + "' in CNV list!");
		}
		else
		{
			return -1;
		}
	}

	if (matches.count()>1)
	{
		if (throw_on_error)
		{
			THROW(ArgumentException, "Found multiple annotation columns for '" + name + "' in CNV list!");
		}
		else
		{
			return -2;
		}
	}

	return matches.at(0);
}

long long CnvList::totalCnvSize()
{
	long long total_size = 0;
	for(const CopyNumberVariant& variant : variants_)
	{
		total_size += variant.size();
	}
	return total_size;
}
