#include "ToolBase.h"
#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QXmlStreamReader>

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
		setDescription("Imports ORPHA diseases/genes into the NGSD.");
		addInfile("terms", "Terms XML file from 'https://github.com/Orphanet/Orphadata.org/tree/master/Disorders%20cross%20referenced%20with%20other%20nomenclatures'.", false);
		addInfile("genes", "Terms<>genes XML file from 'https://github.com/Orphanet/Orphadata.org/tree/master/Disorders%20with%20their%20associated%20genes'.", false);
		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");
	}

	virtual void main()
	{
		//init
		QString terms = getInfile("terms");
		QString genes = getInfile("genes");
		NGSD db(getFlag("test"));
		QTextStream out(stdout);

		//check tables exist
		db.tableExists("disease_term");
		db.tableExists("disease_gene");

		//clear tables if not empty
		if (!db.tableEmpty("disease_term") || !db.tableEmpty("disease_gene"))
		{
			if (getFlag("force"))
			{
				db.clearTable("disease_term");
				db.clearTable("disease_gene");
			}
			else
			{
				THROW(DatabaseException, "Tables already contain data! Use '-force' to overwrite old data!");
			}
		}

		//prepare SQL queries
		SqlQuery qi_disease = db.getQuery();
		qi_disease.prepare("INSERT INTO disease_term (source, identifier, name, synonyms) VALUES ('OrphaNet', :0, :1, :2)");
		SqlQuery qi_gene = db.getQuery();
		qi_gene.prepare("INSERT INTO disease_gene (disease_term_id, gene) VALUES (:0, :1)");

		//import disease terms
		out << "Importing ORPHA diseases..." << endl;
		QSharedPointer<QFile> fp = Helper::openFileForReading(terms);
		QXmlStreamReader xml(fp.data());
		xml.readNextStartElement(); //root element JDBOR
		while (xml.readNextStartElement())
		{
			out << xml.name() << endl;
			if (xml.name()=="DisorderList")
			{
				while (xml.readNextStartElement())
				{
					if (xml.name()=="Disorder")
					{
						bool skip = false;
						while (xml.readNextStartElement())
						{
							if (xml.name()=="OrphaNumber")
							{
								out << endl << "###: " << xml.readElementText() << endl;
							}
							else if (xml.name()=="Name")
							{
								out << "name: " << xml.readElementText()<< endl;
							}
							else if (xml.name()=="SynonymList")
							{
								while (xml.readNextStartElement())
								{
									if (xml.name()=="Synonym")
									{
										out << "Syn: " << xml.readElementText()<< endl;
									}
									else xml.skipCurrentElement();
								}
							}
							else if (xml.name()=="DisorderFlagList")
							{
								while (xml.readNextStartElement())
								{
									if (xml.name()=="DisorderFlag")
									{
										while (xml.readNextStartElement())
										{
											if (xml.name()=="Label")
											{
												QString flag = xml.readElementText();
												if (flag=="Obsolete entity" || flag=="offline")
												{
													skip = true;
												}
											}
											else xml.skipCurrentElement();
										}
									}
									else xml.skipCurrentElement();
								}
							}
							else xml.skipCurrentElement();
						}
						if (!skip)
						{
							out << "ADD" << endl;
						}
					}
					else xml.skipCurrentElement();
				}
			}
			else xml.skipCurrentElement();
		}
		if (xml.hasError())
		{
			THROW(FileParseException, "Error parsing XML file " + terms + ":\n" + xml.errorString());
		}
		fp->close();

		//output
		int c_disease = db.getValue("SELECT COUNT(*) FROM disease_term").toInt();
		out << "Imported " << c_disease << " diseases" << endl;

		//import disease-phenotype relations
		out << endl;
		out << "Importing OMIM gene-phenotype relations..." << endl;
		fp = Helper::openFileForReading(genes);
		//TODO
		fp->close();

		//output
		out << "Imported " << db.getValue("SELECT COUNT(*) FROM disease_gene").toInt() << " disease<>gene relations" << endl;

		//augment gene-phenotype infos im missing
		//TODO
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
