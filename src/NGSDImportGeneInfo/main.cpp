#include "ToolBase.h"
#include "NGSD.h"
#include "Helper.h"

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
		setDescription("Imports gene-specific information into NGSD.");
		addInfile("constraint", "ExAC gene contraint file (download ftp://ftp.broadinstitute.org/pub/ExAC_release/current/functional_gene_constraint/fordist_cleaned_exac_nonTCGA_z_pli_rec_null_data.txt).", false);
		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QTextStream out(stdout);

		//check tables exist
		db.tableExists("geneinfo_germline");

		//update gene names to approved symbols
		out << "Updating gene names..." << endl;
		{
			SqlQuery query = db.getQuery();
			query.exec("SELECT symbol FROM geneinfo_germline WHERE symbol NOT IN (SELECT symbol FROM gene)");
			while(query.next())
			{
				QString symbol = query.value(0).toString();
				auto approved = db.geneToApprovedWithMessage(symbol);
				if (!approved.second.startsWith("KEPT:"))
				{
					out << "  skipped " << symbol << ": " << approved.second << endl;
				}
			}
			out << endl;
		}

		//import ExAC pLI scores
		out << "Importing ExAC pLI scores..." << endl;
		{
			SqlQuery update_query = db.getQuery();
			update_query.prepare("INSERT INTO geneinfo_germline (symbol, inheritance, exac_pli, comments) VALUES (:0, 'n/a', :1, '') ON DUPLICATE KEY UPDATE exac_pli=:2");

			auto file = Helper::openFileForReading(getInfile("constraint"));
			while(!file->atEnd())
			{
				QString line = file->readLine().trimmed();
				if (line.isEmpty() || line.startsWith("transcript\t")) continue;

				QStringList parts = line.split('\t');
				if (parts.count()<22) continue;

				//gene
				QString gene = parts[1];
				auto approved = db.geneToApprovedWithMessage(gene);
				if (approved.second.startsWith("ERROR:"))
				{
					out << "  skipped " << gene << ": " << approved.second << endl;
					continue;
				}
				gene = approved.first;
				update_query.bindValue(0, gene);

				//pLI
				QString pLI = QString::number(Helper::toDouble(parts[19], "ExAC pLI score"), 'f', 4);
				update_query.bindValue(1, pLI);
				update_query.bindValue(2, pLI);
				update_query.exec();
			}
			out << endl;
		}

		//gene inheritance from HPO info
		out << endl;
		out << "Setting gene inheritance based on info from HPO..." << endl;
		{

			SqlQuery update_query = db.getQuery();
			update_query.prepare("INSERT INTO geneinfo_germline (symbol, inheritance, exac_pli, comments) VALUES (:0, :1, null, '') ON DUPLICATE KEY UPDATE inheritance=:2");

			QMap<QString, QString> hpo2germline;
			hpo2germline.insert("Autosomal recessive inheritance", "AR");
			hpo2germline.insert("Autosomal dominant inheritance", "AD");
			hpo2germline.insert("X-linked recessive inheritance", "XLR");
			hpo2germline.insert("X-linked dominant inheritance", "XLD");
			hpo2germline.insert("Mitochondrial inheritance", "MT");

			int c_noinfo = 0;
			int c_unchanged = 0;
			int c_update = 0;
			int c_check = 0;
			QStringList genes = db.getValues("SELECT symbol FROM gene");
			foreach(const QString& gene, genes)
			{
				//get info from geneinfo_germline
				QString inh_old = db.getValue("SELECT inheritance FROM geneinfo_germline WHERE symbol='" + gene + "'", true).toString();
				if (inh_old=="") inh_old="n/a";

				QString chr = db.getValues("SELECT DISTINCT gt.chromosome FROM gene_transcript gt, gene g WHERE g.id=gt.gene_id AND g.symbol='" + gene + "'").join(",");

				//convert HPO terms to values compatible with 'geneinfo_germline' (also corrects for impossible chr-inheritance combos)
				QStringList inh_hpo_list;
				QStringList hpo_modes = db.getValues("SELECT ht.name FROM hpo_term ht, hpo_genes hg WHERE hg.hpo_term_id=ht.id AND hg.gene='" + gene + "' AND ht.name LIKE '%inheritance%' ORDER BY ht.name DESC");
				QRegExp digits("\\d");
				foreach(QString mode, hpo_modes)
				{
					if (chr.contains("X") && mode=="X-linked recessive inheritance")
					{
						inh_hpo_list << "XLR";
					}
					if (chr.contains("X") && mode=="X-linked dominant inheritance")
					{
						inh_hpo_list << "XLD";
					}
					if (chr.contains("M") && mode=="Mitochondrial inheritance")
					{
						inh_hpo_list << "MT";
					}
					if (digits.indexIn(chr)!=-1 && mode=="Autosomal recessive inheritance")
					{
						inh_hpo_list << "AR";
					}
					if (digits.indexIn(chr)!=-1 && mode=="Autosomal dominant inheritance")
					{
						inh_hpo_list << "AD";
					}
				}
				QString inh_new = inh_hpo_list.count()==0 ? "n/a" : inh_hpo_list.join("+");

				//compare old and new values and act on it
				if (inh_new=="n/a" && inh_old=="n/a")
				{
					++c_noinfo;
					continue;
				}

				if (inh_new==inh_old || inh_new=="n/a")
				{
					++c_unchanged;
					continue;
				}

				if (inh_old=="n/a")
				{
					/*TODO
					update_query.bindValue(0, gene);
					update_query.bindValue(0, inh_new);
					update_query.bindValue(1, inh_new);
					update_query.exec();
					*/
					++c_update;
				}
				else
				{
					out << "  check inheritance manually: gene=" << gene << " chr=" << chr << " old=" << inh_old << " new=" << inh_new << endl;
					++c_check;
				}

			}
			out << "  genes without inheritance info: " << c_noinfo << endl;
			out << "  genes with unchanged inheritance: " << c_unchanged << endl;
			out << "  genes with updated inheritance: " << c_update << endl;
			out << "  genes that require manual check: " << c_check << endl;
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
