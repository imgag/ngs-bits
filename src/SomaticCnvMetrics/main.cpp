#include "ToolBase.h"
#include "NGSD.h"
#include "SomaticReportSettings.h"
#include "Statistics.h"

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
		setDescription("Calculate somatic CNV metrics based on CNV file.");
		addInfile("in", "Input somatic ClinCNV file in TSV format.", false);

		addInfile("centromeres", "BED file containing centromere coordinates", false);
		addInfile("telomeres", "Input file containing telomere coordinates.", false);
		addString("tid", "tumor processed sample ID (for retrieving CNVs flagged as artefact.)", true);
		addString("nid", "Normal processed sample ID (for retrieving CNVs flagged as artefact.)", true);
		addOutfile("out", "Output file.", true);

		changeLog(2021, 5, 6, "Initial version of the tool to calculate HRD score.");
	}
	virtual void main()
	{
		CnvList cnvs;
		cnvs.load( getInfile("in") );

		BedFile centromeres;
		centromeres.load( getInfile("centromeres") );
		BedFile telomeres;
		telomeres.load( getInfile("telomeres") );

		QString t_ps = getString("tid");
		QString n_ps = getString("nid");

		if(!t_ps.isEmpty() && !n_ps.isEmpty())
		{
			NGSD db;

			if(db.somaticReportConfigId(db.processedSampleId(t_ps), db.processedSampleId(n_ps)) != -1)
			{
				VariantList empty_var;
				QStringList messages;
				SomaticReportSettings settings;
				settings.report_config = db.somaticReportConfig(db.processedSampleId(t_ps), db.processedSampleId(n_ps), empty_var, cnvs, empty_var, messages);
				cnvs = SomaticReportSettings::filterCnvs(cnvs, settings);
			}
		}

		QCCollection result = Statistics::hrdScore(cnvs, centromeres, telomeres);

		int loh_count = result.value("QC:2000062", true).asInt();
		int tai_count = result.value("QC:2000063", true).asInt();
		int lst_count = result.value("QC:2000064", true).asInt();

		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream out(outfile.data());


		out << "#LOH\tTAI\tLST\tHRD-SCORE\n";
		out << loh_count << "\t" << tai_count << "\t" << lst_count << "\t" << loh_count+tai_count+lst_count <<endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
