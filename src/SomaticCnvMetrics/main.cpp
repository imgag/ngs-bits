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
		//optoinal
		addOutfile("out", "Output qcML file. Writes to stdout otherwise.", true);
		addEnum("build", "Reference genome to be used.", true, QStringList() << "hg19" << "hg38", "hg38");
		addString("tid", "Tumor processed sample ID (for retrieving CNVs flagged as artefact from NGSD.)", true);
		addString("nid", "Normal processed sample ID (for retrieving CNVs flagged as artefact from NGSD.)", true);

		changeLog(2021, 5, 6, "Initial version of the tool to calculate HRD score.");
	}
	virtual void main()
	{
		QString in =  getInfile("in");
		QString out = getOutfile("out");
		GenomeBuild build = stringToBuild(getEnum("build"));
		QString t_ps = getString("tid");
		QString n_ps = getString("nid");

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

		QString parameters = "-build " + buildToString(build);
		if( !t_ps.isEmpty() ) parameters += " -tid " + t_ps;
		if( !n_ps.isEmpty() ) parameters += " -nid " + n_ps;
		if( !out.isEmpty() ) parameters += " -out " +out;

		if(!out.isEmpty())
		{
			result.storeToQCML(out, QStringList(), parameters, QMap< QString, int >(), metadata);
		}
		else
		{
			QTextStream ostream(stdout);
			//header
			ostream << "#" << "FILE"  << (t_ps.isEmpty() ? "" : "\tTID") << (n_ps.isEmpty() ? "" : "\tNID") << "\t" << "LOH" << "\t" << "TAI" << "\t" << "LST" << endl;

			QStringList res;
			res << QFileInfo(in).fileName();
			if(!t_ps.isEmpty()) res << t_ps;
			if(!n_ps.isEmpty()) res << n_ps;
			res << QString::number(result.value("QC:2000062", true).asInt()); //LOH
			res << QString::number(result.value("QC:2000063", true).asInt()); //TAI
			res << QString::number(result.value("QC:2000064", true).asInt()); //LST

			ostream << res.join("\t") << endl;
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
