#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "BasicStatistics.h"
#include "Helper.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include "NGSHelper.h"

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
		setDescription("Calculates the differences/overlap between variant lists.");
		addInfile("in1", "Input variant list in TSV format.", false, true);
		addInfile("in2", "Input variant list in TSV format.", false, true);
		//optional
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addInt("window", "Window to consider around indel positions to compensate for differing alignments.", true, 100);
		addFlag("ei", "Exact indel matches only. If unset, all indels in the window are considered matches.");
		addFlag("sm", "Also show matches. If unset, matching variants are not printed.");
	}

	int appendStrippedVariants(VariantList& vl, QString filename, QByteArray source)
	{
		VariantList tmp;
		tmp.load(filename);
		int idx_q = tmp.annotationIndexByName("quality", true, false);
		int idx_g = tmp.annotationIndexByName("genotype", true, false);
		for (int i=0; i<tmp.count(); ++i)
		{
			Variant v = tmp[i];
			v.annotations().clear();
			v.annotations().append(source);
			v.annotations().append("");
			if (idx_q==-1)
			{
				v.annotations().append("na");
			}
			else
			{
				v.annotations().append(tmp[i].annotations()[idx_q]);
			}
			if (idx_g==-1)
			{
				v.annotations().append("na");
			}
			else
			{
				v.annotations().append(tmp[i].annotations()[idx_g]);
			}
			vl.append(v);
		}

		return tmp.count();
	}

	virtual void main()
	{
		//init
		bool sm = getFlag("sm");
		bool ei = getFlag("ei");
		QString in1 = getInfile("in1");
		QString in2 = getInfile("in2");

		//merge input files
		VariantList vl;
		vl.annotations().append(VariantAnnotationHeader("source"));
		vl.annotationDescriptions().append(VariantAnnotationDescription("source", "Source sample."));
		vl.annotations().append(VariantAnnotationHeader("match"));
		vl.annotationDescriptions().append(VariantAnnotationDescription("match", "Match type, exact or fuzzy."));
		vl.annotations().append(VariantAnnotationHeader("quality"));
		vl.annotationDescriptions().append(VariantAnnotationDescription("quality", "Variant quality (if available)."));
		vl.annotations().append(VariantAnnotationHeader("genotype"));
		vl.annotationDescriptions().append(VariantAnnotationDescription("genotype", "Genotype (if available)."));
		int c1 = appendStrippedVariants(vl, in1, "in1");
		int c2 = appendStrippedVariants(vl, in2, "in2");

		//sort by chr/start/end/ref/obs/source
		vl.sortCustom([](const Variant& a, const Variant& b)
						{
							if (a.chr()<b.chr()) return true;
							if (a.chr()>b.chr()) return false;
							if (a.start()<b.start()) return true;
							if (a.start()>b.start()) return false;
							if (a.end()<b.end()) return true;
							if (a.end()>b.end()) return false;
							if (a.ref()<b.ref()) return true;
							if (a.ref()>b.ref()) return false;
							if (a.obs()<b.obs()) return true;
							if (a.obs()>b.obs()) return false;
							return a.annotations()[0] < b.annotations()[0];
						}
					);

		//flag matches
		ChromosomalIndex<VariantList> file_idx(vl);
		for (int i=0; i<vl.count(); ++i)
		{
			Variant& v1 = vl[i];

			//skip exact matches we already found
			if (v1.annotations()[1]=="=") continue;

			//indel => fuzzy position search
			int start = v1.start();
			int end = v1.end();
			if (!v1.isSNV())
			{
				start -= getInt("window");
				end += getInt("window");
			}

			QVector<int> matches = file_idx.matchingIndices(v1.chr(), start, end);
			foreach(int index, matches)
			{
				Variant& v2 = vl[index];

				if (v2.annotations()[0]==v1.annotations()[0]) continue;

				//check if genotypes match
				QByteArray geno_match = "=";
				if (v2.annotations()[3]!=v1.annotations()[3] && v2.annotations()[3]!="" && v1.annotations()[3]!="") geno_match = "g";

				//exact match (SNP and indel)
				if (v1.ref()==v2.ref() && v1.obs()==v2.obs())
				{
					v1.annotations()[1] = geno_match;
					v2.annotations()[1] = geno_match;
				}
				//non-exact indel matches
				else if (!ei && !v1.isSNV() && !v2.isSNV())
				{
					v1.annotations()[1] = "=";
					v2.annotations()[1] = "=";
				}
			}
		}

		//output
		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream out(outfile.data());
		out << "#change\tchr\tstart\tend\tref\tobs\tgenotype\tquality" << endl;
		int u1 = 0;
		int u2 = 0;
		int g = 0;
		for (int i=0; i<vl.count(); ++i)
		{
			Variant& v1 = vl[i];

			QString prefix="";
			if (v1.annotations()[1]=="=")
			{
				if (!sm) continue;
			}
			else if (v1.annotations()[1]=="g")
			{
				++g;
				if (v1.annotations()[0]=="in1")
				{
					prefix = "-g";
				}
				else if (v1.annotations()[0]=="in2")
				{
					prefix = "+g";
				}
			}
			else if (v1.annotations()[0]=="in1")
			{
				prefix = "-";
				++u1;
			}
			else if (v1.annotations()[0]=="in2")
			{
				prefix = "+";
				++u2;
			}
			out << prefix << "\t" << v1.chr().str() << "\t" << v1.start() << "\t" << v1.end() << "\t" << v1.ref() << "\t" << v1.obs() << "\t" << v1.annotations()[3] << "\t" << v1.annotations()[2] << endl;
		}

		//output to console
		QTextStream out2(stdout);
		out2 <<"#" << endl;
		out2 <<"#in1   : " << in1 << endl;
		out2 <<"#count : " << QString::number(c1) << endl;
		out2 <<"#unique: " << QString::number(u1) << " (" << QString::number(100.0 * u1 / c1, 'f', 2) << "%)" << endl;
		out2 <<"#geno  : " << QString::number(g/2) << " (" << QString::number(100.0 * g / c1, 'f', 2) << "%)" << endl;
		out2 <<"#match : " << QString::number(c1-u1) << " (" << QString::number(100.0 * (c1-u1) / c1, 'f', 2) << "%)" << endl;
		out2 <<"#" << endl;
		out2 <<"#in2   : " << in2 << endl;
		out2 <<"#count : " << QString::number(c2) << endl;
		out2 <<"#unique: " << QString::number(u2) << " (" << QString::number(100.0 * u2 / c2, 'f', 2) << "%)" << endl;
		out2 <<"#geno  : " << QString::number(g/2) << " (" << QString::number(100.0 * g / c2, 'f', 2) << "%)" << endl;
		out2 <<"#match : " << QString::number(c2-u2) << " (" << QString::number(100.0 * (c2-u2) / c2, 'f', 2) << "%)" << endl;

		//exit with error code
		if (u1!=0 || u2!=0 || g!=0)
		{
			THROW(ToolFailedException, "Detected differences in input files!");
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
