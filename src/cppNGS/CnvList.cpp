#include "CnvList.h"
#include "Exceptions.h"
#include "Helper.h"
#include "TSVFileStream.h"
#include "BasicStatistics.h"
#include "KeyValuePair.h"
#include "NGSHelper.h"
#include <QFileInfo>

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
		if(annotation_headers[i] == "tumor_CN_change") //ClinCNV somatic
		{
			return annotations_[i].toInt();
		}
		else if (annotation_headers[i]=="CN_change") //ClinCNV germline
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

void CopyNumberVariant::setCopyNumber(int cn, const QByteArrayList& annotation_headers, bool throw_if_not_found)
{
	bool found = false;
	for (int i=0; i<annotation_headers.count(); ++i)
	{
		if (annotation_headers[i]=="CN_change") //ClinCNV germline
		{
			annotations_[i] = QByteArray::number(cn);
			found = true;
		}
	}

	if (!found && throw_if_not_found)
	{
		THROW(ProgrammingException, "Copy-number could not be determine for CNV: " + toString());
	}
}

CnvList::CnvList()
	: type_(CnvListType::INVALID)
	, comments_()
	, annotation_headers_()
	, variants_()
{
}

bool CnvList::isValid() const
{
	return type_!=CnvListType::INVALID;
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
	loadInternal(filename, false);
}

void CnvList::loadHeaderOnly(QString filename)
{
	loadInternal(filename, true);
}

