#include "VariantList.h"
#include "VariantAnnotationDescription.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Log.h"
#include "ChromosomalIndex.h"
#include "NGSHelper.h"
#include "VcfFile.h"

#include <QFile>
#include <QTextStream>
#include <QRegExp>
#include <QBitArray>

#include <zlib.h>

Variant::Variant()
	: chr_()
	, start_(-1)
	, end_(-1)
	, ref_()
	, obs_()
    , filters_()
	, annotations_()
{
}

Variant::Variant(const Chromosome& chr, int start, int end, const Sequence& ref, const Sequence& obs, const QList<QByteArray>& annotations, int filter_index)
	: chr_(chr)
	, start_(start)
	, end_(end)
	, ref_(ref.trimmed())
	, obs_(obs.trimmed())
    , filters_()
	, annotations_(annotations)
{
    if (filter_index>0)
    {
		QList<QByteArray> tags = annotations[filter_index].split(';');
		foreach(QByteArray tag, tags)
		{
			tag = tag.trimmed();

			if (tag!="" && tag!="." && tag.toUpper()!="PASS" && tag.toUpper()!="PASSED")
			{
				filters_.append(tag);
			}
		}
	}
}

QByteArrayList Variant::vepAnnotations(int csq_index, int field_index) const
{
	QByteArrayList output;

	QByteArrayList transcripts = annotations()[csq_index].split(',');
	foreach(const QByteArray& transcript, transcripts)
	{
		QByteArrayList csq_fields = transcript.split('|');
		output << csq_fields[field_index];
	}

	return output;
}

void Variant::addFilter(QByteArray tag, int filter_column_index)
{
	tag = tag.trimmed();

	//update column
	QByteArray value = annotations_[filter_column_index].trimmed().toUpper();
	if (value.isEmpty() || value=="." || value=="PASS" || value=="PASSED")
	{
		annotations_[filter_column_index] = tag;
	}
	else
	{
		annotations_[filter_column_index].append(";" + tag);
	}

	//update filters
	filters_.append(tag);
}

bool Variant::operator<(const Variant& rhs) const
{
	if (chr_<rhs.chr_) return true; //compare chromosome
	else if (chr_>rhs.chr_) return false;
	else if (start_<rhs.start_) return true; //compare start position
	else if (start_>rhs.start_) return false;
	else if (end_<rhs.end_) return true; //compare end position
	else if (end_>rhs.end_) return false;
	else if (ref_<rhs.ref_) return true; //compare ref sequence
	else if (ref_>rhs.ref_) return false;
	else if (obs_<rhs.obs_) return true; //compare obs sequence
	else if (obs_>rhs.obs_) return false;
	return false;
}

void Variant::normalize(const Sequence& empty_seq)
{
	Variant::normalize(start_, ref_, obs_);
	end_ = start_ + ref_.length() - 1;
	if (ref_.isEmpty())
	{
		ref_ = empty_seq;
		end_ += 1;
	}
	if (obs_.isEmpty())
	{
		obs_ = empty_seq;
	}
}

QString Variant::toHGVS(const FastaFileIndex& genome_index) const
{
	//check that variant is normalized
	int start = start_;
	Sequence ref_orig = ref_;
	Sequence ref = ref_orig.replace("-", "");
	Sequence obs_orig = obs_;
	Sequence obs = obs_orig.replace("-", "");
	obs.replace("-", "");
	normalize(start, ref, obs);
	if (start!=start_ || ref!=ref_orig || obs!=obs_orig)
	{
		THROW(ProgrammingException, "Cannot convert un-normalized variant " + toString() + " to HGVS!");
	}

	//init
	QByteArray prefix = (chr().isM() ? "m." : "g.");
	int obs_len = obs.length();
	int ref_len = ref.length();

	//SNV
	if (isSNV())
	{
		return prefix + QByteArray::number(start) + ref + '>' + obs;
	}

	//DEL
	if (ref_len>0 && obs_len==0) //del
	{
		if (ref_len==1)
		{
			return prefix + QString::number(start) + "del";
		}
		else
		{
			return prefix + QString::number(start) + '_' + QString::number(start + ref_len - 1) + "del";
		}
	}

	//INS or DUP
	if (obs_len>0 && ref_len==0)
	{
		//DUP (base before)
		if (obs==genome_index.seq(chr(), start+1-obs_len, obs_len))
		{
			if (obs_len==1)
			{
				return prefix + QString::number(start) + "dup";
			}
			else
			{
				return prefix + QString::number(start+1-obs_len) + '_' + QString::number(start) + "dup";
			}
		}
		//DUP (base after)
		else if (obs==genome_index.seq(chr(), start+1, obs_len))
		{
			if (obs_len==1)
			{
				return prefix + QString::number(start+1) + "dup";
			}
			else
			{
				return prefix + QString::number(start+1) + '_' + QString::number(start + obs_len) + "dup";
			}
		}
		//INS
		else
		{
			return prefix + QString::number(start) + '_' + QString::number(start + ref_len + 1) + "ins" + obs;
		}
	}

	//INV
	if (obs==NGSHelper::changeSeq(ref, true, true))
	{
		return prefix + QString::number(start) + '_' + QString::number(start + ref_len - 1) + "inv";
	}

	//INDEL
	if (ref_len==1)
	{
		return prefix + QString::number(start) + "delins" + obs;
	}
	else
	{
		return prefix + QString::number(start) + '_' + QString::number(start + ref_len - 1) + "delins" + obs;
	}

	THROW(ProgrammingException, "Could not convert variant " + toString(false) + " to string! This should not happen!");
}

VariantList::LessComparatorByFile::LessComparatorByFile(QString filename)
	: filename_(filename)
{
	//build chromosome (as QString) to rank (as int) dictionary from file (rank=position in file)
	QStringList lines=Helper::loadTextFile(filename_);
	int rank=0;
	foreach(QString line,lines)
	{
		rank++;
		Chromosome chr(line.split('\t')[0]);
		chrom_rank_[chr.num()]=rank;
	}
}


bool VariantList::LessComparatorByFile::operator()(const Variant& a, const Variant& b) const
{
	int a_chr_num = a.chr().num();
	int b_chr_num = b.chr().num();
	if (!chrom_rank_.contains(a_chr_num))
	{
		THROW(FileParseException, "Reference file for sorting does not contain chromosome '" + a.chr().str() + "'!");
	}
	if (!chrom_rank_.contains(b_chr_num))
	{
		THROW(FileParseException, "Reference file for sorting does not contain chromosome '" + b.chr().str() + "'!");
	}

	if (chrom_rank_[a_chr_num]<chrom_rank_[b_chr_num]) return true; //compare rank of chromosome
	else if (chrom_rank_[a_chr_num]>chrom_rank_[b_chr_num]) return false;
	else if (a.start()<b.start()) return true; //compare start position
	else if (a.start()>b.start()) return false;
	else if (a.end()<b.end()) return true; //compare end position
	else if (a.end()>b.end()) return false;
	else if (a.ref()<b.ref()) return true; //compare ref sequence
	else if (a.ref()>b.ref()) return false;
	else if (a.obs()<b.obs()) return true; //compare obs sequence
	else if (a.obs()>b.obs()) return false;
	return false;
}

