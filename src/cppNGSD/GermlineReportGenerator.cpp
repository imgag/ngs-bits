#include "GermlineReportGenerator.h"
#include "Helper.h"
#include "XmlHelper.h"
#include "Settings.h"
#include "Log.h"
#include "Statistics.h"
#include "LoginManager.h"
#include <QFileInfo>
#include <QXmlStreamWriter>

GermlineReportGeneratorData::GermlineReportGeneratorData(GenomeBuild build_, QString ps_, const VariantList& variants_, const CnvList& cnvs_, const BedpeFile& svs_, const PrsTable& prs_, const ReportSettings& report_settings_, const FilterCascade& filters_, const QMap<QByteArray, QByteArrayList>& preferred_transcripts_)
	: build(build_)
	, ps(ps_)
	, variants(variants_)
	, cnvs(cnvs_)
	, svs(svs_)
	, prs(prs_)
	, report_settings(report_settings_)
	, filters(filters_)
	, preferred_transcripts(preferred_transcripts_)
{
}

GermlineReportGenerator::GermlineReportGenerator(const GermlineReportGeneratorData& data, bool test_mode)
	: db_(test_mode)
	, data_(data)
	, date_(QDate::currentDate())
	, test_mode_(test_mode)
{
	ps_id_ = db_.processedSampleId(data_.ps);
}

