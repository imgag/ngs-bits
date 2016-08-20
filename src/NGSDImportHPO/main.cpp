#include "ToolBase.h"
#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QDebug>

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
		setDescription("Imports HPO terms and gene-phenotype correlations into NGSD.");
		addInfile("obo", "HPO ontology file 'hp.obo' from 'http://purl.obolibrary.org/obo/'.", false);
		addInfile("gene", "HPO phenotype-gene relation file 'ALL_SOURCES_ALL_FREQUENCIES_diseases_to_genes_to_phenotypes.txt' from 'http://compbio.charite.de/jenkins/job/hpo.annotations.monthly/lastStableBuild/artifact/annotation/'", false);
		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");
	}

	QHash<QByteArray, int> addTerm(SqlQuery& query, QByteArray& id, QByteArray& name, QByteArray& def)
	{
		static QHash<QByteArray, int> id2ngsd;

		//add non-obsolete terms
		if (!id.isEmpty() && !name.startsWith("obsolete "))
		{
			query.bindValue(0, id);
			query.bindValue(1, name);
			query.bindValue(2, def);
			query.exec();

			id2ngsd.insert(id, query.lastInsertId().toInt());
		}

		//clear data
		id.clear();
		name.clear();
		def = "";

		return id2ngsd;
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QTextStream out(stdout);

		//check tables exist
		db.tableExists("hpo_term");
		db.tableExists("hpo_parent");
		db.tableExists("hpo_genes");

		//clear tables if not empty
		if (!db.tableEmpty("hpo_term") || !db.tableEmpty("hpo_parent") || !db.tableEmpty("hpo_genes"))
		{
			if (getFlag("force"))
			{
				db.clearTable("hpo_genes");
				db.clearTable("hpo_parent");
				db.clearTable("hpo_term");
			}
			else
			{
				THROW(DatabaseException, "Tables already contain data! Use '-force' to overwrite old data!");
			}
		}

		//prepare SQL queries
		SqlQuery qi_term = db.getQuery();
		qi_term.prepare("INSERT INTO hpo_term (hpo_id, name, definition) VALUES (:0, :1, :2);");
		SqlQuery qi_parent = db.getQuery();
		qi_parent.prepare("INSERT INTO hpo_parent (parent, child) VALUES (:0, :1);");
		SqlQuery qi_gene = db.getQuery();
		qi_gene.prepare("INSERT INTO hpo_genes (hpo_term_id, gene) VALUES (:0, :1);");

		//parse OBO and insert terms
		QByteArray id, name, def = "";
		QHash<QByteArray, QList<QByteArray> > child_parents;
		QSharedPointer<QFile> fp = Helper::openFileForReading(getInfile("obo"));
		while(!fp->atEnd())
		{
			QByteArray line = fp->readLine().trimmed();
			if (line.isEmpty()) continue;

			if (line=="[Term]")
			{
				addTerm(qi_term, id, name, def);
			}
			else if (line=="is_obsolete: true")
			{
				id.clear();
			}
			else if (line.startsWith("id: "))
			{
				id = line.mid(4);
			}
			else if (line.startsWith("name: "))
			{
				name = line.mid(6);
			}
			else if (line.startsWith("def: "))
			{
				def = line.mid(5, line.lastIndexOf('"')-5);
			}
			else if (line.startsWith("is_a: HP:"))
			{
				if (!child_parents.contains(id)) child_parents[id] = QList<QByteArray>();
				child_parents[id].append(line.mid(6, 10));
			}
		}
		fp->close();

		QHash<QByteArray, int> id2ngsd = addTerm(qi_term, id, name, def);
		out << "Imported " << id2ngsd.count() << " non-obsolete HPO terms." << endl;

		//insert parent-child relations between (valid) terms
		int count = 0;
		QHashIterator<QByteArray, QList<QByteArray> > it(child_parents);
		while (it.hasNext())
		{
			it.next();

			QByteArray c_id = it.key();
			int c_db = id2ngsd.value(c_id, -1);
			if (c_db==-1) continue;

			QList<QByteArray> parent_ids = it.value();
			foreach(const QByteArray& p_id, parent_ids)
			{
				int p_db = id2ngsd.value(p_id, -1);
				if (p_db==-1) continue;

				qi_parent.bindValue(0, p_db);
				qi_parent.bindValue(1, c_db);
				qi_parent.exec();
				++count;
			}
		}
		out << "Imported " << count << " parent-child relations between terms." << endl;

		//parse gene-phenotype relations of (valid) terms
		fp = Helper::openFileForReading(getInfile("gene"));
		QHash<int, QSet<QByteArray> > term2genes;
		while(!fp->atEnd())
		{
			QList<QByteArray> parts = fp->readLine().trimmed().split('\t');
			if (parts.count()!=5) continue;

			QByteArray id = parts[3];
			int db = id2ngsd.value(id, -1);
			if (db==-1) continue;

			term2genes[db].insert(parts[1]);
		}
		fp->close();

		//import gene-phenotype relations
		count = 0;
		QHashIterator<int, QSet<QByteArray> > it2(term2genes);
		while (it2.hasNext())
		{
			it2.next();

			int term_id = it2.key();

			QSet<QByteArray> genes = it2.value();
			foreach(const QByteArray& gene, genes)
			{
				qi_gene.bindValue(0, term_id);
				qi_gene.bindValue(1, gene);
				qi_gene.exec();
				++count;
			}
		}
		out << "Imported " << count << " phenotype-gene relations." << endl;

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