VariantList::LessComparator::LessComparator(int quality_index)
	: quality_index_(quality_index)
{
}


bool VariantList::LessComparator::operator()(const Variant& a, const Variant& b) const
{
	if (a.chr()<b.chr()) return true;//compare chromsomes
	else if (a.chr()>b.chr()) return false;
	else if (a.start()<b.start()) return true;//compare start positions
	else if (a.start()>b.start()) return false;
	else if (a.end()<b.end()) return true;//compare end positions
	else if (a.end()>b.end()) return false;
	else if (a.ref()<b.ref()) return true;//compare reference seqs
	else if (a.ref()>b.ref()) return false;
	else if (a.obs()<b.obs()) return true;//compare alternative seqs
	else if (a.obs()>b.obs()) return false;
	else if (quality_index_ != -1)
	{
		QString q_a=a.annotations()[quality_index_];
		QString q_b=b.annotations()[quality_index_];
		if(q_a!="." && q_b!="." && q_a.toDouble()<q_b.toDouble()) return true;
	}
	return false;
}


VariantList::VariantList()
	: comments_()
	, annotation_descriptions_()
	, annotation_headers_()
	, filters_()
	, variants_()
{
}

void VariantList::copyMetaData(const VariantList& rhs)
{
	comments_ = rhs.comments_;
	annotation_descriptions_ = rhs.annotation_descriptions_;
	annotation_headers_ = rhs.annotation_headers_;
	filters_ = rhs.filters_;
}

VariantAnnotationDescription VariantList::annotationDescriptionByName(const QString& description_name, bool sample_specific, bool error_not_found) const
{
	bool found_multiple = false;

	int index = -1;
	for(int i=0; i<annotationDescriptions().count(); ++i)
	{
		if(annotationDescriptions()[i].name()==description_name && annotationDescriptions()[i].sampleSpecific()==sample_specific)
		{
			if(index!=-1)	found_multiple = true;
			index = i;
		}
	}

	if(error_not_found && index==-1)	THROW(ProgrammingException, "Could not find " + (sample_specific ? QString("sample-specific") : QString("")) + " column description '" + description_name + "'.");
	if(error_not_found && found_multiple)	THROW(ProgrammingException, (sample_specific ? QString("Sample-specific d") : QString("D")) + "escription for '" + description_name + "' occurs more than once.");

	if(!error_not_found && (found_multiple || index==-1))
	{
		return VariantAnnotationDescription();
	}
	return annotationDescriptions()[index];
}

int VariantList::annotationIndexByName(const QString& name, bool exact_match, bool error_on_mismatch) const
{
	return annotationIndexByName(name, "", exact_match, error_on_mismatch);
}


int VariantList::annotationIndexByName(const QString& name, QString sample_id, bool exact_match, bool error_on_mismatch) const
{
	//check sample name
	if (!sample_id.isEmpty() && !sampleExists(sample_id))
	{
		THROW(ArgumentException, "Could not find column '" + name + "' for invalid sample " + sample_id + ". Valid sample names are: " + sampleNames().join(", "));
	}

	//find matches
	QList<int> matches;
	for(int i=0; i<annotations().count(); ++i)
	{
		if ((exact_match && annotations()[i].name().compare(name, Qt::CaseInsensitive)==0) || (!exact_match && annotations()[i].name().contains(name, Qt::CaseInsensitive)))
		{
			if(!sample_id.isEmpty() && sample_id!=annotations()[i].sampleID())	continue;
			matches.append(i);
		}
	}

	//error checks
	if (matches.count()<1)
	{
		if (error_on_mismatch)
		{
			THROW(ArgumentException, "Could not find column '" + name + "' " + (!sample_id.isEmpty() ? QString("for sample '" + sample_id + "' ") : QString("")) + "in variant list!");
		}
		else
		{
			return -1;
		}
	}

	if (matches.count()>1)
	{
		if (error_on_mismatch)
		{
			THROW(ArgumentException, "Found multiple columns for '" + name + "' " + (!sample_id.isEmpty() ? QString("for sample '" + sample_id + "' ") : QString("")) + " in variant list!");
		}
		else
		{
			Log::warn("Found multiple columns for '" + name + "' " + (!sample_id.isEmpty() ? QString("for sample '" + sample_id + "' ") : QString("")) + " in variant list!");
			return -2;
		}
	}

	//return result
	return matches.at(0);
}

int VariantList::vepIndexByName(const QString& name, bool error_if_not_found) const
{
	VariantAnnotationDescription anno_desc = annotationDescriptionByName("CSQ", false, false);
	if (anno_desc.name().isEmpty())
	{
		if (error_if_not_found)
		{
			THROW(ArgumentException, "Info field 'CSQ' containing VEP annotation not found!");
		}
		else
		{
			return -1;
		}
	}

	QStringList parts = anno_desc.description().trimmed().split("|");
	parts[0] = "Allele";
	int i_field = parts.indexOf(name);
	if (i_field==-1 && error_if_not_found)
	{
		THROW(ArgumentException, "Field '" + name + "' not found in VEP CSQ field!");
	}

	return i_field;
}

int VariantList::addAnnotation(QString name, QString description, QByteArray default_value)
{
	annotations().append(VariantAnnotationHeader(name));
	for (int i=0; i<variants_.count(); ++i)
	{
		variants_[i].annotations().push_back(default_value);
	}

	annotationDescriptions().append(VariantAnnotationDescription(name, description));

	return annotations().count() - 1;
}

int VariantList::addAnnotationIfMissing(QString name, QString description, QByteArray default_value)
{
	int index = annotationIndexByName(name, true, false);

	if (index==-1)
	{
		index = addAnnotation(name, description, default_value);
	}

	return index;
}

void VariantList::removeAnnotation(int index)
{
	if (index < 0 || index>=annotation_headers_.count())
	{
		THROW(ProgrammingException, "Variant annotation column index " + QString::number(index) + " out of range [0," + QString::number(annotation_headers_.count()-1) + "] in removeAnnotation(index) method!");
	}

	annotation_headers_.removeAt(index);
	for (int i=0; i<variants_.count(); ++i)
	{
		variants_[i].annotations().removeAt(index);
	}
}

void VariantList::removeAnnotationByName(QString name, bool exact_match, bool error_on_mismatch)
{
	int index = annotationIndexByName(name, exact_match, error_on_mismatch);
	if (index!=-1)
	{
		removeAnnotation(index);
	}

	for(int i=0; i<annotationDescriptions().count(); ++i)
	{
		if ((exact_match && annotationDescriptions()[i].name().compare(name, Qt::CaseInsensitive)==0) || (!exact_match && annotationDescriptions()[i].name().contains(name, Qt::CaseInsensitive)))
		{
			annotation_descriptions_.removeAt(i);
		}
	}
}