void GermlineReportGenerator::writeHTML(QString filename)
{
	QSharedPointer<QFile> outfile = Helper::openFileForWriting(filename);
	QTextStream stream(outfile.data());
	writeHtmlHeader(stream, data_.ps);

	//get trio data
	bool is_trio = data_.variants.type() == GERMLINE_TRIO;
	SampleInfo info_father;
	SampleInfo info_mother;
	if (is_trio)
	{
		info_father = data_.variants.getSampleHeader().infoByStatus(false, "male");
		info_mother = data_.variants.getSampleHeader().infoByStatus(false, "female");
	}

	//get data from database
	QString sample_id = db_.sampleId(data_.ps);
	SampleData sample_data = db_.getSampleData(sample_id);
	ProcessedSampleData processed_sample_data = db_.getProcessedSampleData(ps_id_);
	ProcessingSystemData system_data = db_.getProcessingSystemData(db_.processingSystemIdFromProcessedSample(data_.ps));

	//report header (meta information)
	stream << "<h4>" << trans("Technischer Report zur bioinformatischen Analyse") << "</h4>" << endl;

	stream << endl;
	stream << "<p><b>" << trans("Probe") << ": " << data_.ps << "</b> (" << sample_data.name_external << ")" << endl;
	if (is_trio)
	{
		stream << "<br />" << endl;
		stream << "<br />" << trans("Vater") << ": "  << info_father.id << endl;
		stream << "<br />" << trans("Mutter") << ": "  << info_mother.id << endl;
	}
	stream << "<br />" << endl;
	stream << "<br />" << trans("Geschlecht") << ": " << trans(processed_sample_data.gender) << endl;
	stream << "<br />" << trans("Prozessierungssystem") << ": " << processed_sample_data.processing_system << endl;
	stream << "<br />" << trans("Prozessierungssystem-Typ") << ": " << processed_sample_data.processing_system_type << endl;
	stream << "<br />" << trans("Referenzgenom") << ": " << system_data.genome << endl;
	stream << "<br />" << trans("Datum") << ": " << date_.toString("dd.MM.yyyy") << endl;
	stream << "<br />" << trans("Benutzer") << ": " << LoginManager::user() << endl;
	stream << "<br />" << trans("Analysepipeline") << ": "  << data_.variants.getPipeline() << endl;
	stream << "<br />" << trans("Auswertungssoftware") << ": "  << QCoreApplication::applicationName() << " " << QCoreApplication::applicationVersion() << endl;
	stream << "<br />" << trans("KASP-Ergebnis") << ": " << db_.getQCData(ps_id_).value("kasp").asString() << endl;
	stream << "</p>" << endl;

	///Phenotype information
	stream << endl;
	stream << "<p><b>" << trans("Ph&auml;notyp") << "</b>" << endl;
	QList<SampleDiseaseInfo> info = db_.getSampleDiseaseInfo(sample_id, "ICD10 code");
	foreach(const SampleDiseaseInfo& entry, info)
	{
		stream << "<br />ICD10: " << entry.disease_info << endl;
	}
	info = db_.getSampleDiseaseInfo(sample_id, "HPO term id");
	foreach(const SampleDiseaseInfo& entry, info)
	{
		int hpo_id = db_.phenotypeIdByAccession(entry.disease_info.toLatin1(), false);
		if (hpo_id!=-1)
		{
			stream << "<br />HPO: " << entry.disease_info << " (" << db_.phenotype(hpo_id).name() << ")" << endl;
		}
	}
	info = db_.getSampleDiseaseInfo(sample_id, "OMIM disease/phenotype identifier");
	foreach(const SampleDiseaseInfo& entry, info)
	{
		stream << "<br />OMIM: " << entry.disease_info << endl;
	}
	info = db_.getSampleDiseaseInfo(sample_id, "Orpha number");
	foreach(const SampleDiseaseInfo& entry, info)
	{
		stream << "<br />Orphanet: " << entry.disease_info << endl;
	}
	stream << "</p>" << endl;

	///Target region statistics
	if (data_.roi.isValid())
	{
		stream << endl;
		stream << "<p><b>" << trans("Zielregion") << "</b>" << endl;
		stream << "<br /><span style=\"font-size: 8pt;\">" << trans("Die Zielregion umfasst mindestens die CCDS (\"consensus coding sequence\") unten genannter Gene &plusmn;20 Basen flankierender intronischer Sequenz, kann aber auch zus&auml;tzliche Exons und/oder flankierende Basen beinhalten.") << endl;
		stream << "<br />" << trans("Name") << ": " << data_.roi.name << endl;
		if (!data_.roi.genes.isEmpty())
		{
			stream << "<br />" << trans("Ausgewertete Gene") << " (" << QString::number(data_.roi.genes.count()) << "): " << data_.roi.genes.join(", ") << endl;
		}
		stream << "</span></p>" << endl;
	}

	//get column indices
	int i_genotype = data_.variants.getSampleHeader().infoByID(data_.ps).column_index;
	int i_gene = data_.variants.annotationIndexByName("gene", true, true);
	int i_co_sp = data_.variants.annotationIndexByName("coding_and_splicing", true, true);
	int i_omim = data_.variants.annotationIndexByName("OMIM", true, true);
	int i_class = data_.variants.annotationIndexByName("classification", true, true);
	int i_comment = data_.variants.annotationIndexByName("comment", true, true);
	int i_kg = data_.variants.annotationIndexByName("1000G", true, true);
	int i_gnomad = data_.variants.annotationIndexByName("gnomAD", true, true);

	//output: applied filters
	stream << endl;
	stream << "<p><b>" << trans("Filterkriterien") << " " << "</b>" << endl;
	for(int i=0; i<data_.filters.count(); ++i)
	{
		stream << "<br />&nbsp;&nbsp;&nbsp;&nbsp;- " << data_.filters[i]->toText() << endl;
	}

	//determine variant count (inside target region)
	int var_count = data_.variants.count();
	if (data_.roi.isValid())
	{
		FilterResult filter_result(data_.variants.count());
		FilterRegions::apply(data_.variants, data_.roi.regions, filter_result);
		var_count = filter_result.countPassing();
	}

	stream << "<br />" << trans("Gefundene Varianten in Zielregion gesamt") << ": " << var_count << endl;
	selected_small_.clear();
	selected_cnvs_.clear();
	selected_svs_.clear();
	for (auto it = data_.report_settings.selected_variants.cbegin(); it!=data_.report_settings.selected_variants.cend(); ++it)
	{
		if (it->first==VariantType::SNVS_INDELS) selected_small_ << it->second;
		if (it->first==VariantType::CNVS) selected_cnvs_ << it->second;
		if (it->first==VariantType::SVS) selected_svs_ << it->second;
	}
	stream << "<br />" << trans("Anzahl Varianten ausgew&auml;hlt f&uuml;r Report") << ": " << selected_small_.count() << endl;
	stream << "<br />" << trans("Anzahl CNVs ausgew&auml;hlt f&uuml;r Report") << ": " << selected_cnvs_.count() << endl;
	stream << "<br />" << trans("Anzahl SVs ausgew&auml;hlt f&uuml;r Report") << ": " << selected_svs_.count() << endl;
	stream << "</p>" << endl;

	//output: selected variants
	stream << endl;
	stream << "<p><b>" << trans("Varianten nach klinischer Interpretation im Kontext der Fragestellung") << "</b>" << endl;
	stream << "<br />" << trans("In der folgenden Tabelle werden neben wahrscheinlich pathogenen (Klasse 4) und pathogenen (Klasse 5) nur solche Varianten unklarer klinischer Signifikanz (Klasse 3) gelistet, f&uuml;r die in Zusammenschau von Literatur und Klinik des Patienten ein Beitrag zur Symptomatik denkbar ist und f&uuml;r die gegebenenfalls eine weitere Einordnung der klinischen Relevanz durch Folgeuntersuchungen sinnvoll ist. Eine Liste aller detektierten Varianten kann bei Bedarf angefordert werden.") << endl;
	stream << "</p>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><td><b>" << trans("Variante") << "</b></td><td><b>" << trans("Genotyp") << "</b></td>";
	if (is_trio)
	{
		stream << "<td><b>" << trans("Vater") << "</b></td>";
		stream << "<td><b>" << trans("Mutter") << "</b></td>";
	}
	stream << "<td><b>" << trans("Gen(e)") << "</b></td><td><b>" << trans("Details") << "</b></td><td><b>" << trans("Klasse") << "</b></td><td><b>" << trans("Vererbung") << "</b></td><td><b>1000g</b></td><td><b>gnomAD</b></td></tr>" << endl;

	foreach(const ReportVariantConfiguration& var_conf, data_.report_settings.report_config->variantConfig())
	{
		if (var_conf.variant_type!=VariantType::SNVS_INDELS) continue;
		if (!selected_small_.contains(var_conf.variant_index)) continue;

		const Variant& variant = data_.variants[var_conf.variant_index];

		stream << "<tr>" << endl;
		stream << "<td>" << endl;
		stream  << variant.chr().str() << ":" << variant.start() << "&nbsp;" << variant.ref() << "&nbsp;&gt;&nbsp;" << variant.obs() << "</td>";
		QString geno = formatGenotype(data_.build, processed_sample_data.gender.toLatin1(), variant.annotations().at(i_genotype), variant);
		if (var_conf.de_novo) geno += " (de-novo)";
		if (var_conf.mosaic) geno += " (mosaic)";
		if (var_conf.comp_het) geno += " (comp-het)";
		stream << "<td>" << geno << "</td>" << endl;
		if (is_trio)
		{
			stream << "<td>" << formatGenotype(data_.build, "male", variant.annotations().at(info_father.column_index), variant) << "</td>";
			stream << "<td>" << formatGenotype(data_.build, "female", variant.annotations().at(info_mother.column_index), variant) << "</td>";
		}

		stream << "<td>";
		GeneSet genes = GeneSet::createFromText(variant.annotations()[i_gene], ',');
		for(int i=0; i<genes.count(); ++i)
		{
			QByteArray sep = (i==0 ? "" : ", ");
			QByteArray gene = genes[i].trimmed();
			QString inheritance = "";
			GeneInfo gene_info = db_.geneInfo(gene);
			if (gene_info.inheritance!="" && gene_info.inheritance!="n/a")
			{
				inheritance = " (" + gene_info.inheritance + ")";
			}
			stream << sep << gene << inheritance << endl;
		}
		stream << "</td>" << endl;
		stream << "<td>" << formatCodingSplicing(variant.transcriptAnnotations(i_co_sp)) << "</td>" << endl;
		stream << "<td>" << variant.annotations().at(i_class) << "</td>" << endl;
		stream << "<td>" << var_conf.inheritance << "</td>" << endl;
		QByteArray freq = variant.annotations().at(i_kg).trimmed();
		stream << "<td>" << (freq.isEmpty() ? "n/a" : freq) << "</td>" << endl;
		freq = variant.annotations().at(i_gnomad).trimmed();
		stream << "<td>" << (freq.isEmpty() ? "n/a" : freq) << "</td>" << endl;
		stream << "</tr>" << endl;

		//OMIM and comment line
		QString omim = variant.annotations()[i_omim];
		QString comment = variant.annotations()[i_comment];
		if (comment!="" || omim!="")
		{
			QStringList parts;
			if (comment!="") parts << "<span style=\"background-color: #FF0000\">NGSD: " + comment + "</span>";
			if (omim!="")
			{
				QStringList omim_parts = omim.append(" ").split("]; ");
				foreach(QString omim_part, omim_parts)
				{
					if (omim_part.count()<10) continue;
					omim = "OMIM ID: " + omim_part.left(6) + " Details: " + omim_part.mid(8);
				}

				parts << omim;
			}
			stream << "<tr><td colspan=\"" << (is_trio ? "10" : "8") << "\">" << parts.join("<br />") << "</td></tr>" << endl;
		}
	}
	stream << "</table>" << endl;

	//CNVs
	stream << "<p></p>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><td><b>" << trans("CNV") << "</b></td><td><b>" << trans("Regionen") << "</b></td><td><b>" << trans("CN") << "</b></td><td><b>"
		   << trans("Gen(e)") << "</b></td><td><b>" << trans("Klasse") << "</b></td><td><b>" << trans("Vererbung") << "</b></td></tr>" << endl;

	foreach(const ReportVariantConfiguration& var_conf, data_.report_settings.report_config->variantConfig())
	{
		if (var_conf.variant_type!=VariantType::CNVS) continue;
		if (!selected_cnvs_.contains(var_conf.variant_index)) continue;

		const CopyNumberVariant& cnv = data_.cnvs[var_conf.variant_index];

		stream << "<tr>" << endl;
		stream << "<td>" << cnv.toString() << "</td>" << endl;
		stream << "<td>" << std::max(1, cnv.regions()) << "</td>" << endl; //trio CNV lists don't contain number of regions > fix

		QString cn = QString::number(cnv.copyNumber(data_.cnvs.annotationHeaders()));
		if (var_conf.de_novo) cn += " (de-novo)";
		if (var_conf.mosaic) cn += " (mosaic)";
		if (var_conf.comp_het) cn += " (comp-het)";
		stream << "<td>" << cn << "</td>" << endl;
		stream << "<td>" << cnv.genes().join(", ") << "</td>" << endl;
		stream << "<td>" << var_conf.classification << "</td>" << endl;
		stream << "<td>" << var_conf.inheritance << "</td>" << endl;
		stream << "</tr>" << endl;
	}
	stream << "</table>" << endl;

	//--------------------------------------------------------------------------------------
	//SVs
	stream << "<p></p>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><td><b>" << trans("SV") << "</b></td><td><b>" << trans("Position") << "</b></td><td><b>" << trans("Genotyp") << "</b></td><td><b>"
		   << trans("Gen(e)") << "</b></td><td><b>" << trans("Klasse") << "</b></td><td><b>" << trans("Vererbung") << "</b></td></tr>" << endl;

	foreach(const ReportVariantConfiguration& var_conf, data_.report_settings.report_config->variantConfig())
	{
		if (var_conf.variant_type!=VariantType::SVS) continue;
		if (!selected_svs_.contains(var_conf.variant_index)) continue;

		const BedpeLine& sv = data_.svs[var_conf.variant_index];

		stream << "<tr>" << endl;
		//type
		stream << "<td>";

		switch (sv.type()) // determine type String
		{
			case StructuralVariantType::DEL:
				stream << trans("Deletion") << "</td>" << endl;
				break;
			case StructuralVariantType::DUP:
				stream << trans("Duplikation") << "</td>" << endl;
				break;
			case StructuralVariantType::INS:
				stream << trans("Insertion") << "</td>" << endl;
				break;
			case StructuralVariantType::INV:
				stream << trans("Inversion") << "</td>" << endl;
				break;
			case StructuralVariantType::BND:
				stream << trans("Translokation") << "</td>" << endl;
				break;
			default:
				THROW(ArgumentException, "Invalid SV type!")
				break;
		}

		//pos
		BedFile affected_region = sv.affectedRegion();
		stream << "<td>" << affected_region[0].toString(true);
		if (sv.type() == StructuralVariantType::BND) stream << " &lt;-&gt; " << affected_region[1].toString(true);
		stream << "</td>" << endl;


		//genotype
		QByteArray gt = sv.formatValueByKey("GT", data_.svs.annotationHeaders());
		if (gt == "1/1")
		{
			stream << "<td>hom";
		}
		else
		{
			stream << "<td>het";
		}

		if (var_conf.de_novo) stream << " (de-novo)";
		if (var_conf.mosaic) stream << " (mosaic)";
		if (var_conf.comp_het) stream << " (comp-het)";
		stream << "</td>" << endl;

		//genes
		stream << "<td>" << sv.genes(data_.svs.annotationHeaders()).join(", ") << "</td>" << endl;

		//classification
		stream << "<td>" << var_conf.classification << "</td>" << endl;

		//inheritance
		stream << "<td>" << var_conf.inheritance << "</td>" << endl;
		stream << "</tr>" << endl;
	}
	stream << "</table>" << endl;
	stream << "<p></p>" << endl;

	//-----------------------------------------------------------------------------------

	stream << "<p>" << trans("F&uuml;r Informationen zur Klassifizierung von Varianten, siehe allgemeine Zusatzinformationen.") << endl;
	stream << "</p>" << endl;

	stream << "<p>" << trans("Teilweise k&ouml;nnen bei Varianten unklarer Signifikanz (Klasse 3) -  in Abh&auml;ngigkeit von der Art der genetischen Ver&auml;nderung, der Familienanamnese und der Klinik des/der Patienten - weiterf&uuml;hrende Untersuchungen eine &Auml;nderung der Klassifizierung bewirken. Bei konkreten differentialdiagnostischen Hinweisen auf eine entsprechende Erkrankung k&ouml;nnen ggf. weiterf&uuml;hrende genetische Untersuchungen indiziert sein.") << endl;
	stream << "</p>" << endl;

	///classification explaination
	if (data_.report_settings.show_class_details)
	{
		stream << endl;
		stream << "<p><b>" << trans("Klassifikation von Varianten") << ":</b>" << endl;
		stream << "<br />" << trans("Die Klassifikation der Varianten erfolgt in Anlehnung an die Publikation von Plon et al. (Hum Mutat 2008)") << endl;
		stream << "<br /><b>" << trans("Klasse 5: Eindeutig pathogene Ver&auml;nderung / Mutation") << ":</b> " << trans("Ver&auml;nderung, die bereits in der Fachliteratur mit ausreichender Evidenz als krankheitsverursachend bezogen auf das vorliegende Krankheitsbild beschrieben wurde sowie als pathogen zu wertende Mutationstypen (i.d.R. Frameshift- bzw. Stoppmutationen).") << endl;
		stream << "<br /><b>" << trans("Klasse 4: Wahrscheinlich pathogene Ver&auml;nderung") << ":</b> " << trans("DNA-Ver&auml;nderung, die aufgrund ihrer Eigenschaften als sehr wahrscheinlich krankheitsverursachend zu werten ist.") << endl;
		stream << "<br /><b>" << trans("Klasse 3: Variante unklarer Signifikanz (VUS) - Unklare Pathogenit&auml;t") << ":</b> " << trans("Variante, bei der es unklar ist, ob eine krankheitsverursachende Wirkung besteht. Diese Varianten werden tabellarisch im technischen Report mitgeteilt.") << endl;
		stream << "<br /><b>" << trans("Klasse 2: Sehr wahrscheinlich benigne Ver&auml;nderungen") << ":</b> " << trans("Aufgrund der H&auml;ufigkeit in der Allgemeinbev&ouml;lkerung oder der Lokalisation bzw. aufgrund von Angaben in der Literatur sehr wahrscheinlich benigne. Werden nicht mitgeteilt, k&ouml;nnen aber erfragt werden.") << endl;
		stream << "<br /><b>" << trans("Klasse 1: Benigne Ver&auml;nderungen") << ":</b> " << trans("Werden nicht mitgeteilt, k&ouml;nnen aber erfragt werden.") << endl;
		stream << "</p>" << endl;
	}

	///low-coverage analysis
	if (data_.report_settings.show_coverage_details)
	{
		writeCoverageReport(stream);

		writeCoverageReportCCDS(stream, 0, false, false);

		writeCoverageReportCCDS(stream, 5, true, true);

		writeClosedGapsReport(stream, data_.roi.regions);
	}

	//OMIM table
	if (data_.report_settings.show_omim_table)
	{
		stream << endl;
		stream << "<p><b>" << trans("OMIM Gene und Phenotypen") << "</b>" << endl;
		stream << "</p>" << endl;
		stream << "<table>" << endl;
		stream << "<tr><td><b>" << trans("Gen") << "</b></td><td><b>" << trans("Gen MIM") << "</b></td><td><b>" << trans("Phenotyp") << "</b></td><td><b>" << trans("Phenotyp MIM") << "</b></td>";
		if (data_.report_settings.show_one_entry_in_omim_table) stream << "<td><b>" << trans("Hauptphenotyp") << "</b></td>" << endl;
		stream << "</tr>";
		foreach(const QByteArray& gene, data_.roi.genes)
		{
			QString preferred_phenotype_accession;
			if (sample_data.disease_group!="n/a") preferred_phenotype_accession = db_.omimPreferredPhenotype(gene, sample_data.disease_group.toLatin1());

			QList<OmimInfo> omim_infos = db_.omimInfo(gene);
			foreach(const OmimInfo& omim_info, omim_infos)
			{
				QString preferred_phenotype_name ="";
				QStringList names;
				QStringList accessions;
				foreach(const Phenotype& p, omim_info.phenotypes)
				{
					names << p.name();
					accessions << p.accession();
					if (preferred_phenotype_accession!="" && p.accession()==preferred_phenotype_accession)
					{
						preferred_phenotype_name = p.name();
					}
				}

				//show only one entry
				if (data_.report_settings.show_one_entry_in_omim_table)
				{
					if (preferred_phenotype_name!="") //preferred phenotype match => show only preferred phenotype
					{
						names.clear();
						names << preferred_phenotype_name;
						accessions.clear();
						accessions << preferred_phenotype_accession;
					}
					else if (accessions.count()>1) //no preferred phenotype match => show first entry
					{
						//search for for first entry with accession (fallback to first entry)
						int selected_index = 0;
						for (int i=0; i<accessions.count(); ++i)
						{
							if (accessions[i]!="")
							{
								selected_index = i;
								break;
							}
						}

						accessions = QStringList() << accessions[selected_index];
						names = QStringList() << names[selected_index];
					}
				}
				stream << "<tr><td>" << omim_info.gene_symbol << "</td><td>" << omim_info.mim << "</td><td>" << names.join("<br />") << "</td><td>" << accessions.join("<br />") << "</td>";
				if (data_.report_settings.show_one_entry_in_omim_table) stream << "<td>" <<trans(preferred_phenotype_name!="" ? "ja" : "nein") << "</td>" << endl;
				stream << "</tr>";
			}
		}
		stream << "</table>" << endl;
	}

	//PRS table
	if (data_.prs.rowCount()>0)
	{
		stream << endl;
		stream << "<p><b>" << trans("Polygener Risiko-Score (PRS)") << "</b></p>" << endl;
		stream << "<table>" << endl;
		stream << "<tr><td><b>" << trans("Erkrankung") << "</b></td><td><b>" << trans("Publikation") << "</b></td><td><b>" << trans("Score") << "</b></td><td><b>" << trans("Z-Score") << "</b></td><td><b>" << trans("Population (gesch&auml;tzt aus NGS)") << "</b></td></tr>" << endl;
		int trait_idx = data_.prs.headers().indexOf("trait");
		int score_idx = data_.prs.headers().indexOf("score");
		int citation_idx = data_.prs.headers().indexOf("citation");
		for (int r=0; r<data_.prs.rowCount(); ++r)
		{
			const QStringList& row = data_.prs.row(r);
			QString trait = row[trait_idx];
			QString score = row[score_idx];
			QString zscore = "n/a";
			QString population = NGSHelper::populationCodeToHumanReadable(processed_sample_data.ancestry);
			if (trait=="Breast Cancer") // mean and standard deviation taken from BCAC315 data
			{
				double mean = -0.424;
				double stdev = 0.611;
				zscore = QString::number((Helper::toDouble(score, "PRS score") - mean) / stdev, 'f', 3);
			}

			stream << "<tr><td>" << trait << "</td><td>" << row[citation_idx] << "</td><td>" << score << "</td><td>" << zscore << "</td><td>" << population << "</td></tr>";
		}
		stream << "</table>" << endl;
		stream << "<p>" << trans("Die Einsch&auml;tzung der klinischen Bedeutung eines PRS ist nur unter Verwendung eines entsprechenden validierten Risiko-Kalkulations-Programms und unter Ber&uuml;cksichtigung der ethnischen Zugeh&ouml;rigkeit m&ouml;glich (z.B. CanRisk.org f&uuml;r Brustkrebs).") << "</p>" << endl;
	}

	//close stream
	writeHtmlFooter(stream);
	outfile->close();


	//validate written file
	QString validation_error = XmlHelper::isValidXml(filename);
	if (validation_error!="")
	{
		if (test_mode_)
		{
			THROW(ProgrammingException, "Invalid germline report HTML file gererated: " + validation_error);
		}
		else
		{
			Log::warn("Generated HTML report at " + filename + " is not well-formed: " + validation_error);
		}
	}
}

