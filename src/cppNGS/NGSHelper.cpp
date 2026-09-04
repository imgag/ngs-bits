#include "NGSHelper.h"
#include "Helper.h"
#include "FilterCascade.h"
#include "Log.h"
#include <QFileInfo>

namespace
{
	QString copyFromResource()
	{
		//copy from resource file (gzopen cannot access Qt resources)
		QString tmp = Helper::tempFileNameNonRandom("hg38_snps.vcf");
		QFile::copy(":/Resources/hg38_snps.vcf", tmp);

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

VcfFile NGSHelper::getKnownVariants(bool only_snvs, const BedFile& roi, double min_af, double max_af)
{
	//check variant list exists
	QString tmp = copyFromResource();

	//load
	VcfFile output;
	output.setRegion(roi);
	output.load(tmp);

	//remove temporary file
	QFile::remove(tmp);

	//filter variants
	filterVcfFile(output, only_snvs, min_af, max_af);

	return output;
}

VcfFile NGSHelper::getKnownVariants(bool only_snvs, double min_af, double max_af)
{
	//check variant list exists
	QString tmp = copyFromResource();

	//load
	VcfFile output;
	output.setAllowMultiSample(false);
	output.load(tmp);

	//remove temporary file
	QFile::remove(tmp);

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
                        QSet<QString> parts = Helper::listToSet(line.trimmed().split('\t'));
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
	vl_merged.removeDuplicates();

	//append sample columns
	for (int i=0; i<vls.count(); ++i)
	{
		//get genotype/AF index
		int geno_index = -1;
		AnalysisType type = vls[i].type();
		if (type==SOMATIC_SINGLESAMPLE || type==SOMATIC_PAIR || type==CFDNA)
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

char NGSHelper::translateCodon(const QByteArray& codon, bool use_mito_table)
{
	//init
	const static QHash<QByteArray, char> dictionary =   {{"TTT", 'F'}, {"TTC", 'F'}, {"TTA", 'L'},  {"TTG", 'L'}, {"CTT", 'L'}, {"CTC", 'L'},
														  {"CTA", 'L'}, {"CTG", 'L'}, {"TCT", 'S'}, {"TCC", 'S'}, {"TCA", 'S'}, {"TCG", 'S'},
														  {"AGT", 'S'}, {"AGC", 'S'}, {"TAT", 'Y'}, {"TAC", 'Y'}, {"TAA", '*'}, {"TAG", '*'},
														  {"TGA", '*'}, {"TGT", 'C'}, {"TGC", 'C'}, {"TGG", 'W'}, {"CCT", 'P'}, {"CCC", 'P'},
														  {"CCA", 'P'}, {"CCG", 'P'}, {"CAT", 'H'}, {"CAC", 'H'}, {"CAA", 'Q'}, {"CAG", 'Q'},
														  {"CGT", 'R'}, {"CGC", 'R'}, {"CGA", 'R'}, {"CGG", 'R'}, {"AGA", 'R'}, {"AGG", 'R'},
														  {"ATT", 'I'}, {"ATC", 'I'}, {"ATA", 'I'}, {"ATG", 'M'}, {"ACT", 'T'}, {"ACC", 'T'},
														  {"ACA", 'T'}, {"ACG", 'T'}, {"AAT", 'N'}, {"AAC", 'N'}, {"AAA", 'K'}, {"AAG", 'K'},
														  {"GTT", 'V'}, {"GTC", 'V'}, {"GTA", 'V'}, {"GTG", 'V'}, {"GCT", 'A'}, {"GCC", 'A'},
														  {"GCA", 'A'}, {"GCG", 'A'}, {"GAT", 'D'}, {"GAC", 'D'}, {"GAA", 'E'}, {"GAG", 'E'},
														  {"GGT", 'G'}, {"GGC", 'G'}, {"GGA", 'G'}, {"GGG", 'G'}};

	//check
	if (!dictionary.contains(codon)) THROW(ProgrammingException, "Invalid codon: '" + codon + "'");

	//special-handling of mito (see 2. chapter of https://www.ncbi.nlm.nih.gov/Taxonomy/Utils/wprintgc.cgi)
	if (use_mito_table)
	{
		if (codon=="AGA") return '*';
		else if (codon=="AGG") return '*';
		else if (codon=="ATA") return 'M';
		else if (codon=="TGA") return 'W';
	}

	//return
	return dictionary[codon];
}

QByteArray NGSHelper::translateCodonThreeLetterCode(const QByteArray& codon, bool use_mito_table)
{
	char one_letter_code = translateCodon(codon, use_mito_table);
	return threeLetterCode(one_letter_code);
}

QByteArray NGSHelper::translateSequence(const Sequence& sequence, bool use_three_letter_code, bool use_mito_table, bool end_at_stop)
{
	//translate a DNA sequence into an amino acid sequence
	if(sequence.length() % 3 != 0) THROW(ArgumentException, "Coding sequence length must be multiple of three.")

	QByteArray aa_seq;

	for(int i=0; i<sequence.length(); i+=3)
	{
		if (use_three_letter_code)
		{
			aa_seq.append(NGSHelper::translateCodonThreeLetterCode(sequence.mid(i, 3), use_mito_table));
		}
		else
		{
			aa_seq.append(NGSHelper::translateCodon(sequence.mid(i, 3), use_mito_table));
		}

		//only translate up to termination codon
		if(end_at_stop && use_three_letter_code && aa_seq.right(3) == "Ter") break;
		if(end_at_stop && ! use_three_letter_code && aa_seq.right(1) == "*") break;
	}
	return aa_seq;

}

QByteArray NGSHelper::threeLetterCode(char one_letter_code)
{
	//init
	const static QHash<char,QByteArray> dictionary = {{'A',"Ala"},{'R',"Arg"},{'N',"Asn"},{'D',"Asp"},{'C',"Cys"},{'E',"Glu"},
													   {'Q',"Gln"},{'G',"Gly"},{'H',"His"},{'I',"Ile"},{'L',"Leu"},{'K',"Lys"},{'M',"Met"},{'F',"Phe"},{'P',"Pro"},{'S',"Ser"},
													   {'T',"Thr"},{'W',"Trp"},{'Y',"Tyr"},{'V',"Val"},{'*',"Ter"}};

	//check
	if (!dictionary.contains(one_letter_code)) THROW(ProgrammingException, "Invalid AA one-letter code: '" + QString(one_letter_code) + "'");

	//return
	return dictionary[one_letter_code];
}

char NGSHelper::oneLetterCode(const QByteArray& aa_tree_letter_code)
{
	//init
	const static QHash<QByteArray,char> dictionary = { {"Ala",'A'},{"Arg",'R'},{"Asn",'N'},{"Asp",'D'},{"Cys",'C'},{"Glu",'E'}, {"Gln",'Q'},{"Gly",'G'},
														{"His",'H'},{"Ile",'I'},{"Leu",'L'},{"Lys",'K'},{"Met",'M'},{"Phe",'F'},{"Pro",'P'},{"Ser",'S'},
														{"Thr",'T'},{"Trp",'W'},{"Tyr",'Y'},{"Val",'V'},{"*",'*'},{"Ter",'*'}};

	//check
	if (!dictionary.contains(aa_tree_letter_code)) THROW(ProgrammingException, "Invalid AA three-letter code: '" + aa_tree_letter_code + "'");

	//return
	return dictionary[aa_tree_letter_code];
}

const BedFile& NGSHelper::pseudoAutosomalRegion()
{
	static BedFile output;

	//init - taken from https://www.ncbi.nlm.nih.gov/grc/human
	if (output.isEmpty())
	{
		output.append(BedLine("chrX", 10001, 2781479));
		output.append(BedLine("chrX", 155701383, 156030895));
		output.append(BedLine("chrY", 10001, 2781479));
		output.append(BedLine("chrY", 56887903, 57217415));
	}

	return output;
}

QByteArray NGSHelper::cytoBand(Chromosome chr, int pos)
{
	BedFile bands;
	bands.load(":/Resources/hg38_cyto_band.bed");

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
	BedFile bands;
	bands.load(":/Resources/hg38_cyto_band.bed");

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
			QStringList parts = line.split('\t');
			if (parts.count()==3)
			{
				QByteArray gene = parts[0].toUtf8().trimmed();
				QByteArray expressed_allele = parts[1].toUtf8().trimmed();
				QByteArray status = parts[2].toUtf8().trimmed();
				output[gene] = ImprintingInfo{expressed_allele, status};
			}
		}
	}

	return output;
}

void NGSHelper::parseRegion(const QString& text, Chromosome& chr, int& start, int& end, bool allow_chr_only)
{
	//split
	QString simplyfied = text;
	simplyfied.replace("-", " ");
	simplyfied.replace(":", " ");
	simplyfied.replace(",", "");
	simplyfied = simplyfied.trimmed();
    QStringList parts = simplyfied.split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);

	//support for chrosomome only
	if (allow_chr_only && parts.count()==1 && Chromosome(simplyfied).isNonSpecial())
	{
		parts.clear();
		parts << simplyfied;
		parts << "1";
		parts << "999999999";
	}

	//not three parts
	if (parts.count()!=3)
	{
		THROW(ArgumentException, "Could not split chromosomal range '" + text + "' in three parts: " + QString::number(parts.count()) + " parts found.");
	}

	//set output
	chr = Chromosome(parts[0]);
	if (!chr.isValid()) THROW(ArgumentException, "Invalid chromosome given in chromosomal range '" + text + "': " + parts[0]);
	start = Helper::toInt(parts[1], "Start coordinate", text);
	end = Helper::toInt(parts[2], "End coordinate", text);
}

void NGSHelper::parseRegion(const QString& text, Chromosome& chr, QByteArray& start, QByteArray& end, bool allow_chr_only)
{
	int i_start, i_end;
	parseRegion(text, chr, i_start, i_end, allow_chr_only);
	start = QByteArray::number(i_start);
	end = QByteArray::number(i_end);
}

const BedFile& NGSHelper::centromeres()
{
	static BedFile output;

	//init
	if (output.isEmpty())
	{
		QList<BedLine> coords2 = {
			BedLine("chr1", 121700000, 125100000), BedLine("chr2", 91800000, 96000000), BedLine("chr3", 87800000, 94000000), BedLine("chr4", 48200000, 51800000),
			BedLine("chr5", 46100000, 51400000), BedLine("chr6", 58500000, 62600000), BedLine("chr7", 58100000, 62100000), BedLine("chr8", 43200000, 47200000),
			BedLine("chr9", 42200000, 45500000), BedLine("chr10", 38000000, 41600000), BedLine("chr11", 51000000, 55800000), BedLine("chr12", 33200000, 37800000),
			BedLine("chr13", 16500000, 18900000), BedLine("chr14", 16100000, 18200000), BedLine("chr15", 17500000, 20500000), BedLine("chr16", 35300000, 38400000),
			BedLine("chr17", 22700000, 27400000), BedLine("chr18", 15400000, 21500000), BedLine("chr19", 24200000, 28100000), BedLine("chr20", 25700000, 30400000),
			BedLine("chr21", 10900000, 13000000), BedLine("chr22", 13700000, 17400000),	BedLine("chrX", 58100000, 63800000), BedLine("chrY", 10300000, 10600000)
		};
		foreach(const BedLine& bed_line, coords2)
		{
			output.append(bed_line);
		}
	}

	return output;
}

const BedFile& NGSHelper::telomeres()
{
	static BedFile output;

	//init
	if (output.isEmpty())
	{
		QList<BedLine> coords2 = {
			BedLine("chr1", 1, 10000), BedLine("chr1", 248946422, 248956422), BedLine("chr2", 1, 10000), BedLine("chr2", 242183529, 242193529),
			BedLine("chr3", 1, 10000), BedLine("chr3", 198285559, 198295559), BedLine("chr4", 1, 10000), BedLine("chr4", 190204555, 190214555),
			BedLine("chr5", 1, 10000), BedLine("chr5", 181528259, 181538259), BedLine("chr6", 1, 10000), BedLine("chr6", 170795979, 170805979),
			BedLine("chr7", 1, 10000), BedLine("chr7", 159335973, 159345973), BedLine("chr8", 1, 10000), BedLine("chr8", 145128636, 145138636),
			BedLine("chr9", 1, 10000), BedLine("chr9", 138384717, 138394717), BedLine("chr10", 1, 10000), BedLine("chr10", 133787422, 133797422),
			BedLine("chr11", 1, 10000), BedLine("chr11", 135076622, 135086622), BedLine("chr12", 1, 10000), BedLine("chr12", 133265309, 133275309),
			BedLine("chr13", 1, 10000), BedLine("chr13", 114354328, 114364328), BedLine("chr14", 1, 10000), BedLine("chr14", 107033718, 107043718),
			BedLine("chr15", 1, 10000), BedLine("chr15", 101981189, 101991189), BedLine("chr16", 1, 10000), BedLine("chr16", 90328345, 90338345),
			BedLine("chr17", 1, 10000), BedLine("chr17", 83247441, 83257441), BedLine("chr18", 1, 10000), BedLine("chr18", 80363285, 80373285),
			BedLine("chr19", 1, 10000), BedLine("chr19", 58607616, 58617616), BedLine("chr20", 1, 10000), BedLine("chr20", 64434167, 64444167),
			BedLine("chr21", 1, 10000), BedLine("chr21", 46699983, 46709983), BedLine("chr22", 1, 10000), BedLine("chr22", 50808468, 50818468),
			BedLine("chrX", 1, 10000), BedLine("chrX", 156030895, 156040895), BedLine("chrY", 1, 10000), BedLine("chrY", 57217415, 57227415)
		};
		foreach(const BedLine& bed_line, coords2)
		{
			output.append(bed_line);
		}
	}

	return output;
}

QString NGSHelper::populationCodeToHumanReadable(QString code)
{
	if (code=="AFR") return "African";
	else if (code=="EAS") return "East asian";
	else if (code=="EUR") return "European";
	else if (code=="SAS") return "South asian";
	else if (code=="ADMIXED/UNKNOWN") return "Admixed/Unknown";
	else if (code=="") return "";
	else THROW(ProgrammingException, "Unknown population code '" + code + "'!");
}

void NGSHelper::softClipAlignment(BamAlignment& al, int start_ref_pos, int end_ref_pos)
{
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


	//generate CIGAR char matrix from CIGAR
	QList<QPair<int,int>> matrix;
	CigarData old_CIGAR = al.cigarData();
	for(uint32_t i=0; i<old_CIGAR.size(); ++i)
	{
		uint32_t op = old_CIGAR.opType(i);
		if(op!=BAM_CDEL && op!=BAM_CSOFT_CLIP && op!=BAM_CMATCH && op!=BAM_CINS && op!=BAM_CHARD_CLIP) THROW(ToolFailedException, "Unsupported CIGAR type '" + QString(old_CIGAR.opTypeAsChar(i)) + "'");

		for(uint32_t j=0; j< old_CIGAR.opLength(i); ++j)
		{
			matrix.append(qMakePair(op, op));
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

const QMap<QByteArray, QByteArrayList>& NGSHelper::transcriptMatches()
{
	static QMap<QByteArray, QByteArrayList> output;

	if (output.isEmpty())
	{
		QStringList lines = Helper::loadTextFile(":/Resources/hg38_ensembl_transcript_matches.tsv", true, '#', true);
		foreach(const QString& line, lines)
		{
			QByteArrayList parts = line.toUtf8().split('\t');
			if (parts.count()>=2)
			{
				QByteArray enst = parts[0];
				QByteArray other = parts[1];
				output[enst] << other;
				output[other] << enst;
			}
		}
	}

	return output;
}

MaxEntScanImpact NGSHelper::maxEntScanImpact(const QByteArrayList& score_pairs, QByteArray& score_pairs_with_impact, bool splice_site_only)
{
	if (score_pairs.count()<1) THROW(ArgumentException, "MaxEntScan annotation contains less than one score pair");
	if (score_pairs.count()>3) THROW(ArgumentException, "MaxEntScan annotation contains more than three score pairs");

	QList<MaxEntScanImpact> impacts;
	QByteArrayList score_pairs_new;

	for (int i=0; i<score_pairs.count(); ++i)
	{
		const QByteArray& score_pair = score_pairs[i];

		//no data - this may happen e.g. for intronic variants where the first prediction is not available
		QByteArrayList parts = score_pair.split('>');
		if (parts.count()!=2)
		{
			score_pairs_new << "-";
			continue;
		}

		if (splice_site_only && i>0)
		{
			score_pairs_new << score_pair;
			continue;
		}

		//convert numbers
		bool ok1 = false;
		double ref = parts[0].toDouble(&ok1);
		if (ref<0) ref = 0;
		bool ok2 = false;
		double alt = parts[1].toDouble(&ok2);
		if (alt<0) alt = 0;
		if (!ok1 || !ok2) THROW(ArgumentException, "MaxEntScan annotation contains invalid number: " + score_pair);
		double diff = ref - alt;

		//first score pair (native splice site) - impact implement similar to in https://doi.org/10.1093/bioinformatics/bty960
		if (i==0)
		{
			bool low_impact = true;
			if (diff>0 && ref>=3)
			{
				if (alt<6.2)
				{
					if (diff>=1.15)
					{
						impacts << MaxEntScanImpact::HIGH;
						score_pairs_new << score_pair+"(HIGH)";
						low_impact = false;
					}
					else
					{
						impacts << MaxEntScanImpact::MODERATE;
						score_pairs_new << score_pair+"(MODERATE)";
						low_impact = false;
					}
				}
				else if (alt<=8.5)
				{
					if (diff>1.15)
					{
						impacts << MaxEntScanImpact::MODERATE;
						score_pairs_new << score_pair+"(MODERATE)";
						low_impact = false;
					}
				}
			}

			if (low_impact)
			{
				score_pairs_new << score_pair;
			}
		}

		//second/third score pair (de-novo gain of splice acceptor/donor)
		else
		{
			bool low_impact = true;
			if (diff<-1.15 && ref<3)
			{
				if (alt>8.5)
				{
					impacts << MaxEntScanImpact::HIGH;
					score_pairs_new << score_pair+"(HIGH)";
					low_impact = false;
				}
				else if (alt>=6.2)
				{
					impacts << MaxEntScanImpact::MODERATE;
					score_pairs_new << score_pair+"(MODERATE)";
					low_impact = false;
				}
			}

			if (low_impact)
			{
				score_pairs_new << score_pair;
			}
		}
	}

	//write
	score_pairs_with_impact = score_pairs_new.join(" / ");

	//output
	if (impacts.contains(MaxEntScanImpact::HIGH)) return MaxEntScanImpact::HIGH;
	if (impacts.contains(MaxEntScanImpact::MODERATE)) return MaxEntScanImpact::MODERATE;
	return MaxEntScanImpact::LOW;
}

double NGSHelper::maxSpliceAiScore(QString annotation_string, QString* tooltip)
{
	annotation_string = annotation_string.trimmed();

	//support for legacy format (maximum score per variant only)
	if (annotation_string.isEmpty()) return -1;
	bool ok = false;
	double max_score = annotation_string.toDouble(&ok);
	if (ok) return max_score;

	//new format - comma-speparated list of predictions, e.g. BABAM1|0.03|0.00|0.01|0.00|-2|2|41|2,CTD-2278I10.6|0.03|0.00|0.01|0.00|-2|2|41|2 (GENE|DS_AG|DS_AL|DS_DG|DS_DL|DP_AG|DP_AL|DP_DG|DP_DL)
	max_score = -1.0;
	QStringList tooltip_lines;
	QStringList entries = annotation_string.split(',');
	foreach(QString entry, entries)
	{
		QStringList parts = entry.split('|');
		if (parts.count()!=9)
		{
			Log::warn("Invalid SpliceAI annotation (not 9 fields): " + entry);
			continue;
		}

		//determine maximum score
		for (int i=1; i<5; ++i)
		{
			QString score = parts[i];
			if (score==".") continue;
			bool ok = false;
			double score_val = score.toDouble(&ok);
			if (!ok || score_val<0 || score_val>1)
			{
				Log::warn("Invalid SpliceAI score in field with index " + QString::number(i) + ": " + entry);
				continue;
			}
			max_score = std::max(score_val, max_score);
		}

		//format tooltip
		if (tooltip!=nullptr)
		{
			QString gene = parts[0];
			tooltip_lines <<  gene + " acceptor gain: " + parts[1] + " (" + parts[5] + ")";
			tooltip_lines <<  gene + " acceptor loss: " + parts[2] + " (" + parts[6] + ")";
			tooltip_lines <<  gene + " donor gain: " + parts[3] + " (" + parts[7] + ")";
			tooltip_lines <<  gene + " donor loss: " + parts[4] + " (" + parts[8] + ")";
		}
	}

	if (tooltip!=nullptr)
	{
		*tooltip = tooltip_lines.join("<br>");
	}

	return max_score;
}

QHash<Chromosome, QString> NGSHelper::chromosomeMapping()
{
	QHash<Chromosome, QString> output;

	output.insert("chr1", "NC_000001.11");
	output.insert("chr2", "NC_000002.12");
	output.insert("chr3", "NC_000003.12");
	output.insert("chr4", "NC_000004.12");
	output.insert("chr5", "NC_000005.10");
	output.insert("chr6", "NC_000006.12");
	output.insert("chr7", "NC_000007.14");
	output.insert("chr8", "NC_000008.11");
	output.insert("chr9", "NC_000009.12");
	output.insert("chr10", "NC_000010.11");
	output.insert("chr11", "NC_000011.10");
	output.insert("chr12", "NC_000012.12");
	output.insert("chr13", "NC_000013.11");
	output.insert("chr14", "NC_000014.9");
	output.insert("chr15", "NC_000015.10");
	output.insert("chr16", "NC_000016.10");
	output.insert("chr17", "NC_000017.11");
	output.insert("chr18", "NC_000018.10");
	output.insert("chr19", "NC_000019.10");
	output.insert("chr20", "NC_000020.11");
	output.insert("chr21", "NC_000021.9");
	output.insert("chr22", "NC_000022.11");
	output.insert("chrX", "NC_000023.11");
	output.insert("chrY", "NC_000024.10");
	output.insert("chrMT", "NC_012920.1");

	return output;
}

double NGSHelper::supportReadAf(const BedpeFile &svs, int sv_index, QByteArray sample, QByteArray read_type)
{
	if (read_type!="SR" && read_type!="PR") THROW(ArgumentException, "Invalid read type '"+read_type+"' given in NGSHelper::supportReadAf()");

	try
	{
		//get sample data
		int i_format = svs.annotationIndexByName("FORMAT");
		int i_sample = svs.annotationIndexByName(sample);
		QByteArrayList values = svs[sv_index].getSampleFormatData(i_format, i_sample, read_type).split(',');
		if(values.count()!=2) THROW(ArgumentException, "Value of '"+read_type+"' could not be split in two parts!");

		//get counts
		int count_ref = Helper::toInt(values[0], "ref count");
		int count_alt = Helper::toInt(values[1], "alt count");
		if(count_alt+count_ref==0) return 0;

		return (double)count_alt / (count_alt+count_ref);
	}
	catch (Exception& e)
	{
		return -1;
	}
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
		if (info.name==id)
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
	QList<int> output;
	foreach(const SampleInfo& info, *this)
	{
		if (affected==info.isAffected())
		{
			output << info.column_index;
		}
	}

	return output;
}

QSet<QString> SampleHeaderInfo::sampleNames() const
{
	QSet<QString> output;
	foreach(const SampleInfo& info, *this)
	{
		output << info.name;
	}

	return output;
}