QStringList VariantList::sampleNames() const
{
	QStringList output;
	foreach(const VariantAnnotationHeader& act_anno, annotations())
	{
		const QString& sample_id = act_anno.sampleID();
		if (sample_id.isEmpty()) continue;
		if (!output.contains(sample_id))
		{
			output.append(sample_id);
		}
	}

	return output;
}

bool VariantList::sampleExists(const QString& sample) const
{
	foreach(const VariantAnnotationHeader& act_anno, annotations())
	{
		if (act_anno.sampleID()==sample) return true;
	}

	return false;
}

VariantListFormat VariantList::load(QString filename, VariantListFormat format, const BedFile* roi, bool invert)
{
	//determine format
	if (format==AUTO)
	{
		QString fn_lower = filename.toLower();
		if(fn_lower.indexOf(':')>1 && fn_lower.count(':')==1)
		{
			fn_lower = fn_lower.left(fn_lower.indexOf(':'));
		}

		if (fn_lower.endsWith(".vcf"))
		{
			format = VCF;
		}
		else if (fn_lower.endsWith(".vcf.gz"))
		{
			format = VCF_GZ;
		}
		else if (fn_lower.endsWith(".tsv") || fn_lower.contains(".gsvar"))
		{
			format = TSV;
		}
		else
		{
			THROW(ArgumentException, "Could not determine format of file '" + fn_lower + "' from file extension. Valid extensions are 'vcf', 'vcf:SampleID', tsv' and 'GSvar'.")
		}
	}

	//create ROI index (if given)
	QScopedPointer<ChromosomalIndex<BedFile>> roi_idx;
	if (roi!=nullptr)
	{
		if (!roi->isSorted())
		{
			THROW(ArgumentException, "Target region unsorted, but needs to be sorted (given for reading file " + filename + ")!");
		}
		roi_idx.reset(new ChromosomalIndex<BedFile>(*roi));
	}

	//load variant list
	if (format==VCF)
	{
		loadFromVCF(filename, roi_idx.data(), invert);
		return VCF;
	}
	else if (format==VCF_GZ)
	{
		loadFromVCFGZ(filename, roi_idx.data(), invert);
		return VCF_GZ;
	}
	else
	{
		loadFromTSV(filename, roi_idx.data(), invert);
		return TSV;
	}
}

void VariantList::store(QString filename, VariantListFormat format)
{
	//determine format
	if (format==AUTO)
	{
		QString fn_lower = filename.toLower();
		if (fn_lower.endsWith(".vcf"))
		{
			format = VCF;
		}
		else if (fn_lower.endsWith(".tsv") || fn_lower.contains(".gsvar"))
		{
			format = TSV;
		}
		else
		{
			THROW(ArgumentException, "Could not determine format of file '" + filename + "' from file extension. Valid extensions are 'vcf', 'tsv' and 'GSvar'.")
		}
	}

	if (format==VCF)
	{
		storeToVCF(filename);
	}
	else
	{
		storeToTSV(filename);
	}
}

void VariantList::loadFromTSV(QString filename, ChromosomalIndex<BedFile>* roi_idx, bool invert)
{
	constexpr int special_cols = 5;

	//remove old data
	clear();

	//parse from stream
	QSharedPointer<QFile> file = Helper::openFileForReading(filename, true);
    int filter_index = -1;
	while(!file->atEnd())
	{
		QByteArray line = file->readLine();
		while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

		//skip empty lines
		if(line.length()==0) continue;

		if (line.startsWith("##"))//comment/description line
		{
			QList <QByteArray> parts = line.split('=');
			if (line.startsWith("##DESCRIPTION=") && parts.count()>2)
			{
				annotationDescriptions().append(VariantAnnotationDescription(parts[1], parts.mid(2).join('='), VariantAnnotationDescription::STRING, false, "."));
			}
			else if (line.startsWith("##FILTER=") && parts.count()>2)
			{
				filters_[parts[1]] = parts.mid(2).join('=');
			}
			else
			{
				comments_.append(line); //comment line
			}
			continue;
		}
		if (line.startsWith("#"))//header
		{
			QList <QByteArray> fields = line.split('\t');
            for (int i=special_cols; i<fields.count(); ++i)
			{
                if (fields[i]=="filter")
                {
                    filter_index = i - special_cols;
                }

				annotations().append(VariantAnnotationHeader(fields[i]));
			}
			continue;
		}

        //error when special colums are not present
        QList<QByteArray> fields = line.split('\t');
        if (fields.count()<special_cols)
		{
			THROW(FileParseException, "Variant TSV file line with less than five fields found: '" + line.trimmed() + "'");
		}

		//Skip variants that are not in the target region (if given)
		Chromosome chr = fields[0];
		int start = atoi(fields[1]);
		int end = atoi(fields[2]);
		if (roi_idx!=nullptr)
		{

			bool in_roi = roi_idx->matchingIndex(chr, start, end)!=-1;
			if ((!in_roi && !invert) || (in_roi && invert))
			{
				continue;
			}
		}

		append(Variant(chr, start, end, fields[3], fields[4], fields.mid(special_cols), filter_index));
	}
}

void VariantList::storeToTSV(QString filename)
{
	//open stream
	QSharedPointer<QFile> file = Helper::openFileForWriting(filename);
	QTextStream stream(file.data());

	//comments
	if (comments_.count()>0)
	{
		QStringListIterator i(comments_);
		while (i.hasNext())
		{
			QString comment=i.next();
			if (comment.startsWith("##fileformat=")) continue;//don't write VCF specific meta info
			stream <<comment<<endl;
		}
	}

	//column descriptions
	if (annotation_headers_.count()>0)
	{
		foreach(const VariantAnnotationDescription& act_anno, annotation_descriptions_)
		{
			//don't write empty description information
			if(act_anno.description()=="") continue;
			if(act_anno.name()==".")	continue;

			if (act_anno.sampleSpecific())
			{
				stream <<"##DESCRIPTION=" << act_anno.name() << "_ss=" << act_anno.description();
			}
			else
			{
				stream <<"##DESCRIPTION=" << act_anno.name() << "=" << act_anno.description();
			}
			stream << endl;
		}
	}

	//filter headers
	auto it = filters().cbegin();
	while(it != filters().cend())
	{

		stream << "##FILTER=" << it.key() << "=" << it.value() << endl;
		++it;
	}

	//header
	stream << "#chr\tstart\tend\tref\tobs";
	if (annotation_headers_.count()>0)
	{
		foreach(const VariantAnnotationHeader& act_anno, annotation_headers_)
		{
			if (!act_anno.sampleID().isEmpty())
			{
				if(act_anno.name()==".")
				{
					continue;
				}
				stream << "\t" << act_anno.name() << "_ss";
			}
			else
			{
				stream << "\t" << act_anno.name();
			}
		}
	}

	stream << endl;

	//variants
	foreach(const Variant& v, variants_)
	{
		stream << v.chr().str() << "\t" << v.start() << "\t" << v.end() << "\t" << v.ref() << "\t" << v.obs();
		for(int i=0; i<v.annotations().count(); ++i)
		{
			const QByteArray& entry = v.annotations()[i];
			if(annotation_headers_[i].name()==".")	continue;
			stream << "\t" << entry;
		}
		stream << endl;
	}
}

