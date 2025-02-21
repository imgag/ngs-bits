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
		addInfile("constraint", "gnomAD gene contraint file (download and unzip https://storage.googleapis.com/gcp-public-data--gnomad/release/2.1.1/constraint/gnomad.v2.1.1.lof_metrics.by_gene.txt.bgz).", false);
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
        out << "Updating gene names..." << QT_ENDL;
		{
			SqlQuery query = db.getQuery();
			query.exec("SELECT symbol FROM geneinfo_germline WHERE symbol NOT IN (SELECT symbol FROM gene)");
			while(query.next())
			{
				QString symbol = query.value(0).toString();
				auto approved = db.geneToApprovedWithMessage(symbol);
				if (!approved.second.startsWith("KEPT:"))
				{
                    out << "  skipped " << symbol << ": " << approved.second << QT_ENDL;
				}
			}
            out << QT_ENDL;
		}

		//import gnomAD o/e scores
        out << "Importing gnomAD constraints..." << QT_ENDL;
		{
			int c_inserted = 0;

			SqlQuery update_query = db.getQuery();
			update_query.prepare("INSERT INTO geneinfo_germline (symbol, inheritance, gnomad_oe_syn, gnomad_oe_mis, gnomad_oe_lof, comments) VALUES (:0, 'n/a', :1, :2, :3, '') ON DUPLICATE KEY UPDATE gnomad_oe_syn=VALUES(gnomad_oe_syn), gnomad_oe_mis=VALUES(gnomad_oe_mis), gnomad_oe_lof=VALUES(gnomad_oe_lof)");
			int i_syn = -1;
			int i_mis = -1;
			int i_lof = -1;
			int i_can = -1;

			auto file = Helper::openFileForReading(getInfile("constraint"));
			while(!file->atEnd())
			{
				QString line = file->readLine().trimmed();
				if (line.isEmpty()) continue;

				QStringList parts = line.split('\t');
				if (parts.count()<25) continue;

				//header
				if (parts[0]=="gene")
				{
					i_syn = parts.indexOf("oe_syn");
					i_mis = parts.indexOf("oe_mis");
					i_lof = parts.indexOf("oe_lof");
					i_can = parts.indexOf("canonical");

					//check that headers are ok
					if (i_syn==-1 || i_mis==-1 || i_lof==-1)
					{
						THROW(FileParseException, "Could not determine column indices for o/e columns in header line!");
					}

					continue;
				}

				//skip canonical transcripts (for downward-compatibility with version 2.1)
				if  (i_can!=-1 && parts[i_can].trimmed()!="true") continue;

				//gene
				QString gene = parts[0];
				auto approved = db.geneToApprovedWithMessage(gene);
				if (approved.second.startsWith("ERROR:"))
				{
                    out << "  skipped " << gene << ": " << approved.second << QT_ENDL;
					continue;
				}
				gene = approved.first;
				update_query.bindValue(0, gene);

				//gnomAD o/e
				if (parts[i_syn]=="NA" || parts[i_syn]=="NaN")
				{
					update_query.bindValue(1,  QVariant(QVariant::Double));
				}
				else
				{
					update_query.bindValue(1, QString::number(Helper::toDouble(parts[i_syn], "gnomad o/e (syn)"), 'f', 2));
				}

				if (parts[i_mis]=="NA" || parts[i_mis]=="NaN")
				{
					update_query.bindValue(2,  QVariant(QVariant::Double));
				}
				else
				{
					update_query.bindValue(2, QString::number(Helper::toDouble(parts[i_mis], "gnomad o/e (mis)"), 'f', 2));
				}

				if (parts[i_lof]=="NA" || parts[i_lof]=="NaN")
				{
					update_query.bindValue(3,  QVariant(QVariant::Double));
				}
				else
				{
					update_query.bindValue(3, QString::number(Helper::toDouble(parts[i_lof], "gnomad o/e (lof)"), 'f', 2));
				}
				update_query.exec();

				++c_inserted;
			}

            out << "  imported constraint info for " << c_inserted << " genes" << QT_ENDL;
            out << QT_ENDL;
		}

		//gene inheritance from HPO info
        out << QT_ENDL;
        out << "Setting gene inheritance based on info from HPO..." << QT_ENDL;
		{

			SqlQuery update_query = db.getQuery();
			update_query.prepare("INSERT INTO geneinfo_germline (symbol, inheritance, gnomad_oe_syn, gnomad_oe_mis, gnomad_oe_lof, comments) VALUES (:0, :1, NULL, NULL, NULL, '') ON DUPLICATE KEY UPDATE inheritance=VALUES(inheritance)");

			int c_noinfo = 0;
			int c_unchanged = 0;
			int c_update = 0;
			int c_check = 0;
			QStringList genes = db.getValues("SELECT symbol FROM gene");
			foreach(const QString& gene, genes)
			{
				//get info from geneinfo_germline
				QString inh_old = db.getValue("SELECT inheritance FROM geneinfo_germline WHERE symbol=:0", true, gene).toString();
				if (inh_old=="") inh_old="n/a";

				QString chr = db.getValues("SELECT DISTINCT gt.chromosome FROM gene_transcript gt, gene g WHERE g.id=gt.gene_id AND g.symbol=:0", gene).join(",");

				//convert HPO terms to values compatible with 'geneinfo_germline' (also corrects for impossible chr-inheritance combos)
				QStringList inh_hpo_list;
				QStringList hpo_modes = db.getValues("SELECT ht.name FROM hpo_term ht, hpo_genes hg WHERE hg.hpo_term_id=ht.id AND hg.gene=:0 AND ht.name LIKE '%inheritance%' ORDER BY ht.name DESC", gene);
                QRegularExpression digits("\\d");
				foreach(QString mode, hpo_modes)
				{
					if (mode=="X-linked recessive inheritance")
					{
						if (chr.contains("X"))
						{
							inh_hpo_list << "XLR";
						}
						else
						{
                            out << "  skipped invalid inheritance mode '" << mode << "' for gene " << gene << " (chromosome " << chr << ")" << QT_ENDL;
						}
					}
					if (mode=="X-linked dominant inheritance")
					{
						if (chr.contains("X"))
						{
							inh_hpo_list << "XLD";
						}
						else
						{
                            out << "  skipped invalid inheritance mode '" << mode << "' for gene " << gene << " (chromosome " << chr << ")" << QT_ENDL;
						}
					}
					if (mode=="Mitochondrial inheritance")
					{
						if (chr.contains("M"))
						{
							inh_hpo_list << "MT";
						}
						else
						{
                            out << "  skipped invalid inheritance mode '" << mode << "' for gene " << gene << " (chromosome " << chr << ")" << QT_ENDL;
						}
					}
					if (mode=="Autosomal recessive inheritance")
					{
                        if (digits.match(chr).hasMatch())
						{
							inh_hpo_list << "AR";
						}
						else
						{
                            out << "  skipped invalid inheritance mode '" << mode << "' for gene " << gene << " (chromosome " << chr << ")" << QT_ENDL;
						}
					}
					if (mode=="Autosomal dominant inheritance")
					{
                        if (digits.match(chr).hasMatch())
						{
							inh_hpo_list << "AD";
						}
						else
						{
                            out << "  skipped invalid inheritance mode '" << mode << "' for gene " << gene << " (chromosome " << chr << ")" << QT_ENDL;
						}
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
					update_query.bindValue(0, gene);
					update_query.bindValue(1, inh_new);
					update_query.exec();
					++c_update;
				}
				else
				{
                    out << "  check inheritance manually: gene=" << gene << " chr=" << chr << " old=" << inh_old << " new=" << inh_new << QT_ENDL;
					++c_check;
				}

			}
            out << "  genes without inheritance info: " << c_noinfo << QT_ENDL;
            out << "  genes with unchanged inheritance: " << c_unchanged << QT_ENDL;
            out << "  genes with updated inheritance: " << c_update << QT_ENDL;
            out << "  genes that require manual check: " << c_check << QT_ENDL;
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