void CnvList::loadInternal(QString filename, bool header_only)
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
			else if (type=="CLINCNV_TUMOR_ONLY") type_ = CnvListType::CLINCNV_TUMOR_ONLY;
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
		int no_of_regions_idx = file.colIndex("no_of_regions", false);
		i_region_count = no_of_regions_idx>=0 ? no_of_regions_idx : -2; //not present
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
	else if (type()==CnvListType::CLINCNV_TUMOR_ONLY)
	{
		//mandatory columns
		i_region_count = file.colIndex("no_of_regions", false);
		annotation_indices.removeAll(i_region_count);
		//remove
		int i_size = file.colIndex("length_KB", true);
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

	//parse content
	if (!header_only)
	{
		while (!file.atEnd())
		{
			QByteArrayList parts = file.readLine();
			if(parts.empty()) continue;

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
}


void CnvList::store(QString filename)
{
	// check if CnvListType is valid
	if (type()==CnvListType::INVALID) THROW(NotImplementedException, "Invalid CnvListType! Cannot create file.");

	//open stream
	QSharedPointer<QFile> file = Helper::openFileForWriting(filename, true);
	QTextStream stream(file.data());

	//write header lines

	//analysis type
	stream << "##ANALYSISTYPE=" << typeAsString() << "\n";

	//comments
	foreach (const QByteArray& comment_line, comments_)
	{
		stream << comment_line << "\n";
	}

	//description
	foreach (QByteArray header, annotation_headers_)
	{
		if (annotation_header_desc_[header].trimmed() != "")
		{
			stream << "##DESCRIPTION=" << header << "=" << annotation_header_desc_[header] << "\n";
		}
	}

	// header line
	stream << "#chr\tstart\tend";
	QByteArrayList header_line = annotation_headers_;

	if (type()==CnvListType::CLINCNV_GERMLINE_SINGLE)
	{
		// assemble header line
		header_line.insert(2, "no_of_regions");
		header_line.insert(3, "length_KB");
		header_line.insert(5, "genes");
	}
	else if (type()==CnvListType::CLINCNV_GERMLINE_MULTI)
	{
		// assemble header line
		header_line.insert(0, "sample");
		header_line.insert(1, "size");
		header_line.insert(9, "genes");
	}
	else if (type()==CnvListType::CNVHUNTER_GERMLINE_SINGLE)
	{
		// assemble header line
		header_line.insert(0, "sample");
		header_line.insert(1, "size");
		header_line.insert(2, "region_count");
		header_line.insert(8, "genes");
	}
	else if (type()==CnvListType::CLINCNV_TUMOR_NORMAL_PAIR)
	{
		// assemble header line
		header_line.insert(0, "sample");
		header_line.insert(1, "size");
		header_line.insert(9, "number_of_regions");
		header_line.insert(10, "genes");
	}
	else if(type() == CnvListType::CLINCNV_TUMOR_ONLY)
	{
		// assemble header line
		header_line.insert(5, "no_of_regions");
		header_line.insert(6, "length_KB");
		header_line.insert(8, "genes");
	}
	else
	{
		THROW(NotImplementedException, "Export of this CnvListType is not supported!");
	}

	stream << "\t" << header_line.join("\t") << "\n";

	// CNVs
	foreach (CopyNumberVariant variant, variants_)
	{
		// write position:
		stream << variant.chr().strNormalized(true) << "\t" << variant.start() << "\t" << variant.end();
		QByteArrayList cnv_annotations = variant.annotations();

		if (type()==CnvListType::CLINCNV_GERMLINE_SINGLE)
		{
			// assemble CNV line
			cnv_annotations.insert(2, QByteArray::number(variant.regions()));
			cnv_annotations.insert(3, QByteArray::number(((variant.size() - 1)/1000.0), 'f', 3).rightJustified(8, ' '));
			cnv_annotations.insert(5, variant.genes().toStringList().join(",").toUtf8());
		}
		else if (type()==CnvListType::CLINCNV_GERMLINE_MULTI)
		{
			// assemble CNV line
			cnv_annotations.insert(0, "multi");
			cnv_annotations.insert(1, QByteArray::number(variant.size() - 1));
			cnv_annotations.insert(9, variant.genes().toStringList().join(", ").toUtf8());
		}
		else if (type()==CnvListType::CNVHUNTER_GERMLINE_SINGLE)
		{
			// assemble CNV line
			cnv_annotations.insert(0, "");
			cnv_annotations.insert(1, QByteArray::number(variant.size()));
			cnv_annotations.insert(2, QByteArray::number(variant.regions()));
			cnv_annotations.insert(8, variant.genes().toStringList().join(",").toUtf8());
		}
		else if (type()==CnvListType::CLINCNV_TUMOR_NORMAL_PAIR)
		{
			// assemble header line
			cnv_annotations.insert(0, "somatic");
			cnv_annotations.insert(1, QByteArray::number(variant.size()));
			cnv_annotations.insert(9, QByteArray::number(variant.regions()));
			cnv_annotations.insert(10, variant.genes().toStringList().join(",").toUtf8());
		}
		else if(type() == CnvListType::CLINCNV_TUMOR_ONLY)
		{
			// assemble CNV line
			cnv_annotations.insert(5, QByteArray::number(variant.regions()));
			cnv_annotations.insert(6, QByteArray::number(((variant.size() - 1)/1000.0), 'f', 3).rightJustified(8, ' '));
			cnv_annotations.insert(8, variant.genes().toStringList().join(",").toUtf8());
		}
		stream << "\t" << cnv_annotations.join("\t") << "\n";
	}

	stream.flush();
	file.data()->close();
}

QString CnvList::typeAsString() const
{
	if (type()==CnvListType::CNVHUNTER_GERMLINE_SINGLE) return "CNVHUNTER_GERMLINE_SINGLE";
	else if (type()==CnvListType::CNVHUNTER_GERMLINE_MULTI) return "CNVHUNTER_GERMLINE_MULTI";
	else if (type()==CnvListType::CLINCNV_GERMLINE_SINGLE) return "CLINCNV_GERMLINE_SINGLE";
	else if (type()==CnvListType::CLINCNV_GERMLINE_MULTI) return "CLINCNV_GERMLINE_MULTI";
	else if (type()==CnvListType::CLINCNV_TUMOR_NORMAL_PAIR) return "CLINCNV_TUMOR_NORMAL_PAIR";
	else if (type()==CnvListType::CLINCNV_TUMOR_ONLY) return "CLINCNV_TUMOR_ONLY";
	else if (type()==CnvListType::INVALID) return "INVALID";

	THROW(NotImplementedException, "Unknown CnvListType!");
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
	else if (list_type==CnvListType::CLINCNV_GERMLINE_SINGLE || list_type==CnvListType::CLINCNV_GERMLINE_MULTI || list_type==CnvListType::CLINCNV_TUMOR_NORMAL_PAIR || list_type==CnvListType::CLINCNV_TUMOR_ONLY)
	{
		return CnvCallerType::CLINCNV;
	}
	else
	{
		THROW(ProgrammingException, "CNV list type not handled in CnvList::caller()!");
	}
}

QString CnvList::callerAsString() const
{
	CnvCallerType caller_type = caller();
	if (caller_type==CnvCallerType::CLINCNV)
	{
		return "ClinCNV";
	}
	else if (caller_type==CnvCallerType::CNVHUNTER)
	{
		return "CnvHunter";
	}
	else
	{
		THROW(ProgrammingException, "CNV caller type not handled in CnvList::callerAsString()!");
	}
}

QByteArray CnvList::build()
{
	//parse header line, e.g. "##GENOME_BUILD=GRCh38"
	foreach(const QByteArray& line, comments_)
	{
		if (line.startsWith("##GENOME_BUILD="))
		{
			return line.split('=').last().trimmed();
		}
	}

	return "";
}

QByteArray CnvList::qcMetric(QString name, bool throw_if_missing) const
{
	QByteArray value;

	foreach(QByteArray comment, comments_)
	{
		if (comment.contains(":"))
		{
			comment = comment.mid(2); //remove '##'

			int sep_pos = comment.indexOf(':');
			QByteArray key = comment.mid(0, sep_pos);

			//normal match
			if (key==name)
			{
				value = comment.mid(sep_pos+1).trimmed();
			}

			//special handling for CnvHunter/trio output (metrics are prefixed with processed sample name)
			key = key.split(' ').mid(1).join(' ');
			if (key==name)
			{
				value = comment.mid(sep_pos+1).trimmed();
			}
		}
	}

	if (value.isEmpty() && throw_if_missing)
	{
		THROW(ProgrammingException, "Cannot find QC metric '" + name + "' in CNV list header!");
	}

	return value;
}

QByteArray CnvList::headerDescription(QByteArray name) const
{
	return annotation_header_desc_.value(name, "");
}

void CnvList::setHeaderDesciption(QByteArray name, QByteArray desciption)
{
	annotation_header_desc_[name] = desciption;
}

int CnvList::annotationIndexByName(const QByteArray& name, bool throw_on_error, bool contains) const
{
	QList<int> matches;
	for(int i=0; i<annotation_headers_.count(); ++i)
	{
		if (!contains && annotation_headers_[i] == name )
		{
			matches.append(i);
		}
		if(contains && annotation_headers_[i].contains(name))
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

long long CnvList::totalCnvSize() const
{
	long long total_size = 0;
	foreach(const CopyNumberVariant& variant, variants_)
	{
		total_size += variant.size();
	}
	return total_size;
}


KeyValuePair CnvList::split(const QByteArray& string, char sep)
{
	QByteArrayList parts = string.split(sep);

	QString key = parts.takeFirst().trimmed().mid(2); //remove '##'
	QString value = parts.join(sep).trimmed();

	return KeyValuePair(key, value);
}

CnvListCallData CnvList::getCallData(const CnvList& cnvs, QString filename, QString ps_name, bool ignore_inval_header_lines)
{
	//parse file header
	CnvListCallData out;

	out.caller = cnvs.callerAsString();

	foreach(const QByteArray& line, cnvs.comments())
	{
		if (line.contains(":"))
		{
			KeyValuePair pair = split(line, ':');

			if (pair.key.endsWith(" version"))
			{
				out.caller_version = pair.value;
			}
			else if (pair.key.endsWith(" finished on"))
			{
				out.call_date = QDateTime::fromString(pair.value, "yyyy-MM-dd hh:mm:ss");
			}
			else //quality metrics
			{
				if (pair.key.startsWith(ps_name + " ")) //remove sample name prefix (CnvHunter only)
				{
					pair.key = pair.key.mid(ps_name.length()+1).trimmed();
				}

				out.quality_metrics.insert(pair.key, pair.value);
			}
		}
		else if(!ignore_inval_header_lines)
		{
			THROW(FileParseException, "Invalid header line '" + line + "' in file '" + filename + "'!");
		}
	}

	if (out.call_date.isNull()) //fallback if CNV file does not contain any dates
	{
		if(filename != "") out.call_date = QFileInfo(filename).created();
		else THROW(ArgumentException, "Cannot determine date of CnvHunter file, not given in header and there is no filename given.");
	}

	return out;

}

int CnvList::determineReferenceCopyNumber(const CopyNumberVariant& cnv, const QString& gender, GenomeBuild build)
{
	if (cnv.chr().isAutosome()) return 2;
	if (cnv.chr().isY()) return 1;
	if (cnv.chr().isX())
	{
		if(gender == "female") return 2;
		//check for overlap in pseudoautosomal region (has to be checked only for X, since Y-part is masked in mapping)
		if(gender == "male")
		{
			//calculate intersection with PAR
			BedFile intersection = NGSHelper::pseudoAutosomalRegion(build);
			intersection.intersect(BedFile(cnv.chr(), cnv.start(), cnv.end()));
			if ((intersection.baseCount()/cnv.size()) > 0.5)
			{
				return 2;
			}
			else
			{
				return 1;
			}
		}
	}
	//return -1 if gender is not set or chr is undefined
	return -1;
}
