#include "ToolBase.h"
#include "NGSD.h"

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
		setDescription("Returns a list of users that evaluated a sample.");
		addInfile("in", "Input TSV file with processed sample IDs in first column. If unset, reads from STDIN.", true);
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2026,  6, 16, "First version.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QSharedPointer<QFile> in = Helper::openFileForReading(getInfile("in"), true);
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), true);

		out->write("#ps\treport_config_created\treport_config_last_update\treport_config_finalized\tdiag_status\n");
		while(!in->atEnd())
		{
			QByteArray line = in->readLine().trimmed();
			if (line.isEmpty() || line[0]=='#') continue;

			QString ps = line.split('\t').first();
			QString ps_id = db.processedSampleId(ps);

			QByteArray user_rc_create = db.getValue("SELECT u.name FROM report_configuration rc, user u WHERE u.id=rc.created_by AND rc.processed_sample_id='"+ps_id+"'", true, "").toString().toUtf8();
			QByteArray user_rc_update = db.getValue("SELECT u.name FROM report_configuration rc, user u WHERE u.id=rc.last_edit_by AND rc.processed_sample_id='"+ps_id+"'", true, "").toString().toUtf8();
			QByteArray user_rc_final = db.getValue("SELECT u.name FROM report_configuration rc, user u WHERE u.id=rc.finalized_by AND rc.processed_sample_id='"+ps_id+"'", true, "").toString().toUtf8();
			QByteArray user_status = db.getValue("SELECT u.name FROM diag_status ds, user u WHERE u.id=ds.user_id AND ds.processed_sample_id='"+ps_id+"'", true, "").toString().toUtf8();

			out->write(ps.toUtf8()+"\t"+user_rc_create+"\t"+user_rc_update+"\t"+user_rc_final+"\t"+user_status+"\n");
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
