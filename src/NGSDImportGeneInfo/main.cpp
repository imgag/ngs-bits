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
		addInfile("constraint", "gnomAD gene contraint file (download and unzip https://storage.googleapis.com/gcp-public-data--gnomad/release/4.1.1/constraint/gnomad.v4.1.1.constraint_metrics.tsv.bgz).", false);
		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");

		changeLog(2026,  4, 17, "Update to gnomAD 4.1.1 constraints.");
		changeLog(2019,  7, 16, "Update to gnomAD 2.1.1 constraints.");
		changeLog(2016, 11, 23, "First version.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QTextStream out(stdout);

		//check tables exist
		db.tableExists("gene");
		db.tableExists("geneinfo_germline");

		//check for outdated gene names in table
		QStringList genes = db.getValues("SELECT symbol FROM geneinfo_germline WHERE symbol NOT IN (SELECT symbol FROM gene)");
		if (!genes.isEmpty())
		{
			out << "Note: 'geneinfo_germline' contains gene symbols that are not approved genes names: " << genes.join(", ") << Qt::endl;
			out << Qt::endl;
		}

		//get gene mapping
		out << "Getting ENSG to gene name mapping from NGSD..." << Qt::endl;
		QHash<QByteArray, QByteArray> ensg2symbol;
		{
			SqlQuery query = db.getQuery();
			query.exec("SELECT ensembl_id, symbol FROM gene");
			while(query.next())
			{
				ensg2symbol.insert(query.value(0).toByteArray(), query.value(1).toByteArray());
			}
		}

		//clear constraints
		db.getQuery().exec("UPDATE geneinfo_germline SET gnomad_oe_syn=NULL, gnomad_oe_mis=NULL, gnomad_oe_lof=NULL, gnomad_pli=NULL");

		//import gnomAD o/e and pLI
		out << "Importing gnomAD constraints..." << Qt::endl;
		{
			int c_inserted = 0;
			int c_skipped_no_gene_symbol = 0;

			SqlQuery update_query = db.getQuery();
			update_query.prepare("INSERT INTO geneinfo_germline (symbol, inheritance, gnomad_oe_syn, gnomad_oe_mis, gnomad_oe_lof, gnomad_pli, comments) VALUES (:0, 'n/a', :1, :2, :3, :4, '') ON DUPLICATE KEY UPDATE gnomad_oe_syn=VALUES(gnomad_oe_syn), gnomad_oe_mis=VALUES(gnomad_oe_mis), gnomad_oe_lof=VALUES(gnomad_oe_lof), gnomad_pli=VALUES(gnomad_pli)");
			int i_syn = -1;
			int i_mis = -1;
			int i_lof = -1;
			int i_pli = -1;
			int i_canonical = -1;
			int i_mane = -1;

			VersatileFile file(getInfile("constraint"));
			file.open(QFile::ReadOnly|QIODevice::Text);
			while(!file.atEnd())
			{
				QByteArray line = file.readLine(true);
				if (line.isEmpty()) continue;

				QByteArrayList parts = line.split('\t');
				if (parts.count()<111) continue;

				//header line
				if (parts[0]=="gene")
				{
					i_syn = parts.indexOf("syn.oe");
					i_mis = parts.indexOf("mis.oe");
					i_lof = parts.indexOf("lof.oe");
					i_pli = parts.indexOf("lof.pLI");
					i_canonical = parts.indexOf("canonical");
					i_mane = parts.indexOf("mane_select");

					//check that headers are ok
					if (i_syn==-1 || i_mis==-1 || i_lof==-1 || i_pli==-1 || i_canonical==-1 || i_mane==-1)
					{
						THROW(FileParseException, "Could not determine column indices for mandatory columns in header line: " + line);
					}

					continue;
				}

				//skip not relevent transcripts
				if  (parts[i_canonical].trimmed()!="true" && parts[i_mane].trimmed()!="true") continue;

				//gene
				QByteArray gene = ensg2symbol.value(parts[1], "");
				if (gene.isEmpty())
				{
					++c_skipped_no_gene_symbol;
					continue;
				}
				update_query.bindValue(0, gene);

				//gnomAD o/e
				if (parts[i_syn]=="NA")
				{                    
                    update_query.bindValue(1,  QVariant(QMetaType(QMetaType::Double)));                    
				}
				else
				{
					update_query.bindValue(1, QString::number(Helper::toDouble(parts[i_syn], "gnomad o/e (syn)"), 'f', 2));
				}

				if (parts[i_mis]=="NA")
				{                    
                    update_query.bindValue(2,  QVariant(QMetaType(QMetaType::Double)));                    
				}
				else
				{
					update_query.bindValue(2, QString::number(Helper::toDouble(parts[i_mis], "gnomad o/e (mis)"), 'f', 2));
				}

				if (parts[i_lof]=="NA")
				{
					update_query.bindValue(3,  QVariant(QMetaType(QMetaType::Double)));
				}
				else
				{
					update_query.bindValue(3, QString::number(Helper::toDouble(parts[i_lof], "gnomad o/e (lof)"), 'f', 2));
				}


				if (parts[i_lof]=="NA")
				{
					update_query.bindValue(4,  QVariant(QMetaType(QMetaType::Double)));
				}
				else
				{
					update_query.bindValue(4, QString::number(Helper::toDouble(parts[i_pli], "gnomad o/e (pLi)"), 'f', 3));
				}

				update_query.exec();

				++c_inserted;
			}

			out << "  skipped " << c_skipped_no_gene_symbol << " lines because no gene symbol could be determined based on ENSG" << Qt::endl;
			out << "  imported constraint info for " << c_inserted << " genes" << Qt::endl;
            out << Qt::endl;
		}

		//gene inheritance from HPO info
        out << Qt::endl;
        out << "Setting gene inheritance based on info from HPO..." << Qt::endl;
		{

			SqlQuery update_query = db.getQuery();
			update_query.prepare("INSERT INTO geneinfo_germline (symbol, inheritance, comments) VALUES (:0, :1, '') ON DUPLICATE KEY UPDATE inheritance=VALUES(inheritance)");

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
                            out << "  skipped invalid inheritance mode '" << mode << "' for gene " << gene << " (chromosome " << chr << ")" << Qt::endl;
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
                            out << "  skipped invalid inheritance mode '" << mode << "' for gene " << gene << " (chromosome " << chr << ")" << Qt::endl;
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
                            out << "  skipped invalid inheritance mode '" << mode << "' for gene " << gene << " (chromosome " << chr << ")" << Qt::endl;
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
                            out << "  skipped invalid inheritance mode '" << mode << "' for gene " << gene << " (chromosome " << chr << ")" << Qt::endl;
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
                            out << "  skipped invalid inheritance mode '" << mode << "' for gene " << gene << " (chromosome " << chr << ")" << Qt::endl;
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
                    out << "  check inheritance manually: gene=" << gene << " chr=" << chr << " old=" << inh_old << " new=" << inh_new << Qt::endl;
					++c_check;
				}

			}
            out << "  genes without inheritance info: " << c_noinfo << Qt::endl;
            out << "  genes with unchanged inheritance: " << c_unchanged << Qt::endl;
            out << "  genes with updated inheritance: " << c_update << Qt::endl;
            out << "  genes that require manual check: " << c_check << Qt::endl;
		}

		//add DB import info (version parsed from filename; if parsing fails use full filename)
		QString version = QFileInfo(getInfile("constraint")).fileName();
		QString tmp = version;
		tmp.replace("gnomad.v", "").replace(".constraint_metrics.tsv", "").replace(".bgz", "").replace("NGSDImportGeneInfo_", "");
		if (QRegularExpression("^[0-9.]+$").match(tmp).hasMatch())
		{
			version = tmp;
		}
		db.setDatabaseInfo("gnomAD constraints", version);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	qputenv("QT_QPA_PLATFORM", "offscreen");
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
