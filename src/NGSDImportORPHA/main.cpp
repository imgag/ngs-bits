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

	QHash<QString, GeneSet> parseDiseaseGeneRelations(NGSD& db, QTextStream& out)
	{
		QHash<QString, GeneSet> output;

		GeneSet approved_genes = db.approvedGeneNames();

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
							if (xml.name()=="OrphaNumber")
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
														QByteArray gene = xml.readElementText().toLatin1();
														if (!approved_genes.contains(gene))
														{
															output[number] << gene;
														}
														else
														{
															out << "Warning: Skipping non-approved gene name '" << gene << "' for term '" << number << "'!" << endl;
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
				db.clearTable("disease_term");
				db.clearTable("disease_gene");
			}
			else
			{
				THROW(DatabaseException, "Tables already contain data! Use '-force' to overwrite old data!");
			}
		}

		//parse disease-gene relation
		out << "Parsing gene-disease relations..." << endl;
		QHash<QString, GeneSet> disease_genes = parseDiseaseGeneRelations(db, out);

		//prepare SQL queries
		SqlQuery qi_disease = db.getQuery();
		qi_disease.prepare("INSERT INTO disease_term (source, identifier, name, synonyms) VALUES ('OrphaNet', :0, :1, :2)");
		SqlQuery qi_gene = db.getQuery();
		qi_gene.prepare("INSERT INTO disease_gene (disease_term_id, gene) VALUES (:0, :1)");

		//import disease terms and genes
		out << "Importing ORPHA information..." << endl;
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
								if (xml.name()=="OrphaNumber")
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
								out << number << "\t" << name << "\t" << types.join(", ") << "\t" << disease_genes[number].count() << endl;
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
		out << "Imported " << c_disease << " diseases" << endl;
		int c_disease_gene = db.getValue("SELECT COUNT(*) FROM disease_gene").toInt();
		out << "Imported " << c_disease_gene << " disease-gene relations" << endl;

		//augment gene-phenotype infos if missing
		//TODO
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