void VariantList::loadFromVCF(QString filename, ChromosomalIndex<BedFile>* roi_idx, bool invert)
{
	//remove old data
	clear();

	//model the mandatory VCF fields "ID","QUAL" and "FILTER" as sample independent annotations
	annotationDescriptions().append(VariantAnnotationDescription("ID", "ID of the variant, often dbSNP rsnumber"));
	annotationDescriptions().append(VariantAnnotationDescription("QUAL", "Phred-scaled quality score", VariantAnnotationDescription::FLOAT));
	annotationDescriptions().append(VariantAnnotationDescription("FILTER", "Filter status"));

	//parse from stream
	int line_number = 0;
	QList<QByteArray> header_fields;
	QSharedPointer<QFile> file = Helper::openFileForReading(filename, true);
	while(!file->atEnd())
	{
		processVcfLine(header_fields, line_number, file->readLine(), roi_idx, invert);
	}
}

void VariantList::loadFromVCFGZ(QString filename, ChromosomalIndex<BedFile>* roi_idx, bool invert)
{
	//remove old data
	clear();

	//model the mandatory VCF fields "ID","QUAL" and "FILTER" as sample independent annotations
	annotationDescriptions().append(VariantAnnotationDescription("ID", "ID of the variant, often dbSNP rsnumber"));
	annotationDescriptions().append(VariantAnnotationDescription("QUAL", "Phred-scaled quality score", VariantAnnotationDescription::FLOAT));
	annotationDescriptions().append(VariantAnnotationDescription("FILTER", "Filter status"));

	//parse from stream
	int line_number = 0;
	QList<QByteArray> header_fields;

	gzFile file = gzopen(filename.toLatin1().data(), "rb"); //read binary: always open in binary mode because windows and mac open in text mode
	if (file==NULL)
	{
		THROW(FileAccessException, "Could not open file '" + filename + "' for reading!");
	}

	char* buffer = new char[131072];
	while(!gzeof(file))
	{

		char* read_line = gzgets(file, buffer, 131072);

		//handle errors like truncated GZ file
		if (read_line==nullptr)
		{
			int error_no = Z_OK;
			QByteArray error_message = gzerror(file, &error_no);
			if (error_no!=Z_OK && error_no!=Z_STREAM_END)
			{
				THROW(FileParseException, "Error while reading file '" + filename + "': " + error_message);
			}
		}

		processVcfLine(header_fields, line_number, QByteArray(read_line), roi_idx, invert);
	}
	gzclose(file);
	delete[] buffer;
}

