#include "ToolBase.h"
#include "VariantList.h"
#include "Helper.h"
#include "Exceptions.h"
#include "ChromosomalIndex.h"
#include <QTextStream>
#include <QFileInfo>
#include <QDir>

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
		setDescription("Creates a variant overview table from several samples.");
		addInfileList("in", "Input variant lists in TSV format.", false);
		addOutfile("out", "Output variant list file in TSV format.", false);
		//optional
		addInt("window", "Window to consider around indel positions to compensate for differing alignments.", true, 100);
		addString("add_cols", "Comma-separated list of input columns that shall be added to the output.", true, "");
	}

	virtual void main()
	{
		//init
		QStringList in = getInfileList("in");
		int window = getInt("window");
		QStringList cols;
		cols << "genotype" << "variant_type" << "gene" << "coding_and_splicing";
		cols << getString("add_cols").split(',', QString::SkipEmptyParts);

		//load variant lists
		QVector<VariantList> vls;
		QVector<QVector<int> > vls_anno_indices;
		for (int i=0; i<in.count(); ++i)
		{
			VariantList vl;
			vl.load(in[i], VariantList::TSV);

			//check the all required fields are present in the input file
			QVector<int> anno_indices;
			foreach(QString col, cols)
			{
				if (col=="genotype") continue;
				int index = vl.annotationIndexByName(col, true, true);
				anno_indices.append(index);
			}

			vls_anno_indices.append(anno_indices);
			vls.append(vl);
		}

		//create joined variant list of all input files (with reduced annotation set)
		VariantList vl_merged;
		foreach(QString col, cols)
		{
			if (col=="genotype") continue;
			vl_merged.annotations().append(VariantAnnotationDescription(col, ""));
		}
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
		vl_merged.removeDuplicates(true);

		//append sample columns
		for (int i=0; i<vls.count(); ++i)
		{
			//get genotype index
			int geno_index = vls[i].annotationIndexByName("genotype", true, false);
			if(geno_index==-1)	geno_index = vls[i].annotationIndexByName("tumor_maf", true, true);

			//add column header
			vl_merged.annotations().append(VariantAnnotationDescription(QFileInfo(in[i]).baseName(), ""));

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
					QVector<int> matches = cidx.matchingIndices(v.chr(), v.start()-window, v.end()+window);
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

		vl_merged.store(getOutfile("out"), VariantList::TSV);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