void GermlineReportGenerator::writeXML(QString filename, QString html_document)
{
	QSharedPointer<QFile> outfile = Helper::openFileForWriting(filename);

	QXmlStreamWriter w(outfile.data());
	w.setAutoFormatting(true);
	w.writeStartDocument();

	//element DiagnosticNgsReport
	w.writeStartElement("DiagnosticNgsReport");
	w.writeAttribute("version", "4");
	w.writeAttribute("type", data_.report_settings.report_type);

	//element ReportGeneration
	w.writeStartElement("ReportGeneration");
	w.writeAttribute("date", date_.toString("yyyy-MM-dd"));
	w.writeAttribute("user_name", LoginManager::user());
	w.writeAttribute("software", QCoreApplication::applicationName() + " " + QCoreApplication::applicationVersion());
	w.writeAttribute("outcome", db_.getDiagnosticStatus(ps_id_).outcome);
	w.writeEndElement();

	//element Sample
	w.writeStartElement("Sample");
	w.writeAttribute("name", data_.ps);

	SampleData sample_data = db_.getSampleData(db_.sampleId(data_.ps));
	w.writeAttribute("name_external", sample_data.name_external);
	ProcessedSampleData processed_sample_data = db_.getProcessedSampleData(ps_id_);
	w.writeAttribute("processing_system", processed_sample_data.processing_system);
	w.writeAttribute("processing_system_type", processed_sample_data.processing_system_type);
	w.writeEndElement();

	//element TargetRegion (optional)
	if (data_.roi.isValid())
	{
		w.writeStartElement("TargetRegion");
		w.writeAttribute("name", data_.roi.name);
		w.writeAttribute("regions", QString::number(data_.roi.regions.count()));
		w.writeAttribute("bases", QString::number(data_.roi.regions.baseCount()));
		QString gap_percentage = cache_["gap_percentage"]; //cached from HTML report
		if (!gap_percentage.isEmpty())
		{
			w.writeAttribute("gap_percentage", gap_percentage);
		}
		QString ccds_sequenced = cache_["ccds_sequenced"]; //cached from HTML report
		if (!ccds_sequenced.isEmpty())
		{
			w.writeAttribute("ccds_bases_sequenced", ccds_sequenced);
		}

		//contained genes
		foreach(const QByteArray& gene, data_.roi.genes)
		{
			w.writeStartElement("Gene");
			w.writeAttribute("name", gene);
			w.writeEndElement();
		}

		w.writeEndElement();
	}

	//element VariantList
	w.writeStartElement("VariantList");
	w.writeAttribute("overall_number", QString::number(data_.variants.count()));
	w.writeAttribute("genome_build", buildToString(data_.build, true));

	//element Variant
	int geno_idx = data_.variants.getSampleHeader().infoByID(data_.ps).column_index;
	int qual_idx = data_.variants.annotationIndexByName("quality");
	foreach(const ReportVariantConfiguration& var_conf, data_.report_settings.report_config->variantConfig())
	{
		if (var_conf.variant_type!=VariantType::SNVS_INDELS) continue;
		if (!var_conf.showInReport()) continue;
		if (!selected_small_.contains(var_conf.variant_index)) continue;
		if (var_conf.report_type!=data_.report_settings.report_type) continue;

		const Variant& variant = data_.variants[var_conf.variant_index];
		w.writeStartElement("Variant");
		w.writeAttribute("chr", variant.chr().str());
		w.writeAttribute("start", QString::number(variant.start()));
		w.writeAttribute("end", QString::number(variant.end()));
		w.writeAttribute("ref", variant.ref());
		w.writeAttribute("obs", variant.obs());
		double allele_frequency = 0.0;
		int depth = 0;
		AnalysisType type = data_.variants.type();
		foreach(QByteArray entry, variant.annotations()[qual_idx].split(';'))
		{
			if(entry.startsWith("AF="))
			{
				QByteArray value = entry.mid(3);
				if (type==GERMLINE_TRIO || type==GERMLINE_MULTISAMPLE)
				{
					//determine index of report sample in quality entry
					SampleHeaderInfo header_info = data_.variants.getSampleHeader();
					int index = 0;
					while (index<header_info.count())
					{
						if (header_info[index].column_name==data_.ps) break;
						++index;
					}

					QByteArrayList parts = value.split(',');
					if (index>=parts.count()) THROW(ProgrammingException, "Invalid AF quality entry. Could not determine index " + QString::number(index) + " in comma-separated string '" + value + "'!");
					value = parts[index];
				}
				allele_frequency = Helper::toDouble(value, "variant allele-frequency of " + variant.toString());
			}
			if(entry.startsWith("DP="))
			{
				QByteArray value = entry.mid(3);
				if (type==GERMLINE_TRIO || type==GERMLINE_MULTISAMPLE)
				{
					//determine index of report sample in quality entry
					SampleHeaderInfo header_info = data_.variants.getSampleHeader();
					int index = 0;
					while (index<header_info.count())
					{
						if (header_info[index].column_name==data_.ps) break;
						++index;
					}

					QByteArrayList parts = value.split(',');
					if (index>=parts.count()) THROW(ProgrammingException, "Invalid DP quality entry. Could not determine index " + QString::number(index) + " in comma-separated string '" + value + "'!");
					value = parts[index];
				}
				depth = Helper::toInt(value, "variant depth of " + variant.toString());
			}
		}
		w.writeAttribute("allele_frequency", QString::number(allele_frequency, 'f', 2));
		w.writeAttribute("depth", QString::number(depth));
		w.writeAttribute("genotype", formatGenotype(data_.build, processed_sample_data.gender.toLatin1(), variant.annotations()[geno_idx], variant));
		w.writeAttribute("causal", var_conf.causal ? "true" : "false");
		w.writeAttribute("de_novo", var_conf.de_novo ? "true" : "false");
		w.writeAttribute("comp_het", var_conf.comp_het ? "true" : "false");
		w.writeAttribute("mosaic", var_conf.mosaic ? "true" : "false");
		if (var_conf.inheritance!="n/a")
		{
			w.writeAttribute("inheritance", var_conf.inheritance);
		}
		ClassificationInfo class_info = db_.getClassification(variant);
		if (class_info.classification!="" && class_info.classification!="n/a")
		{
			w.writeAttribute("class", class_info.classification);
			w.writeAttribute("class_comments", class_info.comments);
		}
		if (!var_conf.comments.trimmed().isEmpty())
		{
			w.writeAttribute("comments_1st_assessor", var_conf.comments.trimmed());
		}
		if (!var_conf.comments2.trimmed().isEmpty())
		{
			w.writeAttribute("comments_2nd_assessor", var_conf.comments2.trimmed());
		}
		//element TranscriptInformation
		GeneSet genes;
		int i_co_sp = data_.variants.annotationIndexByName("coding_and_splicing", true, false);
		if (i_co_sp!=-1)
		{
			foreach(const VariantTranscript& trans, variant.transcriptAnnotations(i_co_sp))
			{
				w.writeStartElement("TranscriptInformation");
				w.writeAttribute("gene", trans.gene);
				w.writeAttribute("transcript_id", trans.id);
				w.writeAttribute("type", trans.type);
				QByteArray hgvs_c = trans.hgvs_c;
				if (hgvs_c.startsWith("c.")) hgvs_c = hgvs_c.mid(2);
				w.writeAttribute("hgvs_c", hgvs_c);
				QByteArray hgvs_p = trans.hgvs_p;
				if (hgvs_p.startsWith("p.")) hgvs_p = hgvs_p.mid(2);
				w.writeAttribute("hgvs_p", hgvs_p);
				QString exon_nr = trans.exon;
				if (exon_nr.startsWith("exon"))
				{
					exon_nr.replace("exon", "Exon ");
				}
				if (exon_nr.startsWith("intron"))
				{
					exon_nr.replace("intron", "Intron ");
				}
				w.writeAttribute("exon", exon_nr);
				w.writeEndElement();

				genes << trans.gene;
			}
		}

		//element GeneDiseaseInformation
		if (var_conf.causal)
		{
			foreach(const QByteArray& gene, genes)
			{
				//OrphaNet
				SqlQuery query = db_.getQuery();
				query.exec("SELECT dt.* FROM disease_gene dg, disease_term dt WHERE dt.id=dg.disease_term_id AND dg.gene='" + gene + "'");
				while(query.next())
				{
					w.writeStartElement("GeneDiseaseInformation");
					w.writeAttribute("gene", gene);
					w.writeAttribute("source", query.value("source").toString());
					w.writeAttribute("identifier", query.value("identifier").toString());
					w.writeAttribute("name", query.value("name").toString());
					w.writeEndElement();
				}

				//OMIM
				QList<OmimInfo> omim_infos = db_.omimInfo(gene);
				foreach(const OmimInfo& omim_info, omim_infos)
				{
					//gene
					w.writeStartElement("GeneDiseaseInformation");
					w.writeAttribute("gene", omim_info.gene_symbol);
					w.writeAttribute("source", "OMIM gene");
					w.writeAttribute("identifier", omim_info.mim);
					w.writeAttribute("name", omim_info.gene_symbol);
					w.writeEndElement();

					//phenotypes
					foreach(const Phenotype& pheno, omim_info.phenotypes)
					{
						w.writeStartElement("GeneDiseaseInformation");
						w.writeAttribute("gene", omim_info.gene_symbol);
						w.writeAttribute("source", "OMIM phenotype");
						QString accession = pheno.accession().trimmed();
						if (accession=="") accession = "n/a";
						w.writeAttribute("identifier", accession);
						w.writeAttribute("name", pheno.name());
						w.writeEndElement();
					}
				}
			}
		}

		//element GeneInheritanceInformation
		foreach(const QByteArray& gene, genes)
		{
			GeneInfo gene_info = db_.geneInfo(gene);
			if (gene_info.inheritance!="n/a")
			{
				w.writeStartElement("GeneInheritanceInformation");
				w.writeAttribute("gene", gene);
				w.writeAttribute("inheritance", gene_info.inheritance);
				w.writeEndElement();
			}
		}

		//end of variant
		w.writeEndElement();
	}
	w.writeEndElement();

	//element CnvList
	bool no_cnv_calling = data_.cnvs.caller()==CnvCallerType::INVALID;
	w.writeStartElement("CnvList");
	w.writeAttribute("cnv_caller", no_cnv_calling ? "NONE" :  data_.cnvs.callerAsString());
	w.writeAttribute("overall_number", QString::number(data_.cnvs.count()));
	w.writeAttribute("genome_build", buildToString(data_.build, true));
	QString cnv_callset_id = db_.getValue("SELECT id FROM cnv_callset WHERE processed_sample_id=" + ps_id_, true).toString().trimmed();
	if (no_cnv_calling || cnv_callset_id.isEmpty()) cnv_callset_id = "-1";
	QString cnv_calling_quality = db_.getValue("SELECT quality FROM cnv_callset WHERE id=" + cnv_callset_id, true).toString().trimmed();
	w.writeAttribute("quality", cnv_calling_quality.isEmpty() ? "n/a" : cnv_calling_quality);
	if(data_.cnvs.caller()==CnvCallerType::CLINCNV && !cnv_callset_id.isEmpty())
	{
		QHash<QString, QString> qc_metrics = db_.cnvCallsetMetrics(cnv_callset_id.toInt());

		QString iterations = qc_metrics["number of iterations"].trimmed();
		if(!iterations.isEmpty()) w.writeAttribute("number_of_iterations", iterations);

		QString high_quality_cnvs = qc_metrics["high-quality cnvs"].trimmed();
		if(!high_quality_cnvs.isEmpty()) w.writeAttribute("number_of_hq_cnvs", high_quality_cnvs);
	}

	foreach(const ReportVariantConfiguration& var_conf, data_.report_settings.report_config->variantConfig())
	{
		if (var_conf.variant_type!=VariantType::CNVS) continue;
		if (!var_conf.showInReport()) continue;
		if (!selected_cnvs_.contains(var_conf.variant_index)) continue;
		if (var_conf.report_type!=data_.report_settings.report_type) continue;

		const CopyNumberVariant& cnv = data_.cnvs[var_conf.variant_index];

		//element Cnv
		w.writeStartElement("Cnv");
		w.writeAttribute("chr", cnv.chr().str());
		w.writeAttribute("start", QString::number(cnv.start()));
		w.writeAttribute("end", QString::number(cnv.end()));
		w.writeAttribute("start_band", NGSHelper::cytoBand(data_.build, cnv.chr(), cnv.start()));
		w.writeAttribute("end_band", NGSHelper::cytoBand(data_.build, cnv.chr(), cnv.end()));
		int cn = cnv.copyNumber(data_.cnvs.annotationHeaders());
		w.writeAttribute("type", cn>=2 ? "dup" : "del"); //2 can be dup in chrX/chrY
		w.writeAttribute("cn", QString::number(cn));
		w.writeAttribute("regions", QString::number(std::max(1, cnv.regions()))); //trio CNV lists don't contain number of regions > fix
		w.writeAttribute("causal", var_conf.causal ? "true" : "false");
		w.writeAttribute("de_novo", var_conf.de_novo ? "true" : "false");
		w.writeAttribute("comp_het", var_conf.comp_het ? "true" : "false");
		w.writeAttribute("mosaic", var_conf.mosaic ? "true" : "false");
		if (var_conf.inheritance!="n/a")
		{
			w.writeAttribute("inheritance", var_conf.inheritance);
		}
		if (var_conf.classification!="n/a")
		{
			w.writeAttribute("class", var_conf.classification);
		}
		if (!var_conf.comments.trimmed().isEmpty())
		{
			w.writeAttribute("comments_1st_assessor", var_conf.comments.trimmed());
		}
		if (!var_conf.comments2.trimmed().isEmpty())
		{
			w.writeAttribute("comments_2nd_assessor", var_conf.comments2.trimmed());
		}

		//element Gene
		foreach(const QByteArray& gene, cnv.genes())
		{
			w.writeStartElement("Gene");
			w.writeAttribute("name", gene);
			w.writeEndElement();
		}

		//element ExternalLink
		w.writeStartElement("ExternalLink");
		w.writeAttribute("url", "http://dgv.tcag.ca/gb2/gbrowse/dgv2_"+buildToString(data_.build)+"/?name=" + cnv.toString());
		w.writeAttribute("type", "DGV");
		w.writeEndElement();
		w.writeStartElement("ExternalLink");
		w.writeAttribute("url", "https://genome.ucsc.edu/cgi-bin/hgTracks?db="+buildToString(data_.build)+"&position=" + cnv.toString());
		w.writeAttribute("type", "UCSC");
		w.writeEndElement();

		w.writeEndElement();
	}
	w.writeEndElement();

	//element ReportDocument
	w.writeStartElement("ReportDocument");
	QString format = QFileInfo(html_document).suffix().toUpper();
	w.writeAttribute("format", format);
	QByteArray base64_data = "";
	if (!test_mode_)
	{
		QFile file(html_document);
		file.open(QIODevice::ReadOnly);
		base64_data = file.readAll();
		file.close();
	}
	base64_data = base64_data.toBase64();
	w.writeCharacters(base64_data);
	w.writeEndElement();

	w.writeEndDocument();
	outfile->close();

	//validate written XML file
	QString xml_error = XmlHelper::isValidXml(filename, ":/resources/GermlineReport_v4.xsd");
	if (xml_error!="")
	{
		THROW(ProgrammingException, "Invalid germline report XML file gererated: " + xml_error);
	}
}

