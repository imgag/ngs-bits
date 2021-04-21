#include "NGSHelper.h"
#include "Exceptions.h"
#include "Helper.h"
#include "BasicStatistics.h"
#include "BamReader.h"
#include "GeneSet.h"

#include <QTextStream>
#include <QFileInfo>
#include <QDateTime>
#include <cmath>
#include <vector>

namespace {

	QString copyFromResource(const QString& build)
	{
		//check variant list exists
		QString snp_file = ":/Resources/" + build + "_snps.vcf";
		if (!QFile::exists(snp_file)) THROW(ProgrammingException, "Unsupported genome build '" + build + "'!");

		//copy from resource file (gzopen cannot access Qt resources)
		QString tmp = Helper::tempFileName(".vcf");
		QFile::copy(snp_file, tmp);

		return tmp;
	}

	void filterVcfFile(VcfFile& output, bool only_snvs, double min_af, double max_af)
	{
		FilterResult filter_result(output.count());

		//check input
		if (min_af<0.0 || min_af>1.0)
		{
			THROW(ArgumentException, "Minumum allele frequency out of range (0.0-1.0): " + QByteArray::number(min_af));
		}
		if (max_af<0.0 || max_af>1.0)
		{
			THROW(ArgumentException, "Maximum allele frequency out of range (0.0-1.0): " + QByteArray::number(max_af));
		}

		//filter by AF
		if (min_af>0.0 || max_af<1.0)
		{
			for (int i=0; i<output.count(); ++i)
			{
				double af = output[i].info("AF").toDouble();
				filter_result.flags()[i] = af>=min_af && af<=max_af;
			}
		}

		//filter only SNVs
		if (only_snvs)
		{
			FilterVariantIsSNV filter;
			filter.apply(output, filter_result);
		}

		//apply filters
		filter_result.removeFlagged(output);
	}
} // end anonymous namespace

VcfFile NGSHelper::getKnownVariants(QString build, bool only_snvs, const BedFile& roi, double min_af, double max_af)
{
	//check variant list exists
	QString tmp = copyFromResource(build);

	//load
	VcfFile output;
	output.load(tmp, roi, false);

	//filter variants
	filterVcfFile(output, only_snvs, min_af, max_af);

	return output;
}

VcfFile NGSHelper::getKnownVariants(QString build, bool only_snvs, double min_af, double max_af)
{
	//check variant list exists
	QString tmp = copyFromResource(build);

	//load
	VcfFile output;
	output.load(tmp, false);

	//filter variants
	filterVcfFile(output, only_snvs, min_af, max_af);

	return output;
}

