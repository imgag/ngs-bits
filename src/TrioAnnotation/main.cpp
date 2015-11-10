#include "BedFile.h"
#include "ToolBase.h"
#include "NGSHelper.h"
#include "BasicStatistics.h"
#include "api/BamReader.h"
#include "FastaFileIndex.h"
#include "Helper.h"
#include <math.h>
#include <QTextStream>
#include <QSet>

using namespace BamTools;

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	virtual void setup()
	{
		setDescription("Annotates a child sample with trio information. Assumes that the child is the index patient and that the parents are unaffected.");
		addInfile("in", "Input variant list of child in TSV format.", false, true);
		addOutfile("out", "Output file in TSV format.", false, true);
		//optional
		addInt("min_depth", "Minimum depth in all three samples.", true, 10);
		addFloat("max_maf", "Maximum minor allele frequency in 1000G,ExAC and ESP6500EA database annotation.", true, 0.01);
		addInt("max_ngsd", "Maximum homozygous occurances in NGSD.", true, 30);
	}

	QStringList splitGenes(QString genes)
	{
		genes.replace(',', ';');
		genes.replace(' ', ';');

		QStringList output;
		QStringList gene_list = genes.split(';');
		foreach(QString gene, gene_list)
		{
			gene = gene.trimmed();
			if (gene.isEmpty()) continue;
			output.append(gene);
		}

		return output;
	}

	enum Genotype
	{
		WT=0,
		HET=1,
		HOM=2
	};

	struct Genos
	{
		Genos()
			: f(WT)
			, m(WT)
			, c(WT)
		{
		}

		Genotype f;
		Genotype m;
		Genotype c;

		double f_af;
		double m_af;
		double c_af;
	};

	double mendelianErrorCount(const QMap<int, Genos>& trio_genos)
	{
		int me = 0;

		QMap<int, Genos>::const_iterator it = trio_genos.begin();
		while(it!=trio_genos.end())
		{
			//hom, hom => het/wt
			if (it.value().f==HOM && it.value().m==HOM && it.value().c!=HOM) me += 1;
			//hom, het => wt
			if (it.value().f==HOM && it.value().m==HET && it.value().c==WT) me += 1;
			if (it.value().f==HET && it.value().m==HOM && it.value().c==WT) me += 1;
			//het, wt  => hom
			if (it.value().f==HET && it.value().m==WT && it.value().c==HOM) me += 1;
			if (it.value().f==WT && it.value().m==HET && it.value().c==HOM) me += 1;
			//wt, wt  => het/hom
			if (it.value().f==WT && it.value().m==WT && it.value().c!=WT) me += 1;

			++it;
		}

		return me;
	}

	Genotype str2geno(const QByteArray& genotype)
	{
		if(genotype=="WT") return WT;
		else if(genotype=="HET") return HET;
		else if(genotype=="HOM") return HOM;
		else THROW(ArgumentException, "Cannot convert genotype '" + genotype + "' to enum value!")
	}

	virtual void main()
	{
		int min_depth = getInt("min_depth");
		double max_maf = getFloat("max_maf");
		int max_ngsd = getInt("max_ngsd");

		//load variant list
		VariantList vl;
		vl.load(getInfile("in"));
		QTextStream out(stdout);
		out << "Variants overall: " << vl.count() << endl;

		//get indices
		int i_quality = vl.annotationIndexByName("quality", true, true);
		int i_gene = vl.annotationIndexByName("gene", true, true);
		int i_co_sp = vl.annotationIndexByName("coding_and_splicing", true, true);
		int i_1000g = vl.annotationIndexByName("1000g", true, true);
		int i_exac = vl.annotationIndexByName("ExAC", true, true);
		int i_esp = vl.annotationIndexByName("ESP6500EA", true, true);
		int i_class = vl.annotationIndexByName("classification", true, true);
		int i_hom = vl.annotationIndexByName("ihdb_allsys_hom", true, true);
		int i_het = vl.annotationIndexByName("ihdb_allsys_het", true, true);

		//open BAM reads
		QMap<int, QList<QByteArray> > trio_col;
		QMap<int, Genos> trio_genos;

		//find VUS>2 or rare variants
		QSet<int> rare;
		for (int i=0; i<vl.count(); ++i)
		{
			const Variant& v = vl[i];

			//mark VUS>2 variants
			if (v.annotations()[i_class].toInt()>2)
			{
				trio_col[i].append("VUS"+v.annotations()[i_class]);
				rare.insert(i);
			}

			//filter out common variants (public databases)
			if (v.annotations()[i_1000g].toDouble()>max_maf) continue;
			if (v.annotations()[i_exac].toDouble()>max_maf) continue;
			if (v.annotations()[i_esp].toDouble()>max_maf) continue;

			//filter out common variants (in-house database)
			if (v.annotations()[i_hom].toInt()>max_ngsd) continue;

			//filter out gonosomal variants
			if (v.chr().isGonosome()) continue;

			rare.insert(i);
		}
		out << "Variants rare and autosomal: " << rare.count() << endl;

		//check depth
		foreach(int i, rare)
		{
			const Variant& v = vl[i];
			QList<QByteArray> quality_entries = v.annotations()[i_quality].split(';');
			foreach(const QByteArray& entry, quality_entries)
			{
				if (entry.startsWith("TRIO="))
				{
					QList<QByteArray> trio_entries = entry.mid(5).split(',');
					if (trio_entries.count()!=9) THROW(ProgrammingException, "Trio column has more/less than 9 entries!");
					int c_depth = Helper::toInt(trio_entries[1], "child depth");
					int m_depth = Helper::toInt(trio_entries[4], "mother depth");
					int f_depth = Helper::toInt(trio_entries[7], "father depth");
					if (c_depth>=min_depth && m_depth>=min_depth && f_depth>=min_depth)
					{
						Genos tmp;
						tmp.c = str2geno(trio_entries[0]);
						tmp.m = str2geno(trio_entries[3]);
						tmp.f = str2geno(trio_entries[6]);
						tmp.c_af = Helper::toDouble(trio_entries[2], "child AF");
						tmp.m_af = Helper::toDouble(trio_entries[5], "mother AF");
						tmp.f_af = Helper::toDouble(trio_entries[8], "father AF");
						trio_genos.insert(i, tmp);
					}
				}
			}
		}
		out << "Variants with sufficient depth: " << trio_genos.count() << endl;

		//print QC statistics
		out << endl;
		int me_count = mendelianErrorCount(trio_genos);
		out << "Mendelian errors: " << QString::number(me_count) << " of " << QString::number(trio_genos.count()) << " ~" << QString::number(100.0*me_count/trio_genos.count(), 'f', 2) << "%" << endl;
		out << endl;

		//find de-novo
		int count = 0;
		QMap<int, Genos>::const_iterator it = trio_genos.begin();
		while(it!=trio_genos.end())
		{
			//filter for genotype and for allele frequency (to get rid of artefacts)
			if (it.value().c!=WT && it.value().f==WT && it.value().f_af<0.05 && it.value().m==WT && it.value().m_af<0.05)
			{
				if (vl[it.key()].annotations()[i_het].toInt()<=max_ngsd)
				{
					trio_col[it.key()].append("DENOVO");
					count += 1;
				}
			}
			++it;
		}
		out << "De-novo variants: " << QString::number(count) << endl;

		//find recessive variants
		count = 0;
		it = trio_genos.begin();
		while(it!=trio_genos.end())
		{
			if (it.value().f==HET && it.value().m==HET && it.value().c==HOM)
			{
				trio_col[it.key()].append("RECESSIVE");
				count += 1;
			}
			++it;
		}
		out << "Recessive variants: " << QString::number(count) << endl;

		//find hemizygous variants
		count = 0;
		it = trio_genos.begin();
		while(it!=trio_genos.end())
		{
			if ((it.value().f==HET && it.value().m==WT && it.value().c==HOM)
				||
				(it.value().f==WT && it.value().m==HET && it.value().c==HOM))
			{
				trio_col[it.key()].append("HEMIZYGOUS");
				count += 1;
			}
			++it;
		}
		out << "Hemizygous variants: " << QString::number(count) << endl;

		//find genes that might have compound-heterozygous variants (one from each parent)
		QSet<QString> genes_mother;
		QSet<QString> genes_father;
		it = trio_genos.begin();
		while(it!=trio_genos.end())
		{
			//filter for LOW/MODERATE/HIGH impact - otherwise we get too many UTR and intronic variants
			QString co_sp = vl[it.key()].annotations()[i_co_sp];
			if (co_sp.contains(":HIGH:") || co_sp.contains(":MODERATE:") || co_sp.contains(":LOW:"))
			{
				if (it.value().f==WT && it.value().m==HET && it.value().c==HET)
				{
					QStringList gene_list = splitGenes(vl[it.key()].annotations()[i_gene]);
					foreach(QString gene, gene_list)
					{
						genes_mother.insert(gene);
					}
				}
				if (it.value().f==HET && it.value().m==WT && it.value().c==HET)
				{
					QStringList gene_list = splitGenes(vl[it.key()].annotations()[i_gene]);
					foreach(QString gene, gene_list)
					{
						genes_father.insert(gene);
					}
				}
			}
			++it;
		}
		QSet<QString> comp_genes = genes_mother.intersect(genes_father);

		//mark variants in compound-heterozygous genes
		count = 0;
		it = trio_genos.begin();
		while(it!=trio_genos.end())
		{
			//filter for LOW/MODERATE/HIGH impact - otherwise we get too many UTR and intronic variants
			QString co_sp = vl[it.key()].annotations()[i_co_sp];
			if (co_sp.contains(":HIGH:") || co_sp.contains(":MODERATE:") || co_sp.contains(":LOW:"))
			{
				if (it.value().f==WT && it.value().m==HET && it.value().c==HET)
				{
					QStringList gene_list = splitGenes(vl[it.key()].annotations()[i_gene]);
					foreach(QString gene, comp_genes)
					{
						if (gene_list.contains(gene))
						{
							trio_col[it.key()].append("COMPOUND_M");
							count += 1;
						}
					}
				}
				if (it.value().f==HET && it.value().m==WT && it.value().c==HET)
				{
					QStringList gene_list = splitGenes(vl[it.key()].annotations()[i_gene]);
					foreach(QString gene, comp_genes)
					{
						if (gene_list.contains(gene))
						{
							trio_col[it.key()].append("COMPOUND_F");
							count += 1;
						}
					}
				}
			}
			++it;
		}
		out << "Compound-heterozygous variants: " << QString::number(count) << endl;

		//remove old trio column if present
		int index = vl.annotationIndexByName("trio", true, false);
		if (index!=-1)
		{
			vl.removeAnnotation(index);
		}

		//store variant list with new column
		vl.annotations().append(VariantAnnotationDescription("trio", "Trio information from TrioAnnotation tool.", VariantAnnotationDescription::STRING));
		for (int i=0; i<vl.count(); ++i)
		{
			if (trio_col.contains(i))
			{
				QByteArray tmp;
				for (int t=0; t<trio_col[i].count(); ++t)
				{
					if (t!=0)
					{
						tmp += ',';
					}
					tmp += trio_col[i][t];
				}
				vl[i].annotations().append(tmp);
			}
			else
			{
				vl[i].annotations().append("");
			}
		}
		vl.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