void GermlineReportGenerator::overrideDate(QDate date)
{
	if (!test_mode_) THROW(ProgrammingException, "This function can only be used in test mode!");

	date_ = date;
}

BedFile GermlineReportGenerator::precalculatedGaps(QString low_cov_file, const BedFile& roi, int min_cov, const BedFile& processing_system_target_region)
{
	//check depth cutoff
	if (min_cov!=20) THROW(ArgumentException, "Depth cutoff is not 20!");

	//load low-coverage file
	BedFile gaps;
	gaps.load(low_cov_file);

	//extract processing system ROI statistics
	int regions = -1;
	long long bases = -1;
	foreach(QString line, gaps.headers())
	{
		if (line.startsWith("#ROI bases: "))
		{
			bool ok = true;
			bases = line.mid(12).trimmed().toLongLong(&ok);
			if (!ok) bases = -1;
		}
		if (line.startsWith("#ROI regions: "))
		{
			bool ok = true;
			regions = line.mid(14).trimmed().toInt(&ok);
			if (!ok) regions = -1;
		}
	}
	if (regions<0 || bases<0) THROW(ArgumentException, "Low-coverage file header does not contain target region statistics: " + low_cov_file);

	if (processing_system_target_region.count()!=regions || processing_system_target_region.baseCount()!=bases) THROW(ArgumentException, "Low-coverage file is outdated. It does not match processing system target region: " + low_cov_file);

	//calculate gaps inside target region
	gaps.intersect(roi);

	//add target region bases not covered by processing system target file
	BedFile uncovered(roi);
	uncovered.subtract(processing_system_target_region);
	gaps.add(uncovered);
	gaps.merge();

	return gaps;
}


void GermlineReportGenerator::writeHtmlHeader(QTextStream& stream, QString sample_name)
{
	stream << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" << endl;
	stream << "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl;
	stream << "	<head>" << endl;
	stream << "	   <title>Report " << sample_name << "</title>" << endl;
	stream << "	   <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />" << endl;
	stream << "	   <style type=\"text/css\">" << endl;
	stream << "		<!--" << endl;
	stream << "body" << endl;
	stream << "{" << endl;
	stream << "	font-family: Calibri, sans-serif;" << endl;
	stream << "	font-size: 8pt;" << endl;
	stream << "}" << endl;
	stream << "h4" << endl;
	stream << "{" << endl;
	stream << "	font-family: Calibri, sans-serif;" << endl;
	stream << "	font-size: 10pt;" << endl;
	stream << "}" << endl;
	stream << "table" << endl;
	stream << "{" << endl;
	stream << "	border-collapse: collapse;" << endl;
	stream << "	border: 1px solid black;" << endl;
	stream << "	width: 100%;" << endl;
	stream << "}" << endl;
	stream << "th, td" << endl;
	stream << "{" << endl;
	stream << "	border: 1px solid black;" << endl;
	stream << "	font-size: 8pt;" << endl;
	stream << "	text-align: left;" << endl;
	stream << "}" << endl;
	stream << "p" << endl;
	stream << "{" << endl;
	stream << " margin-bottom: 0cm;" << endl;
	stream << "}" << endl;
	stream << "		-->" << endl;
	stream << "	   </style>" << endl;
	stream << "	</head>" << endl;
	stream << "	<body>" << endl;
}

void GermlineReportGenerator::writeHtmlFooter(QTextStream& stream)
{
	stream << "	</body>" << endl;
	stream << "</html>" << endl;
}

