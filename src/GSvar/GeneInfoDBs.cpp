#include "GeneInfoDBs.h"
#include "Exceptions.h"
#include "HttpHandler.h"
#include "Settings.h"
#include "GUIHelper.h"
#include <QMessageBox>
#include <QApplication>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

void GeneInfoDBs::openUrl(QString db_name, QString gene_symbol)
{
	foreach(const GeneDB& db, all())
	{
		if (db.name==db_name)
		{
			QString url = db.url;
			if (url.contains("[gene_id_ncbi]"))
			{
				static HttpHandler http_handler(false); //static to allow caching of credentials
				try
				{
					HttpHeaders add_headers;
					add_headers.insert("Accept", "application/json");

					QByteArray reply = http_handler.get("https://rest.genenames.org/fetch/symbol/"+gene_symbol, add_headers);
					QJsonDocument json = QJsonDocument::fromJson(reply);
					QJsonArray docs = json.object().value("response").toObject().value("docs").toArray();
					if (docs.count()!=1)
					{
						THROW(Exception, "Could not convert gene symbol to NCBI identifier: HGNC REST API returned " + QString::number(docs.count()) + " entries!");
					}
					QString ncbi_id = docs.at(0).toObject().value("entrez_id").toString();
					url.replace("[gene_id_ncbi]", ncbi_id);
				}
				catch(Exception& e)
				{
					QMessageBox::warning(GUIHelper::mainWindow(), "Could not get NCBI gene identifier for gene " + gene_symbol, e.message());
					return;
				}
			}
			else
			{
				url.replace("[gene]", gene_symbol);
			}
			QDesktopServices::openUrl(QUrl(url));
			return;
		}
	}

	THROW(ProgrammingException, "Unknown gene DB '" + db_name + "' in GeneInfoDBs::openUrl!");
}

QList<GeneDB>& GeneInfoDBs::all()
{
	static QList<GeneDB> dbs_;

	if (dbs_.isEmpty())
	{
		dbs_ << GeneDB{"ClinGen", "https://search.clinicalgenome.org/kb/gene-dosage?search=[gene]", QIcon("://Icons/ClinGen.png"), false};
		dbs_ << GeneDB{"ClinVar", "https://www.ncbi.nlm.nih.gov/clinvar/?term=[gene]%5Bgene%5D", QIcon("://Icons/ClinGen.png"), false};
		dbs_ << GeneDB{"GeneCards", "https://www.genecards.org/cgi-bin/carddisp.pl?gene=[gene]", QIcon("://Icons/GeneCards.png"), false};
		dbs_ << GeneDB{"GTEx", "https://www.gtexportal.org/home/gene/[gene]", QIcon("://Icons/GTEx.png"), false};
		dbs_ << GeneDB{"gnomAD", "https://gnomad.broadinstitute.org/gene/[gene]?dataset=gnomad_r4", QIcon("://Icons/gnomAD.png"), false};

		if (Settings::boolean("use_free_hgmd_version"))
		{
			dbs_ << GeneDB{"HGMD", "http://www.hgmd.cf.ac.uk/ac/gene.php?gene=[gene]", QIcon("://Icons/HGMD.png"), false}; //no HTTPS
		}
		else
		{
			dbs_ << GeneDB{"HGMD", "https://my.qiagendigitalinsights.com/bbp/view/hgmd/pro/gene.php?gene=[gene]", QIcon("://Icons/HGMD.png"), false};
		}
		dbs_ << GeneDB{"OMIM", "https://omim.org/search/?search=[gene]", QIcon("://Icons/OMIM.png"), false};
		dbs_ << GeneDB{"SysID", "https://sysid.cmbi.umcn.nl/search?search=[gene]", QIcon("://Icons/SysID.png"), false};
		dbs_ << GeneDB{"cBioPortal", "https://www.cbioportal.org/ln?q=[gene]:MUT", QIcon("://Icons/cbioportal.png"), true};
		dbs_ << GeneDB{"CKB", "https://ckb.jax.org/gene/show?geneId=[gene_id_ncbi]", QIcon("://Icons/ckb.png"), true};
		dbs_ << GeneDB{"GeneReviews", "https://www.ncbi.nlm.nih.gov/books/NBK1116/?term=[gene]", QIcon("://Icons/GeneReviews.png"), true};
	}

	return dbs_;
}