void VariantList::processVcfLine(QList<QByteArray>& header_fields, int& line_number, QByteArray line, ChromosomalIndex<BedFile>* roi_idx, bool invert)
{
	while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

	//skip empty lines
	if(line.length()==0) return;

	//annotation description line
	if (line.startsWith("##INFO") || line.startsWith("##FORMAT"))
	{
		bool sample_dependent_data;
		QString info_or_format;
		if (line.startsWith("##INFO"))
		{
			info_or_format="INFO";
			sample_dependent_data = false;
			line=line.mid(8);//remove "##INFO=<"
		}
		else
		{
			info_or_format="FORMAT";
			sample_dependent_data = true;
			line=line.mid(10);//remove "##FORMAT=<"

		}

		//parse sample-independent annotation
		QList <QByteArray> comma_splitted_line=line.split(',');

		if (comma_splitted_line.count()<4)
		{
			THROW(FileParseException, "Malformed "+info_or_format +" line: has less than 4 entries " + line.trimmed() + "'");
		}

		//parse ID field
		QByteArray ID_entry=comma_splitted_line[0];
		QList <QByteArray> splitted_ID_entry=ID_entry.split('=');
		if (!(splitted_ID_entry[0].startsWith("ID")))
		{
			THROW(FileParseException, "Malformed "+info_or_format +" line: does not start with ID-field " + splitted_ID_entry[0] + "'");
		}
		VariantAnnotationDescription new_annotation_description(splitted_ID_entry[1], "", VariantAnnotationDescription::STRING, sample_dependent_data, ".");
		comma_splitted_line.pop_front();//pop ID-field
		//parse number field
		QByteArray number_entry=comma_splitted_line.first();
		QList <QByteArray> splitted_number_entry=number_entry.split('=');
		if (!(splitted_number_entry[0].trimmed().startsWith("Number")))
		{
			THROW(FileParseException, "Malformed "+info_or_format +" line: second field is not a number field " + splitted_number_entry[0] + "'");
		}
		new_annotation_description.setNumber(splitted_number_entry[1]);
		comma_splitted_line.pop_front();//pop number-field
		//parse type field
		QList <QByteArray> splitted_type_entry=comma_splitted_line.first().split('=');
		if (splitted_type_entry[0].trimmed()!="Type")
		{
			THROW(FileParseException, "Malformed "+info_or_format +" line: third field is not a type field " + line.trimmed() + "'");
		}
		QHash <QByteArray, VariantAnnotationDescription::AnnotationType >convertor;
		convertor["Integer"]=VariantAnnotationDescription::INTEGER;
		convertor["Float"]=VariantAnnotationDescription::FLOAT;
		convertor["Character"]=VariantAnnotationDescription::CHARACTER;
		convertor["String"]=VariantAnnotationDescription::STRING;
		if (!(sample_dependent_data))
		{
			convertor["Flag"]=VariantAnnotationDescription::FLAG;
		}
		QByteArray s_type=splitted_type_entry[1];
		if (!(convertor.keys().contains(s_type)))
		{
			THROW(FileParseException, "Malformed "+info_or_format +" line: undefined value for type " + line.trimmed() + "'");
		}
		new_annotation_description.setType(convertor[s_type]);
		comma_splitted_line.pop_front();//pop type-field
		//parse description field
		QByteArray description_entry=comma_splitted_line.front();
		QList <QByteArray> splitted_description_entry=description_entry.split('=');
		if (splitted_description_entry[0].trimmed()!="Description")
		{
			THROW(FileParseException, "Malformed "+info_or_format +" line: fourth field is not a description field " + line.trimmed() + "'");
		}
		//ugly, but because the description may content commas, too...
		comma_splitted_line.pop_front();//pop type-field
		comma_splitted_line.push_front(splitted_description_entry[1]);//re-add description value between '=' and possible ","
		QStringList description_value_parts;//convert to QStringList
		for(int i=0; i<comma_splitted_line.size(); ++i)
		{
			description_value_parts.append(comma_splitted_line[i]);
		}
		QString description_value=description_value_parts.join(",");//join parts
		description_value=description_value.mid(1);//remove '"'
		description_value.chop(2);//remove '">'
		new_annotation_description.setDescription(description_value);

		//check if annotation description is a possible duplicate
		bool found = false;
		foreach(VariantAnnotationDescription vad, annotationDescriptions())
		{
			if(vad.name()==new_annotation_description.name() && vad.sampleSpecific()==new_annotation_description.sampleSpecific())
			{
				Log::warn("Duplicate metadata information for field named '" + new_annotation_description.name() + "'. Skipping metadata line " + QString::number(line_number) + ".");
				found = true;
				break;
			}
		}
		if(found) return;

		annotationDescriptions().append(new_annotation_description);

		return;
	}

	//filter lines
	if (line.startsWith("##FILTER=<ID="))
	{
		QStringList parts = QString(line.mid(13, line.length()-15)).split(",Description=\"");
		if(parts.count()!=2) THROW(FileParseException, "Malformed FILTER line: conains more/less than two parts: " + line);
		filters_[parts[0]] = parts[1];
		return;
	}

	//other meta-information lines
	if (line.startsWith("##"))
	{
		addCommentLine(line);
		return;
	}

	//header line
	if (line.startsWith("#CHROM"))
	{
		header_fields = line.mid(1).split('\t');

		if (header_fields.count()<VcfFile::MIN_COLS)//8 are mandatory
		{
			THROW(FileParseException, "VCF file header line with less than 8 fields found: '" + line.trimmed() + "'");
		}
		if ((header_fields[0]!="CHROM")||(header_fields[1]!="POS")||(header_fields[2]!="ID")||(header_fields[3]!="REF")||(header_fields[4]!="ALT")||(header_fields[5]!="QUAL")||(header_fields[6]!="FILTER")||(header_fields[7]!="INFO"))
		{
			THROW(FileParseException, "VCF file header line with at least one illegal named mandatory column: '" + line.trimmed() + "'");
		}

		// set annotation headers
		annotations().append(VariantAnnotationHeader("ID"));
		annotations().append(VariantAnnotationHeader("QUAL"));
		annotations().append(VariantAnnotationHeader("FILTER"));
		// (1) for all INFO fields (sample independent annotations)
		for(int i=0; i<annotationDescriptions().count(); ++i)
		{
			if(annotationDescriptions()[i].name()=="ID" || annotationDescriptions()[i].name()=="QUAL" || annotationDescriptions()[i].name()=="FILTER")	continue;	//skip annotations that are already there
			if(annotationDescriptions()[i].sampleSpecific())	continue;
			annotations().append(VariantAnnotationHeader(annotationDescriptions()[i].name()));
		}
		// (2) for all samples and their FORMAT fields (sample dependent annotations)
		for(int i=9; i<header_fields.count(); ++i)
		{
			QString sample_id = QString(header_fields[i]);
			int sample_specific_count = 0;

			for(int ii=0; ii<annotationDescriptions().count(); ++ii)
			{
				if(!annotationDescriptions()[ii].sampleSpecific()) continue;
				++sample_specific_count;
				annotations().append(VariantAnnotationHeader(annotationDescriptions()[ii].name(),sample_id));
			}

			if(sample_specific_count==0)
			{
				annotations().append(VariantAnnotationHeader(".",sample_id));
				annotationDescriptions().append(VariantAnnotationDescription(".", "Default column description since no FORMAT fields were defined.", VariantAnnotationDescription::STRING, true, "1", false));//add dummy description
			}
		}

		// (3) FORMAT column available
		if(header_fields.count()<=9)
		{
			QString sample_id = "Sample";
			int sample_specific_count = 0;
			for(int i=0; i<annotationDescriptions().count(); ++i)
			{
				if(!annotationDescriptions()[i].sampleSpecific()) continue;
				annotations().append(VariantAnnotationHeader(annotationDescriptions()[i].name(),sample_id));
				++sample_specific_count;
			}

			if(sample_specific_count==0)
			{
				annotations().append(VariantAnnotationHeader(".", sample_id));
				annotationDescriptions().append(VariantAnnotationDescription(".", "Default column description since no FORMAT fields were defined.", VariantAnnotationDescription::STRING, true, "1", false));//add dummy description
			}
		}
		return;
	}

	//variant line
	QList<QByteArray> line_parts = line.split('\t');
	if (line_parts.count()<VcfFile::MIN_COLS)
	{
		THROW(FileParseException, "VCF data line needs at least 7 tab-separated columns! Found " + QString::number(line_parts.count()) + " column(s) in line number " + QString::number(line_number) + ": " + line);
	}

	//Skip variants that are not in the target region (if given)
	Chromosome chr = line_parts[0];
	int start = atoi(line_parts[1]);
	Sequence ref_bases = line_parts[3].toUpper();
	int end = start + ref_bases.length()-1;
	if (roi_idx!=nullptr)
	{
		bool in_roi = roi_idx->matchingIndex(chr, start, end)!=-1;
		if ((!in_roi && !invert) || (in_roi && invert))
		{
			return;
		}
	}

	//extract sample-independent annotations
	QList <QByteArray> annos;
	annos << line_parts[2] << line_parts[5] << line_parts[6]; //id, quality, filter
	for(int i=3; i<annotations().count();++i)
	{
		annos.append(QByteArray());
	}

	if ((line_parts.count()>=8)&&(line_parts[7]!="."))
	{
		QList<QByteArray> anno_parts = line_parts[7].split(';');
		for (int i=0; i<anno_parts.count(); ++i)
		{
			QList<QByteArray> key_value = anno_parts[i].split('=');

			QByteArray value;
			if (key_value.count()==1) //no value (flag)
			{
				value = "TRUE";
			}
			else
			{
				value = key_value[1];
				value = (value=="." ? "" : value);
			}

			int index = annotations().indexOf(VariantAnnotationHeader(key_value[0]));
			if(index==-1)
			{
				annotations().append(VariantAnnotationHeader(key_value[0]));
				annotationDescriptions().append(VariantAnnotationDescription(key_value[0], "no description available"));
				//Log::info("No metadata information for INFO field " + key_value[0] + " was found.");

				for(int ii=0;ii<variants_.count();++ii)
				{
					variants_[ii].annotations().append(QByteArray());
				}

				index = annos.count();
				annos.append(value);
			}
			annos[index] = value;
		}
	}

	//extract sample-dependent annotations
	if (line_parts.count()>=10)//if present: extract sample dependent annotations
	{
		QList<QByteArray> names = line_parts[8].split(':');
		for(int i=9; i<header_fields.count(); ++i)
		{
			QString sample_id = QString(header_fields[i]);
			if(sample_id.isEmpty() && header_fields.count()==10)	sample_id = "Sample";

			QList<QByteArray> values = line_parts[i].split(':');
			for (int ii=0; ii<names.count(); ++ii)
			{
				QByteArray value = "";
				if (values[ii]!=".") value = values[ii];

				int index = annotations().indexOf(VariantAnnotationHeader(names[ii],sample_id));
				if(index==-1 && names[ii]==".")
				{
					THROW(FileParseException, "Invalid empty FORMAT field of sample '"+sample_id+"'!");
				}
				if(index==-1)
				{
					//Log::info("No metadata information for FORMAT field " + names[ii] + ".");
					annotations().append(VariantAnnotationHeader(names[ii],sample_id));
					annotationDescriptions().append(VariantAnnotationDescription(names[ii],"no description available",VariantAnnotationDescription::STRING,true));
					for(int iii=0;iii<variants_.count();++iii)
					{
						variants_[iii].annotations().append(QByteArray());
					}

					index = annos.count();
					annos.append(value);
				}
				annos[index] = value;
			}
		}
	}

	append(Variant(chr, start, end, ref_bases, line_parts[4].toUpper(), annos, 2));
}