QString GermlineReportGenerator::trans(const QString& text)
{
	//init translation tables (once)
	static QHash<QString, QString> en2de;
	if (en2de.isEmpty())
	{
		en2de["male"] = "m&auml;nnlich";
		en2de["female"] = "weiblich";
	}

	static QHash<QString, QString> de2en;
	if (de2en.isEmpty())
	{
		de2en["male"] = "male";
		de2en["female"] = "female";
		de2en["Technischer Report zur bioinformatischen Analyse"] = "Technical Report for Bioinformatic Analysis";
		de2en["Probe"] = "Sample";
		de2en["Prozessierungssystem"] = "Processing system";
		de2en["Prozessierungssystem-Typ"] = "Processing system type";
		de2en["Referenzgenom"] = "Reference genome";
		de2en["Datum"] = "Date";
		de2en["Benutzer"] = "User";
		de2en["Analysepipeline"] = "Analysis pipeline";
		de2en["Auswertungssoftware"] = "Analysis software";
		de2en["KASP-Ergebnis"] = " KASP result";
		de2en["Ph&auml;notyp"] = "Phenotype information";
		de2en["Filterkriterien"] = "Criteria for variant filtering";
		de2en["Gefundene Varianten in Zielregion gesamt"] = "Variants in target region";
		de2en["Anzahl Varianten ausgew&auml;hlt f&uuml;r Report"] = "Variants selected for report";
		de2en["Anzahl CNVs ausgew&auml;hlt f&uuml;r Report"] = "CNVs selected for report";
		de2en["Anzahl SVs ausgew&auml;hlt f&uuml;r Report"] = "SVs selected for report";
		de2en["Varianten nach klinischer Interpretation im Kontext der Fragestellung"] = "List of prioritized variants";
		de2en["Vererbung"] = "Inheritance";
		de2en["Klasse"] = "Class";
		de2en["Details"] = "Details";
		de2en["Genotyp"] = "Genotype";
		de2en["Variante"] = "Variant";
		de2en["Gen"] = "Gene";
		de2en["F&uuml;r Informationen zur Klassifizierung von Varianten, siehe allgemeine Zusatzinformationen."] = "For further information regarding the classification see Additional Information.";
		de2en["Teilweise k&ouml;nnen bei Varianten unklarer Signifikanz (Klasse 3) -  in Abh&auml;ngigkeit von der Art der genetischen Ver&auml;nderung, der Familienanamnese und der Klinik des/der Patienten - weiterf&uuml;hrende Untersuchungen eine &Auml;nderung der Klassifizierung bewirken. Bei konkreten differentialdiagnostischen Hinweisen auf eine entsprechende Erkrankung k&ouml;nnen ggf. weiterf&uuml;hrende genetische Untersuchungen indiziert sein."] = "Depending on the type of genetic alteration, family history and clinical features of the patient further investigations might change the classification of variants of unknown significance (class 3). In case of a suspected clinical diagnosis genetic counseling is necessary to evaluate the indication/possibility of further genetic studies.";
		de2en["Klassifikation von Varianten"] = "Classification of variants";
		de2en["Die Klassifikation der Varianten erfolgt in Anlehnung an die Publikation von Plon et al. (Hum Mutat 2008)"] = "Classification and interpretation of variants: The classification of variants is based on the criteria of Plon et al. (PMID: 18951446). A short description of each class can be found in the following";
		de2en["Klasse 5: Eindeutig pathogene Ver&auml;nderung / Mutation"] = "Class 5, pathogenic variant";
		de2en["Ver&auml;nderung, die bereits in der Fachliteratur mit ausreichender Evidenz als krankheitsverursachend bezogen auf das vorliegende Krankheitsbild beschrieben wurde sowie als pathogen zu wertende Mutationstypen (i.d.R. Frameshift- bzw. Stoppmutationen)."] = "The variant is considered to be the cause of the patient's disease.";
		de2en["Klasse 4: Wahrscheinlich pathogene Ver&auml;nderung"] = "Class 4, probably pathogenic variants";
		de2en["DNA-Ver&auml;nderung, die aufgrund ihrer Eigenschaften als sehr wahrscheinlich krankheitsverursachend zu werten ist."] = "The identified variant is considered to be the probable cause of the patient's disease. This information should be used cautiously for clinical decision-making, as there is still a degree of uncertainty.";
		de2en["Klasse 3: Variante unklarer Signifikanz (VUS) - Unklare Pathogenit&auml;t"] = "Class 3, variant of unclear significance (VUS)";
		de2en["Variante, bei der es unklar ist, ob eine krankheitsverursachende Wirkung besteht. Diese Varianten werden tabellarisch im technischen Report mitgeteilt."] = "The variant has characteristics of being an independent disease-causing mutation, but insufficient or conflicting evidence exists.";
		de2en["Klasse 2: Sehr wahrscheinlich benigne Ver&auml;nderungen"] = "Class 2, most likely benign variants";
		de2en["Aufgrund der H&auml;ufigkeit in der Allgemeinbev&ouml;lkerung oder der Lokalisation bzw. aufgrund von Angaben in der Literatur sehr wahrscheinlich benigne. Werden nicht mitgeteilt, k&ouml;nnen aber erfragt werden."] = "The variant is not likely to be the cause of the tested disease. Class 2 variants are not reported, but can be provided upon request.";
		de2en["Klasse 1: Benigne Ver&auml;nderungen"] = "Class 1, benign variants";
		de2en["Werden nicht mitgeteilt, k&ouml;nnen aber erfragt werden."] = "The variant is not considered to be the cause of the tested disease. Class 1 variants are not reported, but can be provided upon request.";
		de2en["Zielregion"] = "Target region";
		de2en["Die Zielregion umfasst mindestens die CCDS (\"consensus coding sequence\") unten genannter Gene &plusmn;20 Basen flankierender intronischer Sequenz, kann aber auch zus&auml;tzliche Exons und/oder flankierende Basen beinhalten."] = "The target region includes CCDS (\"consensus coding sequence\") of the genes listed below &plusmn;20 flanking bases of the intronic sequence. It may comprise additional exons and/or flanking bases.";
		de2en["Name"] = "Name";
		de2en["Ausgewertete Gene"] = "Genes analyzed";
		de2en["OMIM Gene und Phenotypen"] = "OMIM gene and phenotypes";
		de2en["Phenotyp"] = "phenotype";
		de2en["Gen MIM"] = "gene MIM";
		de2en["Phenotyp MIM"] = "phenotype MIM";
		de2en["Gen(e)"] = "Genes";
		de2en["Details zu Programmen der Analysepipeline"] = "Analysis pipeline tool details";
		de2en["Parameter"] = "Parameters";
		de2en["Version"] = "Version";
		de2en["Tool"] = "Tool";
		de2en["Abdeckungsstatistik"] = "Coverage statistics";
		de2en["Durchschnittliche Sequenziertiefe"] = "Average sequencing depth";
		de2en["Durchschnittliche Sequenziertiefe (chrMT)"] = "Average sequencing depth (chrMT)";
		de2en["Komplett abgedeckte Gene"] = "Genes without gaps";
		de2en["Anteil Regionen mit Tiefe &lt;"] = "Percentage of regions with depth &lt;";
		de2en["Fehlende Basen in nicht komplett abgedeckten Genen"] = "Number of missing bases for genes with gaps";
		de2en["Details Regionen mit Tiefe &lt;"] = "Details regions with depth &lt;";
		de2en["Koordinaten (hg19)"] = "Coordinates (hg19)";
		de2en["Chromosom"] = "Chromosome";
		de2en["Basen"] = "Bases";
		de2en["Abdeckungsstatistik f&uuml;r CCDS"] = "Coverage statistics for CCDS";
		de2en["Gr&ouml;&szlig;e"] = "Size";
		de2en["Transcript"] = "Transcript";
		de2en["gesamt"] = "overall";
		de2en["mit Tiefe"] = "with depth";
		de2en["Geschlecht"] = "sample sex";
		de2en["Vater"] = "father";
		de2en["Mutter"] = "mother";
		de2en["Regionen"] = "regions";
		de2en["Gene"] = "genes";
		de2en["CNV"] = "CNV";
		de2en["CN"] = "CN";
		de2en["n/a"] = "n/a";
		de2en["SV"] = "SV";
		de2en["Position"] = "Position";
		de2en["Deletion"] = "deletion";
		de2en["Duplikation"] = "duplication";
		de2en["Insertion"] = "insertion";
		de2en["Inversion"] = "inversion";
		de2en["Translokation"] = "translocation";
		de2en["In der folgenden Tabelle werden neben wahrscheinlich pathogenen (Klasse 4) und pathogenen (Klasse 5) nur solche Varianten unklarer klinischer Signifikanz (Klasse 3) gelistet, f&uuml;r die in Zusammenschau von Literatur und Klinik des Patienten ein Beitrag zur Symptomatik denkbar ist und f&uuml;r die gegebenenfalls eine weitere Einordnung der klinischen Relevanz durch Folgeuntersuchungen sinnvoll ist. Eine Liste aller detektierten Varianten kann bei Bedarf angefordert werden."] = "In addition to likely pathogenic variants (class 4) and pathogenic variants (class 5), the following table contains only those variants of uncertain significance (class 3), for which a contribution to the clinical symptoms of the patient is conceivable and for which a further evaluation of the clinical relevance by follow-up examinations may be useful.  A list of all detected variants can be provided on request.";
		de2en["L&uuml;ckenschluss"] = "Gaps closed";
		de2en["L&uuml;cken die mit Sanger-Sequenzierung geschlossen wurden:"] = "Gaps closed by Sanger sequencing:";
		de2en["L&uuml;cken die mit visueller Inspektion der Rohdaten &uuml;berpr&uuml;ft wurden:"] = "Gaps checked by visual inspection of raw data:";
		de2en["Basen gesamt:"] = "Base sum:";
		de2en["Polygener Risiko-Score (PRS)"] = "Polygenic Risk Scores (PRS)";
		de2en["Erkrankung"] = "Trait";
		de2en["Score"] = "Score";
		de2en["Publikation"] = "Publication";
		de2en["Hauptphenotyp"] = "preferred phenotype";
		de2en["ja"] = "yes";
		de2en["nein"] = "no";
		de2en["Z-Score"] = "z-score";
		de2en["Population (gesch&auml;tzt aus NGS)"] = "population (estimated from NGS)";
		de2en["Die Einsch&auml;tzung der klinischen Bedeutung eines PRS ist nur unter Verwendung eines entsprechenden validierten Risiko-Kalkulations-Programms und unter Ber&uuml;cksichtigung der ethnischen Zugeh&ouml;rigkeit m&ouml;glich (z.B. CanRisk.org f&uuml;r Brustkrebs)."] = "A validated risk estimation program must be used to judge the clinical importance of a PRS, e.g. CanRisk.org for breast cancer. The ethnicity of the patient must also be considered.";
	}

	//translate
	if (data_.report_settings.language=="german")
	{
		if (en2de.contains(text)) return en2de[text];

		return text;
	}
	else if (data_.report_settings.language=="english")
	{
		if (de2en.contains(text)) return de2en[text];

		if (test_mode_)
		{
			THROW(ProgrammingException, "Could not translate '" + text + "' to " + data_.report_settings.language + "!");
		}
		else
		{
			Log::warn("Could not translate '" + text + "' to " + data_.report_settings.language + "!");
		}

		return text;
	}

	THROW(ProgrammingException, "Unsupported language '" + data_.report_settings.language + "'!");
}

