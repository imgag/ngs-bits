#include "BedFile.h"
#include "ToolBase.h"
#include "NGSHelper.h"
#include "BasicStatistics.h"
#include "api/BamReader.h"
#include "FastaFileIndex.h"
#include "Helper.h"
#include <cmath>
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
		addEnum("gender", "Gender of the child.", false, QStringList() << "male" << "female");
		//optional
		addInt("min_depth", "Minimum depth in all three samples.", true, 10);
		addFloat("max_maf", "Maximum minor allele frequency in 1000G,ExAC and gnomAD database annotation.", true, 0.01);
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

	QPair<int, int> mendelianErrorCount(const QMap<int, Genos>& trio_genos, const VariantList& vl)
	{
		int me = 0;
		int all = 0;

		for (auto it = trio_genos.cbegin(); it!=trio_genos.cend(); ++it)
		{
			if (!vl[it.key()].chr().isAutosome()) continue;

			++all;

			//hom, hom => het/wt
			if (it.value().f==HOM && it.value().m==HOM && it.value().c!=HOM) me += 1;
			//hom, het => wt
			else if (it.value().f==HOM && it.value().m==HET && it.value().c==WT) me += 1;
			else if (it.value().f==HET && it.value().m==HOM && it.value().c==WT) me += 1;
			//het, wt  => hom
			else if (it.value().f==HET && it.value().m==WT && it.value().c==HOM) me += 1;
			else if (it.value().f==WT && it.value().m==HET && it.value().c==HOM) me += 1;
			//wt, wt  => het/hom
			else if (it.value().f==WT && it.value().m==WT && it.value().c!=WT) me += 1;

		}

		return qMakePair(me, all);
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
		bool child_is_male = getEnum("gender")=="male";

		//load variant list
		VariantList vl;
		vl.load(getInfile("in"));
		QTextStream out(stdout);
		out << "Variants overall: " << vl.count() << endl;

		//get indices
		int i_filter = vl.annotationIndexByName("filter", true, true);
		int i_quality = vl.annotationIndexByName("quality", true, true);
		int i_gene = vl.annotationIndexByName("gene", true, true);
		int i_co_sp = vl.annotationIndexByName("coding_and_splicing", true, true);
		int i_1000g = vl.annotationIndexByName("1000g", true, true);
		int i_exac = vl.annotationIndexByName("ExAC", true, true);
		int i_kaviar = vl.annotationIndexByName("Kaviar", true, false);
		int i_gnomad = vl.annotationIndexByName("gnomAD", true, false);
		int i_class = vl.annotationIndexByName("classification", true, true);
		int i_hom = vl.annotationIndexByName("ihdb_allsys_hom", true, true);
		int i_het = vl.annotationIndexByName("ihdb_allsys_het", true, true);

		//add filter column entry headers
		QMap<QString, QString> filters;
		filters["trio_denovo"] = "Trio analyis: Variant is de-novo in child.";
		filters["trio_recessive"] = "Trio analyis: Variant is recessively inherited from parents.";
		filters["trio_hemizygous"] = "Trio analyis: Variant is hemizygous.";
		if (child_is_male)
		{
			filters["trio_hemizygous_chrX"] = "Trio analyis: Variant is hemizygous (only for males on chrX).";
		}
		filters["trio_comp_m"] = "Trio analyis: Variant is compound-heteroygous inherited from mother.";
		filters["trio_comp_f"] = "Trio analyis:Variant is compound-heteroygous inherited from father.";
		QMapIterator<QString, QString> i(filters);
		while (i.hasNext())
		{
			i.next();
			vl.filters().insert(i.key(), i.value());
		}

		//open BAM reads
		QMap<int, QList<QByteArray> > trio_col;
		QMap<int, Genos> trio_genos;

		//find classification 3/4/5 or rare variants
		QSet<int> rare;
		for (int i=0; i<vl.count(); ++i)
		{
			const Variant& v = vl[i];

			//keep class 3/4/5 variants
			bool is_numeric = false;
			int classification = v.annotations()[i_class].toInt(&is_numeric);
			if (is_numeric && classification>2)
			{
				rare.insert(i);
				continue;
			}

			//filter out common variants (public databases)
			if (v.annotations()[i_1000g].toDouble()>max_maf) continue;
			if (v.annotations()[i_exac].toDouble()>max_maf) continue;
			if (i_kaviar!=-1 && v.annotations()[i_kaviar].toDouble()>max_maf) continue;
			if (i_gnomad!=-1 && v.annotations()[i_gnomad].toDouble()>max_maf) continue;

			//filter out common variants (NGSD)
			if (v.annotations()[i_hom].toInt()>max_ngsd) continue;

			rare.insert(i);
		}
		out << "Variants rare or class 3/4/5: " << rare.count() << endl;

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
		QPair<int, int> me_count = mendelianErrorCount(trio_genos, vl);
		out << "Mendelian errors: " << QString::number(me_count.first) << " of " << QString::number(me_count.second) << " autosomal variants ~" << QString::number(100.0*me_count.first/me_count.second, 'f', 2) << "%" << endl;
		out << endl;

		//find de-novo
		int count = 0;		
		for (auto it = trio_genos.cbegin(); it!=trio_genos.cend(); ++it)
		{
			//filter for genotype and for allele frequency (to get rid of artefacts)
			if (it.value().c!=WT && it.value().f==WT && it.value().f_af<0.05 && it.value().m==WT && it.value().m_af<0.05)
			{
				if (vl[it.key()].annotations()[i_het].toInt()<=max_ngsd)
				{
					trio_col[it.key()].append("trio_denovo");
					count += 1;
				}
			}
		}
		out << "De-novo variants: " << QString::number(count) << endl;

		//find recessive variants
		count = 0;
		for (auto it = trio_genos.cbegin(); it!=trio_genos.cend(); ++it)
		{
			if (child_is_male && !vl[it.key()].chr().isAutosome()) continue;

			if (it.value().f==HET && it.value().m==HET && it.value().c==HOM)
			{
				trio_col[it.key()].append("trio_recessive");
				count += 1;
			}
		}
		out << "Recessive variants: " << QString::number(count) << endl;

		//find hemizygous variants
		count = 0;
		for (auto it = trio_genos.cbegin(); it!=trio_genos.cend(); ++it)
		{
			if (!vl[it.key()].chr().isAutosome()) continue;

			if ((it.value().f==HET && it.value().m==WT && it.value().c==HOM)
				||
				(it.value().f==WT && it.value().m==HET && it.value().c==HOM))
			{
				trio_col[it.key()].append("trio_hemizygous");
				count += 1;
			}
		}
		out << "Hemizygous variants: " << QString::number(count) << endl;

		//find genes that might have compound-heterozygous variants (one from each parent)
		QSet<QString> genes_mother;
		QSet<QString> genes_father;
		for (auto it = trio_genos.cbegin(); it!=trio_genos.cend(); ++it)
		{
			if (!vl[it.key()].chr().isAutosome()) continue;

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
		}
		QSet<QString> comp_genes = genes_mother.intersect(genes_father);

		//mark variants in compound-heterozygous genes
		count = 0;
		for (auto it = trio_genos.cbegin(); it!=trio_genos.cend(); ++it)
		{
			if (!vl[it.key()].chr().isAutosome()) continue;

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
							trio_col[it.key()].append("trio_comp_m");
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
							trio_col[it.key()].append("trio_comp_f");
							count += 1;
						}
					}
				}
			}
		}
		out << "Compound-heterozygous variants: " << QString::number(count) << endl;

		//Mark hemizygous variants on chrX for male index patients
		if (child_is_male)
		{
			count = 0;
			for (auto it = trio_genos.cbegin(); it!=trio_genos.cend(); ++it)
			{
				if (!vl[it.key()].chr().isX()) continue;

				if (it.value().f==WT && it.value().m==HET && it.value().c==HOM)
				{
					trio_col[it.key()].append("trio_hemizygous_chrX");
					count += 1;
				}
			}
			out << "Hemizygous variants (chrX of males): " << QString::number(count) << endl;
		}

		//store variant list with trio information in 'filter' column
		for (int i=0; i<vl.count(); ++i)
		{
			if (trio_col.contains(i))
			{
				QByteArray tmp = vl[i].annotations()[i_filter].trimmed();
				for (int t=0; t<trio_col[i].count(); ++t)
				{
					QByteArray name = trio_col[i][t];
					if (!vl.filters().contains(name))
					{
						THROW(ProgrammingException, "Undeclared trio filter value '" + name + "'!");
					}
					if (!tmp.isEmpty()) tmp += ';';
					tmp += name;
				}
				vl[i].annotations()[i_filter] = tmp;
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