void VariantList::storeToVCF(QString filename)
{
	//open stream
	QSharedPointer<QFile> file = Helper::openFileForWriting(filename);
	QTextStream stream(file.data());

	//write ##fileformat and other metainformation
	foreach(const QString& comment, comments())
	{
		stream << comment << "\n";
	}

	//write annotations information (##INFO and ##FORMAT lines)
	for (int j=3; j<annotationDescriptions().count(); ++j) //why 3: skip ID Quality Filter
	{
		const VariantAnnotationDescription& anno_description = annotationDescriptions()[j];
		if(!anno_description.print())
		{
			continue;
		}

		stream << "##" << (anno_description.sampleSpecific() ? "FORMAT" : "INFO") << "=";
		stream << "<ID=" << anno_description.name();
		stream << ",Number=" << anno_description.number();
		stream << ",Type=" << annotationTypeToString(anno_description.type());
		QString desc = anno_description.description();
		stream << ",Description=\"" << (desc!="" ? desc : "no description available") << "\"";
		stream << ">\n";
	}

	//write filter headers
	auto it = filters().cbegin();
	while(it != filters().cend())
	{
		stream << "##FILTER=<ID=" << it.key() << ",Description=\"" << it.value() << "\">\n";
		++it;
	}

	//write header line
	stream << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT";
	QStringList samples = sampleNames();
	foreach(const QString& sample, samples)
	{
		stream << "\t" << sample;
	}
	stream << "\n";

	//write variants
	foreach(const Variant& v, variants_)
	{
		QString ID = v.annotations()[0];//will only work correctly if source was a  VCF file
		QString quality = v.annotations()[1];//will only work correctly if source was a VCF file
		QString filter = v.annotations()[2];//will only work correctly if source was a VCF file
		QStringList info_entries;
		QStringList format_entries;
		QHash <QString, QStringList> sample_entries_by_sample;
		QString sample;

		for (int i=3; i<v.annotations().count(); ++i) //why 3: skip ID Quality Filter
		{
			const VariantAnnotationHeader& anno_header = annotations()[i];
			const VariantAnnotationDescription& anno_desc = annotationDescriptionByName(anno_header.name(), !anno_header.sampleID().isEmpty());
			QByteArray anno_val = v.annotations()[i];

			if (anno_desc.sampleSpecific())
			{
				if (anno_val!="" || samples.count()>1)
				{
					if (sample.isEmpty()) sample = anno_header.sampleID();
					if (sample==anno_header.sampleID())
					{
						format_entries << anno_desc.name();
					}

					if (anno_val=="") anno_val = ".";
					sample_entries_by_sample[anno_header.sampleID()].append(anno_val);
				}
			}
			else
			{
				if (anno_val!="")
				{
					if (anno_desc.type()==VariantAnnotationDescription::FLAG) //Flags should not have values in VCF
					{
						info_entries << anno_desc.name();
					}
					else
					{
						info_entries << anno_desc.name() + "=" + anno_val;
					}
				}
			}
		}

		stream << v.chr().str() << "\t" << v.start() << "\t" << ID << "\t" << v.ref() << "\t"  << v.obs() << "\t" << quality << "\t" << filter;
		stream << "\t" << (info_entries.isEmpty() ? "." : info_entries.join(";"));
		stream << "\t" << (format_entries.isEmpty() ? "." : format_entries.join(":"));
		foreach(const QString& sample, samples)
		{
			const QStringList& sample_entries = sample_entries_by_sample[sample];
			stream << "\t" << (sample_entries.isEmpty() ? "." : sample_entries.join(":"));
		}
		stream << endl;
	}
}

QString VariantList::annotationTypeToString(VariantAnnotationDescription::AnnotationType type)
{
	static QHash<VariantAnnotationDescription::AnnotationType,QString> hash;

	//initialize hash
	if (hash.isEmpty())
	{
		hash[VariantAnnotationDescription::INTEGER] = "Integer";
		hash[VariantAnnotationDescription::FLOAT] = "Float";
		hash[VariantAnnotationDescription::FLAG] = "Flag";
		hash[VariantAnnotationDescription::CHARACTER] = "Character";
		hash[VariantAnnotationDescription::STRING] = "String";
	}

	//get output
	QString output = hash.value(type, "");
	if(output=="")
	{
		THROW(ProgrammingException, "Unknown AnnotationType '" + QString::number(type) + "'!");
	}

	return output;
}

void VariantList::sort(bool use_quality)
{
	//skip this if no variants are there - otherwise finding the quality column might fail...
	if (variants_.count()==0) return;

	//check if there is a quality column (from VCF/TSV)
	int quality_index = -1;
	if (use_quality)
	{
		quality_index = annotationIndexByName("QUAL", true, false);
		if (quality_index == -1)
		{
			THROW(ArgumentException, "Variant list does not contain 'QUAL' column (sorting by quality is supported for VCF only!");
        }
	}

	sortCustom(LessComparator(quality_index));
}

void VariantList::sortByFile(QString filename)
{
	sortCustom(LessComparatorByFile(filename));
}

void VariantList::removeDuplicates(bool sort_by_quality)
{
	sort(sort_by_quality);

	//remove duplicates (same chr, start, obs, ref) - avoid linear time remove() calls by copying the data to a new vector.
	QVector<Variant> output;
	output.reserve(variants_.count());
	for (int i=0; i<variants_.count()-1; ++i)
	{
		int j = i+1;
		if (variants_[i].chr()!=variants_[j].chr() || variants_[i].start()!=variants_[j].start() || variants_[i].obs()!=variants_[j].obs() || variants_[i].ref()!=variants_[j].ref())
		{
			output.append(variants_[i]);
		}
	}
	if (!variants_.isEmpty())
	{
		output.append(variants_.last());
	}

	//swap the old and new vector
	variants_.swap(output);
}

void VariantList::clear()
{
	clearVariants();
	comments_.clear();
	clearAnnotations();
	filters_.clear();
}

void VariantList::clearAnnotations()
{
	annotation_headers_.clear();
	annotation_descriptions_.clear();
	for(int i=0; i<variants_.count(); ++i)
	{
		variants_[i].annotations().clear();
	}
}

void VariantList::clearVariants()
{
	variants_.clear();
}