void GermlineReportGenerator::writeCoverageReport(QTextStream& stream)
{
	//get target region coverages (from NGSD or calculate)
	QString avg_cov = "";
	QCCollection stats;
	bool roi_is_system_target_region = data_.processing_system_roi.count()==data_.roi.regions.count() && data_.processing_system_roi.baseCount()==data_.roi.regions.baseCount();
	if (roi_is_system_target_region || !data_.report_settings.recalculate_avg_depth)
	{
		try
		{
			stats = db_.getQCData(ps_id_);
		}
		catch(...)
		{
		}
	}
	if (stats.count()==0)
	{
		Log::warn("Average target region depth from NGSD cannot be used! Recalculating it...");

		QString ref_file = Settings::string("reference_genome");
		stats = Statistics::mapping(data_.roi.regions, data_.ps_bam, ref_file);
	}
	for (int i=0; i<stats.count(); ++i)
	{
		if (stats[i].accession()=="QC:2000025") avg_cov = stats[i].toString();
	}
	stream << endl;
	stream << "<p><b>" << trans("Abdeckungsstatistik") << "</b>" << endl;
	stream << "<br />" << trans("Durchschnittliche Sequenziertiefe") << ": " << avg_cov << endl;
	BedFile mito_bed;
	mito_bed.append(BedLine("chrMT", 1, 16569));
	Statistics::avgCoverage(mito_bed, data_.ps_bam, 1, false, true);
	stream << "<br />" << trans("Durchschnittliche Sequenziertiefe (chrMT)") << ": " << mito_bed[0].annotations()[0] << endl;
	stream << "</p>" << endl;

	if (data_.report_settings.roi_low_cov)
	{
		//calculate low-coverage regions
		BedFile low_cov;
		try
		{
			low_cov = GermlineReportGenerator::precalculatedGaps(data_.ps_lowcov, data_.roi.regions, data_.report_settings.min_depth, data_.processing_system_roi);
		}
		catch(Exception e)
		{
			Log::warn("Low-coverage statistics needs to be calculated. Pre-calulated gap file cannot be used because: " + e.message());
			low_cov = Statistics::lowCoverage(data_.roi.regions, data_.ps_bam, data_.report_settings.min_depth);
		}

		//annotate low-coverage regions with gene names
		low_cov.clearAnnotations();
		for(int i=0; i<low_cov.count(); ++i)
		{
			BedLine& line = low_cov[i];
			GeneSet genes = db_.genesOverlapping(line.chr(), line.start(), line.end(), 20); //extend by 20 to annotate splicing regions as well
			line.annotations().append(genes.join(", "));
		}

		//group by gene name
		QMap<QByteArray, BedFile> grouped;
		for (int i=0; i<low_cov.count(); ++i)
		{
			QList<QByteArray> genes = low_cov[i].annotations()[0].split(',');
			foreach(QByteArray gene, genes)
			{
				gene = gene.trimmed();

				//skip non-gene regions
				// - remains of VEGA database in old HaloPlex designs
				// - SNPs for sample identification
				if (gene=="") continue;

				grouped[gene].append(low_cov[i]);
			}
		}

		//output
		if (!data_.roi.genes.isEmpty())
		{
			QStringList complete_genes;
			foreach(const QByteArray& gene, data_.roi.genes)
			{
				if (!grouped.contains(gene))
				{
					complete_genes << gene;
				}
			}
			stream << "<br />" << trans("Komplett abgedeckte Gene") << ": " << complete_genes.join(", ") << endl;
		}
		QString gap_perc = "";
		if (data_.roi.regions.baseCount()>0)
		{
			gap_perc = QString::number(100.0*low_cov.baseCount()/data_.roi.regions.baseCount(), 'f', 2);
		}
		stream << "<br />" << trans("Anteil Regionen mit Tiefe &lt;") << data_.report_settings.min_depth << ": " << gap_perc << "%" << endl;
		cache_["gap_percentage"] = gap_perc;
		if (!data_.roi.genes.isEmpty())
		{
			QStringList incomplete_genes;
			foreach(const QByteArray& gene, data_.roi.genes)
			{
				if (grouped.contains(gene))
				{
					incomplete_genes << gene + " <span style=\"font-size: 8pt;\">" + QString::number(grouped[gene].baseCount()) + "</span> ";
				}
			}
			stream << "<br />" << trans("Fehlende Basen in nicht komplett abgedeckten Genen") << ": " << incomplete_genes.join(", ") << endl;
		}

		stream << "<p>" << trans("Details Regionen mit Tiefe &lt;") << data_.report_settings.min_depth << ":" << endl;
		stream << "</p>" << endl;
		stream << "<table>" << endl;
		stream << "<tr><td><b>" << trans("Gen") << "</b></td><td><b>" << trans("Basen") << "</b></td><td><b>" << trans("Chromosom") << "</b></td><td><b>" << trans("Koordinaten (hg19)") << "</b></td></tr>" << endl;
		for (auto it=grouped.cbegin(); it!=grouped.cend(); ++it)
		{
			stream << "<tr>" << endl;
			stream << "<td>" << endl;
			const BedFile& gaps = it.value();
			QString chr = gaps[0].chr().strNormalized(true);
			QStringList coords;
			for (int i=0; i<gaps.count(); ++i)
			{
				coords << QString::number(gaps[i].start()) + "-" + QString::number(gaps[i].end());
			}
			stream << it.key() << "</td><td>" << gaps.baseCount() << "</td><td>" << chr << "</td><td>" << coords.join(", ") << endl;

			stream << "</td>" << endl;
			stream << "</tr>" << endl;
		}
		stream << "</table>" << endl;
	}
}

void GermlineReportGenerator::writeClosedGapsReport(QTextStream& stream, const BedFile& roi)
{
	ChromosomalIndex<BedFile> roi_idx(roi);
	//init
	SqlQuery query = db_.getQuery();
	query.prepare("SELECT chr, start, end, status FROM gaps WHERE processed_sample_id='" + ps_id_ + "' AND status=:0 ORDER BY chr,start,end");

	stream << endl;
	stream << "<p><b>" << trans("L&uuml;ckenschluss") << "</b></p>" << endl;

	//closed by Sanger
	{
		int base_sum = 0;
		stream << "<p>" << trans("L&uuml;cken die mit Sanger-Sequenzierung geschlossen wurden:") << "<br />";
		stream << "<table>" << endl;
		stream << "<tr><td><b>" << trans("Gen") << "</b></td><td><b>" << trans("Basen") << "</b></td><td><b>" << trans("Chromosom") << "</b></td><td><b>" << trans("Koordinaten (hg19)") << "</b></td></tr>" << endl;
		query.bindValue(0, "closed");
		query.exec();
		while(query.next())
		{
			Chromosome chr = query.value("chr").toString();
			int start = query.value("start").toInt();
			int end = query.value("end").toInt();
			if (roi_idx.matchingIndex(chr, start, end)==-1) continue;

			base_sum += end-start+1;

			stream << "<tr>" << endl;
			stream << "<td>" << db_.genesOverlapping(chr, start, end).join(", ") << "</td><td>" << QString::number(end-start+1) << "</td><td>" << chr.strNormalized(true) << "</td><td>" << QString::number(start) << "-" << QString::number(end) << "</td>";
			stream << "</tr>" << endl;
		}
		stream << "</table>" << endl;
		stream << trans("Basen gesamt:") << QString::number(base_sum);
		stream << "</p>" << endl;
	}

	//closed by visual inspection
	{
		int base_sum = 0;
		stream << "<p>" << trans("L&uuml;cken die mit visueller Inspektion der Rohdaten &uuml;berpr&uuml;ft wurden:") << "<br />";
		stream << "<table>" << endl;
		stream << "<tr><td><b>" << trans("Gen") << "</b></td><td><b>" << trans("Basen") << "</b></td><td><b>" << trans("Chromosom") << "</b></td><td><b>" << trans("Koordinaten (hg19)") << "</b></td></tr>" << endl;
		query.bindValue(0, "checked visually");
		query.exec();
		while(query.next())
		{
			Chromosome chr = query.value("chr").toString();
			int start = query.value("start").toInt();
			int end = query.value("end").toInt();
			if (roi_idx.matchingIndex(chr, start, end)==-1) continue;

			base_sum += end-start+1;

			stream << "<tr>" << endl;
			stream << "<td>" << db_.genesOverlapping(chr, start, end).join(", ") << "</td><td>" << QString::number(end-start+1) << "</td><td>" << chr.strNormalized(true) << "</td><td>" << QString::number(start) << "-" << QString::number(end) << "</td>";
			stream << "</tr>" << endl;
		}
		stream << "</table>" << endl;
		stream << trans("Basen gesamt:") << QString::number(base_sum);
		stream << "</p>" << endl;
	}
}

void GermlineReportGenerator::writeCoverageReportCCDS(QTextStream& stream, int extend, bool gap_table, bool gene_details)
{
	QString ext_string = (extend==0 ? "" : " +-" + QString::number(extend) + " ");
	stream << endl;
	stream << "<p><b>" << trans("Abdeckungsstatistik f&uuml;r CCDS") << " " << ext_string << "</b></p>" << endl;
	if (gap_table) stream << "<p><table>" << endl;
	if (gap_table) stream << "<tr><td><b>" << trans("Gen") << "</b></td><td><b>" << trans("Transcript") << "</b></td><td><b>" << trans("Gr&ouml;&szlig;e") << "</b></td><td><b>" << trans("Basen") << "</b></td><td><b>" << trans("Chromosom") << "</b></td><td><b>" << trans("Koordinaten (hg19)") << "</b></td></tr>";
	QMap<QByteArray, int> gap_count;
	long long bases_overall = 0;
	long long bases_sequenced = 0;
	GeneSet genes_noncoding;
	GeneSet genes_notranscript;
	foreach(const QByteArray& gene, data_.roi.genes)
	{
		int gene_id = db_.geneToApprovedID(gene);

		//approved gene symbol
		QByteArray symbol = db_.geneSymbol(gene_id);

		//longest coding transcript
		Transcript transcript = db_.longestCodingTranscript(gene_id, Transcript::CCDS, true);
		if (!transcript.isValid())
		{
			transcript = db_.longestCodingTranscript(gene_id, Transcript::CCDS, true, true);
			if (!transcript.isValid() || transcript.codingRegions().baseCount()==0)
			{
				genes_notranscript.insert(gene);
				if (gap_table) stream << "<tr><td>" + symbol + "</td><td>n/a</td><td>n/a</td><td>n/a</td><td>n/a</td><td>n/a</td></tr>";
				continue;
			}
			else
			{
				genes_noncoding.insert(gene);
			}
		}

		//gaps
		BedFile roi = transcript.codingRegions();
		if (extend>0)
		{
			roi.extend(extend);
			roi.merge();
		}
		BedFile gaps;
		try
		{
			gaps = GermlineReportGenerator::precalculatedGaps(data_.ps_lowcov, roi, data_.report_settings.min_depth, data_.processing_system_roi);
		}
		catch(Exception e)
		{
			Log::warn("Low-coverage statistics for transcript " + transcript.name() + " needs to be calculated. Pre-calulated gap file cannot be used because: " + e.message());
			gaps = Statistics::lowCoverage(roi, data_.ps_bam, data_.report_settings.min_depth);
		}

		long long bases_transcipt = roi.baseCount();
		long long bases_gaps = gaps.baseCount();
		QStringList coords;
		for (int i=0; i<gaps.count(); ++i)
		{
			coords << QString::number(gaps[i].start()) + "-" + QString::number(gaps[i].end());
		}
		if (gap_table) stream << "<tr><td>" + symbol + "</td><td>" << transcript.name() << "</td><td>" << bases_transcipt << "</td><td>" << bases_gaps << "</td><td>" << roi[0].chr().strNormalized(true) << "</td><td>" << coords.join(", ") << "</td></tr>";
		gap_count[symbol] += bases_gaps;
		bases_overall += bases_transcipt;
		bases_sequenced += bases_transcipt - bases_gaps;
	}
	if (gap_table) stream << "</table></p>" << endl;

	//show warning if non-coding transcripts had to be used
	if (!genes_noncoding.isEmpty())
	{
		stream << "<br />Warning: Using the longest *non-coding* transcript for genes " << genes_noncoding.join(", ") << " (no coding transcripts for GRCh37 defined)";
	}
	if (!genes_notranscript.isEmpty())
	{
		stream << "<br />Warning: No transcript defined for genes " << genes_notranscript.join(", ");
	}

	//overall statistics
	stream << "<p>CCDS " << ext_string << trans("gesamt") << ": " << bases_overall << endl;
	stream << "<br />CCDS " << ext_string << trans("mit Tiefe") << " &ge;" << data_.report_settings.min_depth << ": " << bases_sequenced << " (" << QString::number(100.0 * bases_sequenced / bases_overall, 'f', 2)<< "%)" << endl;
	long long gaps = bases_overall - bases_sequenced;
	stream << "<br />CCDS " << ext_string << trans("mit Tiefe") << " &lt;" << data_.report_settings.min_depth << ": " << gaps << " (" << QString::number(100.0 * gaps / bases_overall, 'f', 2)<< "%)" << endl;
	stream << "</p>" << endl;

	//gene statistics
	if (gene_details)
	{
		QByteArrayList genes_complete;
		QByteArrayList genes_incomplete;
		for (auto it = gap_count.cbegin(); it!=gap_count.cend(); ++it)
		{
			if (it.value()==0)
			{
				genes_complete << it.key();
			}
			else
			{
				genes_incomplete << it.key() + " <span style=\"font-size: 8pt;\">" + QByteArray::number(it.value()) + "</span> ";
			}
		}
		stream << "<p>";
		stream << trans("Komplett abgedeckte Gene") << ": " << genes_complete.join(", ") << endl;
		stream << "<br />" << trans("Fehlende Basen in nicht komplett abgedeckten Genen") << ": " << genes_incomplete.join(", ") << endl;
		stream << "</p>";
	}

	if (extend==0) cache_["ccds_sequenced"] = QString::number(bases_sequenced);
}

