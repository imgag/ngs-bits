#include "ToolBase.h"
#include "FastqFileStream.h"
#include "Helper.h"
#include "TsvFile.h"
#include "ExportCBioPortalStudy.h"
#include <QFile>

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
		setDescription("Converts a FASTQ file to FASTA format.");
		addInfile("samples", "Input TSV file with samples (tumor, normal, rna) to be exported and their clinical data.", false);
		addInfile("study_data", "Input TSV file with Infos about the study that should be created.", false);
		addInfile("attribute_data", "Input TSV file with Infos about the sample attributes that should be contained in the study.", false);
		addString("out", "Output folder that will contain all files for the cBioPortal study.", false);
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	virtual void main()
	{
		StudyData study;
		CancerData cancer;

		parseStudyData(getInfile("study_data"), study, cancer);


		CBioPortalExportSettings export_settings(study, false);
		export_settings.cancer = cancer;

		TsvFile samples;
		samples.load(getInfile("samples"));

		//gather sample infos:
		//gather necessary files
		NGSD db(getFlag("test"));

		SqlQuery q_get_sample = db.getQuery();
		q_get_sample.prepare("SELECT name FROM sample WHERE name_external = %1");


		int idx_ext_name = samples.columnIndex("external_name");

		for (int i=0; i< samples.rowCount(); i++)
		{
			QStringList row = samples.row(i);
			q_get_sample.bindValue(1, row[idx_ext_name]);

			q_get_sample.executedQuery();

		}


		//get variant lists

		//compare to Mainwindow export helper
	}


	void parseStudyData(QString file, StudyData& study, CancerData& cancer)
	{
		TsvFile study_data;
		study_data.load(file);

		QStringList keys = study_data.extractColumn(0);

		int idx_study_name = getIndex(keys, "study_name");
		int idx_study_ident = getIndex(keys, "study_identifier");
		int idx_study_desc = getIndex(keys, "study_description");
		int idx_study_ref = getIndex(keys, "study_reference");
		int idx_cancer_name = getIndex(keys, "cancer_name");
		int idx_cancer_desc = getIndex(keys, "cancer_description");
		int idx_cancer_parent = getIndex(keys, "cancer_parent");
		int idx_cancer_color = getIndex(keys, "cancer_color");

		QStringList values = study_data.extractColumn(1);

		study.name = values[idx_study_name];
		study.description = values[idx_study_desc];
		study.identifier = values[idx_study_ident];
		study.reference_genome = values[idx_study_ref];
		study.cancer_type = values[idx_cancer_name];

		cancer.description = values[idx_cancer_desc];
		cancer.parent = values[idx_cancer_parent];
		cancer.color = values[idx_cancer_color];
	}

	int getIndex(QStringList& keys, QString key)
	{
		int tmp = keys.indexOf(key);
		if (tmp == -1)
		{
			THROW(FileParseException, "Couldn't find necessary key '" + key + "' in the study_data file.");
		}
		return tmp;
	}
};



#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
