#include "GeneInfoDBs.h"
#include "Exceptions.h"
#include <QDesktopServices>

void GeneInfoDBs::openUrl(QString db_name, QString gene_symbol)
{
	foreach(const GeneDB& db, all())
	{
		if (db.name==db_name)
		{
			QString url = db.url;
			QDesktopServices::openUrl(QUrl(url.replace("[gene]", gene_symbol)));
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
		dbs_ << GeneDB{"ClinGen", "https://www.ncbi.nlm.nih.gov/projects/dbvar/clingen/clingen_gene.cgi?sym=[gene]", QIcon("://Icons/ClinGen.png"), false};
		dbs_ << GeneDB{"GeneCards", "http://www.genecards.org/cgi-bin/carddisp.pl?gene=[gene]", QIcon("://Icons/GeneCards.png"), false};
		dbs_ << GeneDB{"gnomAD", "https://gnomad.broadinstitute.org/gene/[gene]", QIcon("://Icons/gnomAD.png"), false};
		dbs_ << GeneDB{"HGMD", "https://portal.biobase-international.com/hgmd/pro/gene.php?gene=[gene]", QIcon("://Icons/HGMD.png"), false};
		dbs_ << GeneDB{"OMIM", "https://omim.org/search/?search=[gene]", QIcon("://Icons/OMIM.png"), false};
		dbs_ << GeneDB{"SysID", "https://sysid.cmbi.umcn.nl/search?search=[gene]", QIcon("://Icons/SysID.png"), false};
		dbs_ << GeneDB{"cBioPortal", "https://www.cbioportal.org/ln?q=[gene]:MUT", QIcon("://Icons/cbioportal.png"), true};
	}

	return dbs_;
}
