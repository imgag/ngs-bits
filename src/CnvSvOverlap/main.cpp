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
		setDescription("Reports overlaps between a given CNV and SV file.");
		addInfile("cnv", "Input CNV file (in TSV format).", false);
		addInfile("sv", "Input SV file (in Bedpe format).", false);
		addOutfile("out", "Output txt file with the results", false);

		changeLog(2020, 6, 2, "Initial version of the tool.");
	}

	virtual void main()
	{
		QTextStream out(stdout);

		//load CNVs
		CnvList cnvs;
		cnvs.load(getInfile("cnv"));

		//load SVs
		BedpeFile svs;
		svs.load(getInfile("sv"));

		//split CNV list in deletions and duplications
		CnvList cnvs_del;
		CnvList cnvs_dup;

		for (int i = 0; i < cnvs.count(); ++i)
		{
			if (cnvs[i].copyNumber(cnvs.annotationHeaders()) < 2) cnvs_del.append(cnvs[i]);
			else if (cnvs[i].copyNumber(cnvs.annotationHeaders()) > 2) cnvs_dup.append(cnvs[i]);
		}


		for (int i = 0; i < svs.count(); ++i)
		{
			CnvList overlapping_cnvs;
			// search for overlapping deletions/duplications
			BedLine sv_region = svs[i].affectedRegion()[0];

			if (svs[i].type() == StructuralVariantType::DEL)
			{
				for (int j = 0; j < cnvs_del.count(); ++j)
				{
					if (sv_region.overlapsWith(cnvs_del[j].chr(), cnvs_del[j].start(), cnvs_del[j].end()))
					{
						float overlap = computeOverlap(sv_region.start(), sv_region.end(), cnvs_del[j].start(), cnvs_del[j].end());
						if (overlap > 0.5)
						{
							out << svs[i].toString() << ":\t\t" << cnvs_del[j].toString() << "\t" << overlap << "\n";
							overlapping_cnvs.append(cnvs_del[j]);
						}
					}
				}

			}
			else if (svs[i].type() == StructuralVariantType::DUP)
			{
				for (int j = 0; j < cnvs_dup.count(); ++j)
				{
					if (sv_region.overlapsWith(cnvs_dup[j].chr(), cnvs_dup[j].start(), cnvs_dup[j].end()))
					{
						float overlap = computeOverlap(sv_region.start(), sv_region.end(), cnvs_dup[j].start(), cnvs_dup[j].end());
						if (overlap > 0.5)
						{
							out << svs[i].toString() << ":\t\t" << cnvs_dup[j].toString() << "\t" << overlap << "\n";
							overlapping_cnvs.append(cnvs_dup[j]);
						}


					}
				}
			}
		}
	}
private:
	float computeOverlap(int start1, int end1, int start2, int end2)
	{
		int sv_cnv_overlap = std::min(end1, end2) - std::max(start1, start2);
		int sv_cnv_union = std::max(end1, end2) - std::min(start1, start2);
		return (float) sv_cnv_overlap / sv_cnv_union;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
