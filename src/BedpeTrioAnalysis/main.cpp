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

		for (int i = 0; i < svs.count(); ++i)
		{
			const BedpeLine& sv_index = svs[i];
			QByteArray gt_index;
			if (sv_index.formatValueByKey("GT", svs.annotationHeaders()).trimmed() == "1/1")
			{
				gt_index = "hom";
			}
			else
			{
				gt_index = "het";
			}

			// search for SV in SVs of parents
			QByteArray gt_father;
			int i_sv_father = svs_father.findMatch(sv_index, false, false);
			if (i_sv_father != -1)
			{
				const BedpeLine& sv_father = svs_father[i];
				if (sv_father.formatValueByKey("GT", svs_father.annotationHeaders()).trimmed() == "1/1")
				{
					gt_father = "hom";
				}
				else
				{
					gt_father = "het";
				}
			}
			else
			{
				// SV not found --> wildtype
				gt_father = "wt";
			}

			QByteArray gt_mother;
			int i_sv_mother = svs_mother.findMatch(sv_index, false, false);
			if (i_sv_mother != -1)
			{
				const BedpeLine& sv_mother = svs_mother[i];
				if (sv_mother.formatValueByKey("GT", svs_mother.annotationHeaders()).trimmed() == "1/1")
				{
					gt_mother = "hom";
				}
				else
				{
					gt_mother = "het";
				}
			}
			else
			{
				// SV not found --> wildtype
				gt_mother = "wt";
			}
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