void VariantList::leftAlign(QString ref_file, bool sort_by_quality)
{
	//open refererence genome file
	FastaFileIndex reference(ref_file);

	//init
	for (QVector<Variant>::iterator variant = variants_.begin(); variant != variants_.end(); ++variant)
	{
		Chromosome chr = variant->chr();
		int pos = variant->start();
		Sequence ref = variant->ref().toUpper();
		Sequence alt = variant->obs().toUpper();

		//skip SNVs
		if (variant->isSNV()) continue;

		//skip SNVs disguised as indels (ACGT => AXGT)
		if (!alt.contains(','))
		{
			Variant v2 = *variant;
			v2.normalize("");
			if (v2.isSNV())
			{
				variant->setStart(v2.start());
				variant->setEnd(v2.end());
				variant->setRef(v2.ref());
				variant->setObs(v2.obs());
				continue;
			}
		}

		//normalize (remove surrounding bases)
		QList<Sequence> alt_ma = alt.split(',');//list of alternative alleles
		QVector<int> pos_ma(alt_ma.count(), pos);//vector with size equal to number of alleles and values of pos
		QVector<Sequence> ref_ma(alt_ma.count(), ref);//vector with size equal to number of alleles and values of ref
		bool complex = false;
		bool tsv_style= false;//(e.g. deletion in VCF: "AG"->"A", deletion in tsv: "G"->"-")
		for (int i=0; i<alt_ma.count(); ++i)
		{
			if(ref_ma[i]=="-")//if it is a simple insertion from a tsv-file
			{
				ref_ma[i]="";
				tsv_style=true;
			}
			if(alt_ma[i]=="-")//if it is a simple deletion from a tsv-file
			{
				alt_ma[i]="";
				tsv_style=true;
			}
			//remove common bases at beginning or end of ref and alt and shift pos if beginning was removed
			Variant::normalize(pos_ma[i], ref_ma[i], alt_ma[i]);
			//skip complex indels
			if (ref_ma[i].length()!=0 && alt_ma[i].length()!=0)
			{
				complex = true;
				break;
			}
		}

		//skip complex variants
		if (complex) continue;

		//left-align
		bool same_ref_ma = true;
		for (int i=0; i<alt_ma.count(); ++i)
		{
			//INSERTION
			//if the reference bases were completely included in the alt bases
			if (ref_ma[i].length()==0)
			{
				//block shift insertion
				Sequence block = Variant::minBlock(alt_ma[i]);//the normalized alt bases[i](or the smallest non-alternating part of it)
				pos_ma[i] -= block.length();//position is moved to left by size of sequence
				//as long as the ref matches the normalized alt, position is moved to left by size of sequence
				while(reference.seq(chr, pos_ma[i], block.length())==block)
				{
					pos_ma[i] -= block.length();
				}
				//after the ref didn't matches the normalized alt, position is moved back to the right by size of sequence
				pos_ma[i] += block.length();

				//prepend prefix base
				if(!tsv_style)
				{
					pos_ma[i] -= 1;//position is moved to left by one
				}
				ref_ma[i] = reference.seq(chr, pos_ma[i], 1);//ref_ma= ref base at position
				alt_ma[i] = ref_ma[i] + alt_ma[i];//ref_ma= ref base is added to alt bases
				//single-base shift insertion
				//as long as the ref base at position matches the right-end base of the alt seq
				//shift insertion to the lef (because e.g. multiples of AGA could be multiples of AAG)
				while(ref_ma[i]==alt_ma[i].right(1))
				{
					pos_ma[i] -= 1;//
					ref_ma[i] = reference.seq(chr, pos_ma[i], 1);
					alt_ma[i] = ref_ma[i] + alt_ma[i].left(alt_ma[i].length()-1);
				}
				if (tsv_style)
				{
					ref_ma[i]="-";//VCF "G" -> TSV "-"
					alt_ma[i]=alt_ma[i].right(alt_ma[i].length()-1);//VCF "GA" -> TSV "A"
				}
			}
			//DELETION
			//if the alternative bases were completely included in reference bases
			//everything as a above, except ref and alt exchanged
			else if (alt_ma[i].length()==0)
			{
				//block shift deletion
				Sequence block = Variant::minBlock(ref_ma[i]);
				while(reference.seq(chr, pos_ma[i], block.length())==block)
				{
					pos_ma[i] -= block.length();
				}
				pos_ma[i] += block.length();

				//prepend prefix base
				pos_ma[i] -= 1;
				alt_ma[i] = reference.seq(chr, pos_ma[i], 1);
				ref_ma[i] = alt_ma[i] + ref_ma[i];

				//single-base shift deletion
				while(ref_ma[i].right(1)==alt_ma[i])
				{
					pos_ma[i] -= 1;
					alt_ma[i] = reference.seq(chr, pos_ma[i], 1);
					ref_ma[i] = alt_ma[i] + ref_ma[i].left(ref_ma[i].length()-1);
				}

				if (tsv_style)
				{
					alt_ma[i]="-";//VCF "G" -> TSV "-"
					ref_ma[i]=ref_ma[i].right(ref_ma[i].length()-1);//VCF "GA" -> TSV "A"
					pos_ma[i] += 1;//go one base to the right, since the common "G" is not saved in TSV-entries
				}
			}

			if (ref_ma[i]!=ref_ma[0] || pos_ma[i]!=pos_ma[0])
			{
				same_ref_ma = false;
			}
		}

		//skip multi-allelic variants the have different reference sequence after normalization and left-alignment
		if(!same_ref_ma) continue;

		//save left-aligned and normalized ref,alt and pos
		variant->setRef(ref_ma[0]);
		variant->setStart(pos_ma[0]);
		variant->setEnd(pos_ma[0]+ref_ma[0].length()-1);
		alt = alt_ma[0];
		for (int i=1; i<alt_ma.count(); ++i)//for each of multiples alt alleles
		{
			alt.append("," + alt_ma[i]);
		}
		variant->setObs(alt);
	}

	//by shifting all indels to the left, we might have produced duplicates - remove them
	removeDuplicates(sort_by_quality);
}

void VariantList::checkValid() const
{
	foreach(const Variant& variant, variants_)
	{
		if (!variant.chr().isValid())
		{
			THROW(ArgumentException, "Invalid variant chromosome string in variant '" + variant.toString());
		}

		if (variant.start()<1 || variant.end()<1 || variant.start()>variant.end())
		{
			THROW(ArgumentException, "Invalid variant position range in variant '" + variant.toString());
		}

		if (variant.ref()!="-" && !QRegExp("[ACGTN]+").exactMatch(variant.ref()))
		{
			THROW(ArgumentException, "Invalid variant reference sequence in variant '" + variant.toString());
		}
		if (variant.obs()!="-" && variant.obs()!="." && !QRegExp("[ACGTN,]+").exactMatch(variant.obs()))
		{
			THROW(ArgumentException, "Invalid variant observed sequence in variant '" + variant.toString());
		}

		if (variant.annotations().count()!=annotation_headers_.count())
		{
			THROW(ArgumentException, "Invalid variant annotation data: Expected " + QString::number(annotation_headers_.count()) + " values, but " + QString::number(variant.annotations().count()) + " values found");
		}
	}
}

