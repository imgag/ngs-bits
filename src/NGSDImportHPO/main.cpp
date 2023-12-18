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
		addInfile("obo", "HPO ontology file from 'https://github.com/obophenotype/human-phenotype-ontology/releases/download/v2023-10-09/hp.obo'.", false);
		addInfile("anno", "HPO annotations file from 'https://github.com/obophenotype/human-phenotype-ontology/releases/download/v2023-10-09/phenotype_to_genes.txt'", false);

		//optional
		addInfile("omim", "OMIM 'morbidmap.txt' file for additional disease-gene information, from 'https://omim.org/downloads/'.", true);
		addInfile("clinvar", "ClinVar VCF file for additional disease-gene information. Download and unzip from 'http://ftp.ncbi.nlm.nih.gov/pub/clinvar/vcf_GRCh38/archive_2.0/2023/clinvar_20231121.vcf.gz'.", true);
		addInfile("hgmd", "HGMD phenobase file (Manually download and unzip 'hgmd_phenbase-2023.3.dump').", true);

		// optional (for evidence information):
		addInfile("hpophen", "HPO 'phenotype.hpoa' file for additional phenotype-disease evidence information. Download from wget https://github.com/obophenotype/human-phenotype-ontology/releases/download/v2023-10-09/phenotype.hpoa", true);
		addInfile("gencc", "gencc 'gencc-submissions.csv' file for additional disease-gene evidence information. Download from https://search.thegencc.org/download", true);
		addInfile("decipher", "G2P 'DDG2P.csv' file for additional gene-disease-phenotype evidence information. Download from https://www.deciphergenomics.org/about/downloads/data", true);

		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");
		addFlag("debug", "Enables debug output");

		changeLog(2021, 12, 22, "Added support for GenCC and DECIPHER.");
		changeLog(2020,  7,  7, "Added support of HGMD gene-phenotype relations.");
		changeLog(2020,  3,  5, "Added support for new HPO annotation file.");
		changeLog(2020,  3,  9, "Added optimization for hpo-gene relations.");
		changeLog(2020,  3, 10, "Removed support for old HPO annotation file.");
		changeLog(2020,  7,  6, "Added support for HGMD phenobase file.");
	}

	/// simple sruct to keep a set of source databases
	struct SourceDetails {
		QList<PhenotypeSource> sources;
		QStringList original_evidence;
		QList<PhenotypeEvidenceLevel> translated_evidence;

		SourceDetails()
		{
			sources = QList<PhenotypeSource>();
		}

		SourceDetails(const QByteArray& s, const QByteArray& original_evi, PhenotypeEvidenceLevel translated_evi=PhenotypeEvidenceLevel::NA)
		{
			sources = QList<PhenotypeSource>();
			original_evidence = QStringList();
			translated_evidence = QList<PhenotypeEvidenceLevel>();

			sources.append(Phenotype::sourceFromString(s));
			original_evidence.append(QString(original_evi));
			translated_evidence.append(translated_evi);
		}

		bool contains(const PhenotypeSource& s)
		{
			return sources.contains(s);
		}

		bool contains(const QByteArray& s)
		{
			return sources.contains(Phenotype::sourceFromString(s));
		}

		bool contains(const QString& s)
		{
			return sources.contains(Phenotype::sourceFromString(s));
		}

		int getIndexOfSource(const PhenotypeSource& s)
		{
			return sources.indexOf(s);
		}

		void append(const PhenotypeSource& s,const QString& original_evi, PhenotypeEvidenceLevel translated_evi)
		{
			sources.append(s);
			original_evidence.append(original_evi);
			translated_evidence.append(translated_evi);
		}

		void append(const QByteArray& s, const QString& original_evi, PhenotypeEvidenceLevel translated_evi)
		{
			sources.append(Phenotype::sourceFromString(s));
			original_evidence.append(original_evi);
			translated_evidence.append(translated_evi);
		}

		void unite(const SourceDetails& second)
		{
			for (int i=0; i<second.sources.count(); i++)
			{
				if (contains(second.sources[i]))
				{
					int idx = getIndexOfSource(second.sources[i]);
					if ((int) second.translated_evidence[i] > (int) translated_evidence[idx])
					{
						original_evidence[idx] = second.original_evidence[i];
						translated_evidence[idx] = second.translated_evidence[i];
					}
				}
				else
				{
					append(second.sources[i], second.original_evidence[i], second.translated_evidence[i]);
				}
			}
		}

		QString toCsvString() const
		{
			QString s = "";

			for (int i=0; i<sources.count(); i++)
			{
				if (i == 0)
				{
					s= "(" + Phenotype::sourceToString(sources[i]) + ", " + original_evidence[i] + ", " + Phenotype::evidenceToString(translated_evidence[i]) + ")";
				}
				else
				{
					s += "; (" + Phenotype::sourceToString(sources[i]) + ", " + original_evidence[i] + ", " + Phenotype::evidenceToString(translated_evidence[i]) + ")";
				}
			}
			return s;
		}
	};

	// struct used to find the first instance of where a relation was parsed from. Only used for debugging and testing
	struct ExactSources
	{
		QString disease2gene;
		QString term2disease;
		QString term2gene;

		ExactSources():
		disease2gene("")
		, term2disease("")
		, term2gene("")
		{
		}

		ExactSources(const QString& disease2phenotype, const QString& phenotype2gene, const QString& term2gene):
		disease2gene(disease2phenotype)
		, term2disease(phenotype2gene)
		, term2gene(term2gene)
		{
		}

		void combine(const ExactSources& src)
		{
			if (disease2gene.length() == 0)
			{
				disease2gene = src.disease2gene;
			}

			if (term2disease.length() == 0)
			{
				term2disease = src.term2disease;
			}

			if (term2gene.length() == 0)
			{
				term2gene = src.term2gene;
			}
		}

		QString toString() const
		{
			return QString("Exact Sources: disease2gene \t  '%1', \t term2disease \t '%2', \t term2gene \t '%3'").arg(disease2gene, term2disease, term2gene);
		}
	};

	// struct to annotate a gene/disease/phenotype with a source db and an evidence strength
	struct AnnotatedItem
	{
		QByteArray item;
		SourceDetails src;
		PhenotypeEvidenceLevel evi;
		ExactSources exactSources; // for debuging and testing

		AnnotatedItem()
		{
		}

		AnnotatedItem(const QByteArray& item, const QByteArray& s, const QByteArray& original_evi, PhenotypeEvidenceLevel evi):
			item(item)
		  , evi(evi)
		{
			src = SourceDetails(s, original_evi, evi);
		}

		AnnotatedItem(const QByteArray& item, SourceDetails src, PhenotypeEvidenceLevel evi):
			item(item)
		  , src(src)
		  , evi(evi)
		{
		}

		AnnotatedItem(const QByteArray& item, SourceDetails src, PhenotypeEvidenceLevel evi, ExactSources exactSources):
			item(item)
		  , src(src)
		  , evi(evi)
		  , exactSources(exactSources)
		{
		}

		bool operator==(const AnnotatedItem& other)
		{
			return item == other.item;
		}
	};

	/// a QList of AnnotatedItems with some convenience insertion methods
	class AnnotatedList
	{
		public:
			void add(const QByteArray& item, const QByteArray& source, const QByteArray& original_evi, PhenotypeEvidenceLevel evidence=PhenotypeEvidenceLevel::NA, ExactSources exactSource=ExactSources())
			{
				add(item, SourceDetails(source, original_evi, evidence), evidence, exactSource);
			}

			void add(const QByteArray& item, SourceDetails source, PhenotypeEvidenceLevel evidence=PhenotypeEvidenceLevel::NA, ExactSources exactSource=ExactSources())
			{
				if (hash.contains(item))
				{
					AnnotatedItem& present_item = hash[item];
					present_item.src.unite(source);
					if ((int) present_item.evi < (int) evidence)
					{
						present_item.evi = evidence;
						present_item.exactSources.combine(exactSource);
					}

				}
				else
				{
					hash.insert(item, AnnotatedItem(item, source, evidence, exactSource));
				}
			}

			QList<AnnotatedItem> items() const
			{
				return hash.values();
			}

		protected:
			QHash<QByteArray, AnnotatedItem> hash;

	};

	QHash<QByteArray, int> importHpoOntology(const NGSD& db)
	{
		QTextStream out(stdout);
		//prepare SQL queries
		SqlQuery qi_obs = db.getQuery();
		qi_obs.prepare("INSERT INTO hpo_obsolete (hpo_id, name, definition, replaced_by) VALUES (:0, :1, :2, :3);");
		SqlQuery qi_term = db.getQuery();
		qi_term.prepare("INSERT INTO hpo_term (hpo_id, name, definition, synonyms) VALUES (:0, :1, :2, :3);");
		SqlQuery qi_parent = db.getQuery();
		qi_parent.prepare("INSERT INTO hpo_parent (parent, child) VALUES (:0, :1);");

		QHash<QByteArray, int> id2ngsd;
		OntologyTermCollection terms(getInfile("obo"), false);
		for (int i=0; i<terms.size(); ++i)
		{
			const OntologyTerm& term = terms.get(i);
			if (term.isObsolete()) continue;

			qi_term.bindValue(0, term.id());
			qi_term.bindValue(1, term.name());
			qi_term.bindValue(2, term.definition());
			qi_term.bindValue(3, term.synonyms().count()==0 ? "" : term.synonyms().join('\n'));
			qi_term.exec();

			id2ngsd.insert(term.id(), qi_term.lastInsertId().toInt());
		}
		out << "Imported " << id2ngsd.count() << " non-obsolete HPO terms." << endl;

		//insert parent-child relations between (valid) terms
		int c_ins_parent = 0;
		for (int i=0; i<terms.size(); ++i)
		{
			const OntologyTerm& term = terms.get(i);
			if (term.isObsolete()) continue;

			int c_db = id2ngsd.value(term.id(), -1);
			if (c_db==-1) continue;

			foreach(const QByteArray& p_id, term.parentIDs())
			{
				int p_db = id2ngsd.value(p_id, -1);
				if (p_db==-1)
				{
					out << "Notice: Parent term '" << p_id << "' is not a valid term!" << endl;
					continue;
				}

				qi_parent.bindValue(0, p_db);
				qi_parent.bindValue(1, c_db);
				qi_parent.exec();

				++c_ins_parent;
			}
		}
		out << "Imported " << c_ins_parent << " parent-child relations between terms from HPO." << endl;

		//import obsolete terms and link to replacement terms
		int c_ins_obs = 0;
		int c_ins_obs_with_replace = 0;
		for (int i=0; i<terms.size(); ++i)
		{
			const OntologyTerm& term = terms.get(i);
			if (!term.isObsolete()) continue;

			QVariant replace_id_db;
			QByteArray replace_id = term.replacedById();
			if (!replace_id.isEmpty())
			{
				int ngsd_id = id2ngsd.value(replace_id, -1);
				if (ngsd_id==-1)
				{
					out << "Notice: Replacement term '" << replace_id << "' is not a valid term!" << endl;
					continue;
				}
				else
				{
					replace_id_db = ngsd_id;
				}
			}

			qi_obs.bindValue(0, term.id());
			qi_obs.bindValue(1, term.name());
			qi_obs.bindValue(2, term.definition());
			qi_obs.bindValue(3, replace_id_db);
			qi_obs.exec();

			++c_ins_obs;
			if (!replace_id_db.isNull()) ++c_ins_obs_with_replace;
		}
		out << "Imported " << c_ins_obs << " obsolete HPO terms (" << c_ins_obs_with_replace << " with replacement)." << endl;

		return id2ngsd;
	}

	void parseHpoPhen(QHash<QByteArray, int> id2ngsd, QHash<int, AnnotatedList>& term2diseases)
	{
		if (getInfile("hpophen") == "") return;

		int lineCount = 0;
		int added = 0;
		bool debug = getFlag("debug");

		QTextStream out(stdout);
		if (debug) out << "Starting analysis of hpophen file\n";

		// parse phenotype.hpoa file for evidence information
		QSharedPointer<QFile> fp = Helper::openFileForReading(getInfile("hpophen"));

		while(! fp->atEnd())
		{
			lineCount++;
			QByteArray line = fp->readLine();

			if (line.startsWith('#')) continue;

			QByteArrayList parts = line.split('\t');

			if (parts[2].length() > 0) continue; // Qualifier Not: term NOT associated to the disease

			QByteArray term = parts[3].trimmed();
			QByteArray disease = parts[4].trimmed();
			QByteArray evidence = parts[5].trimmed();

			int term_id = id2ngsd.value(term, -1);
			if (term_id == -1)
			{
				if (debug) out << "Term not found in id2ngsd: " << term << "\n";
				continue;
			}


			ExactSources e_src = ExactSources();
			e_src.term2disease = QString("hpoPhen line ") + QString::number(lineCount);
			term2diseases[term_id].add(disease, "HPO", evidence,  translateHpoEvidence(evidence), e_src);
			added++;

			if (debug) out << "Imported term2disease relation:\t" << term << "-" << disease << ":\t" << evidence << "\t fin_evi:\t" << Phenotype::evidenceToString(translateHpoEvidence(evidence)) << "\n";
		}

		out << "Imported " << added << " term-disease relations from HPO (hpophen).\n";
		fp->close();
	}

	void parseDecipher(NGSD& db, const QHash<QByteArray, int>& id2ngsd, QHash<QByteArray, AnnotatedList>& disease2genes, QHash<int, AnnotatedList>& term2diseases, QHash<int, AnnotatedList>& term2genes)
	{
		if (getInfile("decipher") == "") return;
		bool debug = getFlag("debug");
		QTextStream out(stdout);
		if (debug) out << "Parsing Decipher...\n";
		int countT2D = 0;
		int countD2G = 0;
		int countT2G = 0;

		QSharedPointer<QFile> fp = Helper::openFileForReading(getInfile("decipher"));

		QByteArray line = fp->readLine();
		//"gene symbol","gene mim","disease name","disease mim","confidence category","allelic requirement","mutation consequence",phenotypes,"organ specificity list",pmids,panel,"prev symbols","hgnc id","gene disease pair entry date","cross cutting modifier","mutation consequence flag"

		QList<QByteArray> non_hgc_genes;
		QSet<QByteArray> bad_hpo_terms;
		QByteArray source = "Decipher";
		int lineCount = 0;
		QRegExp mim_exp("([0-9]{6})");
		while(! fp->atEnd())
		{
			lineCount++;
			line = fp->readLine().trimmed();
			QByteArrayList parts = line.split(',');

			// merge parts of strings that contained commas:

			parts = reconstructStrings(parts);

			QByteArray gene = parts[0].trimmed();
			QByteArray disease_num = parts[3].trimmed();
			QByteArray disease = "OMIM:" + parts[3].trimmed();
			QByteArray decipher_evi = parts[4].trimmed();
			PhenotypeEvidenceLevel evidence = translateDecipherEvidence(decipher_evi);
			QByteArrayList hpo_terms = parts[7].trimmed().split(';');

			//verify information
			int gene_db_id = db.geneId(gene);

			if (gene_db_id == -1)
			{
				non_hgc_genes.append(gene);
				// add term2disease relations
				foreach (const QByteArray& term, hpo_terms)
				{
					int term_db_id = id2ngsd.value(term, -1);
					if (term_db_id != -1 && mim_exp.indexIn(disease_num) != -1)
					{
						ExactSources e_src = ExactSources();
						e_src.term2disease = QString("Decipher line") + QString::number(lineCount);
						term2diseases[term_db_id].add(disease, source, decipher_evi,  evidence, e_src);
						countT2D++;
						if (debug) out << "Deciper\tTERM2DISEASE\tTERM,DISEASE,GENE\t" << term << "\t" << disease << "\t''\t" << "\tSource:\t" << e_src.term2disease <<"\n";
					}
					else
					{
						bad_hpo_terms.insert(term);
					}
				}
			}
			else
			{
				QByteArray approved_gene_symbol = db.geneSymbol(gene_db_id);

				foreach (const QByteArray& term, hpo_terms)
				{
					int term_db_id = id2ngsd.value(term, -1);
					if (term_db_id != -1)
					{
						ExactSources e_src = ExactSources();
						e_src.term2gene = QString("Decipher line") + QString::number(lineCount);
						term2genes[term_db_id].add(approved_gene_symbol, source, decipher_evi, evidence, e_src);
						countT2G++;
						if (debug) out << "Deciper\tTERM2GENE\tTERM,DISEASE,GENE\t" << term << "\t''\t" << gene << "\tSource:\t" << e_src.term2gene << "\tapproved_gene_symbol:\t" << approved_gene_symbol << "\n";

						if (mim_exp.indexIn(disease_num) != -1)
						{
							e_src = ExactSources();
							e_src.term2disease = QString("Decipher line") + QString::number(lineCount);
							term2diseases[term_db_id].add(disease, source, decipher_evi, evidence, e_src);
							countT2D++;
							if (debug) out << "Deciper\tTERM2DISEASE\tTERM,DISEASE,GENE\t" << term << "\t" << disease << "\t''\t" << "\tSource:\t" << e_src.term2disease <<"\n";
						}

					}
					else
					{
						bad_hpo_terms.insert(term);
					}
				}
				if (mim_exp.indexIn(disease_num) != -1)
				{
					ExactSources e_src = ExactSources();
					e_src.term2disease = QString("Decipher line") + QString::number(lineCount);
					disease2genes[disease].add(approved_gene_symbol, source, decipher_evi, evidence, e_src);
					countD2G++;
					if (debug) out << "Deciper\tTERM2GENE\tTERM,DISEASE,GENE\t" << "''\t" << disease << "\t" << gene << "\tSource:\t" << e_src.term2gene << "\tapproved_gene_symbol:\t" << approved_gene_symbol << "\n";
				}
			}
		}
		fp->close();

		out << "Imported " << countD2G << " disease-gene relations, " << countT2D << " term-disease relations, " << countT2G << " term-gene relations from Decipher.\n";
	}

	void parseGenCC(NGSD& db, QHash<QByteArray, AnnotatedList>& disease2genes)
	{
		if (getInfile("gencc") == "") return;

		// parse gencc_submission.csv file for evidence information
		QTextStream out(stdout);
		QSharedPointer<QFile> fp = Helper::openFileForReading(getInfile("gencc"));
		QString line = fp->readLine(); // header
		QByteArray source = "GenCC";
		int c_imported = 0;
		int c_not_omim = 0;
		int c_invalid_gene = 0;
		int c_no_evidence = 0;
		int lineCount = 0;
		QString sep = "\",\"";
		while(! fp->atEnd())
		{
			//read line - some strings contain newlines...
			lineCount++;
			line = fp->readLine().trimmed();
			while (line.count(sep)<29)
			{
				lineCount++;
				line.append(fp->readLine().trimmed());
			}
			
			//split and check
			QStringList parts = line.split(sep);
			if (parts.count()!=30)
			{
				THROW(FileParseException, "GenCC line does not have 30 parts:\n" + line);
			}
			
			//only OMIM entries
			QByteArray disease = parts[5].trimmed().toLatin1(); // OMIM:XXXXXX, MONDO:XXXXXXX, Orphanet:XXXXX needs mapping from Orphanet and Mondo to Omim
			if (!disease.startsWith("OMIM:"))
			{
				//out << "OMIM:" << disease << endl;
				++c_not_omim;
				continue;
			}
			
			//parse gene
			QByteArray gene_symbol = parts[2].trimmed().toLatin1();
			int gene_db_id = db.geneId(gene_symbol);
			if (gene_db_id == -1)
			{
				//out << "GENE:" << gene_symbol << endl;
				++c_invalid_gene;
				continue;
			}
			
			//parse evidence level
			QByteArray gencc_evi = parts[8].trimmed().toLatin1();
			PhenotypeEvidenceLevel evidence = translateGenccEvidence(gencc_evi, line);
			if (evidence == PhenotypeEvidenceLevel::NA || evidence == PhenotypeEvidenceLevel::AGAINST)
			{
				//out << "EVIDENCE:" << gencc_evi << endl;
				++c_no_evidence;
				continue;
			}

			ExactSources e_src = ExactSources();
			e_src.disease2gene = QString("GenCC line") + QString::number(lineCount);
			disease2genes[disease].add(db.geneSymbol(gene_db_id), source, gencc_evi, evidence, e_src);
			c_imported++;
		}
		fp->close();

		out << "Imported " << c_imported << " disease-gene relations from GenCC" << endl;
		out << "  Skipped " << c_not_omim << " lines without OMIM term." << endl;
		out << "  Skipped " << c_invalid_gene << " lines without valid gene." << endl;
		out << "  Skipped " << c_no_evidence << " lines without evidence." << endl;
	}

	QByteArrayList reconstructStrings(const QByteArrayList& parts, int expected_size=-1)
	{
		// if parts size bigger than expected, try to reconstruct strings that were split:
		if (parts.length() > expected_size)
		{
			QByteArrayList cleaned_parts = QByteArrayList();

			for (int i=0; i<parts.length(); i++)
			{
				if (parts[i].startsWith('"') && ( ! parts[i].endsWith('"'))) // starts with " but doesn't end with "
				{
					QByteArray combined_part = parts[i];
					do
					{
						i++;
						combined_part.append(parts[i]);

					}
					while (! parts[i].endsWith('"'));

					cleaned_parts.append(combined_part);
				}
				else
				{
					cleaned_parts.append(parts[i]);
				}

			}
			return cleaned_parts;
		}

		return parts;
	}

	/// turns a given HPO Evidence value into one from the Evidences enum
	static PhenotypeEvidenceLevel translateHpoEvidence(const QString& hpo_evi)
	{
		//IEA (inferred from electronic annotation): Annotations extracted by parsing the Clinical Features sections of the Online Mendelian Inheritance in Man resource are assigned the evidence code “IEA”.
		//PCS (published clinical study) is used for used for information extracted from articles in the medical literature. Generally, annotations of this type will include the pubmed id of the published study in the DB_Reference field.
		//TAS (traceable author statement) is used for information gleaned from knowledge bases such as OMIM or Orphanet that have derived the information from a published source..
		if (hpo_evi == "IEA")
		{
			return PhenotypeEvidenceLevel::LOW;
		}
		else if (hpo_evi == "TAS")
		{
			return PhenotypeEvidenceLevel::MEDIUM;
		}
		else if (hpo_evi == "PCS")
		{
			return PhenotypeEvidenceLevel::HIGH;
		}
		else
		{
			THROW(ArgumentException, "Given Evidence is not a HPO evidence value: " + QString(hpo_evi));
		}
	}

	/// turns a given OMIM Evidence value into one from the Evidences enum
	static PhenotypeEvidenceLevel translateOmimEvidence(const QByteArray& omim_evi)
	{
			//# Phenotype Mapping key - Appears in parentheses after a disorder :
			//# -----------------------------------------------------------------
			//#
			//# 1 - The disorder is placed on the map based on its association with
			//# a gene, but the underlying defect is not known.
			//# 2 - The disorder has been placed on the map by linkage or other
			//# statistical method; no mutation has been found.
			//# 3 - The molecular basis for the disorder is known; a mutation has been
			//# found in the gene.
			//# 4 - A contiguous gene deletion or duplication syndrome, multiple genes
			//# are deleted or duplicated causing the phenotype.
		if (omim_evi == "(1)")
		{
			return PhenotypeEvidenceLevel::LOW;
		}
		else if (omim_evi == "(2)")
		{
			return PhenotypeEvidenceLevel::LOW;
		}
		else if (omim_evi == "(3)")
		{
			return PhenotypeEvidenceLevel::HIGH;
		}
		else if (omim_evi == "(4)")
		{
			return PhenotypeEvidenceLevel::HIGH;
		}
		else
		{
			THROW(ArgumentException, "Given Evidence is not a Omim evidence value: " + QString(omim_evi));
		}
	}
	/// turns a given Decipher Evidence value into one from the Evidences enum
	static PhenotypeEvidenceLevel translateDecipherEvidence(const QByteArray& decipher_evi)
	{
		//disease confidence: One value from the list of possible categories: both DD and IF, confirmed, possible, probable
		// Confirmed 	Plausible disease-causing mutations* within, affecting or encompassing an interpretable functional region** of a single gene identified in multiple (>3) unrelated cases/families with a developmental disorder***
		//				Plausible disease-causing mutations within, affecting or encompassing cis-regulatory elements convincingly affecting the expression of a single gene identified in multiple (>3) unrelated cases/families with a developmental disorder
		//				As definition 1 and 2 of Probable Gene (see below) with addition of convincing bioinformatic or functional evidence of causation e.g. known inborn error of metabolism with mutation in orthologous gene which is known to have the relevant deficient enzymatic activity in other species; existence of animal mode which recapitulates the human phenotype
		//   Probable 	Plausible disease-causing mutations within, affecting or encompassing an interpretable functional region of a single gene identified in more than one (2 or 3) unrelated cases/families or segregation within multiple individuals within a single large family with a developmental disorder
		//				Plausible disease-causing mutations within, affecting or encompassing cis-regulatory elements convincingly affecting the expression of a single gene identified in in more than one (2 or 3) unrelated cases/families with a developmental disorder
		//				As definitions of Possible Gene (see below) with addition of convincing bioinformatic or functional evidence of causation e.g. known inborn error of metabolism with mutation in orthologous gene which is known to have the relevant deficient enzymatic activity in other species; existence of animal mode which recapitulates the human phenotype
		//   Possible 	Plausible disease-causing mutations within, affecting or encompassingan interpretable functional region of a single gene identified in one case or segregation within multiple individuals within a small family with a developmental disorder
		//				Plausible disease-causing mutations within, affecting or encompassing cis-regulatory elements convincingly affecting the expression of a single gene identified in one case/family with a developmental disorder
		//				Possible disease-causing mutations within, affecting or encompassing an interpretable functional region of a single gene identified in more than one unrelated cases/families or segregation within multiple individuals within a single large family with a developmental disorder
		//   Both RD and IF 	Plausible disease-causing mutations within, affecting or encompassing the coding region of a single gene identified in multiple (>3) unrelated cases/families with both the relevant disease (RD) and an incidental disorder
		if (decipher_evi == "\"both RD and IF\"")
		{ // meaning?
			return PhenotypeEvidenceLevel::LOW;
		}
		else if (decipher_evi == "possible" || decipher_evi == "limited" || decipher_evi == "supportive")
		{
			return PhenotypeEvidenceLevel::LOW;
		}
		else if (decipher_evi == "probable" || decipher_evi == "moderate")
		{
			return PhenotypeEvidenceLevel::MEDIUM;
		}
		else if (decipher_evi == "confirmed" || decipher_evi == "definitive" || decipher_evi == "strong")
		{
			return PhenotypeEvidenceLevel::HIGH;
		}
		else
		{
			THROW(ArgumentException, "Given Evidence is not a Decipher evidence value.: " + QString(decipher_evi));
		}
	}

	/// turns a given GenCC Evidence value into one from the Evidences enum
	static PhenotypeEvidenceLevel translateGenccEvidence(const QByteArray& gencc_evi, const QString& line)
	{
		//Definitive, Strong, Moderate, Supportive, Limited, Disputed, Refuted, Animal, No Known
		if (gencc_evi == "No Known")
		{
			return PhenotypeEvidenceLevel::NA;
		}
		else if (gencc_evi == "No Known Disease Relationship")
		{
			return PhenotypeEvidenceLevel::NA;
		}
		else if (gencc_evi == "Animal")
		{
			return PhenotypeEvidenceLevel::LOW;
		}
		else if (gencc_evi == "Refuted" || gencc_evi == "Refuted Evidence")
		{
			return PhenotypeEvidenceLevel::AGAINST;
		}
		else if (gencc_evi == "Disputed" || gencc_evi == "Disputed Evidence")
		{
			return PhenotypeEvidenceLevel::AGAINST;
		}
		else if (gencc_evi == "Limited")
		{
			return PhenotypeEvidenceLevel::LOW;
		}
		else if (gencc_evi == "Supportive")
		{
			return PhenotypeEvidenceLevel::LOW;
		}
		else if (gencc_evi == "Moderate")
		{
			return PhenotypeEvidenceLevel::MEDIUM;
		}
		else if (gencc_evi == "Strong")
		{
			return PhenotypeEvidenceLevel::HIGH;
		}
		else if (gencc_evi == "Definitive")
		{
			return PhenotypeEvidenceLevel::HIGH;
		}
		else
		{
			THROW(ArgumentException, "Given Evidence is not a GenCC evidence value: " + QString(gencc_evi) + " in line:\n" + line);
		}
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
		db.tableExists("hpo_obsolete");

		//check if gene table exists and contains HGNC genes:
		db.tableExists("gene");
		SqlQuery test_gene_table = db.getQuery();
		test_gene_table.exec("SELECT count(*) FROM gene;");
		while (test_gene_table.next())
		{
			if (test_gene_table.value(0) == 0)
			{
				THROW(DatabaseException, "Table 'gene' is empty. Please import HGNC database before importing HPO.")
			}
		}

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

		// import HPO Ontology to DB
		QHash<QByteArray, int> id2ngsd = importHpoOntology(db);

		QHash<int, AnnotatedList> term2genes;
		QHash<int, AnnotatedList > term2diseases;
		QHash<QByteArray, AnnotatedList> disease2genes;

		// parse Evidence files if provided
		// parse g2pDDG2P_11_11_2021.csv file
		parseDecipher(db, id2ngsd, disease2genes, term2diseases, term2genes);
		// parse gencc-submissions.csv file
		parseGenCC(db, disease2genes);
		// parse phenotype.hpoa file
		parseHpoPhen(id2ngsd, term2diseases);


		//parse term-disease and disease-gene relations from HPO
		{
			QSharedPointer<QFile> fp = Helper::openFileForReading(getInfile("anno"));
			QSet<QByteArray> non_hgnc_genes;
			PhenotypeList inheritance_terms = db.phenotypeChildTerms(db.phenotypeIdByAccession("HP:0000005"), true); //Mode of inheritance
			int lineCount = 0;
			int added_t2g = 0;
			int added_t2d = 0;
			int added_d2g = 0;
			QString exactSource;
			while(!fp->atEnd())
			{
				lineCount++;
				exactSource = QString("Anno line ") + QString::number(lineCount);

				QByteArrayList parts = fp->readLine().split('\t');
				if (parts.count()<5) continue;
				if (parts.count()>5) THROW(FileParseException, "Found line with more than 5 tab-separated parts in " + getInfile("anno") + ". The file might by outdated!");
				
				//skip header line
				if (parts[0]=="hpo_id") continue;

				// parse line
				QByteArray disease = parts[4].trimmed();
				QByteArray gene = parts[3].trimmed();
				QByteArray term_accession = parts[0].trimmed();

				int gene_db_id = db.geneId(gene);
				int term_db_id = id2ngsd.value(term_accession, -1);

				if (term_db_id!=-1)
				{
					if (inheritance_terms.containsAccession(term_accession))
					{
						if (gene_db_id!=-1)
						{
							if (debug) out << "HPO-GENE: " << term_accession << " - " << gene << "\n";

							ExactSources e_src = ExactSources();
							e_src.term2gene = exactSource;
							term2genes[term_db_id].add(db.geneSymbol(gene_db_id), "HPO", "", PhenotypeEvidenceLevel::NA, e_src);
							++added_t2g;
						}
					}
					else
					{
						if (debug) out << "HPO-DISEASE: " << term_accession << " - " << disease << "\n";

						ExactSources e_src = ExactSources();
						e_src.term2disease = exactSource;
						term2diseases[term_db_id].add(disease, "HPO", "", PhenotypeEvidenceLevel::NA, e_src);
						++added_t2d;
					}
				}

				if (gene_db_id!=-1)
				{
					if (debug) out << "DISEASE-GENE (HPO): " << disease << " - " << db.geneSymbol(gene_db_id) << "\n";

					ExactSources e_src = ExactSources();
					e_src.disease2gene = exactSource;
					disease2genes[disease].add(db.geneSymbol(gene_db_id), "HPO", "", PhenotypeEvidenceLevel::NA, e_src);
					++added_d2g;
				}
				else
				{
					non_hgnc_genes << gene;
				}
			}
			fp->close();

			out << "Imported " << added_d2g << " disease-gene relations, " << added_t2d << " term-disease relations, " << added_t2g << " term-gene relations from HPO (anno).\n";
			foreach(const QByteArray& gene, non_hgnc_genes)
			{
				out << "Skipped gene '" << gene << "' because it is not an approved HGNC symbol!" << endl;
			}
		}

		//parse disease-gene relations from OMIM
		QString omim_file = getInfile("omim");
		if (omim_file!="")
		{
			int lineCount = 0;
			int count = 0;
			if (debug) out << "Parsing OMIM file...\n";
			//parse disease-gene relations
			int c_skipped_invalid_gene = 0;
			QSharedPointer<QFile> fp = Helper::openFileForReading(omim_file);
			QRegExp mim_exp("([0-9]{6})");
			QRegExp evi_exp("(\\([1-4]{1}\\))");

			while(!fp->atEnd())
			{
				lineCount++;
				QByteArrayList parts = fp->readLine().trimmed().split('\t');
				if (parts.count()<4) continue;

				QByteArray pheno = parts[0].trimmed();
				QByteArrayList genes = parts[1].split(',');
				QByteArray mim_number = parts[2].trimmed(); // mim number for gene
				QByteArray omim_evi = "";

				if (mim_exp.indexIn(pheno)!=-1)
				{
					mim_number = mim_exp.cap().toUtf8(); // mim number for phenotype
				}

				if (evi_exp.indexIn(pheno) != -1)
				{
					omim_evi = evi_exp.cap().toUtf8(); // evidence for relation
				}

				foreach(QByteArray gene, genes)
				{
					//make sure the gene symbol is approved by HGNC
					gene = gene.trimmed();
					int approved_id = db.geneId(gene);
					if (approved_id==-1)
					{
						if (debug) out << "Skipped gene '" << gene << "' because it is not an approved HGNC symbol!\n";
						++c_skipped_invalid_gene;
						continue;
					}

					if (debug) out << "DISEASE-GENE (OMIM): OMIM:" << mim_number << " - " << db.geneSymbol(approved_id) << "\n";

					ExactSources e_src = ExactSources();
					e_src.disease2gene = QString("OMIM line ") + QString::number(lineCount);
					disease2genes["OMIM:"+mim_number].add(db.geneSymbol(approved_id), "OMIM", omim_evi, translateOmimEvidence(omim_evi), e_src);
					count++;
				}
			}
			fp->close();
			out << "Imported " << count << " disease-gene relations from OMIM.\n";
		}

		//parse disease-gene relations from ClinVar
		QString clinvar_file = getInfile("clinvar");
		if (clinvar_file!="")
		{
			if (debug) out << "Prasing ClinVar..." << endl;
			int added_t2g = 0;
			int added_d2g = 0;
			//parse disease-gene relations
			int c_skipped_invalid_gene = 0;
			QSharedPointer<QFile> fp = Helper::openFileForReading(clinvar_file);
			int lineCount = 0;
			while(!fp->atEnd())
			{
				lineCount++;
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
					int approved_id = db.geneId(gene);
					if (approved_id==-1)
					{
						if (debug) out << "Skipped gene '" << gene << "' because it is not an approved HGNC symbol!" << endl;
						++c_skipped_invalid_gene;
						continue;
					}
					QByteArray gene_approved = db.geneSymbol(approved_id);

					foreach(const QByteArray& disease, diseases)
					{
						if (debug) out << "DISEASE-GENE (ClinVar): " << disease << " - " << gene_approved << "\n";

						ExactSources e_src = ExactSources();
						e_src.disease2gene = QString("ClinVar line ") + QString::number(lineCount);
						disease2genes[disease].add(gene_approved, "ClinVar", "", PhenotypeEvidenceLevel::NA, e_src);
						++added_d2g;
					}
					foreach(const QByteArray& hpo, hpos)
					{
						if (debug) out << "HPO-GENE (ClinVar): " << hpo << " - " << gene_approved << "\n";
						int term_db_id = id2ngsd.value(hpo, -1);
						if (term_db_id != -1)
						{
							ExactSources e_src = ExactSources();
							e_src.term2gene = QString("ClinVar line ") + QString::number(lineCount);
							term2genes[term_db_id].add(gene_approved, "ClinVar", "", PhenotypeEvidenceLevel::NA, e_src);
							++added_t2g;
						}
					}
				}
			}
			fp->close();

			out << "Imported " << added_d2g << " disease-gene relations, " << added_t2g << " term-gene relations from ClinVar.\n";
		}

		// parse hpo-gene relations from HGMD (Phenobase dbdump file):
		QString hgmd_file = getInfile("hgmd");
		if(hgmd_file != "")
		{
			int added_t2g = 0;
			if (debug) out << "Parsing HGMD Phenobase dump file...\n" << endl;
			// define look-up tables
			QMultiMap<int, QByteArray> phenid2gene_mapping = QMap<int, QByteArray>();
			QMultiMap<QByteArray,int> cui2phenid_mapping = QMap<QByteArray,int>();
			QMultiMap<QByteArray,QByteArray> hpo2cui_mapping = QMap<QByteArray,QByteArray>();

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
							int approved_id = db.geneId(gene);
							if (approved_id==-1)
							{
								if (debug) out << "Skipped gene '" << gene << "' because it is not an approved HGNC symbol (HGMD file)!" << endl;
								continue;
							}
							QByteArray gene_approved = db.geneSymbol(approved_id);

							// add gene to hpo list:
							if (debug) out << "HPO-GENE (HGMD): " << hpo << " - " << gene_approved << endl;
							int term_db_id = id2ngsd.value(hpo, -1);
							if (term_db_id != -1)
							{
								ExactSources e_src = ExactSources();
								e_src.term2gene = QString("HGMD unknown line");
								term2genes[term_db_id].add(gene_approved, "HGMD", "", PhenotypeEvidenceLevel::NA, e_src); // is there some evidence in the file that could be parsed?
								++added_t2g;
							}
						}
					}
				}

				// show progress
				if(debug && (i%1000 == 0))
				{
					out << "\t" << i << " of " << hpo2cui_mapping.keys().size() << "hpo terms parsed \n";
				}
			}

			out << "Imported " << added_t2g << " term-gene relations from HGMD.\n";
		}

		// import gathered data:
		out << "Gathering all term2gene relations" << endl;

		foreach (int term_id, term2diseases.keys())
		{
			foreach (const AnnotatedItem& disease, term2diseases[term_id].items())
			{
				foreach (const AnnotatedItem& gene, disease2genes[disease.item].items())
				{
					// if one of the evidencess is NA take the other one. If both have a value take the lower ranked one.
					PhenotypeEvidenceLevel evi;
					if (disease.evi == PhenotypeEvidenceLevel::NA)
					{
						evi = gene.evi;
					}
					else if (gene.evi == PhenotypeEvidenceLevel::NA)
					{
						evi = disease.evi;
					}
					else
					{
						evi = (int) disease.evi < (int) gene.evi ? disease.evi : gene.evi;
					}

					SourceDetails src = SourceDetails(); // list all the combined sources?
					src.unite(disease.src);
					src.unite(gene.src);
					ExactSources exactSource = disease.exactSources;
					exactSource.combine(gene.exactSources);

					term2genes[term_id].add(gene.item, src, evi, exactSource);
				}
			}
		}

		out << "Starting import into NGSD" << endl;
		// build insert statements with 10000 tuples each (big insertions are way faster than single element insert statements)
		QStringList tuples;
		foreach (int term_id, term2genes.keys())
		{
			foreach (const AnnotatedItem& gene, term2genes[term_id].items())
			{
				if (getFlag("debug"))
				{
					out << "Gene:\t" << gene.item << "\tHPO term id:\t" << term_id << "\t" << "final evidence:\t" << Phenotype::evidenceToString(gene.evi) << "  \torigin:\t" << gene.exactSources.toString() << "\n";
				}
				tuples << QString("(%1, '%2', '%3', '%4')").arg(QString::number(term_id), QString(gene.item), gene.src.toCsvString(), Phenotype::evidenceToString(gene.evi));
			}
		}
		//import
		for (int i=0; i<tuples.count(); i+=10000)
		{
			db.getQuery().exec("INSERT INTO hpo_genes (hpo_term_id, gene, details, evidence) VALUES " + tuples.mid(i, 10000).join(", ") +";");
		}
		out << "Overall imported term-gene relations: " << db.getValue("SELECT COUNT(*) FROM hpo_genes").toInt() << endl;

		out << "Optimizing term-gene relations (removing genes which are present in all leaf nodes from the parent node)...\n";

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
