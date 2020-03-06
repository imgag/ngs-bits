#ifndef GENEINFODBS_H
#define GENEINFODBS_H

#include <QList>
#include <QIcon>

///Gene-centered database
struct GeneDB
{
	QString name;
	QString url; //URL with [gene] as placeholder for gene symbol
	QIcon icon;
};

///Gene-centered database list
class GeneInfoDBs
{
public:
	//Returns all databases
	static QList<GeneDB>& all();

	//Opens a database entry in the browser
	static void openUrl(QString db_name, QString gene_symbol);

protected:
	GeneInfoDBs() = delete;
};

#endif // GENEINFODBS_H
