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
#include <QBitArray>
#include <QUrl>

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

Variant::Variant(const VcfLine& vcf)
	: chr_(vcf.chr())
	, start_(vcf.start())
	, end_(vcf.end())
	, ref_(vcf.ref())
	, obs_(vcf.altString())
	, filters_()
	, annotations_()
{
	//check input
	if (!vcf.isValid()) THROW(Exception, "Cannot convert invalid VCF variant to GSvar variant: " + vcf.toString());
	if(vcf.isMultiAllelic()) THROW(Exception, "Cannot convert multi-allelic VCF variant to GSvar variant: " +vcf.toString());

	//normalize to GSvar format
	normalize("-", true);
}

QByteArrayList Variant::vepAnnotations(int csq_index, int field_index) const
{
	QByteArrayList output;

	QByteArray csq = annotations()[csq_index].trimmed();
    if (csq.size()>0)
	{
		QByteArrayList transcripts = csq.split(',');

		foreach(const QByteArray& transcript, transcripts)
		{
			QByteArrayList csq_fields = transcript.split('|');
			output << csq_fields[field_index];
		}
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

bool Variant::operator==(const Variant& rhs) const
{
	return start_==rhs.start_ && end_==rhs.end_ && chr_==rhs.chr_ && ref_==rhs.ref_ && obs_==rhs.obs_;
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

QString Variant::toString(QChar sep, int max_sequence_length, bool chr_normalized) const
{
	QByteArray ref = ref_;
	QByteArray obs = obs_;
	if (max_sequence_length>0)
	{
		if (ref.length()>max_sequence_length)
		{
			ref = ref.left(max_sequence_length) + "...[" + QByteArray::number(ref.length()) + " bases]";
		}
		if (obs.length()>max_sequence_length)
		{
			obs = obs.left(max_sequence_length) + "...[" + QByteArray::number(obs.length()) + " bases]";
		}
	}
	if (sep.isNull())
	{
		return (chr_normalized ? chr_.strNormalized(true) : chr_.str()) + ":" + QString::number(start_) + "-" + QString::number(end_) + " " + ref + ">" + obs;

	}
	else
	{
        return (chr_normalized ? chr_.strNormalized(true) : chr_.str()) + sep.toLatin1() + QString::number(start_) + sep.toLatin1() + QString::number(end_) + sep.toLatin1() + ref + sep.toLatin1() + obs;
	}
}

void Variant::checkValid() const
{
	if (!chr_.isValid())
	{
		THROW(ArgumentException, "Invalid variant chromosome string in variant '" + toString() + "'");
	}

	if (start_<1 || end_<1 || start_>end_)
	{
		THROW(ArgumentException, "Invalid variant position range in variant '" + toString() + "'");
	}

    if (ref()!="-" && !QRegularExpression(QRegularExpression::anchoredPattern("[ACGTN]+")).match(ref()).hasMatch())
	{
		THROW(ArgumentException, "Invalid variant reference sequence in variant '" + toString() + "'");
	}

    if (obs()!="-" && obs()!="." && !QRegularExpression(QRegularExpression::anchoredPattern("[ACGTN,]+")).match(obs()).hasMatch())
	{
		THROW(ArgumentException, "Invalid variant observed sequence in variant '" + toString() + "'");
	}

	if (ref()==obs())
	{
		THROW(ArgumentException, "Reference and observed sequence are the same in variant '" + toString() + "'");
	}

	if (ref()!="-")
	{
		int expected = end_-start_+1;
		if(ref_.size()!=expected)
		{
			THROW(ArgumentException, "Reference sequence length should be " + QString::number(expected) + ", but is " + QString::number(ref_.size()) + " in variant '" + toString() + "'");
		}
	}
}

void Variant::checkValid(const FastaFileIndex& reference) const
{
	checkValid();

	if (ref()!="-")
	{
		Sequence ref_expected = reference.seq(chr_, start_, end_-start_+1);
		if (ref_!=ref_expected)
		{
			THROW(ArgumentException, "Invalid reference sequence of variant '" + toString() + "': Variant reference sequence is '" + ref_ + "', but the genome sequence is '" + ref_expected + "'");
		}
	}
}

void Variant::leftAlign(const FastaFileIndex& reference)
{
	//skip SNVs
	if (isSNV()) return;

	//skip complex variants
	if (obs_.length()>1 && ref_.length()>1) return;

	ref_ = ref_.toUpper();
	obs_ = obs_.toUpper();

	//INSERTION
	if (ref_=="-")
	{
		ref_.clear();

		//block shift insertion
		Sequence block = Variant::minBlock(obs_);
		start_ -= block.length() - 1; //-1 because: GSvar insertions are inserted after the position
		while(reference.seq(chr_, start_, block.length())==block)
		{
			start_ -= block.length();
		}
		start_ += block.length() - 1; //-1 because: see above

		ref_ = reference.seq(chr_, start_, 1);
		obs_ = ref_ + obs_;

		//shift insertion to the left (because e.g. multiples of AGA could be multiples of AAG)
		while(ref_==obs_.right(1))
		{
			start_ -= 1;
			ref_ = reference.seq(chr_, start_, 1);
			obs_ = ref_ + obs_.left(obs_.length()-1);
		}

		ref_ = "-";
		obs_ = obs_.right(obs_.length()-1);
	}
	//DELETION (everything as a above, except ref and alt exchanged)
	else if (obs_=="-")
	{
		obs_.clear();

		//block shift deletion
		Sequence block = Variant::minBlock(ref_);
		while(reference.seq(chr_, start_, block.length())==block)
		{
			start_ -= block.length();
		}
		start_ += block.length();

		//prepend prefix base
		start_ -= 1;
		obs_ = reference.seq(chr_, start_, 1);
		ref_ = obs_ + ref_;

		//single-base shift deletion
		while(ref_.right(1)==obs_)
		{
			start_ -= 1;
			obs_ = reference.seq(chr_, start_, 1);
			ref_ = obs_ + ref_.left(ref_.length()-1);
		}

		obs_ = "-";
		ref_ = ref_.right(ref_.length()-1);
		start_ += 1;
	}

	end_ = start_ + ref_.length() - 1;
}

void Variant::normalize(const Sequence& empty_seq, bool to_gsvar_format)
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

	if (to_gsvar_format && ref_==empty_seq)
	{
		start_ -= 1;
		end_ -= 1;
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
	if (obs==ref.toReverseComplement())
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

    THROW(ProgrammingException, "Could not convert variant " + toString(QChar(), -1, false) + " to string! This should not happen!");
}

VcfLine Variant::toVCF(const FastaFileIndex& genome_index) const
{
	int pos = start_;
	Sequence ref = ref_;
	Sequence alt = obs_;

	//prepend base for InDels if they are not MNPs
	if (!isSNV() && !isMNP())
	{
		bool prepend_ref_base = false;
		if (ref=="-" || ref.isEmpty()) //insertion
		{
			ref.clear();
			prepend_ref_base = true;
		}
		else if (alt=="-" || alt.isEmpty()) //deletion
		{
			pos -= 1;
			alt.clear();
			prepend_ref_base = true;
		}
		else if(ref[0]!=alt[0]) //complex variant (insertion plus deletion), but not already prefixed
		{
			pos -= 1;
			prepend_ref_base = true;
		}

		if (prepend_ref_base)
		{
			Sequence prefix_base = genome_index.seq(chr_, pos, 1);
			ref = prefix_base + ref;
			alt = prefix_base + alt;
		}
	}

	return VcfLine(chr_, pos, ref, QList<Sequence>() << alt);
}

QString Variant::toGnomAD(const FastaFileIndex& genome_index) const
{
	QString output = chr_.strNormalized(false) + "-";

	if (obs_!="-" && ref_!="-") //SNV
	{
		output += QString::number(start_) + "-" + ref_ + "-" + obs_;
	}
	else
	{
		if (obs_=="-") //deletion
		{
			int pos = start_-1;
			QString base = genome_index.seq(chr_, pos, 1);
			output += QString::number(pos) + "-" + base + ref_ + "-" + base;
		}
		else if (ref_=="-") //insertion
		{
			int pos = start_;
			QString base = genome_index.seq(chr_, pos, 1);
			output += QString::number(start_) + "-" + base + "-" + base + obs_;
		}
	}

	return output;
}

VariantList::LessComparatorByAnnotation::LessComparatorByAnnotation(int annotation_index)
	: annotation_index_(annotation_index)
{
}

bool VariantList::LessComparatorByAnnotation::operator ()(const Variant& a, const Variant& b) const
{
	return ( a.annotations().at(annotation_index_) < b.annotations().at(annotation_index_) );
}


VariantList::LessComparatorByFile::LessComparatorByFile(QString filename)
	: filename_(filename)
{
	//build chromosome (as QString) to rank (as int) dictionary from file (rank=position in file)
	QStringList lines=Helper::loadTextFile(filename_);
	int rank=0;
	foreach(const QString& line, lines)
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

VariantList::LessComparator::LessComparator()
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

QString VariantList::analysisName() const
{
	//determine sample list
	QStringList samples;
	SampleHeaderInfo header_info = getSampleHeader();
	foreach(const SampleInfo& info, header_info)
	{
		samples << info.name;
	}

	return analysisTypeToString(type(), true) + " " + samples.join("/");
}

QString VariantList::mainSampleName() const
{
	QStringList samples;
	switch(type())
	{
		case SOMATIC_SINGLESAMPLE:
		case GERMLINE_SINGLESAMPLE:
		case CFDNA:
			foreach(const SampleInfo& entry, getSampleHeader())
			{
				samples << entry.name;
			}
			break;
		case GERMLINE_TRIO:
		case GERMLINE_MULTISAMPLE:
			foreach(const SampleInfo& entry, getSampleHeader())
			{
				if (entry.isAffected())
				{
					samples << entry.name;
				}
			}
			break;
		case SOMATIC_PAIR:
			foreach(const SampleInfo& entry, getSampleHeader())
			{
				if (entry.isTumor())
				{
					samples << entry.name;
				}
			}
			break;
	}

	if (samples.count()!=1) THROW(ProgrammingException, "Could not determine main processed sample for " + analysisName() + "!");

	return samples[0];
}

void VariantList::copyMetaData(const VariantList& rhs)
{
	comments_ = rhs.comments_;
	annotation_descriptions_ = rhs.annotation_descriptions_;
	annotation_headers_ = rhs.annotation_headers_;
	filters_ = rhs.filters_;
}

VariantAnnotationDescription VariantList::annotationDescriptionByName(const QString& description_name, bool error_not_found) const
{
	bool found_multiple = false;

	int index = -1;
	for(int i=0; i<annotationDescriptions().count(); ++i)
	{
		if(annotationDescriptions()[i].name()==description_name)
		{
			if(index!=-1)	found_multiple = true;
			index = i;
		}
	}

	if(error_not_found && index==-1)	THROW(ProgrammingException, "Could not find column description '" + description_name + "'.");
	if(error_not_found && found_multiple)	THROW(ProgrammingException, "Description for '" + description_name + "' occurs more than once.");

	if(!error_not_found && (found_multiple || index==-1))
	{
		return VariantAnnotationDescription();
	}
	return annotationDescriptions()[index];
}

int VariantList::annotationIndexByName(const QString& name, bool exact_match, bool error_on_mismatch) const
{

	//find matches
	QList<int> matches;
	for(int i=0; i<annotations().count(); ++i)
	{
		if ((exact_match && annotations()[i].name().compare(name, Qt::CaseInsensitive)==0) || (!exact_match && annotations()[i].name().contains(name, Qt::CaseInsensitive)))
		{
			matches.append(i);
		}
	}

	//error checks
	if (matches.count()<1)
	{
		if (error_on_mismatch)
		{
			THROW(ArgumentException, "Could not find column '" + name + "' in variant list!");
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
			THROW(ArgumentException, "Found multiple columns for '" + name + "' in variant list!");
		}
		else
		{
			Log::warn("Found multiple columns for '" + name + "' in variant list!");
			return -2;
		}
	}

	//return result
	return matches.at(0);
}

int VariantList::vepIndexByName(const QString& name, bool error_if_not_found) const
{
	VariantAnnotationDescription anno_desc = annotationDescriptionByName("CSQ", false);
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
	else
	{
		QList<VariantAnnotationDescription>& descs = annotationDescriptions();
		for (int i=0; i<descs.count(); ++i)
		{
			if(descs[i].name()==name)
			{
				descs[i].setDescription(description);
			}
		}
	}

	return index;
}

int VariantList::prependAnnotation(QString name, QString description, QByteArray default_value)
{
	annotations().prepend(VariantAnnotationHeader(name));
	for (int i=0; i<variants_.count(); ++i)
	{
		variants_[i].annotations().prepend(default_value);
	}

	annotationDescriptions().prepend(VariantAnnotationDescription(name, description));

	return 0;
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

void VariantList::load(QString filename, const BedFile& roi, bool invert)
{
	loadInternal(filename, &roi, invert);
}

void VariantList::load(QString filename)
{
	loadInternal(filename);
}

void VariantList::loadHeaderOnly(QString filename)
{
	loadInternal(filename, nullptr, false, true);
}

void VariantList::loadInternal(QString filename, const BedFile* roi, bool invert, bool header_only)
{
	//create cache to avoid copies of the same string in memory (via Qt implicit sharing)
	QHash<QByteArray, QByteArray> str_cache;

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
	constexpr int special_cols = 5;

	//remove old data
	clear();

	//parse from stream
	QSharedPointer<VersatileFile> file = Helper::openVersatileFileForReading(filename, true);
	int filter_index = -1;
	while(!file->atEnd())
	{
		QByteArray line = file->readLine(true);

		//skip empty lines
		if(line.length()==0) continue;

		if (line.startsWith("##"))//comment/description line
		{
			QList <QByteArray> parts = line.split('=');
			if (line.startsWith("##DESCRIPTION=") && parts.count()>2)
			{
				annotationDescriptions().append(VariantAnnotationDescription(parts[1], parts.mid(2).join('='), VariantAnnotationDescription::STRING));
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

		//skip variants if headers shall be loaded only
		if (header_only) break;

		//error when special columns are not present
		QByteArrayList fields = line.split('\t');
		if (fields.count()<special_cols)
		{
			THROW(FileParseException, "Variant TSV file line with less than five fields found: '" + line.trimmed() + "'");
		}

		//Skip variants that are not in the target region (if given)
		Chromosome chr = fields[0];
		int start = Helper::toInt(fields[1], "genomic start position");
		int end = Helper::toInt(fields[2], "genomic end position");
		if (roi_idx!=nullptr)
		{
			bool in_roi = roi_idx->matchingIndex(chr, start, end)!=-1;
			if ((!in_roi && !invert) || (in_roi && invert))
			{
				continue;
			}
		}

		//decode annotation entries that contains URL-encodings
		for (int i = special_cols; i < fields.size(); i++)
		{
			if (fields[i].contains('%'))
			{
				fields[i] = QUrl::fromPercentEncoding(fields[i]).toUtf8();
			}
		}

		//replace repeated strings with cached copy => save ~ 40% of memory
		for(int i=3; i<fields.count(); ++i)
		{
			const QByteArray& field = fields[i];
			if (!str_cache.contains(field))
			{
				str_cache.insert(field, field);
			}

			fields[i] = str_cache[field];
		}

		append(Variant(chr, start, end, fields[3], fields[4], fields.mid(special_cols), filter_index));

		//Check that the number of annotations is correct
		if (variants_.last().annotations().count()!=annotations().count())
		{
			THROW(FileParseException, "Variant with less than expected annotation fields found:\n" + variants_.last().toString() + "\nExpected " + QString::number(annotations().count()) + ", found " + QString::number(variants_.last().annotations().count()) + "!\n\nThis should not happen! Please inform the bioinformatics team!");
		}
	}
}

void VariantList::store(QString filename) const
{
	//open stream
	QSharedPointer<QFile> file = Helper::openFileForWriting(filename, true);
	QTextStream stream(file.data());
    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
    #else
    stream.setCodec("UTF-8");
    #endif

	//comments
	if (comments_.count()>0)
	{
		QStringListIterator i(comments_);
		while (i.hasNext())
		{
			QString comment=i.next();
			if (comment.startsWith("##fileformat=")) continue;//don't write VCF specific meta info
			stream << comment << "\n";
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

			stream <<"##DESCRIPTION=" << act_anno.name() << "=" << act_anno.description();

			stream << "\n";
		}
	}

	//filter headers
	auto it = filters().cbegin();
	while(it != filters().cend())
	{

		stream << "##FILTER=" << it.key() << "=" << it.value() << "\n";
		++it;
	}

	//header
	stream << "#chr\tstart\tend\tref\tobs";
	if (annotation_headers_.count()>0)
	{
		foreach(const VariantAnnotationHeader& act_anno, annotation_headers_)
		{
			stream << "\t" << act_anno.name();
		}
	}

	stream << "\n";

	//variants
	foreach(const Variant& v, variants_)
	{
		stream << v.chr().str() << "\t" << v.start() << "\t" << v.end() << "\t" << v.ref() << "\t" << v.obs();
		for(int i=0; i<v.annotations().count(); ++i)
		{
			QByteArray entry = v.annotations()[i];
			if(annotation_headers_[i].name()==".")	continue;
			stream << "\t" << entry.replace("\n", " ").replace("\t", " ");
		}
		stream << "\n";
	}
}

void VariantList::sort()
{
	sortCustom(LessComparator());
}

void VariantList::sortByAnnotation(int annotation_index)
{
	if (annotation_index<0 || annotation_index>=annotations().count())
	{
		THROW(ArgumentException, "Sorting by annotation failed: Index " + QString::number(annotation_index) + " is not a valid index in the annotation list!");
	}
	sortCustom(LessComparatorByAnnotation(annotation_index));
}

void VariantList::sortByFile(QString filename)
{
	sortCustom(LessComparatorByFile(filename));
}

void VariantList::removeDuplicates()
{
	sort();

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

void VariantList::leftAlign(QString ref_file)
{
	//open refererence genome file
	FastaFileIndex reference(ref_file);

	//init
	for (QVector<Variant>::iterator variant=variants_.begin(); variant!=variants_.end(); ++variant)
	{
		variant->leftAlign(reference);
	}

	//by shifting all indels to the left, we might have produced duplicates - remove them
	removeDuplicates();
}

void VariantList::checkValid() const
{
	foreach(const Variant& variant, variants_)
	{
		variant.checkValid();
		if (variant.annotations().count()!=annotation_headers_.count())
		{
			THROW(ArgumentException, "Invalid variant annotation data: Expected " + QString::number(annotation_headers_.count()) + " values, but " + QString::number(variant.annotations().count()) + " values found");
		}
	}
}

void VariantList::checkValid(const FastaFileIndex& reference) const
{
	foreach(const Variant& variant, variants_)
	{
		variant.checkValid(reference);
		if (variant.annotations().count()!=annotation_headers_.count())
		{
			THROW(ArgumentException, "Invalid variant annotation data: Expected " + QString::number(annotation_headers_.count()) + " values, but " + QString::number(variant.annotations().count()) + " values found");
		}
	}
}

SampleHeaderInfo VariantList::getSampleHeader(bool thow_if_no_samples) const
{
	SampleHeaderInfo output;

	foreach(QString line, comments_)
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
					tmp.name = value;
					output << tmp;
				}
				else
				{
					output.last().properties[key] = value;
				}
			}
		}
	}

	if (thow_if_no_samples && output.count()==0) THROW(ProgrammingException, "No sample information found in the variant list header!");

	//determine column indices
	AnalysisType analysis_type = type();
	for (int i=0; i<output.count(); ++i)
	{
		output[i].column_index = annotationIndexByName(output[i].name, true, analysis_type!=SOMATIC_SINGLESAMPLE && analysis_type!=SOMATIC_PAIR && analysis_type!=CFDNA);
	}

	return output;
}

GenomeBuild VariantList::build()
{
	foreach(const QString& line, comments_)
	{
		if (line.startsWith("##GENOME_BUILD="))
		{
			return stringToBuild(line.mid(15));
		}
	}

	return GenomeBuild::HG19; //fallback to hg19 - the GENOME_BUILD header was added in the GRCh38 branch of megSAP...
}

QString VariantList::getPipeline() const
{
	foreach(const QString& line, comments_)
	{
		if (line.startsWith("##PIPELINE="))
		{
			return line.mid(11).trimmed();
		}
	}

	return "n/a";
}

QDate VariantList::getCreationDate() const
{
	foreach(const QString& line, comments_)
	{
		if (line.startsWith("##CREATION_DATE="))
		{
			return QDate::fromString(line.mid(16).trimmed(), "yyyy-MM-dd");
		}
	}

	return QDate();
}

AnalysisType VariantList::type(bool allow_fallback_germline_single_sample) const
{
	foreach(const QString& line, comments_)
	{
		if (line.startsWith("##ANALYSISTYPE="))
		{
			QString type = line.mid(15).trimmed();
			return stringToAnalysisType(type);
		}
	}

	//fallback for old files without ANALYSISTYPE header and for default-constructed variant lists
	if (allow_fallback_germline_single_sample)
	{
		return GERMLINE_SINGLESAMPLE;
	}

	THROW(FileParseException, "No ANALYSISTYPE line found in variant list header!");
}

QByteArray VariantList::caller() const
{
	foreach(const QString& line, comments_)
	{
		if (line.startsWith("##SOURCE="))
		{
			QString tmp = line.mid(9).trimmed() + ' ';
			int sep_idx = tmp.indexOf(' ');
			return tmp.left(sep_idx).trimmed().toUtf8();
		}
	}

	return "";
}

QByteArray VariantList::callerVersion() const
{
	foreach(const QString& line, comments_)
	{
		if (line.startsWith("##SOURCE="))
		{
			QString tmp = line.mid(9).trimmed() + ' ';
			int sep_idx = tmp.indexOf(' ');
			return tmp.mid(sep_idx).trimmed().toUtf8();
		}
	}

	return "";
}

QDate VariantList::callingDate() const
{
	foreach(const QString& line, comments_)
	{
		if (line.startsWith("##CALLING_DATE="))
		{
			QString date = line.mid(15).trimmed();
			date.truncate(10); // cut off time if present
			return QDate::fromString(date, "yyyy-MM-dd");
		}
	}

	return QDate();
}

int VariantList::indexOf(const Variant& var)
{
	for (int i=0; i<count(); ++i)
	{
		if (variants_[i]==var) return i;
	}

	return -1;
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

QList<VariantTranscript> Variant::parseTranscriptString(QByteArray text, bool allow_old_format_with_7_columns)
{
	QList<VariantTranscript> output;

	foreach(QByteArray transcript, text.split(','))
	{
		transcript = transcript.trimmed();
		if (transcript.isEmpty()) continue;

		QList<QByteArray> parts = transcript.split(':');
		if (allow_old_format_with_7_columns) parts << "";
		if (parts.count()<8)
		{
			THROW(ProgrammingException, "Could not split transcript information from 'coding_and_splicing' column to 8 parts. " + QString::number(parts.count()) + " parts found in: " + transcript);
		}

		VariantTranscript trans;
		trans.gene = parts[0].trimmed();
		trans.id = parts[1].trimmed();
		trans.type = parts[2].trimmed();
		trans.impact = stringToVariantImpact(parts[3].trimmed());
		trans.exon = parts[4].trimmed();
		trans.hgvs_c = parts[5].trimmed();
		trans.hgvs_p = parts[6].trimmed();
		trans.domain = parts[7].trimmed();

		output << trans;
	}

	return output;
}

QByteArray VariantTranscript::toString(char sep) const
{
	return gene + sep + id + sep + type + sep + variantImpactToString(impact) + sep + exon + sep + hgvs_c + sep + hgvs_p + sep + domain;
}

bool VariantTranscript::typeMatchesTerms(const OntologyTermCollection& terms) const
{
	foreach(const QByteArray& type, type.split('&'))
	{
		if(terms.containsByName(type.trimmed()))
		{
			return true;
		}
	}
	return false;
}

QByteArray VariantTranscript::idWithoutVersion() const
{
	if (!id.contains(".")) return id;

	return id.left(id.lastIndexOf("."));
}

Variant Variant::fromString(const QString& text_orig)
{
	//replace special characters
	QString text = text_orig.trimmed();
	text.replace("\t", " ");
	text.replace(":", " ");
	text.replace(">", " ");
    text.replace(QRegularExpression("-([0-9])"), " \\1"); //replace '-' between start/end but preserve '-' in ref/obs of indels
    text.replace(QRegularExpression("([0-9]+)"), "\\1 "); //special handling if space after end position is missing
	text = text.simplified();

	//split
    QStringList parts = text.split(QRegularExpression("\\s+"));
	if (parts.count()!=5) THROW(ArgumentException, "Input text has " + QString::number(parts.count()) + " part(s), but must consist of 5 parts (chr, start, end, ref, obs)!");

	return Variant(parts[0], parts[1].toInt(), parts[2].toInt(), parts[3].toUtf8(), parts[4].toUtf8());
}

QString analysisTypeToString(AnalysisType type, bool human_readable)
{
	if (human_readable)
	{
		if (type==GERMLINE_SINGLESAMPLE) return "single-sample analysis";
		if (type==GERMLINE_TRIO) return "trio analysis";
		if (type==GERMLINE_MULTISAMPLE) return "multi-sample analysis";
		if (type==SOMATIC_SINGLESAMPLE) return "tumor-only analysis";
		if (type==SOMATIC_PAIR) return "tumor/normal analysis";
		if (type==CFDNA) return "cfDNA analysis";
	}
	else
	{
		if (type==GERMLINE_SINGLESAMPLE) return "GERMLINE_SINGLESAMPLE";
		if (type==GERMLINE_TRIO) return "GERMLINE_TRIO";
		if (type==GERMLINE_MULTISAMPLE) return "GERMLINE_MULTISAMPLE";
		if (type==SOMATIC_SINGLESAMPLE) return "SOMATIC_SINGLESAMPLE";
		if (type==SOMATIC_PAIR) return "SOMATIC_PAIR";
		if (type==CFDNA) return "CFDNA";
	}

	THROW(ProgrammingException, "Unhandled analysis type with integer value '" + QString::number(type) + "'!");
}

AnalysisType stringToAnalysisType(QString type)
{
	if (type=="GERMLINE_SINGLESAMPLE") return GERMLINE_SINGLESAMPLE;
	if (type=="GERMLINE_TRIO") return GERMLINE_TRIO;
	if (type=="GERMLINE_MULTISAMPLE") return GERMLINE_MULTISAMPLE;
	if (type=="SOMATIC_SINGLESAMPLE") return SOMATIC_SINGLESAMPLE;
	if (type=="SOMATIC_PAIR") return SOMATIC_PAIR;
	if (type=="CFDNA") return CFDNA;

	THROW(ProgrammingException, "Unknown analysis type with string representation '" + type + "'!");
}