void NGSHelper::createSampleOverview(QStringList in, QString out, int indel_window, bool cols_auto, QStringList cols)
{
	//determine columns contained in all samples from file headers (keep order)
	if (cols_auto)
	{
		bool init = true;
		foreach(QString filename, in)
		{
			auto file = Helper::openFileForReading(filename, false);
			while (!file->atEnd())
			{
				QString line = file->readLine();
				if (!line.startsWith('#')) break;
				if (line.startsWith("#chr"))
				{
					if (init)
					{
						QStringList parts = line.trimmed().split('\t');
						foreach(QString part, parts)
						{
							//skip base columns
							if (part=="#chr" || part=="start" || part=="end" || part=="ref" || part=="obs") continue;

							//skip sample-specific germline columns
							if (part=="genotype" || part=="quality") continue;

							//skip sample-specific somatic columns
							if (part=="tumor_af" || part=="tumor_dp" || part=="normal_af" || part=="normal_dp")	continue;

							cols.append(part);
						}
						init = false;
					}
					else
					{
						QSet<QString> parts = line.trimmed().split('\t').toSet();
						for (int i=cols.count()-1; i>=0; --i)
						{
							if (!parts.contains(cols[i]))
							{
								cols.removeAt(i);
							}
						}
					}
				}
			}
			file->close();
		}
	}

	//load variant lists
	QVector<VariantList> vls;
	QVector<QVector<int> > vls_anno_indices;
	QList <VariantAnnotationDescription> vls_anno_descriptions;
	foreach(QString filename, in)
	{
		VariantList vl;
		vl.load(filename);

		//check the all required fields are present in the input file
		QVector<int> anno_indices;
		foreach(QString col, cols)
		{
			if (col=="genotype") continue;
			int index = vl.annotationIndexByName(col, true, true);
			anno_indices.append(index);

			foreach(VariantAnnotationDescription vad, vl.annotationDescriptions())
			{
				if(vad.name()==col)
				{
					bool already_found = false;
					foreach(VariantAnnotationDescription vad2, vls_anno_descriptions)
					{
						if(vad2.name()==col) already_found = true;
					}

					if(!already_found) vls_anno_descriptions.append(vad);
				}
			}
		}

		vls_anno_indices.append(anno_indices);
		vls.append(vl);
	}

	//set up combined variant list (annotation and filter descriptions)
	VariantList vl_merged;
	foreach(const VariantList& vl, vls)
	{
		auto it = vl.filters().begin();
		while(it!=vl.filters().end())
		{
			if (!vl_merged.filters().contains(it.key()))
			{
				vl_merged.filters().insert(it.key(), it.value());
			}
			++it;
		}
	}
	foreach(int index, vls_anno_indices[0])
	{
		vl_merged.annotations().append(vls[0].annotations()[index]);
	}
	foreach(VariantAnnotationDescription vad, vls_anno_descriptions)
	{
		vl_merged.annotationDescriptions().append(vad);
	}

	//merge variants
	vl_merged.reserve(2 * vls[0].count());
	for (int i=0; i<vls.count(); ++i)
	{
		for(int j=0; j<vls[i].count(); ++j)
		{
			Variant v = vls[i][j];
			QList<QByteArray> annos = v.annotations();
			v.annotations().clear();
			foreach(int index, vls_anno_indices[i])
			{
				v.annotations().append(annos[index]);
			}
			vl_merged.append(v);
		}
	}

	//remove duplicates from variant list
	vl_merged.removeDuplicates(false);

	//append sample columns
	for (int i=0; i<vls.count(); ++i)
	{
		//get genotype/AF index
		int geno_index = -1;
		AnalysisType type = vls[i].type();
		if (type==SOMATIC_SINGLESAMPLE || type==SOMATIC_PAIR)
		{
			geno_index = vls[i].annotationIndexByName("tumor_af", true, true);
		}
		else if (type==GERMLINE_SINGLESAMPLE || type==GERMLINE_TRIO || type==GERMLINE_MULTISAMPLE)
		{
			QList<int> affected_cols = vls[i].getSampleHeader().sampleColumns(true);
			if (affected_cols.count()==1)
			{
				geno_index = affected_cols[0];
			}
			else
			{
				THROW(ToolFailedException, "No/several affected in sample header of input file '" + in[i] + "'.");
			}
		}
		else
		{
			THROW(ToolFailedException, "Unsupported analysis type in input file '" + in[i] + "'.");
		}

		//add column header
		vl_merged.annotationDescriptions().append(VariantAnnotationDescription(QFileInfo(in[i]).baseName(), ""));
		vl_merged.annotations().append(VariantAnnotationHeader(QFileInfo(in[i]).baseName()));

		//create index over variant list to speed up the search
		const VariantList& vl = vls[i];
		ChromosomalIndex<VariantList> cidx(vl);

		//add sample-specific columns
		for (int j=0; j<vl_merged.count(); ++j)
		{
			Variant& v = vl_merged[j];
			QByteArray entry = "no";
			if (v.isSNV()) //SNP
			{
				QVector<int> matches = cidx.matchingIndices(v.chr(), v.start(), v.end());
				for (int k=0; k<matches.count(); ++k)
				{
					int match = matches[k];
					if (match!=-1 && vl[match].ref()==v.ref() && vl[match].obs()==v.obs())
					{
						entry = "yes (" + vl[match].annotations()[geno_index] + ")";
					}
				}
			}
			else //indel
			{
				QVector<int> matches = cidx.matchingIndices(v.chr(), v.start()-indel_window, v.end()+indel_window);
				if (matches.count()>0)
				{
					//exact match (start, obs, ref)
					bool done = false;
					for (int k=0; k<matches.count(); ++k)
					{
						const Variant& v2 = vl[matches[k]];
						if (!done && v2.start()==v.start() && v2.ref()==v.ref() && v2.obs()==v.obs())
						{
							entry = "yes (" + v2.annotations()[geno_index] + ")";
							done = true;
						}
					}

					//same indel nearby (ref, obs)
					for (int k=0; k<matches.count(); ++k)
					{
						const Variant& v2 = vl[matches[k]];
						if (!done && v2.ref()==v.ref() && v2.obs()==v.obs())
						{
							entry = "near (" + v2.annotations()[geno_index] + ")";
							done = true;
						}
					}

					//different indel nearby
					for (int k=0; k<matches.count(); ++k)
					{
						const Variant& v2 = vl[matches[k]];
						if (!done && !v2.isSNV())
						{
							entry = "different (" + v2.annotations()[geno_index] + ")";
							done = true;
						}
					}
				}
			}
			v.annotations().append(entry);
		}
	}

	vl_merged.store(out);
}

