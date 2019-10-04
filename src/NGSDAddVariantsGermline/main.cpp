#include "ToolBase.h"
#include "NGSD.h"
#include "TSVFileStream.h"
#include "KeyValuePair.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>


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
		setDescription("Imports variants of a processed sample into the NGSD.");
		addString("ps", "Processed sample name", false);
		//optional
		addInfile("var", "Small variant list in GSvar format (as produced by megSAP).", true, true);
		addInfile("cnv", "CNV list in TSV format (as produced by megSAP).", true, true);
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("debug", "Enable verbose debug output.");
	}

	static KeyValuePair split(const QByteArray& string, char sep)
	{
		QByteArrayList parts = string.split(sep);

		QString key = parts.takeFirst().trimmed().mid(2); //remove '##'
		QString value = parts.join(sep).trimmed();

		return KeyValuePair(key, value);
	}

	///returns the most frequent string
	static int mostFrequent(const QStringList& parts)
	{
		int max = 0;
		QString max_cn;
		QHash<QString, int> cn_counts;
		foreach(const QString& cn, parts)
		{
			int count_new = cn_counts[cn] + 1;
			if (count_new>max)
			{
				max = count_new;
				max_cn = cn;
			}
			cn_counts[cn] = count_new;
		}

		return Helper::toInt(max_cn, "copy-number");
	}


	void importSmallVariants(NGSD& db, QTextStream& out, QString ps_name, bool debug)
	{
		QString filename = getInfile("var");
		if (filename=="") return;

		//TODO
	}

	void importCNVs(NGSD& db, QTextStream& out, QString ps_name, bool debug)
	{
		QString filename = getInfile("cnv");
		if (filename=="") return;

		out << "\n";
		out << "### importing CNVs for " << ps_name << " ###\n";
		out << "filename: " << filename << "\n";

		//get processed sample id
		QString ps_id = db.processedSampleId(ps_name);

		//check if CNV callset already exists > delete old callset
		QString last_callset_id = db.getValue("SELECT id FROM cnv_callset WHERE processed_sample_id=:0", true, ps_id).toString();
		if (last_callset_id!="")
		{
			out << "NOTE: CNV callset was already present - deleting it!\n";
			db.getQuery().exec("DELETE FROM cnv WHERE cnv_callset_id='" + last_callset_id + "'");
			db.getQuery().exec("DELETE FROM cnv_callset WHERE id='" + last_callset_id + "'");
		}

		//parse file header
		QString analysis_type;
		QString caller_version;
		QDateTime call_date;
		QJsonObject quality_metrics;
		TSVFileStream in(filename);
		foreach(QByteArray line, in.comments())
		{
			if (line.startsWith("##ANALYSISTYPE="))
			{
				KeyValuePair pair = split(line, '=');
				analysis_type = pair.value;
			}
			else if (line.contains(":"))
			{
				KeyValuePair pair = split(line, ':');

				if (pair.key.endsWith(" version"))
				{
					caller_version = pair.value;
				}
				else if (pair.key.endsWith(" finished on"))
				{
					call_date = QDateTime::fromString(pair.value, "yyyy-MM-dd hh:mm:ss");
				}
				else //quality metrics
				{
					if (pair.key.startsWith(ps_name + " ")) //remove sample name prefix (CnvHunter only)
					{
						pair.key = pair.key.mid(ps_name.length()+1).trimmed();
					}

					quality_metrics.insert(pair.key, pair.value);
				}
			}
			else
			{
				THROW(FileParseException, "Invalid header line '" + line + "' in file '" + filename + "'!");
			}
		}
		QByteArray caller;
		if (analysis_type=="CNVHUNTER_GERMLINE_SINGLE")
		{
			caller = "CnvHunter";

			if (call_date.isNull()) //fallback for CnvHunter file which does not contain the date!
			{
				call_date = QFileInfo(filename).created();
			}
		}
		else if (analysis_type=="CLINCNV_GERMLINE_SINGLE")
		{
			caller = "ClinCNV";
		}
		else if (analysis_type=="")
		{
			THROW(FileParseException, "CNV file '" + filename + "' does not contain '##ANALYSISTYPE=' line!\nIt is outdated - please re-run the CNV calling!");
		}
		else
		{
			THROW(FileParseException, "CNV file '" + filename + "' contains invalid analysis type '" + analysis_type + "'!\n");
		}

		QJsonDocument json_doc;
		json_doc.setObject(quality_metrics);

		//output
		out << "caller: " << caller << "\n";
		out << "caller version: " << caller_version << "\n";
		if (debug)
		{
			out << "DEBUG: callset quality: " << json_doc.toJson(QJsonDocument::Compact) << "\n";
		}

		//import CNV call set
		SqlQuery q_set = db.getQuery();
		q_set.prepare("INSERT INTO `cnv_callset` (`processed_sample_id`, `caller`, `caller_version`, `call_date`, `quality_metrics`, `quality`) VALUES (:0,:1,:2,:3,:4,:5)");
		q_set.bindValue(0, ps_id);
		q_set.bindValue(1, caller);
		q_set.bindValue(2, caller_version);
		q_set.bindValue(3, call_date);
		q_set.bindValue(4, json_doc.toJson(QJsonDocument::Compact));
		q_set.bindValue(5, "n/a");
		q_set.exec();
		QString callset_id = q_set.lastInsertId().toString();

		//prepare CNV query
		SqlQuery q_cnv = db.getQuery();
		q_cnv.prepare("INSERT INTO `cnv` (`cnv_callset_id`, `chr`, `start`, `end`, `cn`, `quality_metrics`) VALUES ('" + callset_id + "',:0,:1,:2,:3,:4)");

		//import CNVs
		int c_imported = 0;
		while(!in.atEnd())
		{
			QByteArrayList parts = in.readLine();

			//parse line
			int cn = -1;
			QJsonObject quality_metrics;

			for(int i=0; i<in.columns(); ++i)
			{
				const QByteArray& col_name = in.header()[i];
				QString entry = parts[i];

				if (caller == "CnvHunter")
				{
					if (col_name=="region_copy_numbers")
					{
						cn = mostFrequent(entry.split(','));
					}
					if (col_name=="region_count" || col_name=="region_zscores" || col_name=="region_copy_numbers")
					{
						quality_metrics.insert(col_name, entry);
					}
				}
				else if (caller == "ClinCNV")
				{
					if (col_name=="CN_change")
					{
						cn = Helper::toInt(entry, "copy-number");
					}
					if (col_name=="loglikelihood" || col_name=="no_of_regions" || col_name=="qvalue")
					{
						quality_metrics.insert(col_name, entry);
					}
				}
			}
			QJsonDocument json_doc;
			json_doc.setObject(quality_metrics);

			//debug output
			if (debug)
			{
				out << "DEBUG: " << parts[0] << ":" << parts[1] << "-" << parts[2] << " cn:" << cn << " quality: " << json_doc.toJson(QJsonDocument::Compact) << "\n";
			}

			//insert into database
			q_cnv.bindValue(0, parts[0]);
			q_cnv.bindValue(1, parts[1]);
			q_cnv.bindValue(2, parts[2]);
			q_cnv.bindValue(3, cn);
			q_cnv.bindValue(4, json_doc.toJson(QJsonDocument::Compact));
			q_cnv.exec();

			++c_imported;
		}
		out << "imported cnvs: " << c_imported << "\n";

	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream stream(out.data());
		QString ps_name = getString("ps");
		bool debug = getFlag("debug");

		//import
		importSmallVariants(db, stream, ps_name, debug);
		importCNVs(db, stream, ps_name, debug);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
