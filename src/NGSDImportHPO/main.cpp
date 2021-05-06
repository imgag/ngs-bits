#include "ToolBase.h"
#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include "OntologyTermCollection.h"

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
		addInfile("anno", "HPO annotations file from 'https://ci.monarchinitiative.org/view/hpo/job/hpo.annotations/lastSuccessfulBuild/artifact/rare-diseases/util/annotation/phenotype_to_genes.txt'", false);

		//optional
		addInfile("omim", "OMIM 'morbidmap.txt' file for additional disease-gene information, from 'https://omim.org/downloads/'.", true);
		addInfile("clinvar", "ClinVar VCF file for additional disease-gene information. Download and unzip from 'ftp://ftp.ncbi.nlm.nih.gov/pub/clinvar/vcf_GRCh37/archive_2.0/2021/clinvar_20210110.vcf.gz'.", true);
		addInfile("hgmd", "HGMD phenobase file (Manually download and unzip 'hgmd_phenbase-2020.4.dump').", true);
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");
		addFlag("debug", "Enables debug output");

		changeLog(2020, 7, 7, "Added support of HGMD gene-phenotype relations.");
		changeLog(2020, 3, 5, "Added support for new HPO annotation file.");
		changeLog(2020, 3, 9, "Added optimization for hpo-gene relations.");
		changeLog(2020, 3, 10, "Removed support for old HPO annotation file.");
		changeLog(2020, 7, 6, "Added support for HGMD phenobase file.");
	}

	int importTermGeneRelations(SqlQuery& qi_gene, const QHash<int, QSet<QByteArray> >& term2diseases, const QHash<QByteArray, GeneSet>& disease2genes)
	{
		int c_imported = 0;

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
					c_imported += qi_gene.numRowsAffected();
				}
			}
		}

		return c_imported;
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

		//parse OBO and insert terms into NGSD
		QHash<QByteArray, int> id2ngsd;
		OntologyTermCollection terms(getInfile("obo"), true);
		for (int i=0; i<terms.size(); ++i)
		{
			const OntologyTerm& term = terms.get(i);

			qi_term.bindValue(0, term.id());
			qi_term.bindValue(1, term.name());
			qi_term.bindValue(2, term.definition());
			qi_term.bindValue(3, term.synonyms().count()==0 ? "" : term.synonyms().join('\n'));
			qi_term.exec();

			id2ngsd.insert(term.id(), qi_term.lastInsertId().toInt());
		}
		out << "Imported " << db.getValue("SELECT COUNT(*) FROM hpo_term").toInt() << " non-obsolete HPO terms." << endl;

		//insert parent-child relations between (valid) terms
		for (int i=0; i<terms.size(); ++i)
		{
			const OntologyTerm& term = terms.get(i);

			int c_db = id2ngsd.value(term.id(), -1);
			if (c_db==-1) continue;

			foreach(const QByteArray& p_id, term.parentIDs())
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
		QSharedPointer<QFile> fp = Helper::openFileForReading(getInfile("anno"));
		QSet<QByteArray> non_hgnc_genes;
		PhenotypeList inheritance_terms = db.phenotypeChildTerms(db.phenotypeIdByAccession("HP:0000005"), true); //Mode of inheritance

		QHash<int, QSet<QByteArray> > term2diseases;
		QHash<QByteArray, GeneSet> disease2genes;
		while(!fp->atEnd())
		{
			QByteArray line =  fp->readLine();
			QByteArrayList parts =line.split('\t');

			if (parts.count()<7) continue;

			// parse line
			QByteArray disease = parts[6].trimmed();
			QByteArray gene = parts[3].trimmed();
			QByteArray term_accession = parts[0].trimmed();

			int gene_db_id = db.geneToApprovedID(gene);
			int term_db_id = id2ngsd.value(term_accession, -1);

			if (term_db_id!=-1)
			{
				if (inheritance_terms.containsAccession(term_accession))
				{
					if (gene_db_id!=-1)
					{
						if (debug) out << "HPO-GENE: " << term_accession << " - " << gene << endl;
						qi_gene.bindValue(0, term_db_id);
						qi_gene.bindValue(1, db.geneSymbol(gene_db_id));
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

		int c_imported = importTermGeneRelations(qi_gene, term2diseases, disease2genes);
		out << "Imported " << c_imported << " term-gene relations from HPO." << endl;

		//parse disease-gene relations from OMIM
		QString omim_file = getInfile("omim");
		disease2genes.clear();
		if (omim_file!="")
		{
			//parse disease-gene relations
			int c_skipped_invalid_gene = 0;
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

				foreach(QByteArray gene, genes)
				{
					//make sure the gene symbol is approved by HGNC
					gene = gene.trimmed();
					int approved_id = db.geneToApprovedID(gene);
					if (approved_id==-1)
					{
						if (debug) out << "Skipped gene '" << gene << "' because it is not an approved HGNC symbol!" << endl;
						++c_skipped_invalid_gene;
						continue;
					}

					if (debug) out << "DISEASE-GENE (OMIM): OMIM:" << mim_number << " - " << db.geneSymbol(approved_id) << endl;

					disease2genes["OMIM:"+mim_number].insert(db.geneSymbol(approved_id));
				}
			}
			fp->close();

			int c_imported = importTermGeneRelations(qi_gene, term2diseases, disease2genes);
			out << "Imported " << c_imported << " additional term-gene relations (via disease) from OMIM." << endl;
			out << "  Skipped " << c_skipped_invalid_gene << " genes (no HGNC-approved gene name)." << endl;
		}


		//parse disease-gene relations from ClinVar
		QString clinvar_file = getInfile("clinvar");
		disease2genes.clear();
		QHash<QByteArray, GeneSet> hpo2genes;
		if (clinvar_file!="")
		{
			//parse disease-gene relations
			int c_skipped_invalid_gene = 0;
			fp = Helper::openFileForReading(clinvar_file);
			while(!fp->atEnd())
			{
				QByteArray line = fp->readLine().trimmed();
				if (!line.contains("CLNSIG=Pathogenic") && !line.contains("CLNSIG=Likely_pathogenic")) continue;

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
					if (approved_id==-1)
					{
						if (debug) out << "Skipped gene '" << gene << "' because it is not an approved HGNC symbol!" << endl;
						++c_skipped_invalid_gene;
						continue;
					}
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

			int c_imported = importTermGeneRelations(qi_gene, term2diseases, disease2genes);
			out << "Imported " << c_imported << " additional term-gene relations (via disease) from ClinVar." << endl;
			out << "  Skipped " << c_skipped_invalid_gene << " genes (no HGNC-approved gene name)." << endl;

			//import hpo-gene regions (from ClinVar)
			c_imported = 0;
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
						if(qi_gene.numRowsAffected() > 0) ++c_imported;
					}
				}
			}
			out << "Imported " << c_imported << " additional term-gene relations (direct) from ClinVar." << endl;
		}


		// parse hpo-gene relations from HGMD (Phenobase dbdump file):
		QString hgmd_file = getInfile("hgmd");
		if(hgmd_file != "")
		{
			if (debug) out << "Parsing HGMD Phenobase dump file...\n" << endl;
			// define look-up tables
			QMultiMap<int, QByteArray> phenid2gene_mapping = QMap<int, QByteArray>();
			QMultiMap<QByteArray,int> cui2phenid_mapping = QMap<QByteArray,int>();
			QMultiMap<QByteArray,QByteArray> hpo2cui_mapping = QMap<QByteArray,QByteArray>();
			QHash<QByteArray, GeneSet> hpo2genes;

			QSharedPointer<QFile> fp = Helper::openFileForReading(hgmd_file);
			int line_number = 0;
			while(!fp->atEnd())
			{
				line_number++;
				// show progress
				if(debug && line_number%100 == 0) out << "\tparsed " << line_number << " lines..." << endl;
				QByteArray line = fp->readLine().trimmed();
				if (line.isEmpty()) continue;

				// parse concept table
				if(line.startsWith("INSERT INTO `concept` VALUES "))
				{
				   // parse insert line
				   QString value_string = line.mid(31);
				   value_string.chop(3);
				   QStringList tuples = value_string.split("'),('");

				   // parse tuples
				   foreach(const QString& tuple, tuples)
				   {
					   QStringList tuple_entries = tuple.split("','");

					   // check if tuple size is correct
					   if(tuple_entries.size() != 10)
					   {
						   THROW(FileParseException, "Invalid number of columns in INSERT Statement in line " + QString::number(line_number) + "\n" + tuple);
					   }

					   // ignore all non HPO entries:
					   if(tuple_entries.at(2).trimmed() != "HPO") continue;

					   QByteArray cui = tuple_entries.at(0).toUtf8();
					   QByteArray hpo = tuple_entries.at(3).toUtf8();

					   // skip already stored realations (due to old/different description/synonyms)
					   if(hpo2cui_mapping.contains(hpo, cui)) continue;

					   hpo2cui_mapping.insert(hpo, cui);
				   }
				}
				else if(line.startsWith("INSERT INTO `hgmd_mutation` VALUES "))
				{
					// parse insert line
					QString value_string = line.mid(36);
					value_string.chop(2);
					QStringList tuples = value_string.split("),(");

					// parse tuples
					foreach(const QString& tuple, tuples)
					{
						QStringList tuple_entries = tuple.split(",");

						// check if tuple size is correct
						if(tuple_entries.size() != 3)
						{
							THROW(FileParseException, "Invalid number of columns in INSERT Statement in line " + QString::number(line_number) + "\n" + tuple);
						}

						//parse gene name
						QByteArray gene_name = tuple_entries.at(1).toUtf8();
						int phen_id = Helper::toInt(tuple_entries.at(2).toUtf8(), "phen_id", QString::number(line_number));
						//remove quotes
						gene_name.remove(0, 1);
						gene_name.remove((gene_name.size() - 1), 1);

						// skip already stored mappings to save memory
						if(phenid2gene_mapping.contains(phen_id, gene_name)) continue;

						phenid2gene_mapping.insert(phen_id, gene_name);
					}
				}
				else if(line.startsWith("INSERT INTO `phenotype_concept` VALUES "))
				{
					// parse insert line
					QString value_string = line.mid(40);
					value_string.chop(2);
					QStringList tuples = value_string.split("),(");

					// parse tuples
					foreach(const QString& tuple, tuples)
					{
						QStringList tuple_entries = tuple.split(",");

						// check if tuple size is correct
						if(tuple_entries.size() != 3)
						{
							THROW(FileParseException, "Invalid number of columns in INSERT Statement in line " + QString::number(line_number) + "\n" + tuple);
						}

						//parse cui
						QByteArray cui = tuple_entries.at(2).toUtf8();
						int phen_id = Helper::toInt(tuple_entries.at(0).toUtf8(), "phen_id", QString::number(line_number));
						//remove quotes
						cui.remove(0, 1);
						cui.remove((cui.size() - 1), 1);

						// skip already stored mappings to save memory
						if(cui2phenid_mapping.contains(cui, phen_id)) continue;

						cui2phenid_mapping.insert(cui, phen_id);
					}
				}
			}


			// create hpo-gene relation
			if (debug) out << "Creating HPO-gene relation from HGMD file..." << endl;

			int hgmd_skipped_invalid_gene = 0;
			int hgmd_genes_added = 0;
			int i = 0;
			foreach (const QByteArray& hpo, hpo2cui_mapping.keys())
			{
				i++;

				// get the corresponding cui for each hpo term:
				foreach(const QByteArray& cui, hpo2cui_mapping.values(hpo))
				{
					// get all phenotype ids for given cui
					if(!cui2phenid_mapping.contains(cui))
					{
					   if (debug) out << "No phenotype id found for CUI '" << cui << "' (HGMD file)!" << endl;
					   continue;
					}
					foreach(int phen_id, cui2phenid_mapping.values(cui))
					{
						// get all genes for a given phenotype id
						if(!phenid2gene_mapping.contains(phen_id))
						{
							if (debug) out << "No gene found for phenotype id " << QByteArray::number(phen_id) << " (HGMD file)!" << endl;
							continue;
						}
						foreach(const QByteArray& gene, phenid2gene_mapping.values(phen_id))
						{
							//make sure the gene symbol is approved by HGNC
							int approved_id = db.geneToApprovedID(gene);
							if (approved_id==-1)
							{
									if (debug) out << "Skipped gene '" << gene << "' because it is not an approved HGNC symbol (HGMD file)!" << endl;
									++hgmd_skipped_invalid_gene;
									continue;
							}
							QByteArray gene_approved = db.geneSymbol(approved_id);

							// add gene to hpo list:
							if (debug) out << "HPO-GENE (HGMD): " << hpo << " - " << gene_approved << endl;
							hgmd_genes_added++;
							hpo2genes[hpo].insert(gene_approved);
						}
					}
				}

				// show progress
				if(debug && (i%1000 == 0))
				{
					out << "\t" << i << " of " << hpo2cui_mapping.keys().size() << "hpo terms parsed \n";
				}
			}

			if (debug) out << "Importing HPO-gene relation from HGMD into the NGSD..." << endl;

			int hgmd_imported = 0;
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
						if(qi_gene.numRowsAffected() > 0) ++hgmd_imported;
					}
				}
			}
			out << "Imported " << hgmd_imported << " additional term-gene relations from HGMD." << endl;
			out << "  Skipped " << hgmd_skipped_invalid_gene << " genes (no HGNC-approved gene name)." << endl;
		}


		out << "Overall imported term-gene relations: " << db.getValue("SELECT COUNT(*) FROM hpo_genes").toInt() << endl;

		out << "Optimizing term-gene relations...\n";
		out << "(removing genes which are present in leaf nodes from parent node)" << endl;

		Phenotype root = Phenotype("HP:0000001", "All");
		int removed_genes = 0;
		optimizeHpoGeneTable(root, db, id2ngsd, removed_genes);

		// compute import stats

		// get first level of subtrees:
		PhenotypeList subtree_roots = db.phenotypeChildTerms(db.phenotypeIdByAccession(root.accession()), false);
		QList<PhenotypeList> subtrees;
		foreach (const Phenotype& pt, subtree_roots)
		{
			subtrees.append(db.phenotypeChildTerms(db.phenotypeIdByAccession(pt.accession()), true));
		}
		QVector<int> subtree_counts(subtree_roots.count());

		//calulate stats:
		QStringList hpo_terms = db.getValues("SELECT ht.hpo_id, hg.gene FROM hpo_genes hg INNER JOIN hpo_term ht ON hg.hpo_term_id = ht.id");
		foreach (const QString& hpo_term, hpo_terms)
		{
			QByteArray pt = hpo_term.toUtf8();
			for (int i = 0; i < subtree_roots.count(); ++i)
			{
				if (subtrees[i].containsAccession(pt)) subtree_counts[i]++;
			}
		}

		out << "Imported HPO-Gene relations: \n";
		out << " Overall:\t" << hpo_terms.size() << "\n";
		for (int i = 0; i < subtree_roots.count(); ++i)
		{
			out << " " << subtree_roots[i].name() << ":\t" << subtree_counts.at(i) << "\n";
		}

		out << removed_genes << " duplicate genes removed during optimization" << endl;
	}

	void optimizeHpoGeneTable(const Phenotype& root, NGSD& db, const QHash<QByteArray, int>& pt2id, int& removed_genes)
	{
		// get all child nodes
		int root_id = db.phenotypeIdByAccession(root.accession());
		PhenotypeList children = db.phenotypeChildTerms(root_id, false);

		// abort if leaf node
		if (children.count() == 0) return;

		// get all genes which are associated with the sub-trees
		GeneSet genes_children;
		foreach (const Phenotype& child, children)
		{
			genes_children.insert(db.phenotypeToGenes(db.phenotypeIdByAccession(child.accession()), true, false));
		}

		// intersect with genes present in the root node
		GeneSet genes_to_remove = genes_children.intersect(db.phenotypeToGenes(root_id, false, false));

		if (genes_to_remove.count() != 0)
		{
			// get phenotype id
			int pt_id = pt2id.value(root.accession());

			// remove all duplicate genes from parent node
			SqlQuery remove_gene_query = db.getQuery();
			remove_gene_query.prepare("DELETE FROM hpo_genes WHERE hpo_term_id=" + QByteArray::number(pt_id) + " AND gene=:0");
			foreach (const QByteArray& gene, genes_to_remove)
			{
				remove_gene_query.bindValue(0, gene);
				remove_gene_query.exec();
				removed_genes++;
			}
		}

		// start optimization for all child nodes
		foreach (const Phenotype& child, children)
		{
			optimizeHpoGeneTable(child, db, pt2id, removed_genes);
		}

		return;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