QByteArray NGSHelper::expandAminoAcidAbbreviation(QChar amino_acid_change_in)
{
	const static QHash<QChar,QByteArray> dictionary = {{'A',"Ala"},{'R',"Arg"},{'N',"Asn"},{'D',"Asp"},{'C',"Cys"},{'E',"Glu"},
													   {'Q',"Gln"},{'G',"Gly"},{'H',"His"},{'I',"Ile"},{'L',"Leu"},{'K',"Lys"},{'M',"Met"},{'F',"Phe"},{'P',"Pro"},{'S',"Ser"},
													   {'T',"Thr"},{'W',"Trp"},{'Y',"Tyr"},{'V',"Val"},{'*',"*"}};

	QByteArray amino_acid_change_out;
	if(dictionary.keys().contains(amino_acid_change_in))
	{
		amino_acid_change_out = dictionary.value(amino_acid_change_in);
	}
	else
	{
		amino_acid_change_out = "";
	}

	return amino_acid_change_out;
}

const BedFile& NGSHelper::pseudoAutosomalRegion(const QString& build)
{
	static QMap<QString, BedFile> output;

	//init - taken from https://www.ncbi.nlm.nih.gov/grc/human
	if (output.isEmpty())
	{
		output["hg19"].append(BedLine(Chromosome("chrX"), 60001, 2699520));
		output["hg19"].append(BedLine(Chromosome("chrX"), 154931044, 155260560));
		output["hg19"].append(BedLine(Chromosome("chrY"), 10001, 2649520));
		output["hg19"].append(BedLine(Chromosome("chrY"), 59034050, 59363566));

		output["hg38"].append(BedLine(Chromosome("chrX"), 10001, 2781479));
		output["hg38"].append(BedLine(Chromosome("chrX"), 155701383, 156030895));
		output["hg38"].append(BedLine(Chromosome("chrY"), 10001, 2781479));
		output["hg38"].append(BedLine(Chromosome("chrY"), 56887903, 57217415));
	}

	//check build is supported
	if  (!output.contains(build)) THROW(ProgrammingException, "Unsupported genome build '" + build + "' in NGSHelper::pseudoAutosomalRegion!");

	return output[build];
}

QByteArray NGSHelper::cytoBand(Chromosome chr, int pos)
{
	//init
	static BedFile bands;
	if (bands.count()==0)
	{
		bands.load(":/Resources/cyto_band.bed");
	}

	//search for band
	for (int i=0; i<bands.count(); ++i)
	{
		if (bands[i].overlapsWith(chr, pos, pos))
		{
			return chr.strNormalized(false) + bands[i].annotations()[0];
		}
	}

	THROW(ProgrammingException, "Could not find band for coordinate " + chr.str() + ":" + QString::number(pos));
}

BedLine NGSHelper::cytoBandToRange(QByteArray cytoband)
{
	//init
	static BedFile bands;
	if (bands.count()==0)
	{
		bands.load(":/Resources/cyto_band.bed");
	}

	//determine chromosome
	if (cytoband.contains('-'))
	{
		QByteArrayList parts = cytoband.split('-');
		if (parts.count()!=2)
		{
			THROW(ArgumentException, "Cytoband range '" + cytoband + "' contains more than one '-'!");
		}
		else
		{
			BedLine range1 = cytoBandToRange(parts[0]);
			BedLine range2 = cytoBandToRange(parts[1]);

			if (range1.chr()!=range2.chr()) THROW(ArgumentException, "Cytoband '" + cytoband + "' contains range with non-matching chromosomes!");

			int start =  std::min(range1.start(), range2.start());
			int end =  std::max(range1.end(), range2.end());
			return BedLine(range1.chr(), start, end);
		}
	}
	else
	{
		int sep = cytoband.indexOf('p');
		if (sep==-1) sep = cytoband.indexOf('q');
		if (sep==-1) THROW(ArgumentException, "Cytoband '" + cytoband + "' contains no 'p' or 'q'!");

		Chromosome chr(cytoband.left(sep));
		if (!chr.isAutosome() && !chr.isGonosome()) THROW(ArgumentException, "Cytoband '" + cytoband + "' contains invalid chromosome '" + chr.str() + "'!");
		QByteArray band = cytoband.mid(sep);

		for (int i=0; i<bands.count(); ++i)
		{
			if (bands[i].chr()!=chr) continue;

			if (bands[i].annotations()[0]==band)
			{
				return BedLine(chr, bands[i].start(), bands[i].end());
			}
		}
		THROW(ArgumentException, "Cytoband '" + cytoband + "' contains unknown band name '" + band + "'!");
	}
}

