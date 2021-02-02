#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "FilterCascade.h"
#include "BedFile.h"
#include "Helper.h"
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
		setDescription("Combines the SV BEDPE files from a trio analysis into one file");
		addInfile("father", "Input SV file of the father (in BEDPE format).", false);
		addInfile("mother", "Input SV file of the mother (in BEDPE format).", false);
		addInfile("index", "Input SV file of the index (in BEDPE format).", false);
		addOutfile("out", "Output SV file (in BEDPE format).", false);

		changeLog(2021, 2, 1, "Initial version of the tool.");
	}

	void parseParentSVs(QMap<StructuralVariantType,BedpeFile>& sv_parent, QMap<StructuralVariantType,QMap<QByteArray,BedpeLine>>& sv_parent_lookup, QString sv_file_path)
	{
		BedpeFile svs;
		svs.load(sv_file_path);

		// init QMaps
		sv_parent.clear();
		sv_parent.insert(StructuralVariantType::BND, BedpeFile());
		sv_parent[StructuralVariantType::BND].setAnnotationHeaders(svs.annotationHeaders());
		sv_parent.insert(StructuralVariantType::DEL, BedpeFile());
		sv_parent[StructuralVariantType::DEL].setAnnotationHeaders(svs.annotationHeaders());
		sv_parent.insert(StructuralVariantType::DUP, BedpeFile());
		sv_parent[StructuralVariantType::DUP].setAnnotationHeaders(svs.annotationHeaders());
		sv_parent.insert(StructuralVariantType::INS, BedpeFile());
		sv_parent[StructuralVariantType::INS].setAnnotationHeaders(svs.annotationHeaders());
		sv_parent.insert(StructuralVariantType::INV, BedpeFile());
		sv_parent[StructuralVariantType::INV].setAnnotationHeaders(svs.annotationHeaders());
		sv_parent_lookup.clear();
		sv_parent_lookup.insert(StructuralVariantType::BND, QMap<QByteArray,BedpeLine>());
		sv_parent_lookup.insert(StructuralVariantType::DEL, QMap<QByteArray,BedpeLine>());
		sv_parent_lookup.insert(StructuralVariantType::DUP, QMap<QByteArray,BedpeLine>());
		sv_parent_lookup.insert(StructuralVariantType::INS, QMap<QByteArray,BedpeLine>());
		sv_parent_lookup.insert(StructuralVariantType::INV, QMap<QByteArray,BedpeLine>());
		for (int i = 0; i < svs.count(); ++i)
		{
			const BedpeLine& sv = svs[i];
			sv_parent[sv.type()].append(sv);
		}
	}

	virtual void main()
	{
		// load index SVs
		BedpeFile svs;
		svs.load(getInfile("index"));
		//load SVs from the parents
		BedpeFile svs_father;
		svs_father.load(getInfile("father"));
		BedpeFile svs_mother;
		svs_mother.load(getInfile("mother"));

		// compare annotation headers
		if(svs.annotationHeaders() != svs_father.annotationHeaders())
		{
			THROW(ArgumentException, "The annotations of the BEDPE files '" +  getInfile("father") + "' and '" + getInfile("index")
				  + "' do not match. Cannot combine these file! Make sure all files were created with the same pipeline verion and contain the same annotations.");
		}
		if(svs.annotationHeaders() != svs_mother.annotationHeaders())
		{
			THROW(ArgumentException, "The annotations of the BEDPE files '" +  getInfile("mother") + "' and '" + getInfile("index")
				  + "' do not match. Cannot combine these file! Make sure all files were created with the same pipeline verion and contain the same annotations.");
		}

        // extend annotation by genotype (index, father, mother)
        auto extended_annotation_headers = QList<QByteArray>() << "index" << "father" << "mother";
        extended_annotation_headers += svs.annotationHeaders();

        // parse all svs of the index
		for (int i = 0; i < svs.count(); ++i)
		{
            QList<QByteArray> genotypes;
            BedpeLine& sv_index = svs[i];
			if (sv_index.formatValueByKey("GT", svs.annotationHeaders()).trimmed() == "1/1")
			{
                 genotypes << "hom";
			}
			else
			{
                 genotypes << "het";
			}

			// search for SV in SVs of parents
			int i_sv_father = svs_father.findMatch(sv_index, false, false);
			if (i_sv_father != -1)
			{
				const BedpeLine& sv_father = svs_father[i];
				if (sv_father.formatValueByKey("GT", svs_father.annotationHeaders()).trimmed() == "1/1")
				{
                    genotypes << "hom";
				}
				else
				{
                    genotypes << "het";
				}
                // remove sv
                svs_father.removeAt(i_sv_father);
			}
			else
			{
				// SV not found --> wildtype
                genotypes << "wt";
			}

			int i_sv_mother = svs_mother.findMatch(sv_index, false, false);
			if (i_sv_mother != -1)
			{
				const BedpeLine& sv_mother = svs_mother[i];
				if (sv_mother.formatValueByKey("GT", svs_mother.annotationHeaders()).trimmed() == "1/1")
				{
                    genotypes << "hom";
				}
				else
				{
                    genotypes << "het";
				}
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
            BedpeLine sv_father = svs[i];

            genotypes << "wt"; //gt of index is wildtype

            // determine genotype of father
            if (sv_father.formatValueByKey("GT", svs_father.annotationHeaders()).trimmed() == "1/1")
            {
                 genotypes << "hom";
            }
            else
            {
                 genotypes << "het";
            }

            // try to find match in SVs of mother
            int i_sv_mother = svs_mother.findMatch(sv_father, false, false);
            if (i_sv_mother != -1)
            {
                const BedpeLine& sv_mother = svs_mother[i];
                if (sv_mother.formatValueByKey("GT", svs_mother.annotationHeaders()).trimmed() == "1/1")
                {
                    genotypes << "hom";
                }
                else
                {
                    genotypes << "het";
                }
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
            svs.append(sv_father);
        }

        // parse remaining SVs of the mother
        for (int i = 0; i < svs_mother.count(); ++i)
        {
            QList<QByteArray> genotypes;
            BedpeLine sv_mother = svs_mother[i];

            genotypes << "wt" << "wt"; //gt of index and father is wildtype

            // determine genotype of father
            if (sv_mother.formatValueByKey("GT", svs_mother.annotationHeaders()).trimmed() == "1/1")
            {
                 genotypes << "hom";
            }
            else
            {
                 genotypes << "het";
            }

            //append SV to index SV file
            sv_mother.setAnnotations(genotypes + sv_mother.annotations());
            svs.append(sv_mother);
        }

        // sort Bedpe file before writing to disk
        svs.sort();

        // write SVs to file
        svs.toTSV(getOutfile("out"));
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
