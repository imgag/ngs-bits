#include "NGSHelper.h"
#include "Exceptions.h"
#include "Helper.h"
#include "BasicStatistics.h"
#include "BamReader.h"
#include "GeneSet.h"
#include "ToolBase.h"
#include "Log.h"

#include <QTextStream>
#include <QFileInfo>

#include <cmath>
#include <vector>

namespace {

	QString copyFromResource(GenomeBuild build)
	{
		//check variant list exists
		QString snp_file = ":/Resources/" + buildToString(build) + "_snps.vcf";
		if (!QFile::exists(snp_file)) THROW(ProgrammingException, "Unsupported genome build '" + buildToString(build) + "'!");

		//copy from resource file (gzopen cannot access Qt resources)
		QString tmp = Helper::tempFileNameNonRandom(buildToString(build) + "_snps.vcf");
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

VcfFile NGSHelper::getKnownVariants(GenomeBuild build, bool only_snvs, const BedFile& roi, double min_af, double max_af)
{
	//check variant list exists
	QString tmp = copyFromResource(build);

	//load
	VcfFile output;
	output.load(tmp, roi, false);

	//remove temporary file
	QFile::remove(tmp);

	//filter variants
	filterVcfFile(output, only_snvs, min_af, max_af);

	return output;
}

VcfFile NGSHelper::getKnownVariants(GenomeBuild build, bool only_snvs, double min_af, double max_af)
{
	//check variant list exists
	QString tmp = copyFromResource(build);

	//load
	VcfFile output;
	output.load(tmp, false);

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
	const static QHash<QByteArray, char> dictionary =   {{"TTT", 'F'}, {"TTC", 'F'}, {"TTA", 'L'}, {"TTG", 'L'}, {"CTT", 'L'}, {"CTC", 'L'},
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

const BedFile& NGSHelper::pseudoAutosomalRegion(GenomeBuild build)
{
	static QMap<GenomeBuild, BedFile> output;

	//init - taken from https://www.ncbi.nlm.nih.gov/grc/human
	if (output.isEmpty())
	{
		output[GenomeBuild::HG19].append(BedLine("chrX", 60001, 2699520));
		output[GenomeBuild::HG19].append(BedLine("chrX", 154931044, 155260560));
		output[GenomeBuild::HG19].append(BedLine("chrY", 10001, 2649520));
		output[GenomeBuild::HG19].append(BedLine("chrY", 59034050, 59363566));

		output[GenomeBuild::HG38].append(BedLine("chrX", 10001, 2781479));
		output[GenomeBuild::HG38].append(BedLine("chrX", 155701383, 156030895));
		output[GenomeBuild::HG38].append(BedLine("chrY", 10001, 2781479));
		output[GenomeBuild::HG38].append(BedLine("chrY", 56887903, 57217415));
	}

	return output[build];
}

QByteArray NGSHelper::cytoBand(GenomeBuild build, Chromosome chr, int pos)
{
	//init
	static BedFile bands;
	if (bands.count()==0)
	{
		bands.load(":/Resources/" + buildToString(build) + "_cyto_band.bed");
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

BedLine NGSHelper::cytoBandToRange(GenomeBuild build, QByteArray cytoband)
{
	//init
	static BedFile bands;
	if (bands.count()==0)
	{
		bands.load(":/Resources/" + buildToString(build) + "_cyto_band.bed");
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
			BedLine range1 = cytoBandToRange(build, parts[0]);
			BedLine range2 = cytoBandToRange(build, parts[1]);

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
				QByteArray gene = parts[0].toUtf8().trimmed();
				QByteArray expressed_allele = parts[1].toUtf8().trimmed();
				QByteArray status = parts[2].toUtf8().trimmed();
				output[gene] = ImprintingInfo{expressed_allele, status};
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

const BedFile& NGSHelper::centromeres(GenomeBuild build)
{
	static QMap<GenomeBuild, BedFile> output;

	//init
	if (output.isEmpty())
	{
		QList<BedLine> coords = {
			BedLine("chr1", 121535434, 124535434), BedLine("chr2", 92326171, 95326171),	BedLine("chr3", 90504854, 93504854), BedLine("chr4", 49660117, 52660117),
			BedLine("chr5", 46405641, 49405641), BedLine("chr6", 58830166, 61830166), BedLine("chr7", 58054331, 61054331), BedLine("chr8", 43838887, 46838887),
			BedLine("chr9", 47367679, 50367679), BedLine("chr10", 39254935, 42254935), BedLine("chr11", 51644205, 54644205), BedLine("chr12", 34856694, 37856694),
			BedLine("chr13", 16000000, 19000000), BedLine("chr14", 16000000, 19000000), BedLine("chr15", 17000000, 20000000), BedLine("chr16", 35335801, 38335801),
			BedLine("chr17", 22263006, 25263006), BedLine("chr18", 15460898, 18460898), BedLine("chr19", 24681782, 27681782), BedLine("chr20", 26369569, 29369569),
			BedLine("chr21", 11288129, 14288129), BedLine("chr22", 13000000, 16000000), BedLine("chrX", 58632012, 61632012), BedLine("chrY", 10104553, 13104553)
		};
		foreach(const BedLine& bed_line, coords)
		{
			output[GenomeBuild::HG19].append(bed_line);
		}

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
			output[GenomeBuild::HG38].append(bed_line);
		}
	}

	return output[build];
}

const BedFile& NGSHelper::telomeres(GenomeBuild build)
{
	static QMap<GenomeBuild, BedFile> output;

	//init
	if (output.isEmpty())
	{
		QList<BedLine> coords = {
			BedLine("chr1", 1, 10000), BedLine("chr1", 249240621, 249250621), BedLine("chr2", 1, 10000), BedLine("chr2", 243189373, 243199373),
			BedLine("chr3", 1, 10000), BedLine("chr3", 198012430, 198022430), BedLine("chr4", 1, 10000), BedLine("chr4", 191144276, 191154276),
			BedLine("chr5", 1, 10000), BedLine("chr5", 180905260, 180915260), BedLine("chr6", 1, 10000), BedLine("chr6", 171105067, 171115067),
			BedLine("chr7", 1, 10000), BedLine("chr7", 159128663, 159138663), BedLine("chr8", 1, 10000), BedLine("chr8", 146354022, 146364022),
			BedLine("chr9", 1, 10000), BedLine("chr9", 141203431, 141213431), BedLine("chr10", 1, 10000), BedLine("chr10", 135524747, 135534747),
			BedLine("chr11", 1, 10000),	BedLine("chr11", 134996516, 135006516), BedLine("chr12", 1, 10000), BedLine("chr12", 133841895, 133851895),
			BedLine("chr13", 1, 10000),	BedLine("chr13", 115159878, 115169878),	BedLine("chr14", 1, 10000), BedLine("chr14", 107339540, 107349540),
			BedLine("chr15", 1, 10000),	BedLine("chr15", 102521392, 102531392),	BedLine("chr16", 1, 10000), BedLine("chr16", 90344753, 90354753), //definition of GRCh37 does not contain telomeres for chr17!
			BedLine("chr18", 1, 10000),	BedLine("chr18", 78067248, 78077248), BedLine("chr19", 1, 10000), BedLine("chr19", 59118983, 59128983),
			BedLine("chr20", 1, 10000),	BedLine("chr20", 63015520, 63025520), BedLine("chr21", 1, 10000), BedLine("chr21", 48119895, 48129895),
			BedLine("chr22", 1, 10000),	BedLine("chr22", 51294566, 51304566), BedLine("chrX", 1, 10000), BedLine("chrX", 155260560, 155270560),
			BedLine("chrY", 1, 10000),	BedLine("chrY", 59363566, 59373566)
		};
		foreach(const BedLine& bed_line, coords)
		{
			output[GenomeBuild::HG19].append(bed_line);
		}

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
			output[GenomeBuild::HG38].append(bed_line);
		}
	}

	return output[build];
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

const QMap<QByteArray, QByteArrayList>& NGSHelper::transcriptMatches(GenomeBuild build)
{
	static QMap<GenomeBuild, QMap<QByteArray, QByteArrayList>> output;

	if (!output.contains(build))
	{
		QStringList lines = Helper::loadTextFile(":/Resources/"+buildToString(build)+"_ensembl_transcript_matches.tsv", true, '#', true);
		foreach(const QString& line, lines)
		{
			QByteArrayList parts = line.toUtf8().split('\t');
			if (parts.count()>=2)
			{
				QByteArray enst = parts[0];
				QByteArray other = parts[1];
				output[build][enst] << other;
				output[build][other] << enst;
			}
		}
	}

	return output[build];
}

//Helper struct for GFF parsing
struct TranscriptData
{
	QByteArray name;
	int version;
	QByteArray name_ccds;
	QByteArray gene_symbol;
	QByteArray gene_id;
	QByteArray hgnc_id;
	QByteArray chr;
	int start_coding = 0;
	int end_coding = 0;
	QByteArray strand;
	QByteArray biotype;

	BedFile exons;
};


QHash<QByteArray, QByteArray> parseGffAttributes(const QByteArray& attributes)
{
	QHash<QByteArray, QByteArray> output;

	QByteArrayList parts = attributes.split(';');
	foreach(const QByteArray& part, parts)
	{
		int split_index = part.indexOf('=');
		QByteArray key = part.left(split_index).trimmed();
		QByteArray value = part.mid(split_index+1).trimmed();
		output[key] = value;
	}

	return output;
}

GffData NGSHelper::loadGffFile(QString filename, GffSettings settings)
{
	GffData output;
	output.transcripts.reserve(100000);

	//init
	QHash<QByteArray, TranscriptData> transcripts;
	QHash<QByteArray, QByteArray> gene_to_hgnc;
	int c_skipped_special_chr = 0;
	QSet<QByteArray> special_chrs;
	int c_skipped_no_name_and_hgnc = 0;
	int c_skipped_not_gencode_basic = 0;

	char* buffer = new char[8192];
	gzFile gz_file = gzopen(filename.toUtf8().data(), "rb"); //read binary: always open in binary mode because windows and mac open in text mode
	if (gz_file == NULL) THROW(FileAccessException, "Could not open file '" + filename + "' for reading!");
	gzbuffer(gz_file, 131072);

	while(!gzeof(gz_file))
    {
		char* line_raw  = gzgets(gz_file, buffer, 8192);

		//handle errors like truncated GZ file
		if (line_raw==nullptr)
		{
			int error_no = Z_OK;
			QByteArray error_message = gzerror(gz_file, &error_no);
			if (error_no!=Z_OK && error_no!=Z_STREAM_END)
			{
				THROW(FileParseException, "Error while reading file '" + filename + "': " + error_message);
			}
		}

		QByteArray line(buffer);
		while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);
        if (line.isEmpty()) continue;

        //section end => commit data
        if (line=="###")
        {
            //convert from TranscriptData to Transcript and append to list
			for(auto it = transcripts.begin(); it!=transcripts.end(); ++it)
            {
				TranscriptData& t_data = it.value();
				t_data.exons.merge();

                Transcript t;
                t.setGene(t_data.gene_symbol);
                t.setGeneId(t_data.gene_id);
                t.setHgncId(t_data.hgnc_id);
                t.setName(t_data.name);
                t.setVersion(t_data.version);
                t.setNameCcds(t_data.name_ccds);
                t.setSource(Transcript::ENSEMBL);
				t.setStrand(Transcript::stringToStrand(t_data.strand));
				t.setBiotype(Transcript::stringToBiotype(t_data.biotype));
                int coding_start = t_data.start_coding;
                int coding_end = t_data.end_coding;
                if(t.strand() == Transcript::MINUS)
                {
                   int temp = coding_start;
                   coding_start = coding_end;
                   coding_end = temp;
                }
                t.setRegions(t_data.exons, coding_start, coding_end);

				output.transcripts << t;
			}

            //clear cache
            transcripts.clear();
            continue;
        }

        //skip header lines
		if (line.startsWith("#")) continue;

		int from = 0;
		QVector<int> seps;
		int sep;
		while((sep=line.indexOf('\t', from))!=-1)
		{
			seps << sep;
			from = sep+1;
		}
		QByteArray chr = line.left(seps[0]);
		QByteArray details = line.mid(seps[7]+1, line.length()-seps[7]);

        //gene line
		if (details.startsWith("ID=gene:"))
        {

			QHash<QByteArray, QByteArray> data = parseGffAttributes(details);

			const QByteArray& gene = data["Name"];

			// store mapping for pseudogene table
			const QByteArray& gene_id = data["gene_id"];
			output.ensg2symbol.insert(gene_id, gene);

			if (!Chromosome(chr).isNonSpecial())
            {
				special_chrs << chr;
				++c_skipped_special_chr;
                continue;
            }

            //extract HGNC identifier
			QByteArray hgnc_id = "";
			const QByteArray& description = data["description"];
			int start = description.indexOf("[Source:HGNC Symbol%3BAcc:");
            if (start!=-1)
            {
                start += 26;
				int end = description.indexOf("]", start);
                if (end!=-1)
                {
					hgnc_id = description.mid(start, end-start).trimmed();
                }
            }

			if (gene.isEmpty() && hgnc_id.isEmpty())
			{
				++c_skipped_no_name_and_hgnc;
				continue;
			}

			gene_to_hgnc[gene_id] = hgnc_id;
        }

        //transcript line
		else if (details.startsWith("ID=transcript:"))
        {
			QHash<QByteArray, QByteArray> data = parseGffAttributes(details);

			// store mapping for pseudogene table
			const QByteArray& transcript_id = data["transcript_id"];
			output.enst2ensg.insert(transcript_id, data["Parent"].split(':').at(1));

			// store GENCODE basic data
			bool is_gencode_basic = data.value("tag")=="basic";
			if (is_gencode_basic)
			{
				output.gencode_basic << transcript_id;
			}

			if (settings.skip_not_gencode_basic && !is_gencode_basic)
			{
				++c_skipped_not_gencode_basic;
				continue;
			}

			QByteArray parent_id = data["Parent"].split(':').at(1);

			//skip transcripts of skipped genes
			if(!gene_to_hgnc.contains(parent_id)) continue;

			TranscriptData tmp;
			tmp.name = transcript_id;
			tmp.version = Helper::toInt(data["version"], "transcript version");
			tmp.name_ccds = data.value("ccdsid");
			tmp.gene_symbol = output.ensg2symbol[parent_id];
			tmp.gene_id = parent_id;
			tmp.hgnc_id = gene_to_hgnc[parent_id];
			tmp.chr = chr;
			tmp.strand = line.mid(seps[5]+1, seps[6]-seps[5]-1);
			tmp.biotype = data["biotype"];
			transcripts[data["ID"]] = tmp;
		}

        //exon lines
		else
		{
			QByteArray type = line.mid(seps[1]+1, seps[2]-seps[1]-1);
			if (type=="CDS" || type=="exon" || type=="three_prime_UTR" || type=="five_prime_UTR" )
			{
				int enst_start = details.indexOf("Parent=")+7;
				int enst_end = details.indexOf(";", enst_start);
				if (enst_end==-1) enst_end=details.count();
				QByteArray parent_id = details.mid(enst_start, enst_end-enst_start);

				//skip exons of skipped genes
				if (!transcripts.contains(parent_id)) continue;

				TranscriptData& t_data = transcripts[parent_id];

				//check chromosome matches
				if (chr!=t_data.chr)
				{
					THROW(FileParseException, "Chromosome mismatch between transcript and exon!");
				}

				//update coding start/end
				int start = Helper::toInt(line.mid(seps[2]+1, seps[3]-seps[2]-1), "start position");
				int end = Helper::toInt(line.mid(seps[3]+1, seps[4]-seps[3]-1), "end position");

				if (type=="CDS")
				{
					t_data.start_coding = (t_data.start_coding==0) ? start : std::min(start, t_data.start_coding);
					t_data.end_coding = (t_data.end_coding==0) ? end : std::max(end, t_data.end_coding);
				}

				//add coding exon
				t_data.exons.append(BedLine(chr, start, end));
			}
		}
    }

	//text output
	if (settings.print_to_stdout)
	{
		QTextStream out(stdout);
		if (c_skipped_special_chr>0)
		{
			out << "Notice: " << QByteArray::number(c_skipped_special_chr) << " genes on special chromosomes skipped: " << special_chrs.toList().join(", ") << endl;
		}
		if (c_skipped_no_name_and_hgnc>0)
		{
			out << "Notice: " << QByteArray::number(c_skipped_no_name_and_hgnc) << " genes without symbol and HGNC-ID skipped." << endl;
		}
		if (c_skipped_not_gencode_basic>0)
		{
			out << "Notice: " << QByteArray::number(c_skipped_special_chr) << " transcipts not flagged as 'GENCODE basic'." << endl;
		}
	}

	//close buffers
	delete[] buffer;
	gzclose(gz_file);

	return output;
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