const QMap<QByteArray, ImprintingInfo>& NGSHelper::imprintingGenes()
{
	static QMap<QByteArray, ImprintingInfo> output;

	//init
	if (output.isEmpty())
	{
		QStringList lines = Helper::loadTextFile(":/Resources/imprinting_genes.tsv", true, '#', true);
		foreach(const QString& line, lines)
		{
			QStringList parts = line.split("\t");
			if (parts.count()==3)
			{
				QByteArray gene = parts[0].toLatin1().trimmed();
				QByteArray source_allele = parts[1].toLatin1().trimmed();
				QByteArray status = parts[2].toLatin1().trimmed();
				output[gene] = ImprintingInfo{source_allele, status};
			}
		}
	}

	return output;
}

void NGSHelper::parseRegion(const QString& text, Chromosome& chr, int& start, int& end)
{
	QString simplyfied = text;
	simplyfied.replace("-", " ");
	simplyfied.replace(":", " ");
	simplyfied.replace(",", "");
	QStringList parts = simplyfied.split(QRegularExpression("\\W+"), QString::SkipEmptyParts);
	if (parts.count()!=3) THROW(ArgumentException, "Could not split chromosomal range '" + text + "' in three parts: " + QString::number(parts.count()) + " parts found.");

	chr = Chromosome(parts[0]);
	if (!chr.isValid()) THROW(ArgumentException, "Invalid chromosome given in chromosomal range '" + text + "': " + parts[0]);
	start = Helper::toInt(parts[1], "Start coordinate", text);
	end = Helper::toInt(parts[2], "End coordinate", text);
}

void NGSHelper::softClipAlignment(BamAlignment& al, int start_ref_pos, int end_ref_pos)
{
	QList<CigarOp> old_CIGAR = al.cigarData();

	//backup old CIGAR string
	al.addTag("BS", 'Z', al.cigarDataAsString());

	//check preconditions
	if(start_ref_pos > end_ref_pos)
	{
		THROW(ToolFailedException, "End position is smaller than start position.");
	}
	if(start_ref_pos < al.start() || start_ref_pos > al.end())
	{
		THROW(ToolFailedException, "Start position " + QString::number(start_ref_pos) + " not within alignment (" + QString::number(al.start()) + ":" + QString::number(al.end()) + ").");
	}

	if(end_ref_pos < al.start() || end_ref_pos > al.end())
	{
		THROW(ToolFailedException, "End position " + QString::number(end_ref_pos) + " not within alignment (" + QString::number(al.start()) + ":" + QString::number(al.end()) + ").");
	}
	for(int i=0;i<old_CIGAR.size(); ++i)
	{
		if(old_CIGAR[i].Type!=BAM_CDEL && old_CIGAR[i].Type!=BAM_CSOFT_CLIP && old_CIGAR[i].Type!=BAM_CMATCH && old_CIGAR[i].Type!=BAM_CINS && old_CIGAR[i].Type!=BAM_CHARD_CLIP)
		{
			THROW(ToolFailedException, "Unsupported CIGAR type '" + QString(old_CIGAR[i].Type) + "'");
		}
	}

	//generate CIGAR char matrix from CIGAR
	QList<QPair<int,int>> matrix;
	for (int i=0; i<old_CIGAR.size(); ++i)
	{
		for(int j=0; j<old_CIGAR[i].Length; ++j)
		{
			matrix.append(qMakePair(old_CIGAR[i].Type, old_CIGAR[i].Type));
		}
	}

	//soft clip bases in matrix according to given ref_positions
	int j = 0;
	int current_ref_pos = al.start();
	while(current_ref_pos<=al.end())
	{
		if(j>=matrix.size())
		{
			THROW(ToolFailedException, "Index out of boundary!");
		}

		if(matrix[j].first!=BAM_CHARD_CLIP)
		{
			if(current_ref_pos>=start_ref_pos && current_ref_pos<=end_ref_pos)
			{
				matrix[j].second = BAM_CSOFT_CLIP;
			}
			if(matrix[j].first==BAM_CDEL || matrix[j].first==BAM_CMATCH)
			{
				++current_ref_pos;
			}
		}

		++j;
	}

	//summarize chars within matrix > generate new CIGAR string
	QList<CigarOp> new_CIGAR;
	int tmp_char = -1;
	int tmp_count = 0;
	for(int i=0; i<matrix.size(); ++i)
	{
		//skip soft-clipped deletions
		if(matrix[i].first==BAM_CDEL && matrix[i].second==BAM_CSOFT_CLIP) continue;

		if(matrix[i].second!=tmp_char)
		{
			if(tmp_char!=-1)
			{
				new_CIGAR.append(CigarOp {tmp_char, tmp_count});
			}

			tmp_char = matrix[i].second;
			tmp_count = 0;
		}
		++tmp_count;
	}
	new_CIGAR.append(CigarOp {tmp_char, tmp_count});

	//clean up cigar string; insertions and deletion around soft-clipped regions
	for(int i=1; i<new_CIGAR.size(); ++i)
	{
		bool redo = false;

		// 1. remove deleted bases around soft-clipped bases
		if(new_CIGAR[i-1].Type==BAM_CSOFT_CLIP && new_CIGAR[i].Type==BAM_CDEL)
		{
			new_CIGAR.erase(new_CIGAR.begin()+i);
			redo = true;
		}
		else if(new_CIGAR[i-1].Type==BAM_CDEL && new_CIGAR[i].Type==BAM_CSOFT_CLIP)
		{
			new_CIGAR.erase(new_CIGAR.begin()+(i-1));
			redo = true;
		}
		//2. remove inserted bases around soft-clipped bases
		else if(new_CIGAR[i-1].Type==BAM_CSOFT_CLIP && new_CIGAR[i].Type==BAM_CINS)
		{
			new_CIGAR[i-1].Length += new_CIGAR[i].Length;
			new_CIGAR.erase(new_CIGAR.begin()+i);
			redo = true;
		}
		else if(new_CIGAR[i-1].Type==BAM_CINS && new_CIGAR[i].Type==BAM_CSOFT_CLIP)
		{
			new_CIGAR[i].Length += new_CIGAR[i-1].Length;
			new_CIGAR.erase(new_CIGAR.begin()+(i-1));
			redo = true;
		}

		if(redo)
		{
			--i;
		}
	}

	//correct left-most position if first bases are soft-clipped, consider bases that were already softclipped previously
	int start_index = 0;
	while(matrix[start_index].second==BAM_CHARD_CLIP && start_index < matrix.size())
	{
		++start_index;
	}
	if(matrix[start_index].second==BAM_CSOFT_CLIP)
	{
		int offset = 0;
		while(start_index<matrix.size() && matrix[start_index].second==BAM_CSOFT_CLIP)
		{
			if(matrix[start_index].first==BAM_CMATCH|| matrix[start_index].first==BAM_CDEL)
			{
				++offset;
			}
			++start_index;
		}
		al.setStart(al.start() + offset);
	}

	al.setCigarData(new_CIGAR);
}

