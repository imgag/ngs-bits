#include "Exceptions.h"
#include "ToolBase.h"

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
		setDescription("Combines the SV BEDPE files from a trio analysis into one file");
		addInfile("father", "Input SV file of the father (in BEDPE format).", false);
		addInfile("mother", "Input SV file of the mother (in BEDPE format).", false);
		addInfile("index", "Input SV file of the index (in BEDPE format).", false);
		addOutfile("out", "Output SV file (in BEDPE format).", false);

		changeLog(2021, 2, 1, "Initial version of the tool.");
	}

	QByteArray extractGenotype(const BedpeLine& sv, const QList<QByteArray>& annotation_headers)
	{
		QByteArray genotype = sv.formatValueByKey("GT", annotation_headers, false).trimmed();
		if (genotype == "1/1")
		{
			return "hom";
		}
		else if ((genotype == "0/1") || (genotype == "1/0"))
		{
			return "het";
		}
		return "n/a";
	}

	virtual void main()
	{
		// load index SVs
		BedpeFile svs_index;
		svs_index.load(getInfile("index"));
		//load SVs from the parents
		BedpeFile svs_father;
		svs_father.load(getInfile("father"));
		BedpeFile svs_mother;
		svs_mother.load(getInfile("mother"));


		bool fuzzy_match = true;

		// compare annotation headers
		auto annotation_headers_index = svs_index.annotationHeaders();
		annotation_headers_index.removeAt(annotation_headers_index.indexOf("FORMAT") + 1);
		auto annotation_headers_father = svs_father.annotationHeaders();
		annotation_headers_father.removeAt(annotation_headers_father.indexOf("FORMAT") + 1);
		auto annotation_headers_mother = svs_index.annotationHeaders();
		annotation_headers_mother.removeAt(annotation_headers_mother.indexOf("FORMAT") + 1);

		if(annotation_headers_index != annotation_headers_father)
		{
			THROW(ArgumentException, "The annotations of the BEDPE files '" +  getInfile("father") + "' and '" + getInfile("index")
				  + "' do not match. Cannot combine these file! Make sure all files were created with the same pipeline verion and contain the same annotations.");
		}
		if(annotation_headers_index != annotation_headers_mother)
		{
			THROW(ArgumentException, "The annotations of the BEDPE files '" +  getInfile("mother") + "' and '" + getInfile("index")
				  + "' do not match. Cannot combine these file! Make sure all files were created with the same pipeline verion and contain the same annotations.");
		}

        // extend annotation by genotype (index, father, mother)
		QList<QByteArray> extended_annotation_headers = QList<QByteArray>() << "index" << "father" << "mother";
		extended_annotation_headers += svs_index.annotationHeaders();

        // parse all svs of the index
		for (int i = 0; i < svs_index.count(); ++i)
		{
			QList<QByteArray> genotypes;
			BedpeLine& sv_index = svs_index[i];
			genotypes << extractGenotype(sv_index, svs_index.annotationHeaders());


			// search for SV in SVs of parents
			int i_sv_father = svs_father.findMatch(sv_index, false, false, fuzzy_match);
			if (i_sv_father != -1)
			{
				// add genotype of father
				genotypes << extractGenotype(svs_father[i_sv_father], svs_father.annotationHeaders());

				// remove sv
				svs_father.removeAt(i_sv_father);
			}
			else
			{
				// SV not found --> wildtype
				genotypes << "wt";
			}

			int i_sv_mother = svs_mother.findMatch(sv_index, false, false, fuzzy_match);
			if (i_sv_mother != -1)
			{
				// add genotype of mother
				genotypes << extractGenotype(svs_mother[i_sv_mother], svs_mother.annotationHeaders());

				// remove sv
				svs_mother.removeAt(i_sv_mother);
			}
			else
			{
				// SV not found --> wildtype
				genotypes << "wt";
			}

			sv_index.setAnnotations(genotypes + sv_index.annotations());
		}

		// parse remaining SVs of the father
		for (int i = 0; i < svs_father.count(); ++i)
		{
			QList<QByteArray> genotypes;
			BedpeLine sv_father = svs_father[i];

			genotypes << "wt"; //gt of index is wildtype

			// add genotype of father
			genotypes << extractGenotype(svs_father[i], svs_father.annotationHeaders());

			// try to find match in SVs of mother
			int i_sv_mother = svs_mother.findMatch(sv_father, false, false, fuzzy_match);
			if (i_sv_mother != -1)
			{
				// add genotype of mother
				genotypes << extractGenotype(svs_mother[i_sv_mother], svs_mother.annotationHeaders());

				// remove sv
				svs_mother.removeAt(i_sv_mother);
			}
			else
			{
				// SV not found --> wildtype
				genotypes << "wt";
			}

			//append SV to index SV file

			sv_father.setAnnotations(genotypes + sv_father.annotations());
			svs_index.append(sv_father);
		}

		// parse remaining SVs of the mother
		for (int i = 0; i < svs_mother.count(); ++i)
		{
			QList<QByteArray> genotypes;
			BedpeLine sv_mother = svs_mother[i];

			genotypes << "wt" << "wt"; //gt of index and father is wildtype

			// add genotype of mother
			genotypes << extractGenotype(svs_mother[i], svs_mother.annotationHeaders());

			//append SV to index SV file
			sv_mother.setAnnotations(genotypes + sv_mother.annotations());
			svs_index.append(sv_mother);
		}

        // sort Bedpe file before writing to disk
		svs_index.sort();

		//write SVs to file
		svs_index.toTSV(getOutfile("out"));
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