SampleHeaderInfo VariantList::getSampleHeader(bool error_if_missing) const
{
	SampleHeaderInfo output;

	foreach(QString line, comments())
	{
		line = line.trimmed();

		if (line.startsWith("##SAMPLE=<"))
		{
			//split into key=value pairs
			QStringList parts = line.mid(10, line.length()-11).split(',');
			for (int i=1; i<parts.count(); ++i)
			{
				if (!parts[i].contains("="))
				{
					parts[i-1] += "," + parts[i];
					parts.removeAt(i);
					--i;
				}
			}

			foreach(const QString& part, parts)
			{
				int sep_idx = part.indexOf('=');
				QString key = part.left(sep_idx);
				QString value = part.mid(sep_idx+1);
				if (key=="ID")
				{
					SampleInfo tmp;
					tmp.id = value;
					tmp.column_name = value;
					output << tmp;
				}
				else
				{
					output.last().properties[key] = value;
				}
			}
		}
	}

	//special handling of single-sample analysis
	if (output.count()==1 && annotationIndexByName("genotype", true, false)!=-1)
	{
		output.first().column_name = "genotype";
	}

	if (output.count()==0 && error_if_missing)
	{
		THROW(ProgrammingException, "Could not find any sample information in the variant list header!");
	}

	//determine column index
	AnalysisType analysis_type = type();
	for (int i=0; i<output.count(); ++i)
	{
		output[i].column_index = annotationIndexByName(output[i].column_name, true, analysis_type!=SOMATIC_SINGLESAMPLE && analysis_type!=SOMATIC_PAIR);
	}

	return output;
}

QString VariantList::getPipeline() const
{
	foreach(QString line, comments_)
	{
		if (line.startsWith("##PIPELINE="))
		{
			return line.mid(11).trimmed();
		}
	}

	return "n/a";
}

AnalysisType VariantList::type(bool allow_fallback_germline_single_sample) const
{
	foreach(const QString& line, comments_)
	{
		if (line.startsWith("##ANALYSISTYPE="))
		{
			QString type = line.trimmed().mid(15);
			if (type=="GERMLINE_SINGLESAMPLE") return GERMLINE_SINGLESAMPLE;
			else if (type=="GERMLINE_TRIO") return GERMLINE_TRIO;
			else if (type=="GERMLINE_MULTISAMPLE") return GERMLINE_MULTISAMPLE;
			else if (type=="SOMATIC_SINGLESAMPLE") return SOMATIC_SINGLESAMPLE;
			else if (type=="SOMATIC_PAIR") return SOMATIC_PAIR;
			else THROW(FileParseException, "Invalid analysis type '" + type + "' found in variant list!");
		}
	}

	if (allow_fallback_germline_single_sample)
	{
		return GERMLINE_SINGLESAMPLE; //fallback for old files without ANALYSISTYPE header
	}
	else
	{
		THROW(FileParseException, "No analysis type found in variant list!");
	}
}

void Variant::normalize(int& start, Sequence& ref, Sequence& obs)
{
	//remove common first base
	if((ref.length()!=1 || obs.length()!=1) && ref.length()!=0 && obs.length()!=0 && ref[0]==obs[0])
	{
		ref = ref.mid(1);
		obs = obs.mid(1);
		start += 1;
	}

	//remove common suffix
	while((ref.length()!=1 || obs.length()!=1) && ref.length()!=0 && obs.length()!=0 && ref.right(1)==obs.right(1))
	{
		ref.resize(ref.length()-1);
		obs.resize(obs.length()-1);
	}

	//remove common prefix
	while((ref.length()!=1 || obs.length()!=1) && ref.length()!=0 && obs.length()!=0 && ref[0]==obs[0])
	{
		ref = ref.mid(1);
		obs = obs.mid(1);
		start += 1;
	}
}

Sequence Variant::minBlock(const Sequence& seq)
{
	int len = seq.length();
	for (int size=1; size<=len/2; ++size)
	{
		if (len%size!=0) continue;
		Sequence block = seq.left(size);
		//qDebug() << "minBlock - size: " << size << " block: " << block << " rep: " << block.repeated(len/size);
		if (seq==block.repeated(len/size))
		{
			return block;
		}
	}

	return seq;
}

QPair<int, int> Variant::indelRegion(const Chromosome& chr, int start, int end, Sequence ref, Sequence obs, const FastaFileIndex& reference)
{
	//needed for TSV format
	if (ref=="-") ref = "";
	if (obs=="-") obs = "";

	//SNV or complex indel => return original position
	normalize(start, ref, obs);
	if (ref.length()!=0 && obs.length()!=0)
	{
		return qMakePair(start, end);
	}

	//store original position
	int start_orig = start;
	int end_orig = end;

	//determine start/end
	Sequence block = minBlock(ref + obs);
	int block_length = block.length();
	//qDebug() << "BLOCK: " << block;

	//insertion (start and end are before the insertion position)
	bool is_repeat = false;
	if (ref.length()==0)
	{
		end -= block_length-1;
		while(reference.seq(chr, end + block_length, block_length)==block)
		{
			end += block_length;
			is_repeat = true;
		}
		start += 1;
		while(reference.seq(chr, start - block_length, block_length)==block)
		{
			start -= block_length;
			is_repeat = true;
		}
	}
	//deletion (start is first base of deletion, end is last base of deletion)
	else
	{
		end -= block_length-1;
		while(reference.seq(chr, end + block_length, block_length)==block)
		{
			end += block_length;
			is_repeat = true;
		}
		while(reference.seq(chr, start - block_length, block_length)==block)
		{
			start -= block_length;
			is_repeat = true;
		}
	}

	if (is_repeat) return qMakePair(start, end + block_length - 1);

	//no repeat region => return original position
	return qMakePair(start_orig, end_orig);
}

const QList<VariantTranscript> Variant::transcriptAnnotations(int column_index) const
{
	QList<VariantTranscript> output;

	foreach(QByteArray transcript, annotations()[column_index].split(','))
	{
		transcript = transcript.trimmed();
		if (transcript.isEmpty()) continue;

		QList<QByteArray> parts = transcript.split(':');
		if (parts.count()<8)
		{
			THROW(ProgrammingException, "Could not split transcript information from 'coding_and_splicing' column to 8 parts: " + transcript);
		}

		VariantTranscript trans;
		trans.gene = parts[0].trimmed();
		trans.id = parts[1].trimmed();
		trans.type = parts[2].trimmed();
		trans.impact = parts[3].trimmed();
		trans.exon = parts[4].trimmed();
		trans.hgvs_c = parts[5].trimmed();
		trans.hgvs_p = parts[6].trimmed();
		trans.domain = parts[7].trimmed();

		output << trans;
	}

	return output;
}

QDebug operator<<(QDebug d, const Variant& v)
{
	d.nospace() << v.chr().str() << ":" << v.start() << "-" << v.end() << " " << v.ref() << "=>" << v.obs();
	return d.space();
}


QByteArray VariantTranscript::toString(char sep) const
{
	return gene + sep + id + sep + type + sep + impact + sep + exon + sep + hgvs_c + sep + hgvs_p + sep + domain;
}

bool VariantTranscript::typeMatchesTerms(const OntologyTermCollection& terms) const
{
	foreach(QByteArray type, type.split('&'))
	{
		if(terms.containsByName(type.trimmed()))
		{
			return true;
		}
	}
	return false;
}
