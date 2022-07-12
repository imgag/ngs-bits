#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "BedFile.h"
#include "NGSD.h"
#include "Log.h"
#include "Settings.h"
#include "VcfFile.h"
#include <QDir>
#include <QElapsedTimer>

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
		setDescription("Updates the genotype for a given processed sample.");
		addString("ps", "Processed sample id.", false);
		addInfile("in", "Input BEDPE file.", false);
		//optional
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2022, 3, 7, "Initial commit.");
	}


	virtual void main()
	{
		//init
		QString ps_name = getString("ps");
		QString in = getInfile("in");
		NGSD db(getFlag("test"));
		SqlQuery query = db.getQuery();
		QTextStream out(stdout);

		//check if genotypes are already imported
		QString ps_id = db.processedSampleId(ps_name);
		out << "Processed sample id: " << ps_id << "\n";
		QString callset_id = db.getValue("SELECT id FROM sv_callset WHERE processed_sample_id=:0", false, ps_id).toString();
		out << "Callset id: " << callset_id << "\n";

		QStringList genotypes = db.getValues("SELECT genotype FROM sv_deletion WHERE sv_callset_id=:0", callset_id);
		if (genotypes.at(0) != "n/a")
		{
			out << "ArgumentException: SV genotypes for sample " << ps_name << " already imported!\n";
			THROW(ArgumentException, "SV genotypes for sample " + ps_name + " already imported!");
		}

		//open BEDPE file and update genotype
		BedpeFile svs;
		svs.load(in);
		int i=0;
		int n_multiple_matches = 0;

		try
		{
			//start transaction
			db.transaction();
			for (i = 0; i < svs.count(); ++i)
			{
				const BedpeLine& sv = svs[i];

				//skip SVs on special chromosmes
				if(!sv.chr1().isNonSpecial() || !sv.chr2().isNonSpecial()) continue;

				//parse genotype
				QString genotype = sv.formatValueByKey("GT", svs.annotationHeaders()).trimmed();
				if (genotype == "1/1")
				{
					genotype = "hom";
				}
				else if ((genotype == "0/1") || (genotype == "1/0"))
				{
					genotype = "het";
				}
				else
				{
					THROW(FileParseException, "Invalid genotype '" + genotype + "' found in SV '" + sv.positionRange() + "'!")
				}
				//get SV id
				QStringList sv_ids;
				try
				{
					sv_ids << db.svId(sv, callset_id.toInt(), svs);
				}
				catch (DatabaseException e)
				{
					if(e.message().startsWith("Multiple matching SVs found in NGSD!"))
					{
						out << svs[i].toString() << ":Multiple matching SVs found in NGSD!\n";
						n_multiple_matches++;

						//extract ids and set genotype to 'het' (fallback)
						sv_ids = e.message().split('(').at(1).split(')').at(0).split(',');
						genotype = "het";

					}
					else
					{
						THROW(DatabaseException, e.message());
					}
				}

				QString db_table_name = db.svTableName(sv.type());

				// update db entry
				foreach (const QString& sv_id, sv_ids)
				{
					//out << "UPDATE " << db_table_name << " SET genotype='" << genotype << "' WHERE id=" << sv_id << "\n";
					query.exec("UPDATE " + db_table_name + " SET genotype='" + genotype + "' WHERE id=" + sv_id);
				}

			}

			// all SV successfully updated -> commit transaction
			out << "Update complete. Commit transaction...\n";
			db.commit();

		}
		catch (Exception e )
		{
			out << e.message() << "\n";
			out << svs[i].toString() + "\n";
			out << "Update failed. Rolling back...\n";
			db.rollback();
		}

		out << "Stats:\n";
		out << "Multiple matches: " << QString::number(n_multiple_matches) << endl;

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
