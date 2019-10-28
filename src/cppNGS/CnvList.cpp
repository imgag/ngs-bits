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

CopyNumberVariant::CopyNumberVariant(const Chromosome& chr, int start, int end)
	: chr_(chr)
	, start_(start)
	, end_(end)
	, num_regs_(-1)
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

QString CopyNumberVariant::toStringWithMetaData() const
{
	return toString() + " regions=" + QString::number(num_regs_) + " size=" + QString::number((end_-start_)/1000.0, 'f', 3) + "kb";
}

int CopyNumberVariant::copyNumber(const QByteArrayList& annotation_headers, bool throw_if_not_found) const
{
	for (int i=0; i<annotation_headers.count(); ++i)
	{
		if (annotation_headers[i]=="CN_change") //ClinCNV
		{
			return annotations_[i].toInt();
		}
		else if (annotation_headers[i]=="region_copy_numbers") //CnvHunter
		{
			QByteArrayList parts = annotations_[i].split(',');

			int max = 0;
			QByteArray max_cn;
			QHash<QByteArray, int> cn_counts;
			foreach(const QByteArray& cn, parts)
			{
				int count_new = cn_counts[cn] + 1;
				if (count_new>max)
				{
					max = count_new;
					max_cn = cn;
				}
				cn_counts[cn] = count_new;
			}

			return Helper::toInt(max_cn, "copy-number");
		}
	}

	if (throw_if_not_found)
	{
		THROW(ProgrammingException, "Copy-number could not be determine for CNV: " + toString());
	}

	return -1;
}

CnvList::CnvList()
	: type_(CnvListType::INVALID)
	, comments_()
	, annotation_headers_()
	, variants_()
{
}

void CnvList::clear()
{
	type_ = CnvListType::INVALID;
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
		if (line.startsWith(type_prefix)) //analysis type
		{
			QByteArray type = line.mid(type_prefix.length()).trimmed();
			if (type=="CNVHUNTER_GERMLINE_SINGLE") type_ = CnvListType::CNVHUNTER_GERMLINE_SINGLE;
			else if (type=="CNVHUNTER_GERMLINE_MULTI") type_ = CnvListType::CNVHUNTER_GERMLINE_MULTI;
			else if (type=="CLINCNV_GERMLINE_SINGLE") type_ = CnvListType::CLINCNV_GERMLINE_SINGLE;
			else if (type=="CLINCNV_GERMLINE_MULTI") type_ = CnvListType::CLINCNV_GERMLINE_MULTI;
			else if (type=="CLINCNV_TUMOR_NORMAL_PAIR") type_ = CnvListType::CLINCNV_TUMOR_NORMAL_PAIR;
			else THROW(FileParseException, "CNV file '" + filename + "' contains unknown analysis type: " + type);
		}
		else if (line.startsWith("##DESCRIPTION=")) //header descriptions
		{
			QByteArrayList parts = line.trimmed().split('=');
			if (parts.count()>2)
			{
				setHeaderDesciption(parts[1], parts[2]);
			}
		}
		else //all other header lines
		{
			comments_ << line;
		}
	}
	if (type()==CnvListType::INVALID)
	{
		THROW(FileParseException, "CNV file '" + filename + "' is outdated. It does not contain an ##ANALYSISTYPE header line. Please re-run CNV calling!");
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
	int i_region_count = -1;

	if (type()==CnvListType::CNVHUNTER_GERMLINE_SINGLE)
	{
		//mandatory columns
		i_region_count = file.colIndex("region_count", false);
		annotation_indices.removeAll(i_region_count);
		//remove columns
		int i_sample = file.colIndex("sample", true);
		annotation_indices.removeAll(i_sample);
		int i_size = file.colIndex("size", true);
		annotation_indices.removeAll(i_size);
	}
	else if (type()==CnvListType::CNVHUNTER_GERMLINE_MULTI)
	{
		//mandatory columns
		i_region_count = file.colIndex("region_count", false);
		annotation_indices.removeAll(i_region_count);
		//remove columns
		int i_sample = file.colIndex("sample", true);
		annotation_indices.removeAll(i_sample);
		int i_size = file.colIndex("size", true);
		annotation_indices.removeAll(i_size);
	}
	else if (type()==CnvListType::CLINCNV_GERMLINE_SINGLE)
	{
		//mandatory columns
		i_region_count = file.colIndex("no_of_regions", false);
		annotation_indices.removeAll(i_region_count);
		//remove
		int i_size = file.colIndex("length_KB", true);
		annotation_indices.removeAll(i_size);
	}
	else if (type()==CnvListType::CLINCNV_GERMLINE_MULTI)
	{
		//mandatory columns
		i_region_count = -2; //not present
		//remove
		int i_sample = file.colIndex("sample", true);
		annotation_indices.removeAll(i_sample);
		int i_size = file.colIndex("size", true);
		annotation_indices.removeAll(i_size);
	}
	else if (type()==CnvListType::CLINCNV_TUMOR_NORMAL_PAIR)
	{
		//mandatory columns
		i_region_count = file.colIndex("number_of_regions", false);
		annotation_indices.removeAll(i_region_count);
		//remove
		int i_sample = file.colIndex("sample", true);
		annotation_indices.removeAll(i_sample);
		int i_size = file.colIndex("size", true);
		annotation_indices.removeAll(i_size);
	}
	else
	{
		THROW(NotImplementedException, "Column handling for this CNV list with type not implemented!");
	}

	//check mandatory columns were found
	if (i_region_count==-1) THROW(FileParseException, "No column with region/exon count found!");
	if (i_genes==-1) THROW(FileParseException, "No column with genes found!");

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
		int region_count = 0;
		if (i_region_count>=0)
		{
			 region_count = parts[i_region_count].toInt();
		}

		//genes
		GeneSet genes = GeneSet::createFromText(parts[i_genes], ',');

		//parse annotation headers
		QByteArrayList annos;
		foreach(int index, annotation_indices)
		{
			annos << parts[index];
		}

		variants_.append(CopyNumberVariant(parts[i_chr], parts[i_start].toInt(), parts[i_end].toInt(), region_count, genes, annos));
	}
}

CnvCallerType CnvList::caller() const
{
	CnvListType list_type = type();
	if (list_type==CnvListType::INVALID)
	{
		return CnvCallerType::INVALID;
	}
	else if (list_type==CnvListType::CNVHUNTER_GERMLINE_SINGLE || list_type==CnvListType::CNVHUNTER_GERMLINE_MULTI)
	{
		return CnvCallerType::CNVHUNTER;
	}
	else if (list_type==CnvListType::CLINCNV_GERMLINE_SINGLE || list_type==CnvListType::CLINCNV_GERMLINE_MULTI || list_type==CnvListType::CLINCNV_TUMOR_NORMAL_PAIR)
	{
		return CnvCallerType::CLINCNV;
	}
	else
	{
		THROW(ProgrammingException, "Cnv list type not handled in CnvList::caller()!");
	}
}

QByteArray CnvList::headerDescription(QByteArray name) const
{
	return annotation_header_desc_.value(name, "");
}

void CnvList::setHeaderDesciption(QByteArray name, QByteArray desciption)
{
	annotation_header_desc_[name] = desciption;
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