QByteArray GermlineReportGenerator::formatGenotype(GenomeBuild build, const QByteArray& gender, const QByteArray& genotype, const Variant& variant)
{
	//correct only hom variants on gonosomes outside the PAR for males
	if (gender!="male") return genotype;
	if (genotype!="hom") return genotype;
	if (!variant.chr().isGonosome()) return genotype;
	if (NGSHelper::pseudoAutosomalRegion(build).overlapsWith(variant.chr(), variant.start(), variant.end())) return genotype;

	return "hemi";
}

QString GermlineReportGenerator::formatCodingSplicing(const QList<VariantTranscript>& transcripts)
{
	QMap<QString, QStringList> gene_infos;
	QMap<QString, QStringList> gene_infos_pt;

	foreach(const VariantTranscript& trans, transcripts)
	{
		QByteArray line = trans.gene + ":" + trans.id + ":" + trans.hgvs_c + ":" + trans.hgvs_p;

		gene_infos[trans.gene].append(line);

		if (data_.preferred_transcripts.contains(trans.gene) && data_.preferred_transcripts.value(trans.gene).contains(trans.idWithoutVersion()))
		{
			gene_infos_pt[trans.gene].append(line);
		}
	}

	//return preferred transcripts only, if present
	QStringList output;
	foreach(QString gene, gene_infos.keys())
	{
		if (gene_infos_pt.contains(gene))
		{
			output << gene_infos_pt[gene];
		}
		else
		{
			output << gene_infos[gene];
		}
	}
	return output.join("<br />");
}

void GermlineReportGenerator::writeEvaluationSheet(QString filename, const EvaluationSheetData& evaluation_sheet_data)
{
	QSharedPointer<QFile> file = Helper::openFileForWriting(filename);
	QTextStream stream(file.data());

	//write header
	stream << "<html>" << endl;
	stream << "  <head>" << endl;
	stream << "    <style>" << endl;
	stream << "      @page" << endl;
	stream << "      {" << endl;
	stream << "        size: landscape;" << endl;
	stream << "        margin: 1cm;" << endl;
	stream << "      }" << endl;
	stream << "      table" << endl;
	stream << "      {" << endl;
	stream << "        border-collapse: collapse;" << endl;
	stream << "        border: 1px solid black;" << endl;
	stream << "      }" << endl;
	stream << "      th, td" << endl;
	stream << "      {" << endl;
	stream << "        border: 1px solid black;" << endl;
	stream << "      }" << endl;
	stream << "      .line {" << endl;
	stream << "        display: inline-block;" << endl;
	stream << "        border-bottom: 1px solid #000;" << endl;
	stream << "        width: 250px;" << endl;
	stream << "        margin-left: 10px;" << endl;
	stream << "        margin-right: 10px;" << endl;
	stream << "      }" << endl;
	stream << "      .noborder {" << endl;
	stream << "        border: 0px;" << endl;
	stream << "      }" << endl;
	stream << "    </style>" << endl;
	stream << "  </head>" << endl;
	stream << "  <body>" << endl;
	stream << "    <table class='noborder' width='100%'>" << endl;
	stream << "      <tr>" << endl;
	stream << "        <td class='noborder' valign='top'>" << endl;
	stream << "           <h3>Probe: " << data_.ps << "</h3>" << endl;
	stream << "        </td>" << endl;
	stream << "      </tr>" << endl;
	stream << "    </table>" << endl;
	stream << "    <table class='noborder' width='100%'>" << endl;
	stream << "      <tr>" << endl;
	stream << "        <td class='noborder' valign='top'>" << endl;
	stream << "          <p>DNA/RNA#: <span class='line'>" << evaluation_sheet_data.dna_rna << "</span></p>" << endl;
	stream << "          <br />" << endl;
	stream << "          <p>1. Auswerter: <span class='line'>" << evaluation_sheet_data.reviewer1 << "</span> Datum: <span class='line'>" << evaluation_sheet_data.review_date1.toString("dd.MM.yyyy") << "</span></p>" << endl;
	stream << "          <p><nobr>2. Auswerter: <span class='line'>" << evaluation_sheet_data.reviewer2 << "</span> Datum: <span class='line'>" << evaluation_sheet_data.review_date2.toString("dd.MM.yyyy") << "</span></nobr></p>" << endl;
	stream << "          <br />" << endl;
	stream << "          <p>Auswerteumfang: <span class='line'>" << evaluation_sheet_data.analysis_scope << "</span></p>" << endl;
	stream << "          <br />" << endl;
	stream << "          <table border='0'>" << endl;
	stream << "            <tr> <td colspan='2'><b>ACMG</b></td> </tr>" << endl;
	stream << "            <tr> <td>angefordert: &nbsp;&nbsp; </td> <td>"<< ((evaluation_sheet_data.acmg_requested)?"&#9745;":"&#9633;") << "</td> </tr>" << endl;
	stream << "            <tr> <td>analysiert: &nbsp;&nbsp; </td> <td>"<< ((evaluation_sheet_data.acmg_analyzed)?"&#9745;":"&#9633;") << "</td> </tr>" << endl;
	stream << "            <tr> <td>auff&auml;llig: &nbsp;&nbsp; </td> <td>"<< ((evaluation_sheet_data.acmg_noticeable)?"&#9745;":"&#9633;") << "</td> </tr>" << endl;
	stream << "          </table>" << endl;
	stream << "        </td>" << endl;
	stream << "        <td class='noborder' valign='top' style='width: 1%; white-space: nowrap;'>" << endl;
	stream << "          <table border='0'>" << endl;
	stream << "            <tr> <td colspan='2'><b>Filterung erfolgt</b></td> </tr>" << endl;
	stream << "            <tr> <td style='white-space: nowrap'>Freq.-basiert dominant&nbsp;&nbsp;</td> <td>"<< ((evaluation_sheet_data.filtered_by_freq_based_dominant)?"&#9745;":"&#9633;") << "</td> </tr>" << endl;
	stream << "            <tr> <td>Freq.-basiert rezessiv</td> <td>"<< ((evaluation_sheet_data.filtered_by_freq_based_recessive)?"&#9745;":"&#9633;") << "</td> </tr>" << endl;
	stream << "            <tr> <td>Mitochondrial</td> <td>"<< ((evaluation_sheet_data.filtered_by_mito)?"&#9745;":"&#9633;") << "</td> </tr>" << endl;
	stream << "            <tr> <td>X-chromosomal</td> <td>"<< ((evaluation_sheet_data.filtered_by_x_chr)?"&#9745;":"&#9633;") << "</td> </tr>" << endl;
	stream << "            <tr> <td>CNV</td> <td>"<< ((evaluation_sheet_data.filtered_by_cnv)?"&#9745;":"&#9633;") << "</td> </tr>" << endl;
	stream << "            <tr> <td>Strukturvarianten</td> <td>"<< ((evaluation_sheet_data.filtered_by_svs)?"&#9745;":"&#9633;") << "</td> </tr>" << endl;
	stream << "            <tr> <td>Repeat Expansions</td> <td>"<< ((evaluation_sheet_data.filtered_by_res)?"&#9745;":"&#9633;") << "</td> </tr>" << endl;
	stream << "            <tr> <td>Mosaikvarianten</td> <td>"<< ((evaluation_sheet_data.filtered_by_mosaic)?"&#9745;":"&#9633;") << "</td> </tr>" << endl;
	stream << "            <tr> <td>Ph&auml;notyp-basiert</td> <td>"<< ((evaluation_sheet_data.filtered_by_phenotype)?"&#9745;":"&#9633;") << "</td> </tr>" << endl;
	stream << "            <tr> <td>Multi-Sample-Auswertung</td> <td>"<< ((evaluation_sheet_data.filtered_by_multisample)?"&#9745;":"&#9633;") << "</td> </tr>" << endl;
	stream << "            <tr> <td>Trio stringent</td> <td>"<< ((evaluation_sheet_data.filtered_by_trio_stringent)?"&#9745;":"&#9633;") << "</td> </tr>" << endl;
	stream << "            <tr> <td>Trio relaxed</td> <td>"<< ((evaluation_sheet_data.filtered_by_trio_relaxed)?"&#9745;":"&#9633;") << "</td> </tr>" << endl;
	stream << "          </table>" << endl;
	stream << "          <br />" << endl;
	stream << "        </td>" << endl;
	stream << "      </tr>" << endl;
	stream << "    </table>" << endl;

	//phenotype
	QString sample_id = db_.sampleId(data_.ps);
	QList<SampleDiseaseInfo> disease_infos = db_.getSampleDiseaseInfo(sample_id);
	QString clinical_phenotype;
	QStringList infos;
	foreach(const SampleDiseaseInfo& info, disease_infos)
	{
		if (info.type=="ICD10 code")
		{
			infos << info.type + ": " + info.disease_info;
		}
		if (info.type=="HPO term id")
		{
			int hpo_id = db_.phenotypeIdByAccession(info.disease_info.toLatin1(), false);
			if (hpo_id!=-1)
			{
				infos << db_.phenotype(hpo_id).toString();
			}
		}
		if (info.type=="Orpha number")
		{
			infos << info.type + ": " + info.disease_info;
		}
		if (info.type=="clinical phenotype (free text)")
		{
			clinical_phenotype += info.disease_info + " ";
		}
	}

	stream << "    <br />" << endl;
	stream << "    <b>Klinik:</b>" << endl;
	stream << "    <table class='noborder' width='100%'>" << endl;
	stream << "      <tr>" << endl;
	stream << "        <td class='noborder' valign='top'>" << endl;
	stream << "          " << clinical_phenotype.trimmed() << endl;
	stream << "        </td>" << endl;
	stream << "        <td class='noborder' style='width: 1%; white-space: nowrap;'>" << endl;
	stream << "          " << infos.join("<br />          ") << endl;
	stream << "        </td>" << endl;
	stream << "      </tr>" << endl;
	stream << "    </table>" << endl;

	//write small variants
	stream << "    <p><b>Kausale Varianten:</b>" << endl;
	stream << "      <table border='1'>" << endl;
	printVariantSheetRowHeader(stream, true);
	foreach(const ReportVariantConfiguration& conf, data_.report_settings.report_config->variantConfig())
	{
		if (conf.variant_type!=VariantType::SNVS_INDELS) continue;
		if (conf.causal)
		{
			printVariantSheetRow(stream, conf);
		}
	}
	stream << "      </table>" << endl;
	stream << "    </p>" << endl;

	stream << "    <p><b>Sonstige Varianten:</b>" << endl;
	stream << "      <table border='1'>" << endl;
	printVariantSheetRowHeader(stream, false);
	foreach(const ReportVariantConfiguration& conf, data_.report_settings.report_config->variantConfig())
	{
		if (conf.variant_type!=VariantType::SNVS_INDELS) continue;
		if (!conf.causal)
		{
			printVariantSheetRow(stream, conf);
		}
	}
	stream << "      </table>" << endl;
	stream << "    </p>" << endl;

	//CNVs
	stream << "    <p><b>Kausale CNVs:</b>" << endl;
	stream << "      <table border='1'>" << endl;
	printVariantSheetRowHeaderCnv(stream, true);
	foreach(const ReportVariantConfiguration& conf, data_.report_settings.report_config->variantConfig())
	{
		if (conf.variant_type!=VariantType::CNVS) continue;
		if (conf.causal)
		{
			printVariantSheetRowCnv(stream, conf);
		}
	}
	stream << "      </table>" << endl;
	stream << "    </p>" << endl;

	stream << "    <p><b>Sonstige CNVs:</b>" << endl;
	stream << "      <table border='1'>" << endl;
	printVariantSheetRowHeaderCnv(stream, false);
	foreach(const ReportVariantConfiguration& conf, data_.report_settings.report_config->variantConfig())
	{
		if (conf.variant_type!=VariantType::CNVS) continue;
		if (!conf.causal)
		{
			printVariantSheetRowCnv(stream, conf);
		}
	}
	stream << "      </table>" << endl;
	stream << "    </p>" << endl;

	//SVs
	stream << "    <p><b>Kausale SVs:</b>" << endl;
	stream << "      <table border='1'>" << endl;
	printVariantSheetRowHeaderSv(stream, true);
	foreach(const ReportVariantConfiguration& conf, data_.report_settings.report_config->variantConfig())
	{
		if (conf.variant_type!=VariantType::SVS) continue;
		if (conf.causal)
		{
			printVariantSheetRowSv(stream, conf);
		}
	}
	stream << "      </table>" << endl;
	stream << "    </p>" << endl;

	stream << "    <p><b>Sonstige SVs:</b>" << endl;
	stream << "      <table border='1'>" << endl;
	printVariantSheetRowHeaderSv(stream, false);
	foreach(const ReportVariantConfiguration& conf, data_.report_settings.report_config->variantConfig())
	{
		if (conf.variant_type!=VariantType::SVS) continue;
		if (!conf.causal)
		{
			printVariantSheetRowSv(stream, conf);
		}
	}
	stream << "      </table>" << endl;
	stream << "    </p>" << endl;

	//write footer
	stream << "  </body>" << endl;
	stream << "</html>" << endl;
	stream.flush();

	//validate written file
	QString validation_error = XmlHelper::isValidXml(filename);
	if (validation_error!="")
	{
		if (test_mode_)
		{
			THROW(ProgrammingException, "Invalid germline report HTML file gererated: " + validation_error);
		}
		else
		{
			Log::warn("Generated evaluation sheet at " + filename + " is not well-formed: " + validation_error);
		}
	}
}