bool SampleInfo::isAffected() const
{
	auto it = properties.cbegin();
	while(it != properties.cend())
	{
		//support for old and new disease status annotations
		if ((it.key().toLower()=="diseasestatus" || it.key().toLower()=="status") && it.value().toLower()=="affected")
		{
			return true;
		}

		++it;
	}

	return false;
}

bool SampleInfo::isTumor() const
{
	auto it = properties.cbegin();
	while(it != properties.cend())
	{
		if (it.key().toLower()=="istumor" && it.value().toLower()=="yes")
		{
			return true;
		}

		++it;
	}

	return false;
}

QString SampleInfo::gender() const
{
	auto it = properties.cbegin();
	while(it != properties.cend())
	{
		if (it.key().toLower()=="gender")
		{
			return it.value().toLower();
		}

		++it;
	}

	return "n/a";
}


const SampleInfo& SampleHeaderInfo::infoByID(const QString& id) const
{
	foreach(const SampleInfo& info, *this)
	{
		if (info.id==id)
		{
			return info;
		}
	}

	THROW(ProgrammingException, "No sample with ID '" + id + "' found in sample info header!");
}

const SampleInfo& SampleHeaderInfo::infoByStatus(bool affected, QString gender) const
{
	QList<int> matches;
	for(int i=0; i<count(); ++i)
	{
		if (at(i).isAffected()==affected && (gender=="n/a" || at(i).gender()==gender))
		{
			matches << i;
		}
	}

	if (matches.count()==0)
	{
		THROW(ProgrammingException, "No sample found in header!");
	}

	if (matches.count()>1)
	{
		THROW(ProgrammingException, "More than one sample found in header!");
	}

	return at(matches[0]);
}

QList<int> SampleHeaderInfo::sampleColumns(bool affected) const
{
	QList<int>  output;
	foreach(const SampleInfo& info, *this)
	{
		if (affected==info.isAffected())
		{
			output << info.column_index;
		}
	}

	return output;
}

