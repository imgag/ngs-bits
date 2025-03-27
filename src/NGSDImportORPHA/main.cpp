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
		addInfile("terms", "Terms XML file from 'http://www.orphadata.com/data/xml/en_product1.xml'.", false);
		addInfile("genes", "Terms<>genes XML file from 'http://www.orphadata.com/data/xml/en_product6.xml'.", false);
		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");

		//changelog
		changeLog(2019,  9, 30, "Initial version");
	}

	QHash<QString, GeneSet> parseDiseaseGeneRelations(NGSD& db, QTextStream& out)
	{
		QHash<QString, GeneSet> output;

		const GeneSet& approved_genes = db.approvedGeneNames();

		QString filename = getInfile("genes");
		QSharedPointer<QFile> fp = Helper::openFileForReading(filename);

		QXmlStreamReader xml(fp.data());
		xml.readNextStartElement(); //root element JDBOR
		while (xml.readNextStartElement())
		{
			if (xml.name()=="DisorderList")
			{
				while (xml.readNextStartElement())
				{
					if (xml.name()=="Disorder")
					{
						QString number;
						while (xml.readNextStartElement())
						{
							if (xml.name()=="OrphaCode")
							{
								number = "ORPHA:" + xml.readElementText();
							}
							else if (xml.name()=="DisorderGeneAssociationList")
							{
								while (xml.readNextStartElement())
								{
									if (xml.name()=="DisorderGeneAssociation")
									{
										while (xml.readNextStartElement())
										{
											if (xml.name()=="Gene")
											{
												while (xml.readNextStartElement())
												{
													if (xml.name()=="Symbol")
													{
														QByteArray gene = xml.readElementText().toUtf8();
														gene = db.geneToApproved(gene, true);
														if (approved_genes.contains(gene))
														{
															output[number] << gene;
														}
														else
														{
                                                            out << "Warning: Skipping non-approved gene name '" << gene << "' for term '" << number << "'!" << QT_ENDL;
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
							}
							else xml.skipCurrentElement();
						}
					}
					else xml.skipCurrentElement();
				}
			}
			else xml.skipCurrentElement();
		}
		if (xml.hasError())
		{
			THROW(FileParseException, "Error parsing XML file " + filename + ":\n" + xml.errorString());
		}
		fp->close();

		return output;
	}

	virtual void main()
	{
		//init
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
				db.clearTable("disease_gene");
				db.clearTable("disease_term");
			}
			else
			{
				THROW(DatabaseException, "Tables already contain data! Use '-force' to overwrite old data!");
			}
		}

		//parse disease-gene relation
        out << "Parsing gene-disease relations..." << QT_ENDL;
		QHash<QString, GeneSet> disease_genes = parseDiseaseGeneRelations(db, out);

		//prepare SQL queries
		SqlQuery qi_disease = db.getQuery();
		qi_disease.prepare("INSERT INTO disease_term (source, identifier, name, synonyms) VALUES ('OrphaNet', :0, :1, :2)");
		SqlQuery qi_gene = db.getQuery();
		qi_gene.prepare("INSERT INTO disease_gene (disease_term_id, gene) VALUES (:0, :1)");

		//import disease terms and genes
        out << "Importing ORPHA information..." << QT_ENDL;
		{
			QString terms = getInfile("terms");
			QSharedPointer<QFile> fp = Helper::openFileForReading(terms);
			QXmlStreamReader xml(fp.data());
			xml.readNextStartElement(); //root element JDBOR
			while (xml.readNextStartElement())
			{
				if (xml.name()=="DisorderList")
				{
					while (xml.readNextStartElement())
					{
						if (xml.name()=="Disorder")
						{
							//parse disease entry
							QString number;
							QString name;
							QStringList types;
							QStringList synonyms;
							bool skip = false;
							while (xml.readNextStartElement())
							{
								if (xml.name()=="OrphaCode")
								{
									number = "ORPHA:" + xml.readElementText();
								}
								else if (xml.name()=="Name")
								{
									name = xml.readElementText();
								}
								else if (xml.name()=="SynonymList")
								{
									while (xml.readNextStartElement())
									{
										if (xml.name()=="Synonym")
										{
											synonyms << xml.readElementText();
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
								else if (xml.name()=="DisorderType")
								{
									while (xml.readNextStartElement())
									{
										if (xml.name()=="Name")
										{
											types << xml.readElementText();
										}
										else xml.skipCurrentElement();
									}
								}
								else xml.skipCurrentElement();
							}

							//insert
							if (!skip)
							{
								//insert disease
								qi_disease.bindValue(0, number);
								qi_disease.bindValue(1, name);
								qi_disease.bindValue(2, synonyms.join("\n"));
								qi_disease.exec();
								qlonglong id = qi_disease.lastInsertId().toLongLong();

								//insert disease-gene relation
								if (disease_genes.contains(number))
								{
									foreach(const QByteArray& gene, disease_genes[number])
									{
										qi_gene.bindValue(0, id);
										qi_gene.bindValue(1, gene);
										qi_gene.exec();
									}
								}
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
		}

		//output
		int c_disease = db.getValue("SELECT COUNT(*) FROM disease_term").toInt();
        out << "Imported " << c_disease << " diseases" << QT_ENDL;
		int c_disease_gene = db.getValue("SELECT COUNT(*) FROM disease_gene").toInt();
        out << "Imported " << c_disease_gene << " disease-gene relations" << QT_ENDL;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
