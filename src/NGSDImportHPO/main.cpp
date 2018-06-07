#include "ToolBase.h"
#include "NGSD.h"
#include "Exceptions.h"
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
		setDescription("Imports HPO terms and gene-phenotype relations into the NGSD.");
		addInfile("obo", "HPO ontology file from 'http://purl.obolibrary.org/obo/hp.obo'.", false);
		addInfile("anno", "HPO annotations file from 'http://compbio.charite.de/jenkins/job/hpo.annotations.monthly/lastStableBuild/artifact/annotation/ALL_SOURCES_ALL_FREQUENCIES_diseases_to_genes_to_phenotypes.txt'", false);

		//optional
		addInfile("omim", "OMIM 'morbidmap.txt' file for additional disease-gene information, from 'https://omim.org/downloads/'.", true);
		addInfile("clinvar", "ClinVar VCF file for additional disease-gene information, from 'ftp://ftp.ncbi.nlm.nih.gov/pub/clinvar/vcf_GRCh37/archive_2.0/2018/clinvar_20171231.vcf.gz'.", true);
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");
		addFlag("debug", "Enables debug output");
	}

	void importTermGeneRelations(SqlQuery& qi_gene, const QHash<int, QSet<QByteArray> >& term2diseases, const QHash<QByteArray, GeneSet>& disease2genes)
	{
		for (auto it=term2diseases.begin(); it!=term2diseases.end(); ++it)
		{
			int term_id = it.key();
			const QSet<QByteArray>& diseases = it.value();
			foreach(const QByteArray& disease, diseases)
			{
				GeneSet genes = disease2genes[disease];

				foreach(const QByteArray& gene, genes)
				{
					qi_gene.bindValue(0, term_id);
					qi_gene.bindValue(1, gene);
					qi_gene.exec();
				}
			}
		}
	}

	QHash<QByteArray, int> addTerm(SqlQuery& query, QByteArray& id, QByteArray& name, QByteArray& def, QByteArrayList& synonyms)
	{
		static QHash<QByteArray, int> id2ngsd;

		//add non-obsolete terms
		if (!id.isEmpty() && !name.startsWith("obsolete "))
		{
			query.bindValue(0, id);
			query.bindValue(1, name);
			query.bindValue(2, def);
			query.bindValue(3, synonyms.count()==0 ? "" : synonyms.join('\n'));
			query.exec();

			id2ngsd.insert(id, query.lastInsertId().toInt());
		}

		//clear data
		id.clear();
		name.clear();
		def = "";
		synonyms.clear();

		return id2ngsd;
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QTextStream out(stdout);
		bool debug = getFlag("debug");

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
		qi_term.prepare("INSERT INTO hpo_term (hpo_id, name, definition, synonyms) VALUES (:0, :1, :2, :3);");
		SqlQuery qi_parent = db.getQuery();
		qi_parent.prepare("INSERT INTO hpo_parent (parent, child) VALUES (:0, :1);");
		SqlQuery qi_gene = db.getQuery();
		qi_gene.prepare("INSERT IGNORE INTO hpo_genes (hpo_term_id, gene) VALUES (:0, :1);");

		//parse OBO and insert terms
		QByteArray id, name, def = "";
		QByteArrayList synonyms;
		QHash<QByteArray, QByteArrayList > child_parents;
		QSharedPointer<QFile> fp = Helper::openFileForReading(getInfile("obo"));
		while(!fp->atEnd())
		{
			QByteArray line = fp->readLine().trimmed();
			if (line.isEmpty()) continue;

			if (line=="[Term]")
			{
				addTerm(qi_term, id, name, def, synonyms);
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
			else if (line.startsWith("synonym: ") && line.contains("EXACT"))
			{
				int start = line.indexOf('"');
				int end = line.indexOf('"', start+1);
				QByteArray syn = line.mid(start+1, end-start-1).trimmed();
				qDebug() << syn;
				synonyms << syn;
			}
			else if (line.startsWith("is_a: HP:"))
			{
				if (!child_parents.contains(id)) child_parents[id] = QByteArrayList();
				child_parents[id].append(line.mid(6, 10));
			}
		}
		fp->close();

		QHash<QByteArray, int> id2ngsd = addTerm(qi_term, id, name, def, synonyms);
		out << "Imported " << db.getValue("SELECT COUNT(*) FROM hpo_term").toInt() << " non-obsolete HPO terms." << endl;

		//insert parent-child relations between (valid) terms
		for(auto it=child_parents.begin(); it!=child_parents.end(); ++it)
		{
			QByteArray c_id = it.key();
			int c_db = id2ngsd.value(c_id, -1);
			if (c_db==-1) continue;

			QByteArrayList parent_ids = it.value();
			foreach(const QByteArray& p_id, parent_ids)
			{
				int p_db = id2ngsd.value(p_id, -1);
				if (p_db==-1) continue;

				qi_parent.bindValue(0, p_db);
				qi_parent.bindValue(1, c_db);
				qi_parent.exec();
			}
		}
		out << "Imported " << db.getValue("SELECT COUNT(*) FROM hpo_parent").toInt() << " parent-child relations between terms." << endl;


		//parse term-disease and disease-gene relations from HPO
		fp = Helper::openFileForReading(getInfile("anno"));
		QSet<QByteArray> non_hgnc_genes;
		QList<Phenotype> inheritance_terms = db.phenotypeChildTems(Phenotype("HP:0000005", "Mode of inheritance"), true);
		QHash<int, QSet<QByteArray> > term2diseases;
		QHash<QByteArray, GeneSet> disease2genes;
		while(!fp->atEnd())
		{
			QByteArrayList parts = fp->readLine().split('\t');
			if (parts.count()<5) continue;

			QByteArray disease = parts[0].trimmed();
			QByteArray gene = parts[1].trimmed();
			QByteArray term_accession = parts[3].trimmed();

			int gene_db_id = db.geneToApprovedID(gene);
			int term_db_id = id2ngsd.value(term_accession, -1);

			if (term_db_id!=-1)
			{
				if (inheritance_terms.contains(Phenotype(term_accession, "")))
				{
					if (gene_db_id!=-1)
					{
						if (debug) out << "HPO-GENE: " << term_accession << " - " << gene << endl;
						qi_gene.bindValue(0, term_db_id);
						qi_gene.bindValue(1, gene);
						qi_gene.exec();
					}
				}
				else
				{
					if (debug) out << "HPO-DISEASE: " << term_accession << " - " << disease << endl;
					term2diseases[term_db_id].insert(disease);
				}
			}

			if (gene_db_id!=-1)
			{
				if (debug) out << "DISEASE-GENE (HPO): " << disease << " - " << db.geneSymbol(gene_db_id) << endl;
				disease2genes[disease].insert(db.geneSymbol(gene_db_id));
			}
			else
			{
				non_hgnc_genes << gene;
			}
		}
		fp->close();
		foreach(const QByteArray& gene, non_hgnc_genes)
		{
			out << "Skipped gene '" << gene << "' because it is not an approved HGNC symbol!" << endl;
		}

		importTermGeneRelations(qi_gene, term2diseases, disease2genes);
		out << "Imported term-gene relations from HPO. Overall count: " << db.getValue("SELECT COUNT(*) FROM hpo_genes").toInt() << endl;

		//parse disease-gene relations from OMIM
		QString omim_file = getInfile("omim");
		disease2genes.clear();
		if (omim_file!="")
		{
			//parse disease-gene relations
			fp = Helper::openFileForReading(omim_file);
			QRegExp mim_exp("([0-9]{6})");
			while(!fp->atEnd())
			{
				QByteArrayList parts = fp->readLine().trimmed().split('\t');
				if (parts.count()<4) continue;

				QByteArray pheno = parts[0].trimmed();
				QByteArrayList genes = parts[1].split(',');
				QByteArray mim_number = parts[2].trimmed();

				if (mim_exp.indexIn(pheno)!=-1)
				{
					mim_number = mim_exp.cap().toLatin1();
				}

				foreach(const QByteArray& gene, genes)
				{
					//make sure the gene symbol is approved by HGNC
					int approved_id = db.geneToApprovedID(gene);
					if (approved_id==-1) continue;

					if (debug) out << "DISEASE-GENE (OMIM): OMIM:" << mim_number << " - " << db.geneSymbol(approved_id) << endl;

					disease2genes["OMIM:"+mim_number].insert(db.geneSymbol(approved_id));
				}
			}
			fp->close();

			importTermGeneRelations(qi_gene, term2diseases, disease2genes);
			out << "Imported term-gene relations (via disease) from OMIM. Overall count: " << db.getValue("SELECT COUNT(*) FROM hpo_genes").toInt() << endl;
		}


		//parse disease-gene relations from ClinVar
		QString clinvar_file = getInfile("clinvar");
		disease2genes.clear();
		QHash<QByteArray, GeneSet> hpo2genes;
		if (clinvar_file!="")
		{
			//parse disease-gene relations
			fp = Helper::openFileForReading(clinvar_file);
			while(!fp->atEnd())
			{
				QByteArray line = fp->readLine().trimmed();
				if (!line.contains("Pathogenic") && !line.contains("Likely_pathogenic")) continue;

				QByteArrayList parts = line.split('\t');
				if (parts.count()<8) continue;

				//parse gene/disease info from from INFO field
				GeneSet genes;
				QByteArrayList diseases;
				QByteArrayList hpos;
				QByteArrayList info_parts = parts[7].split(';');
				foreach(const QByteArray& part, info_parts)
				{
					if (part.startsWith("GENEINFO="))
					{
						QByteArrayList geneinfo_parts = part.mid(9).split('|');
						foreach(QByteArray geneinfo, geneinfo_parts)
						{
							int colon_idx = geneinfo.indexOf(':');
							if (colon_idx==-1) continue;
							genes << geneinfo.left(colon_idx);
						}
					}
					if (part.startsWith("CLNDISDB=") || part.startsWith("CLNDISDBINCL="))
					{
						QByteArrayList disease_parts = part.mid(part.indexOf('=')+1).replace(',', '|').split('|');
						foreach(QByteArray disease_part, disease_parts)
						{
							if (disease_part.startsWith("OMIM:"))
							{
								diseases << disease_part.trimmed();
							}
							else if (disease_part.startsWith("Orphanet:ORPHA"))
							{
								diseases << disease_part.replace("Orphanet:ORPHA", "ORPHA:").trimmed();
							}
							else if (disease_part.startsWith("Human_Phenotype_Ontology:"))
							{
								hpos << disease_part.replace("Human_Phenotype_Ontology:", "").trimmed();
							}
						}
					}
				}
				if (genes.isEmpty() || (diseases.isEmpty() && hpos.isEmpty())) continue;

				foreach(const QByteArray& gene, genes)
				{
					//make sure the gene symbol is approved by HGNC
					int approved_id = db.geneToApprovedID(gene);
					if (approved_id==-1) continue;
					QByteArray gene_approved = db.geneSymbol(approved_id);

					foreach(const QByteArray& disease, diseases)
					{
						if (debug) out << "DISEASE-GENE (ClinVar): " << disease << " - " << gene_approved << endl;
						disease2genes[disease].insert(gene_approved);
					}
					foreach(const QByteArray& hpo, hpos)
					{
						if (debug) out << "HPO-GENE (ClinVar): " << hpo << " - " << gene_approved << endl;
						hpo2genes[hpo].insert(gene_approved);
					}
				}
			}
			fp->close();

			importTermGeneRelations(qi_gene, term2diseases, disease2genes);
			out << "Imported term-gene relations (via disease) from ClinVar. Overall count: " << db.getValue("SELECT COUNT(*) FROM hpo_genes").toInt() << endl;

			//import hpo-gene regions (from ClinVar)
			for(auto it = hpo2genes.begin(); it!=hpo2genes.end(); ++it)
			{
				int term_db_id = id2ngsd.value(it.key(), -1);
				if (term_db_id!=-1)
				{
					const GeneSet& genes = it.value();
					foreach(const QByteArray& gene, genes)
					{
						qi_gene.bindValue(0, term_db_id);
						qi_gene.bindValue(1, gene);
						qi_gene.exec();
					}
				}
			}
			out << "Imported term-gene relations (direct) from ClinVar. Count: " << db.getValue("SELECT COUNT(*) FROM hpo_genes").toInt() << endl;
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
