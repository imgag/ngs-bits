#include <QFileInfo>
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
		addString("tid", "tumor processed sample ID (for retrieving CNVs flagged as artefact from NGSD.)", true);
		addString("nid", "Normal processed sample ID (for retrieving CNVs flagged as artefact from NGSD.)", true);
		addEnum("build", "Reference genome to be used.", true, QStringList() << "GRCh37" << "GRCh38", "GRCh37");
		addOutfile("out", "Output QCML file.", true);

		changeLog(2021, 5, 6, "Initial version of the tool to calculate HRD score.");
	}
	virtual void main()
	{
		QString in =  getInfile("in");
		QString t_ps = getString("tid");
		QString n_ps = getString("nid");
		QString build = getEnum("build");
		QString out = getOutfile("out");

		//metadata
		QList<QCValue> metadata;
		metadata << QCValue("source file", QFileInfo(in).fileName(), "", "QC:1000005");

		CnvList cnvs;
		cnvs.load(in);

		//Load somatic report from NGSD and filter out CNVs flagged as artefacts
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

		QCCollection result = Statistics::hrdScore(cnvs, build);

		QString parameters = "";
		if( !t_ps.isEmpty() ) parameters += " -tid " + t_ps;
		if( !n_ps.isEmpty() ) parameters += " -nid " + n_ps;
		parameters += " -build " + build;
		result.storeToQCML(out, QStringList(), parameters, QMap< QString, int >(), metadata);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
