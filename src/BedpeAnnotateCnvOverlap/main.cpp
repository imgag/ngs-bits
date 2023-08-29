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
		setDescription("Annotates a SV file with (high-quality) CNV overlap of a given file.");
		addInfile("in", "Input SV file (in BEDPE format).", false);
		addOutfile("out", "Output SV file (in BEDPE format).", false);
		addInfile("cnv", "Input CNV file (in TSV format).", false);

		changeLog(2020, 6, 2, "Initial version of the tool.");
		changeLog(2020, 6, 18, "Changed tool from reporting to annotation.");
		changeLog(2020, 6, 19, "Added CNV quality filter (loglikelihood).");
	}

	virtual void main()
	{
		//load CNVs
		CnvList cnvs;
		cnvs.load(getInfile("cnv"));

		//load SVs
		BedpeFile svs;
		svs.load(getInfile("in"));

		// get colum index and update header
		int overlap_idx = svs.annotationIndexByName("CNV_OVERLAP", false);

		if(overlap_idx < 0)
		{
			//no previous annotation found -> update header
			QList<QByteArray> annotation_headers = svs.annotationHeaders();
			annotation_headers.append("CNV_OVERLAP");
			svs.setAnnotationHeaders(annotation_headers);
		}


		//split CNV list in deletions and duplications
		CnvList cnvs_del;
		CnvList cnvs_dup;
		int ll_idx = cnvs.annotationIndexByName("loglikelihood", true);

		for (int i = 0; i < cnvs.count(); ++i)
		{
			//skip low-quality CNVs
			if (cnvs[i].annotations()[ll_idx].toDouble() < 20.0) continue;

			// divide list by copy-number
			if (cnvs[i].copyNumber(cnvs.annotationHeaders()) < 2) cnvs_del.append(cnvs[i]);
			else if (cnvs[i].copyNumber(cnvs.annotationHeaders()) >= 2) cnvs_dup.append(cnvs[i]);
		}


		for (int i = 0; i < svs.count(); ++i)
		{
			QByteArray overlap_string;
			if ((svs[i].type() == StructuralVariantType::DEL) || (svs[i].type() == StructuralVariantType::DUP))
			{
				// choose the correct cnv list based on SV type
				CnvList* current_cnv_list;
				if (svs[i].type() == StructuralVariantType::DEL) current_cnv_list = &cnvs_del;
				else current_cnv_list = &cnvs_dup;

				// search for overlapping deletions/duplications
				BedFile overlapping_cnvs;
				BedLine sv_region = svs[i].affectedRegion()[0];

				for (int j = 0; j < current_cnv_list->count(); j++)
				{
					CopyNumberVariant cnv = current_cnv_list->operator [](j);
					if (sv_region.overlapsWith(cnv.chr(), cnv.start(), cnv.end()))
					{
						overlapping_cnvs.append(BedLine(cnv.chr(), cnv.start(), cnv.end()));
					}
				}

				// compute overlap
				overlapping_cnvs.sort();
				overlapping_cnvs.merge();

				//intersect with sv
				overlapping_cnvs.intersect(BedFile(sv_region.chr(), sv_region.start(), sv_region.end()));
				int overlap = overlapping_cnvs.baseCount();
				overlap_string = QByteArray::number((float) overlap / sv_region.length());
			}

			//update/add annotation
			QList<QByteArray> annotations = svs[i].annotations();
			if (overlap_idx < 0)
			{
				annotations.append(overlap_string);
			}
			else
			{
				annotations[overlap_idx] = overlap_string;
			}
			svs[i].setAnnotations(annotations);
		}

		// write annotated BEDPE file to disk
		svs.toTSV(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