void GermlineReportGenerator::printVariantSheetRowHeader(QTextStream& stream, bool causal)
{
	stream << "     <tr>" << endl;
	stream << "       <th>Gen</th>" << endl;
	stream << "       <th>Typ</th>" << endl;
	stream << "       <th>Genotyp</th>" << endl;
	stream << "       <th>Variante</th>" << endl;
	stream << "       <th>Erbgang</th>" << endl;
	if (causal)
	{
		stream << "       <th>c.</th>" << endl;
		stream << "       <th>p.</th>" << endl;
	}
	else
	{
		stream << "       <th>Ausschlussgrund</th>" << endl;
	}
	stream << "       <th>gnomAD</th>" << endl;
	stream << "       <th style='white-space: nowrap'>NGSD hom/het</th>" << endl;
	stream << "       <th style='white-space: nowrap'>Kommentar 1. Auswerter</th>" << endl;
	stream << "       <th style='white-space: nowrap'>Kommentar 2. Auswerter</th>" << endl;
	stream << "       <th>Klasse</th>" << endl;
	stream << "       <th style='white-space: nowrap'>In Report</th>" << endl;
	stream << "     </tr>" << endl;
}

void GermlineReportGenerator::printVariantSheetRow(QTextStream& stream, const ReportVariantConfiguration& conf)
{
	//get column indices
	const Variant& v = data_.variants[conf.variant_index];
	int i_genotype = data_.variants.getSampleHeader().infoByID(data_.ps).column_index;
	int i_co_sp = data_.variants.annotationIndexByName("coding_and_splicing", true, true);
	int i_class = data_.variants.annotationIndexByName("classification", true, true);
	int i_gnomad = data_.variants.annotationIndexByName("gnomAD", true, true);
	int i_ngsd_hom = data_.variants.annotationIndexByName("NGSD_hom", true, true);
	int i_ngsd_het = data_.variants.annotationIndexByName("NGSD_het", true, true);

	//get transcript-specific data
	QStringList genes;
	QStringList types;
	QStringList hgvs_cs;
	QStringList hgvs_ps;
	//for genes with preferred transcripts, determine if the variant is actually in the preferred transcript, or not.
	QHash<QByteArray, bool> variant_in_pt;
	foreach(const VariantTranscript& trans, v.transcriptAnnotations(i_co_sp))
	{
		if (data_.preferred_transcripts.contains(trans.gene))
		{
			if (!variant_in_pt.contains(trans.gene))
			{
				variant_in_pt[trans.gene] = false;
			}
			if (data_.preferred_transcripts[trans.gene].contains(trans.idWithoutVersion()))
			{
				variant_in_pt[trans.gene] = true;
			}
		}
	}
	foreach(const VariantTranscript& trans, v.transcriptAnnotations(i_co_sp))
	{
		if (data_.preferred_transcripts.contains(trans.gene) && variant_in_pt[trans.gene] && !data_.preferred_transcripts[trans.gene].contains(trans.idWithoutVersion()))
		{
			continue;
		}
		genes << trans.gene;
		types << trans.type;
		hgvs_cs << trans.hgvs_c;
		hgvs_ps << trans.hgvs_p;
	}
	genes.removeDuplicates();
	types.removeDuplicates();
	hgvs_cs.removeDuplicates();
	hgvs_ps.removeDuplicates();

	//write line
	stream << "     <tr>" << endl;
	stream << "       <td>" << genes.join(", ") << "</td>" << endl;
	stream << "       <td>" << types.join(", ") << "</td>" << endl;
	stream << "       <td>" << v.annotations()[i_genotype] << "</td>" << endl;
	stream << "       <td style='white-space: nowrap'>" << v.toString(false, 20) << "</td>" << endl;
	stream << "       <td>" << conf.inheritance << "</td>" << endl;
	if (conf.causal)
	{
		stream << "       <td>" << hgvs_cs.join(", ") << "</td>" << endl;
		stream << "       <td>" << hgvs_ps.join(", ") << "</td>" << endl;
	}
	else
	{
		stream << "       <td>" << exclusionCriteria(conf) << "</td>" << endl;
	}
	stream << "       <td>" << v.annotations()[i_gnomad] << "</td>" << endl;
	stream << "       <td>" << v.annotations()[i_ngsd_hom] << " / " << v.annotations()[i_ngsd_het] << "</td>" << endl;
	stream << "       <td>" << conf.comments << "</td>" << endl;
	stream << "       <td>" << conf.comments2 << "</td>" << endl;
	stream << "       <td>" << v.annotations()[i_class] << "</td>" << endl;
	stream << "       <td>" << (conf.showInReport() ? "ja" : "nein") << " (" << conf.report_type << ")</td>" << endl;
	stream << "     </tr>" << endl;
}

void GermlineReportGenerator::printVariantSheetRowHeaderCnv(QTextStream& stream, bool causal)
{
	stream << "     <tr>" << endl;
	stream << "       <th>CNV</th>" << endl;
	stream << "       <th>copy-number</th>" << endl;
	stream << "       <th>Gene</th>" << endl;
	stream << "       <th>Erbgang</th>" << endl;
	if (causal)
	{
		stream << "       <th>Infos</th>" << endl;
	}
	else
	{
		stream << "       <th>Ausschlussgrund</th>" << endl;
	}
	stream << "       <th style='white-space: nowrap'>Kommentar 1. Auswerter</th>" << endl;
	stream << "       <th style='white-space: nowrap'>Kommentar 2. Auswerter</th>" << endl;
	stream << "       <th>Klasse</th>" << endl;
	stream << "       <th style='white-space: nowrap'>In Report</th>" << endl;
	stream << "     </tr>" << endl;
}

void GermlineReportGenerator::printVariantSheetRowCnv(QTextStream& stream, const ReportVariantConfiguration& conf)
{
	const CopyNumberVariant& cnv = data_.cnvs[conf.variant_index];
	stream << "     <tr>" << endl;
	stream << "       <td>" << cnv.toString() << "</td>" << endl;
	stream << "       <td>" << cnv.copyNumber(data_.cnvs.annotationHeaders()) << "</td>" << endl;
	stream << "       <td>" << cnv.genes().join(", ") << "</td>" << endl;
	stream << "       <td>" << conf.inheritance << "</td>" << endl;
	if (conf.causal)
	{
		stream << "       <td>regions:" << cnv.regions() << " size:" << QString::number(cnv.size()/1000.0, 'f', 3) << "kb</td>" << endl;
	}
	else
	{
		stream << "       <td>" << exclusionCriteria(conf) << "</td>" << endl;
	}
	stream << "       <td>" << conf.comments << "</td>" << endl;
	stream << "       <td>" << conf.comments2 << "</td>" << endl;
	stream << "       <td>" << conf.classification << "</td>" << endl;
	stream << "       <td>" << (conf.showInReport() ? "ja" : "nein") << " (" << conf.report_type << ")</td>" << endl;
	stream << "     </tr>" << endl;
}

void GermlineReportGenerator::printVariantSheetRowHeaderSv(QTextStream& stream, bool causal)
{
	stream << "     <tr>" << endl;
	stream << "       <th>SV</th>" << endl;
	stream << "       <th>Typ</th>" << endl;
	stream << "       <th>Gene</th>" << endl;
	stream << "       <th>Erbgang</th>" << endl;
	if (causal)
	{
		stream << "       <th>Infos</th>" << endl;
	}
	else
	{
		stream << "       <th>Ausschlussgrund</th>" << endl;
	}
	stream << "       <th style='white-space: nowrap'>Kommentar 1. Auswerter</th>" << endl;
	stream << "       <th style='white-space: nowrap'>Kommentar 2. Auswerter</th>" << endl;
	stream << "       <th>Klasse</th>" << endl;
	stream << "       <th style='white-space: nowrap'>In Report</th>" << endl;
	stream << "     </tr>" << endl;
}

void GermlineReportGenerator::printVariantSheetRowSv(QTextStream& stream, const ReportVariantConfiguration& conf)
{
	const BedpeLine& sv = data_.svs[conf.variant_index];
	BedFile affected_region = sv.affectedRegion();
	stream << "     <tr>" << endl;
	stream << "       <td>" << affected_region[0].toString(true);
	if(sv.type() == StructuralVariantType::BND) stream << " &lt;-&gt; " << affected_region[1].toString(true);
	stream << "</td>" << endl;
	stream << "       <td>" << BedpeFile::typeToString(sv.type()) << "</td>" << endl;
	stream << "       <td>" << sv.genes(data_.svs.annotationHeaders()).join(", ") << "</td>" << endl;
	stream << "       <td>" << conf.inheritance << "</td>" << endl;
	if (conf.causal)
	{
		stream << "       <td>estimated size:" << QString::number(data_.svs.estimatedSvSize(conf.variant_index)/1000.0, 'f', 3) << "kb</td>" << endl;
	}
	else
	{
		stream << "       <td>" << exclusionCriteria(conf) << "</td>" << endl;
	}
	stream << "       <td>" << conf.comments << "</td>" << endl;
	stream << "       <td>" << conf.comments2 << "</td>" << endl;
	stream << "       <td>" << conf.classification << "</td>" << endl;
	stream << "       <td>" << (conf.showInReport() ? "ja" : "nein") << " (" << conf.report_type << ")</td>" << endl;
	stream << "     </tr>" << endl;
}

QString GermlineReportGenerator::exclusionCriteria(const ReportVariantConfiguration& conf)
{
	QByteArrayList exclustion_criteria;
	if (conf.exclude_artefact) exclustion_criteria << "Artefakt";
	if (conf.exclude_frequency) exclustion_criteria << "Frequenz";
	if (conf.exclude_phenotype) exclustion_criteria << "Phenotyp";
	if (conf.exclude_mechanism) exclustion_criteria << "Pathomechanismus";
	if (conf.exclude_other) exclustion_criteria << "Anderer (siehe Kommentare)";
	return exclustion_criteria.join(", ");
}
