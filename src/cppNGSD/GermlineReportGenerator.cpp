#include "GermlineReportGenerator.h"
#include "Helper.h"
#include "XmlHelper.h"
#include "Settings.h"
#include "Log.h"
#include "Statistics.h"
#include "LoginManager.h"
#include "VariantHgvsAnnotator.h"
#include <QFileInfo>
#include <QXmlStreamWriter>

GermlineReportGeneratorData::GermlineReportGeneratorData(GenomeBuild build_, QString ps_, const VariantList& variants_, const CnvList& cnvs_, const BedpeFile& svs_, const PrsTable& prs_, const ReportSettings& report_settings_, const FilterCascade& filters_, const QMap<QByteArray, QByteArrayList>& preferred_transcripts_, StatisticsService& statistics_service_)
	: build(build_)
	, ps(ps_)
	, variants(variants_)
	, cnvs(cnvs_)
	, svs(svs_)
	, prs(prs_)
	, report_settings(report_settings_)
	, filters(filters_)
	, preferred_transcripts(preferred_transcripts_)
	, statistics_service(statistics_service_)
{
}

GermlineReportGenerator::GermlineReportGenerator(const GermlineReportGeneratorData& data, bool test_mode)
	: db_(test_mode)
	, data_(data)
	, date_(QDate::currentDate())
	, test_mode_(test_mode)
	, genome_idx_(Settings::string("reference_genome"))
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
		stream << "<br />" << trans("Vater") << ": "  << info_father.name << endl;
		stream << "<br />" << trans("Mutter") << ": "  << info_mother.name << endl;
	}
	stream << "<br />" << endl;
	stream << "<br />" << trans("Geschlecht") << ": " << trans(processed_sample_data.gender) << endl;
	stream << "<br />" << trans("Prozessierungssystem") << ": " << processed_sample_data.processing_system << endl;
	stream << "<br />" << trans("Prozessierungssystem-Typ") << ": " << processed_sample_data.processing_system_type << endl;
	stream << "<br />" << trans("Referenzgenom") << ": " << system_data.genome << endl;
	stream << "<br />" << trans("Datum") << ": " << date_.toString("dd.MM.yyyy") << endl;
	stream << "<br />" << trans("Analysepipeline") << ": "  << data_.variants.getPipeline() << endl;
	stream << "<br />" << trans("Auswertungssoftware") << ": "  << QCoreApplication::applicationName() << " " << QCoreApplication::applicationVersion() << endl;
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
		int hpo_id = db_.phenotypeIdByAccession(entry.disease_info.toUtf8(), false);
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
			stream << "<br />" << trans("Ausgewertete Gene") << ": ";
			if (data_.report_settings.show_coverage_details)
			{
				stream << QString::number(data_.roi.genes.count()) << " (" << trans("siehe Abdeckungsstatistik") << ")" << endl;
			}
			else
			{
				stream << data_.roi.genes.join(", ") << endl;
			}
		}
		stream << "</span></p>" << endl;
	}

	//get column indices
	int i_genotype = data_.variants.getSampleHeader().infoByID(data_.ps).column_index;
	int i_gene = data_.variants.annotationIndexByName("gene", true, true);
	int i_omim = data_.variants.annotationIndexByName("OMIM", true, true);
	int i_class = data_.variants.annotationIndexByName("classification", true, true);
	int i_gnomad = data_.variants.annotationIndexByName("gnomAD", true, true);

	//output: applied filters
	stream << endl;
	stream << "<p><b>" << trans("Filterkriterien") << " " << "</b>" << endl;
	for(int i=0; i<data_.filters.count(); ++i)
	{
		stream << "<br />&nbsp;&nbsp;&nbsp;&nbsp;- " << data_.filters[i]->toText() << endl;
	}
	stream << "<br />";

	//determine variant count (inside target region)
	int var_count = data_.variants.count();
	if (data_.roi.isValid())
	{
		FilterResult filter_result(data_.variants.count());
		FilterRegions::apply(data_.variants, data_.roi.regions, filter_result);
		var_count = filter_result.countPassing();
	}

	stream << "<br />" << trans("Gefundene SNVs/InDels in Zielregion gesamt") << ": " << var_count << endl;
	selected_small_.clear();
	selected_cnvs_.clear();
	selected_svs_.clear();
	for (auto it = data_.report_settings.selected_variants.cbegin(); it!=data_.report_settings.selected_variants.cend(); ++it)
	{
		if (it->first==VariantType::SNVS_INDELS) selected_small_ << it->second;
		if (it->first==VariantType::CNVS) selected_cnvs_ << it->second;
		if (it->first==VariantType::SVS) selected_svs_ << it->second;
	}
	stream << "<br />" << trans("Anzahl SNVs/InDels ausgew&auml;hlt f&uuml;r Report") << ": " << selected_small_.count() << endl;
	stream << "<br />" << trans("Anzahl CNVs ausgew&auml;hlt f&uuml;r Report") << ": " << selected_cnvs_.count() << endl;
	stream << "<br />" << trans("Anzahl SVs ausgew&auml;hlt f&uuml;r Report") << ": " << selected_svs_.count() << endl;
	stream << "</p>" << endl;

	stream << "<br />" << trans("Sofern vorhanden, werden in den nachfolgenden Tabellen erfasst: pathogene Varianten (Klasse 5)<sup>*</sup> und wahrscheinlich pathogene Varianten (Klasse 4)<sup>*</sup>, bei denen jeweils ein Zusammenhang mit der klinischen Fragestellung anzunehmen ist, sowie Varianten unklarer klinischer Signifikanz (Klasse 3)<sup>*</sup> f&uuml;r welche in Zusammenschau von Literatur und Klinik des Patienten ein Beitrag zur Symptomatik denkbar ist und f&uuml;r die gegebenenfalls eine weitere Einordnung der klinischen Relevanz durch Folgeuntersuchungen sinnvoll erscheint.") << endl;
	stream << trans("Teilweise k&ouml;nnen - in Abh&auml;ngigkeit von der Art der genetischen Ver&auml;nderung, der Familienanamnese und der Klinik der Patientin/des Patienten - weiterf&uuml;hrende Untersuchungen eine &Auml;nderung der Klassifizierung bewirken.") << endl;
	stream << "<br />" << trans("Eine (unkommentierte) Liste aller detektierten Varianten kann bei Bedarf angefordert werden.") << endl;
	stream << "<br />" << trans("Bei konkreten differentialdiagnostischen Hinweisen auf eine konkrete Erkrankung k&ouml;nnen ggf. weiterf&uuml;hrende genetische Untersuchungen bzw. Untersuchungsmethoden indiziert sein.") << endl;
	stream << "<br />" << trans("<sup>*</sup> F&uuml;r Informationen zur Klassifizierung von Varianten, siehe allgemeine Zusatzinformationen.") << endl;

	//output: select small variants
	stream << "<br /><br /><b>" << trans("Einzelbasenver&auml;nderungen (SNVs) und Insertionen/Deletionen (InDels) nach klinischer Interpretation im Kontext der Fragestellung") << "</b>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><td><b>" << trans("Variante") << "</b></td><td><b>" << trans("Genotyp") << "</b></td>";
	int colspan = 8;
	if (is_trio)
	{
		stream << "<td><b>" << trans("Vater") << "</b></td>";
		stream << "<td><b>" << trans("Mutter") << "</b></td>";
		colspan = 10;
	}
	stream << "<td><b>" << trans("Gen(e)") << "</b></td><td><b>" << trans("Details") << "</b></td><td><b>" << trans("Klasse") << "</b></td><td><b>" << trans("Erbgang") << "</b></td><td><b>" << trans("gnomAD Allelfrequenz") << "<br />(" << trans("Kontrollkohorte") << ")</b></td><td><b>RNA</b></td></tr>" << endl;
	foreach(const ReportVariantConfiguration& var_conf, data_.report_settings.report_config->variantConfig())
	{
		if (var_conf.variant_type!=VariantType::SNVS_INDELS) continue;
		if (!selected_small_.contains(var_conf.variant_index)) continue;

		Variant variant = data_.variants[var_conf.variant_index];
		if (var_conf.isManuallyCurated()) var_conf.updateVariant(variant, genome_idx_, i_genotype);

		stream << "<tr>" << endl;
		stream << "<td>" << endl;
		stream  << variant.chr().str() << ":" << variant.start() << "&nbsp;" << variant.ref() << "&nbsp;&gt;&nbsp;" << variant.obs() << "</td>";
		QString geno = formatGenotype(data_.build, processed_sample_data.gender.toUtf8(), variant.annotations().at(i_genotype), variant);
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
		stream << "<td>" << formatCodingSplicing(variant) << "</td>" << endl;
		stream << "<td>" << variant.annotations().at(i_class) << "</td>" << endl;
		stream << "<td>" << var_conf.inheritance << "</td>" << endl;
		QByteArray gnomad_percentage = "n/a";
		QByteArray freq = variant.annotations().at(i_gnomad).trimmed();
		if (!freq.isEmpty())
		{
			try
			{
				gnomad_percentage = QByteArray::number(100.0 * Helper::toDouble(freq, "gnomAD AF"), 'f', 3) + "%";
			}
			catch (Exception& e)
			{
				Log::warn("Could not convert gnomAD AF to number: " + e.message());
			}
		}
		stream << "<td>" << gnomad_percentage << "</td>" << endl;
		stream << "<td>" << trans(var_conf.rna_info) << "</td>" << endl;
		stream << "</tr>" << endl;

		//OMIM line
		QString omim = variant.annotations()[i_omim].trimmed();
		if (omim!="")
		{
			QStringList omim_parts = omim.append(" ").split("]; ");
			foreach(QString omim_part, omim_parts)
			{
				if (omim_part.count()<10) continue;
				omim = "OMIM ID: " + omim_part.left(6) + " Details: " + omim_part.mid(8);
			}
			stream << "<tr><td colspan=\"" << colspan << "\">" << omim << "</td></tr>" << endl;
		}
	}
	if (selected_small_.count()==0) stream << "<tr><td colspan=\"" << colspan << "\">" << trans("Keine") << "</td></tr>";
	stream << "</table>" << endl;

	//CNVs
	stream << "<br /><b>" << trans("Kopienzahlver&auml;nderungen (CNV) nach klinischer Interpretation im Kontext der Fragestellung") << "</b>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><td><b>" << trans("CNV") << "</b></td><td><b>" << trans("Position") << "</b></td><td><b>" << trans("Gr&ouml;&szlig;e") << "</b></td><td><b>" << trans("Kopienzahl") << "</b></td><td><b>" << trans("Gen(e)") << "</b></td><td><b>" << trans("Klasse") << "</b></td><td><b>" << trans("Erbgang") << "</b></td><td><b>RNA</b></td></tr>" << endl;
	colspan = 8;
	foreach(const ReportVariantConfiguration& var_conf, data_.report_settings.report_config->variantConfig())
	{
		if (var_conf.variant_type!=VariantType::CNVS) continue;
		if (!selected_cnvs_.contains(var_conf.variant_index)) continue;

		CopyNumberVariant cnv = data_.cnvs[var_conf.variant_index];
		if (var_conf.isManuallyCurated()) var_conf.updateCnv(cnv, data_.cnvs.annotationHeaders(), db_);

		int cn = cnv.copyNumber(data_.cnvs.annotationHeaders());

		stream << "<tr>" << endl;
		stream << "<td>" << (cn<2 ? trans("Deletion") : trans("Duplikation")) << "</td>" << endl;
		stream << "<td>" << cnv.toString() << "</td>" << endl;
		stream << "<td>" << QString::number(cnv.size()/1000.0, 'f', 3) << " " << trans("kb") << " / " << std::max(1, cnv.regions()) << " " << trans("Regionen") << "</td>" << endl;
		QString cn_str = QString::number(cn);
		if (var_conf.de_novo) cn_str += " (de-novo)";
		if (var_conf.mosaic) cn_str += " (mosaic)";
		if (var_conf.comp_het) cn_str += " (comp-het)";
		stream << "<td>" << cn_str << "</td>" << endl;
		stream << "<td>" << cnv.genes().join(", ") << "</td>" << endl;
		stream << "<td>" << var_conf.classification << "</td>" << endl;
		stream << "<td>" << var_conf.inheritance << "</td>" << endl;
		stream << "<td>" << trans(var_conf.rna_info) << "</td>" << endl;
		stream << "</tr>" << endl;
	}
	if (selected_cnvs_.count()==0) stream << "<tr><td colspan=\"" << colspan << "\">" << trans("Keine") << "</td></tr>";
	stream << "</table>" << endl;

	//--------------------------------------------------------------------------------------
	//SVs
	stream << "<br /><b>" << trans("Strukturver&auml;nderungen (SV) nach klinischer Interpretation im Kontext der Fragestellung") << "</b>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><td><b>" << trans("SV") << "</b></td><td><b>" << trans("Position") << "</b></td><td><b>" << trans("Gr&ouml;&szlig;e") << "</b></td><td><b>" << trans("Genotyp") << "</b></td><td><b>" << trans("Gen(e)") << "</b></td><td><b>" << trans("Klasse") << "</b></td><td><b>" << trans("Erbgang") << "</b></td><td><b>RNA</b></td></tr>" << endl;
	colspan = 8;

	foreach(const ReportVariantConfiguration& var_conf, data_.report_settings.report_config->variantConfig())
	{
		if (var_conf.variant_type!=VariantType::SVS) continue;
		if (!selected_svs_.contains(var_conf.variant_index)) continue;

		BedpeLine sv = data_.svs[var_conf.variant_index];
		if (var_conf.isManuallyCurated()) var_conf.updateSv(sv, data_.svs.annotationHeaders(), db_);

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
		BedFile affected_region = sv.affectedRegion(false);
		stream << "<td>" << affected_region[0].toString(true);
		if (sv.type() == StructuralVariantType::BND) stream << " &lt;-&gt; " << affected_region[1].toString(true);
		stream << "</td>" << endl;

		//size
		int size = sv.size();
		stream << "<td>";
		if (size!=-1)
		{
			stream << QString::number(size/1000.0, 'f', 3) << " " << trans("kb");
		}
		stream << "</td>";

		//genotype
		QByteArray gt = sv.genotypeHumanReadable(data_.svs.annotationHeaders(), false);
		stream << "<td>" << gt;

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

		//RNA info
		stream << "<td>" << trans(var_conf.rna_info) << "</td>" << endl;
		stream << "</tr>" << endl;
	}
	if (selected_svs_.count()==0) stream << "<tr><td colspan=\"" << colspan << "\">" << trans("Keine") << "</td></tr>";
	stream << "</table>" << endl;

	//-----------------------------------------------------------------------------------

	//other causal variant
	if (data_.report_settings.select_other_causal_variant)
	{
		stream << "<p>&nbsp;</p>" << endl;
		OtherCausalVariant causal_variant = data_.report_settings.report_config->otherCausalVariant();
		stream << "<table>" << endl;
		stream << "<tr><td><b>" << trans("Variantentyp") << "</b></td><td><b>" << trans("Regionen") << "</b></td><td><b>" << trans("Gen(e)") << "</b></td><td><b>" << trans("Erbgang")
			   << "</b></td><td><b>" << trans("Kommentar") << "</b></td></tr>" << endl;

		stream << "<tr>" << endl;
		stream << "<td>" << trans(convertOtherVariantType(causal_variant.type)) << "</td>" << endl;
		stream << "<td>" << causal_variant.coordinates << "</td>" << endl;
		stream << "<td>" << causal_variant.gene << "</td>" << endl;
		stream << "<td>" << causal_variant.inheritance << "</td>" << endl;
		stream << "<td>" << causal_variant.comment << "</td>" << endl;
		stream << "</tr>" << endl;
		stream << "</table>" << endl;
	}
	//--------------------------------------------------------------------------------------

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
		//get target region coverages (from NGSD or calculate)
		double target_region_read_depth = -1.0;
		if (data_.report_settings.recalculate_avg_depth)
		{
			target_region_read_depth = data_.statistics_service.targetRegionReadDepth(data_.roi.regions, data_.ps_bam, data_.threads);
		}
		else
		{
			try
			{
				QCCollection stats = db_.getQCData(ps_id_);
				double avg_depth = stats.value("QC:2000025", true).asDouble();
				target_region_read_depth = avg_depth;
			}
			catch (Exception& e)
			{
				Log::warn("Average target region depth from NGSD cannot be determined! Recalculating it...");

				target_region_read_depth = data_.statistics_service.targetRegionReadDepth(data_.roi.regions, data_.ps_bam, data_.threads);
			}
		}

		//print general information about ROI
		stream << endl;
		stream << "<p><b>" << trans("Abdeckungsstatistik Zielregion") << "</b>" << endl;
		stream << "<br />" << trans("Durchschnittliche Sequenziertiefe") << ": " << QString::number(target_region_read_depth, 'f', 2) << endl;
		BedFile mito_bed;
		mito_bed.append(BedLine("chrMT", 1, 16569));
		data_.statistics_service.avgCoverage(mito_bed, data_.ps_bam, data_.threads);
		stream << "<br />" << trans("Durchschnittliche Sequenziertiefe (chrMT)") << ": " << mito_bed[0].annotations()[0] << endl;
		stream << "</p>" << endl;

		//gap report based on the entire target region
		if (data_.report_settings.cov_based_on_complete_roi)
		{
			stream << "<p><b>" << trans("L&uuml;ckenreport Zielregion") << "</b>" << endl;
			GapDetails details = writeCoverageDetails(stream, data_.roi);
			gap_percentage_ = details.gap_percentage;
			gaps_by_gene_ = details.gaps_per_gene;
			stream << "</p>" << endl;
		}

		//gap report based on exons
		stream << "<p><b>" << trans("L&uuml;ckenreport basierend auf Exons der Zielregion");
		if (data_.report_settings.cov_exon_padding>0)
		{
			stream << " &#177; " << data_.report_settings.cov_exon_padding << " " << trans("Basen");
		}
		stream << "</b>" << endl;
		if (data_.roi.genes.isEmpty())
		{
			stream << "<br />" << trans("Konnte nicht erstellt werden, weil keine Gene der Zielregion definiert wurden.") << endl;
		}
		else
		{
			//determine target region
			GeneSet genes_without_roi;
			TargetRegionInfo exon_roi;
			exon_roi.genes = data_.roi.genes;
			foreach(const QByteArray& gene, exon_roi.genes)
			{
				int gene_id = db_.geneId(gene);
				if (gene_id==-1)
				{
					genes_without_roi << gene;
					continue;
				}

				TranscriptList transcripts = db_.releventTranscripts(gene_id);
				if (transcripts.isEmpty())
				{
					genes_without_roi << gene;
					continue;
				}
				foreach(const Transcript& transcript, transcripts)
				{
					exon_roi.regions.add(transcript.isCoding() ? transcript.codingRegions() : transcript.regions());
				}
			}
			exon_roi.genes.remove(genes_without_roi);

			//set CCDS base count without padding
			exon_roi.regions.merge();
			bases_ccds_sequenced_ = exon_roi.regions.baseCount();

			//pad and sort
			if (data_.report_settings.cov_exon_padding>0) exon_roi.regions.extend(data_.report_settings.cov_exon_padding);
			exon_roi.regions.merge();
			exon_roi.regions.sort();

			//output
			if (!genes_without_roi.isEmpty())
			{
				stream << "<br />" << trans("Gene f&uuml;r die keine genomische Region bestimmt werden konnte") << ": " << genes_without_roi.join(", ") << endl;
			}
			writeCoverageDetails(stream, exon_roi);
		}
		stream << "</p>" << endl;

		writeRNACoverageReport(stream);
	}

	//OMIM table
	if (data_.report_settings.show_omim_table)
	{
		stream << endl;
		stream << "<p><b>" << trans("OMIM Gene und Phenotypen") << "</b>" << endl;
		stream << "</p>" << endl;
		stream << "<table>" << endl;
		stream << "<tr><td><b>" << trans("Gen") << "</b></td><td><b>" << trans("Gen MIM") << "</b></td><td><b>" << trans("Phenotyp MIM") << "</b></td><td><b>" << trans("Phenotyp") << "</b></td>";
		if (data_.report_settings.show_one_entry_in_omim_table) stream << "<td><b>" << trans("Hauptphenotyp") << "</b></td>" << endl;
		stream << "</tr>";
		foreach(const QByteArray& gene, data_.roi.genes)
		{
			QString preferred_phenotype_accession;
			if (sample_data.disease_group!="n/a") preferred_phenotype_accession = db_.omimPreferredPhenotype(gene, sample_data.disease_group.toUtf8());

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
				stream << "<tr><td>" << omim_info.gene_symbol << "</td><td>" << omim_info.mim << "</td><td>" << accessions.join("<br />") << "</td><td>" << names.join("<br />") << "</td>";
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
		int trait_idx = data_.prs.columnIndex("trait");
		int score_idx = data_.prs.columnIndex("score");
		int citation_idx = data_.prs.columnIndex("citation");
		for (int r=0; r<data_.prs.rowCount(); ++r)
		{
			const QStringList& row = data_.prs.row(r);
			QString trait = row[trait_idx];
			QString score = row[score_idx];
			QString zscore = "n/a";
			QString population = NGSHelper::populationCodeToHumanReadable(processed_sample_data.ancestry);
			if (trait=="Breast Cancer") // mean and standard deviation for BCAC313 taken from https://canrisk.atlassian.net/wiki/spaces/FAQS/pages/35979266/What+variants+are+used+in+the+PRS
			{
				double mean = -0.424;
				double stdev = 0.611;
				double zscore_num = (Helper::toDouble(score, "PRS score") - mean) / stdev;
				zscore = QString::number(zscore_num, 'f', 3);
				if (zscore_num>=1.6 && population==NGSHelper::populationCodeToHumanReadable("EUR"))
				{
					zscore = "<b>" + zscore + "</b>";
				}
				if (population!=NGSHelper::populationCodeToHumanReadable("EUR") || processed_sample_data.gender=="male")
				{
					zscore = "(" + zscore + ")";
				}
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
			THROW(ProgrammingException, "Invalid germline report HTML file " + filename + " generated:\n" + validation_error);
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
	w.writeAttribute("version", "10");
	w.writeAttribute("type", data_.report_settings.report_type);

	//element ReportGeneration
	w.writeStartElement("ReportGeneration");
	w.writeAttribute("date", date_.toString("yyyy-MM-dd"));
	w.writeAttribute("user_name", LoginManager::userLogin());
	w.writeAttribute("software", QCoreApplication::applicationName() + " " + QCoreApplication::applicationVersion());
	w.writeAttribute("outcome", db_.getDiagnosticStatus(ps_id_).outcome);
	w.writeEndElement();

	//element ChromosomeAliases
	w.writeStartElement("ChromosomeAliases");
	QMap<Chromosome, QString> table = getChromosomeTable(data_.build);
	foreach (Chromosome key, table.keys())
	{
		w.writeStartElement("Chromosome");
		w.writeAttribute("chr", key.str());
		w.writeAttribute("refseq", table[key]);
		w.writeEndElement();
	}
	w.writeEndElement();

	//element Sample
	w.writeStartElement("Sample");
	w.writeAttribute("name", data_.ps);
	SampleData sample_data = db_.getSampleData(db_.sampleId(data_.ps));
	w.writeAttribute("name_external", sample_data.name_external);
	ProcessedSampleData processed_sample_data = db_.getProcessedSampleData(ps_id_);
	w.writeAttribute("processing_system", processed_sample_data.processing_system);
	w.writeAttribute("processing_system_type", processed_sample_data.processing_system_type);
	QString comments = processed_sample_data.comments.trimmed();
	if (!comments.isEmpty())
	{
		w.writeAttribute("comments", comments);
	}

	QString ancestry = processed_sample_data.ancestry.trimmed();
	if (!ancestry.isEmpty())
	{
		w.writeAttribute("ancestry", ancestry);
	}

	//QC data
	QCCollection qc_data = db_.getQCData(ps_id_);
	for (int i=0; i<qc_data.count(); ++i)
	{
		const QCValue& term = qc_data[i];
		if (term.type()==QCValueType::IMAGE) continue;
		w.writeStartElement("QcTerm");
		w.writeAttribute("id", term.accession());
		w.writeAttribute("name", term.name());
		w.writeAttribute("def", term.description());
		w.writeAttribute("value", term.toString());
		w.writeEndElement();
	}
	w.writeEndElement();

	//add QC data of RNA sample
	QString sample_id = db_.sampleId(data_.ps);

	//get ids of all RNA processed samples corresponding to the current sample
	QList<int> rna_ps_ids;
	foreach (int rna_sample, db_.relatedSamples(sample_id.toInt(), "same sample", "RNA"))
	{
		rna_ps_ids << db_.getValuesInt("SELECT id FROM processed_sample WHERE quality!='bad' AND sample_id=:0", QString::number(rna_sample));
	}

	if (rna_ps_ids.size() > 0)
	{
		std::sort(rna_ps_ids.rbegin(), rna_ps_ids.rend());
		QString rna_ps_id = QString::number(rna_ps_ids.at(0));
		w.writeStartElement("RNASample");
		w.writeAttribute("name", db_.processedSampleName(rna_ps_id));
		qc_data = db_.getQCData(rna_ps_id);
		QSet<QString> valid_accessions = QSet<QString>() << "QC:2000005" << "QC:2000025" << "QC:2000101" << "QC:2000109";
		for (int i=0; i<qc_data.count(); ++i)
		{
			const QCValue& term = qc_data[i];
			if (!valid_accessions.contains(term.accession())) continue; //skip no-valid accession
			w.writeStartElement("QcTerm");
			w.writeAttribute("id", term.accession());
			w.writeAttribute("name", term.name());
			w.writeAttribute("def", term.description());
			w.writeAttribute("value", term.toString());
			w.writeEndElement();
		}
		w.writeEndElement();
	}


	//element TargetRegion (optional)
	if (data_.roi.isValid())
	{
		w.writeStartElement("TargetRegion");
		w.writeAttribute("name", data_.roi.name);
		w.writeAttribute("regions", QString::number(data_.roi.regions.count()));
		w.writeAttribute("bases", QString::number(data_.roi.regions.baseCount()));
		w.writeAttribute("gap_cutoff", QString::number(data_.report_settings.min_depth));
		if (gap_percentage_>0)
		{
			w.writeAttribute("gap_percentage", QString::number(gap_percentage_, 'f', 2));
		}
		if (bases_ccds_sequenced_!=-1)
		{
			w.writeAttribute("ccds_bases_sequenced", QString::number(bases_ccds_sequenced_));
		}

		//contained genes
		foreach(const QByteArray& gene, data_.roi.genes)
		{
			int gene_id = db_.geneId(gene);
			if (gene_id==-1) continue;

			w.writeStartElement("Gene");
			w.writeAttribute("name", gene);
			w.writeAttribute("identifier", db_.geneHgncId(gene_id));
			Transcript transcript = db_.bestTranscript(gene_id);
			w.writeAttribute("bases", QString::number(transcript.regions().baseCount()));

			//omim info
			QList<OmimInfo> omim_infos = db_.omimInfo(gene);
			foreach(const OmimInfo& omim_info, omim_infos)
			{
				foreach(const Phenotype& pheno, omim_info.phenotypes)
				{
					w.writeStartElement("Omim");
					w.writeAttribute("gene", omim_info.mim);
					w.writeAttribute("phenotype", pheno.name());
					if (!pheno.accession().isEmpty())
					{
						w.writeAttribute("phenotype_number", pheno.accession());
					}
					w.writeEndElement();
				}
			}

			//gaps
			const BedFile& gaps = gaps_by_gene_[gene];
			for(int i=0; i<gaps.count(); ++i)
			{
				const BedLine& line = gaps[i];
				w.writeStartElement("Gap");
				w.writeAttribute("chr", line.chr().str());
				w.writeAttribute("start", QString::number(line.start()));
				w.writeAttribute("end", QString::number(line.end()));
				w.writeEndElement();
			}

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
		if (data_.report_settings.report_type!="all" && var_conf.report_type!=data_.report_settings.report_type) continue;

		Variant variant = data_.variants[var_conf.variant_index];

		//manual curation
		ClassificationInfo class_info = db_.getClassification(variant); //get classification infos before modifying the variant
		if (var_conf.isManuallyCurated()) var_conf.updateVariant(variant, genome_idx_, geno_idx);

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
						if (header_info[index].name==data_.ps) break;
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
						if (header_info[index].name==data_.ps) break;
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
		w.writeAttribute("genotype", formatGenotype(data_.build, processed_sample_data.gender.toUtf8(), variant.annotations()[geno_idx], variant));
		w.writeAttribute("causal", var_conf.causal ? "true" : "false");
		w.writeAttribute("de_novo", var_conf.de_novo ? "true" : "false");
		w.writeAttribute("comp_het", var_conf.comp_het ? "true" : "false");
		w.writeAttribute("mosaic", var_conf.mosaic ? "true" : "false");
		if (var_conf.inheritance!="n/a")
		{
			w.writeAttribute("inheritance", var_conf.inheritance);
		}
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
		w.writeAttribute("rna_info", var_conf.rna_info);
		w.writeAttribute("report_type", var_conf.report_type);

		//element TranscriptInformation
		QList<VariantTranscript> transcript_infos;
		if (var_conf.manualVarIsValid(genome_idx_)) //re-calculate based on new variant
		{
			//get all transcripts where the variant is completely contained in the region
			TranscriptList transcripts  = db_.transcriptsOverlapping(variant.chr(), variant.start(), variant.end(), 5000);

			//annotate consequence to transcript
			VariantHgvsAnnotator hgvs_annotator(genome_idx_);
			foreach(const Transcript& trans, transcripts)
			{
				VariantConsequence hgvs = hgvs_annotator.annotate(trans, variant);
				VariantTranscript consequence;
				consequence.gene = trans.gene();
				consequence.id = trans.nameWithVersion();
				consequence.type = hgvs.typesToString();
				consequence.hgvs_c = hgvs.hgvs_c;
				consequence.hgvs_p = hgvs.hgvs_p;
				if (hgvs.exon_number!=-1)
				{
					consequence.exon = "exon"+QByteArray::number(hgvs.exon_number)+"/"+QByteArray::number(trans.regions().count());
				}
				else if (hgvs.intron_number!=-1)
				{
					consequence.exon = "intron"+QByteArray::number(hgvs.intron_number)+"/"+QByteArray::number(trans.regions().count());
				}
				transcript_infos << consequence;
			}
		}
		else  //take from GSvar file
		{
			int i_co_sp = data_.variants.annotationIndexByName("coding_and_splicing", true, false);
			if (i_co_sp!=-1)
			{
				transcript_infos = variant.transcriptAnnotations(i_co_sp);
			}
		}

		GeneSet genes;
		foreach(const VariantTranscript& trans, transcript_infos)
		{
			w.writeStartElement("TranscriptInformation");
			w.writeAttribute("gene", trans.gene);
			int gene_id = db_.geneId(trans.gene);
			w.writeAttribute("gene_identifier", gene_id==-1 ? "n/a" : db_.geneHgncId(gene_id));
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

			bool is_main_transcript = false;
			if (gene_id!=-1)
			{
				TranscriptList relevant_transcripts = db_.releventTranscripts(gene_id);
				if (relevant_transcripts.contains(trans.idWithoutVersion()))
				{
					is_main_transcript = true;
				}
			}
			w.writeAttribute("main_transcript", is_main_transcript ? "true" : "false");

			w.writeEndElement();

			genes << trans.gene;
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
					int gene_id = db_.geneId(gene);
					w.writeAttribute("gene_identifier", gene_id==-1 ? "n/a" : db_.geneHgncId(gene_id));
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
					int gene_id = db_.geneId(omim_info.gene_symbol);
					w.writeAttribute("gene_identifier", gene_id==-1 ? "n/a" : db_.geneHgncId(gene_id));
					w.writeAttribute("source", "OMIM gene");
					w.writeAttribute("identifier", omim_info.mim);
					w.writeAttribute("name", omim_info.gene_symbol);
					w.writeEndElement();

					//phenotypes
					foreach(const Phenotype& pheno, omim_info.phenotypes)
					{
						w.writeStartElement("GeneDiseaseInformation");
						w.writeAttribute("gene", omim_info.gene_symbol);
						int gene_id = db_.geneId(omim_info.gene_symbol);
						w.writeAttribute("gene_identifier", gene_id==-1 ? "n/a" : db_.geneHgncId(gene_id));
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
				int gene_id = db_.geneId(gene);
				w.writeAttribute("gene_identifier", gene_id==-1 ? "n/a" : db_.geneHgncId(gene_id));
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
	if (no_cnv_calling) cnv_callset_id.clear();
	QString cnv_calling_quality;
	if (!cnv_callset_id.isEmpty()) cnv_calling_quality = db_.getValue("SELECT quality FROM cnv_callset WHERE id=" + cnv_callset_id, true).toString().trimmed();
	w.writeAttribute("quality", cnv_calling_quality.isEmpty() ? "n/a" : cnv_calling_quality);
	if(data_.cnvs.caller()==CnvCallerType::CLINCNV && !cnv_callset_id.isEmpty())
	{
		QHash<QString, QString> qc_metrics = db_.cnvCallsetMetrics(cnv_callset_id.toInt());

		QString iterations = qc_metrics["number of iterations"].trimmed();
		if(!iterations.isEmpty()) w.writeAttribute("number_of_iterations", iterations);

		QString high_quality_cnvs = qc_metrics["high-quality cnvs"].trimmed();
		if(!high_quality_cnvs.isEmpty()) w.writeAttribute("number_of_hq_cnvs", high_quality_cnvs);

		QString correlation_ref_samples = qc_metrics["mean correlation to reference samples"].trimmed();
		if(!correlation_ref_samples.isEmpty()) w.writeAttribute("correlation_ref_samples", correlation_ref_samples);
	}

	foreach(const ReportVariantConfiguration& var_conf, data_.report_settings.report_config->variantConfig())
	{
		if (var_conf.variant_type!=VariantType::CNVS) continue;
		if (!var_conf.showInReport()) continue;
		if (!selected_cnvs_.contains(var_conf.variant_index)) continue;
		if (data_.report_settings.report_type!="all" && var_conf.report_type!=data_.report_settings.report_type) continue;

		CopyNumberVariant cnv = data_.cnvs[var_conf.variant_index];

		//manual curation infos
		if (var_conf.isManuallyCurated()) var_conf.updateCnv(cnv, data_.cnvs.annotationHeaders(), db_);

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
		w.writeAttribute("regions", QString::number(std::max(1, cnv.regions())));
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
		w.writeAttribute("rna_info", var_conf.rna_info);
		w.writeAttribute("report_type", var_conf.report_type);

		if (!var_conf.manual_cnv_hgvs_type.isEmpty())
		{
			w.writeAttribute("hgvs_type", var_conf.manual_cnv_hgvs_type);
		}
		if (!var_conf.manual_cnv_hgvs_suffix.isEmpty())
		{
			w.writeAttribute("hgvs_suffix", var_conf.manual_cnv_hgvs_suffix);
		}

		//element Gene
		foreach(const QByteArray& gene, cnv.genes())
		{
			w.writeStartElement("Gene");
			w.writeAttribute("name", gene);
			int gene_id = db_.geneId(gene);
			w.writeAttribute("identifier", gene_id==-1 ? "n/a" : db_.geneHgncId(gene_id));
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

	//SV List
	w.writeStartElement("SvList");

	QString caller = "Unknown";
	foreach (const QByteArray header, data_.svs.headers())
	{
		if (! header.startsWith("##cmdline=")) continue;

		if (header.contains("configManta.py"))
		{
			caller = "Manta";
		}
		break;
	}

	w.writeAttribute("sv_caller", caller);
	w.writeAttribute("overall_number", QString::number(data_.svs.count()));
	w.writeAttribute("genome_build", buildToString(data_.build, true));

	foreach(const ReportVariantConfiguration& var_conf, data_.report_settings.report_config->variantConfig())
	{
		if (!var_conf.showInReport()) continue;
		if (var_conf.variant_type!=VariantType::SVS) continue;
		if (!selected_svs_.contains(var_conf.variant_index)) continue;
		if (data_.report_settings.report_type!="all" && var_conf.report_type!=data_.report_settings.report_type) continue;


		BedpeLine sv = data_.svs[var_conf.variant_index];

		//manual curation
		if (var_conf.isManuallyCurated()) var_conf.updateSv(sv, data_.svs.annotationHeaders(), db_);

		w.writeStartElement("Sv");

		// StructuralVariantTypeToString(sv.type())) gives only shortend versions.
		if (sv.type() == StructuralVariantType::INS)
		{
			w.writeAttribute("type", "Insertion");
		}
		else if (sv.type() == StructuralVariantType::DUP)
		{
			w.writeAttribute("type", "Duplication");
		}
		else if (sv.type() == StructuralVariantType::INV)
		{
			w.writeAttribute("type", "Inversion");
		}
		else if (sv.type() == StructuralVariantType::DEL)
		{
			w.writeAttribute("type", "Deletion");
		}
		else if (sv.type() == StructuralVariantType::BND)
		{
			w.writeAttribute("type", "Breakend");
		}

		w.writeAttribute("chr", sv.chr1().str());
		Variant v;// for genotype formating
		// start and end may be split over start1 and start2 for INS, DEL, DUP, INV
		if (sv.type() == StructuralVariantType::BND)
		{
			w.writeAttribute("start", QString::number(sv.start1()));
			w.writeAttribute("end", QString::number(sv.end1()));

			v.setChr(sv.chr1());
			v.setStart(sv.start1());
			v.setEnd(sv.end1());
		}
		else
		{
			w.writeAttribute("start", QString::number(sv.start1()));
			w.writeAttribute("end", QString::number(sv.end2()));

			v.setChr(sv.chr1());
			v.setStart(sv.start1());
			v.setEnd(sv.end2());
		}

		w.writeAttribute("start_band", NGSHelper::cytoBand(data_.build, sv.chr1(), sv.start1()));
		w.writeAttribute("end_band", NGSHelper::cytoBand(data_.build, sv.chr2(), sv.end2()));

		QByteArray sv_gt = sv.genotypeHumanReadable(data_.svs.annotationHeaders(), false);
		w.writeAttribute("genotype", formatGenotype(data_.build, processed_sample_data.gender.toUtf8(), sv_gt, v));

		if (sv.type() == StructuralVariantType::INS)
		{
			QByteArray alt = sv.annotations()[data_.svs.annotationIndexByName("ALT_A")];

			// write alt string except for the first character
			w.writeAttribute("ins_sequence", alt.right(alt.size()-1));
		}

		if (sv.type() == StructuralVariantType::BND)
		{
			w.writeAttribute("bnd_chr2", sv.chr2().str());
			w.writeAttribute("bnd_start2", QString::number(sv.start2()));
			w.writeAttribute("bnd_end2", QString::number(sv.end2()));
		}

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
		w.writeAttribute("rna_info", var_conf.rna_info);
		w.writeAttribute("report_type", var_conf.report_type);

		if (!var_conf.manual_sv_hgvs_type.isEmpty())
		{
			w.writeAttribute("hgvs_type", var_conf.manual_sv_hgvs_type);
		}
		if (!var_conf.manual_sv_hgvs_suffix.isEmpty())
		{
			w.writeAttribute("hgvs_suffix", var_conf.manual_sv_hgvs_suffix);
		}
		if (!var_conf.manual_sv_hgvs_type_bnd.isEmpty())
		{
			w.writeAttribute("hgvs_bnd_type", var_conf.manual_sv_hgvs_type_bnd);
		}
		if (!var_conf.manual_sv_hgvs_suffix_bnd.isEmpty())
		{
			w.writeAttribute("hgvs_bnd_suffix", var_conf.manual_sv_hgvs_suffix_bnd);
		}

		foreach(const QByteArray& gene, sv.genes(data_.svs.annotationHeaders(), false))
		{
			w.writeStartElement("Gene");
			w.writeAttribute("name", gene);
			int gene_id = db_.geneId(gene);
			w.writeAttribute("identifier", gene_id==-1 ? "n/a" : db_.geneHgncId(gene_id));
			w.writeEndElement();
		}

		w.writeEndElement();
	}
	w.writeEndElement();

	//PRS scores
	w.writeStartElement("PrsList");
	if (data_.prs.rowCount()>0)
	{
		int i_id = data_.prs.columnIndex("pgs_id");
		int i_trait = data_.prs.columnIndex("trait");
		int i_citation = data_.prs.columnIndex("citation");
		int i_score = data_.prs.columnIndex("score");
		int i_percentile = data_.prs.columnIndex("percentile");
		for (int r=0; r<data_.prs.rowCount(); ++r)
		{
			const QStringList& row = data_.prs.row(r);
			w.writeStartElement("Prs");
			w.writeAttribute("id", row[i_id].trimmed());
			w.writeAttribute("trait", row[i_trait].trimmed());
			w.writeAttribute("citation", row[i_citation].trimmed());
			w.writeAttribute("score", row[i_score].trimmed());
			QString percentile = row[i_percentile].trimmed();
			if (!percentile.isEmpty())
			{
				w.writeAttribute("percentile", percentile);
			}
			w.writeEndElement();
		}
	}
	w.writeEndElement();
	if (data_.report_settings.select_other_causal_variant)
	{
		OtherCausalVariant causal_variant = data_.report_settings.report_config->otherCausalVariant();
		w.writeStartElement("OtherCausalVariant");
		w.writeAttribute("type", convertOtherVariantType(causal_variant.type, true));
		w.writeAttribute("coordinates", causal_variant.coordinates);
		w.writeAttribute("gene", causal_variant.gene);
		w.writeAttribute("inheritance", causal_variant.inheritance);
		w.writeAttribute("comments", causal_variant.comment);
		w.writeEndElement();
	}

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
	QString xml_error = XmlHelper::isValidXml(filename, ":/resources/GermlineReport.xsd");
	if (xml_error!="")
	{
		THROW(ProgrammingException, "Invalid germline report XML file " + filename+ " generated:\n" + xml_error);
	}
}

void GermlineReportGenerator::overrideDate(QDate date)
{
	if (!test_mode_) THROW(ProgrammingException, "This function can only be used in test mode!");

	date_ = date;
}

BedFile GermlineReportGenerator::precalculatedGaps(const BedFile& gaps_roi, const BedFile& roi, int min_cov, const BedFile& processing_system_target_region)
{
	//check depth cutoff
	if (min_cov!=20) THROW(ArgumentException, "Depth cutoff is not 20!");

	//extract processing system ROI statistics
	int regions = -1;
	long long bases = -1;
	foreach(QString line, gaps_roi.headers())
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
	if (regions<0 || bases<0) THROW(ArgumentException, "Low-coverage file is outdated: it does not contain target region statistics!");

	if (processing_system_target_region.count()!=regions || processing_system_target_region.baseCount()!=bases) THROW(ArgumentException, "Low-coverage file is outdated: it does not match processing system target region!");

	//calculate gaps inside target region
	BedFile output;
	if (gaps_roi.count()>roi.count())
	{
		output = roi;
		output.intersect(gaps_roi);
	}
	else
	{
		output = gaps_roi;
		output.intersect(roi);
	}

	//add target region bases not covered by processing system target file
	BedFile uncovered(roi);
	uncovered.subtract(processing_system_target_region);
	output.add(uncovered);
	output.merge();

	return output;
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
		en2de["splicing effect validated by RNA dataset"] = "Splicing-Effekt mit RNA-Daten validiert";
		en2de["no splicing effect found in RNA dataset"] = "kein Splicing-Effekt in RNA-Daten gefunden";
		en2de["RNA dataset not usable"] = "RNA-Daten nicht nutzbar";
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
		de2en["Ph&auml;notyp"] = "Phenotype information";
		de2en["Filterkriterien"] = "Criteria for variant filtering";
		de2en["Gefundene SNVs/InDels in Zielregion gesamt"] = "Small variants in target region";
		de2en["Anzahl SNVs/InDels ausgew&auml;hlt f&uuml;r Report"] = "SNVs/InDels selected for report";
		de2en["Anzahl CNVs ausgew&auml;hlt f&uuml;r Report"] = "CNVs selected for report";
		de2en["Anzahl SVs ausgew&auml;hlt f&uuml;r Report"] = "SVs selected for report";
		de2en["Anzahl anderer Varianten ausgew&auml;hlt f&uuml;r Report"] = "Other variants selected for report";
		de2en["Einzelbasenver&auml;nderungen (SNVs) und Insertionen/Deletionen (InDels) nach klinischer Interpretation im Kontext der Fragestellung"] = "List of prioritized small variants";
		de2en["Kopienzahlver&auml;nderungen (CNV) nach klinischer Interpretation im Kontext der Fragestellung"] = "List of prioritized copy-number variants";
		de2en["Strukturver&auml;nderungen (SV) nach klinischer Interpretation im Kontext der Fragestellung"] = "List of prioritized structural variants";
		de2en["Erbgang"] = "Inheritance";
		de2en["gnomAD Allelfrequenz"] = "gnomAD allele frequency";
		de2en["Kontrollkohorte"] = "control cohort";
		de2en["Klasse"] = "Class";
		de2en["Details"] = "Details";
		de2en["Genotyp"] = "Genotype";
		de2en["Variante"] = "Variant";
		de2en["Gen"] = "Gene";
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
		de2en["siehe Abdeckungsstatistik"] = "see coverage statistics";
		de2en["Keine"] = "none";
		de2en["OMIM Gene und Phenotypen"] = "OMIM gene and phenotypes";
		de2en["Phenotyp"] = "phenotype";
		de2en["Gen MIM"] = "gene MIM";
		de2en["Phenotyp MIM"] = "phenotype MIM";
		de2en["Gen(e)"] = "Genes";
		de2en["Details zu Programmen der Analysepipeline"] = "Analysis pipeline tool details";
		de2en["Parameter"] = "Parameters";
		de2en["Version"] = "Version";
		de2en["Tool"] = "Tool";
		de2en["Abdeckungsstatistik Zielregion"] = "Coverage statistics of target region";
		de2en["Durchschnittliche Sequenziertiefe"] = "Average sequencing depth";
		de2en["Durchschnittliche Sequenziertiefe (chrMT)"] = "Average sequencing depth (chrMT)";
		de2en["Komplett abgedeckte Gene"] = "Genes without gaps";
		de2en["Basen mit Tiefe &lt;"] = "Percentage of regions with depth &lt;";
		de2en["Prozent L&uuml;cken"] = "Percentage gaps";
		de2en["Unvollst&auml;ndig abgedeckte Gene (fehlende Basen in bp)"] = "Genes with incomplete coverage (missing bp in brackets)";
		de2en["Details Regionen mit Tiefe &lt;"] = "Details regions with depth &lt;";
		de2en["Koordinaten (hg38)"] = "Coordinates (hg38)";
		de2en["Chromosom"] = "Chromosome";
		de2en["Basen"] = "Bases";
		de2en["L&uuml;ckenreport Zielregion"] = "Gap report based on entire target region";
		de2en["L&uuml;ckenreport basierend auf Exons der Zielregion"] = "Gap report based on exons of target region";
		de2en["Gene f&uuml;r die keine genomische Region bestimmt werden konnte"] = "Genes for which no genomic region could be determined";
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
		de2en["Kopienzahl"] = "copy-number";
		de2en["n/a"] = "n/a";
		de2en["SV"] = "SV";
		de2en["Position"] = "Position";
		de2en["Deletion"] = "deletion";
		de2en["Duplikation"] = "duplication";
		de2en["Insertion"] = "insertion";
		de2en["Inversion"] = "inversion";
		de2en["Translokation"] = "translocation";
		de2en["Variantentyp"] = "variant type";
		de2en["Kommentar"] = "comment";
		de2en["Repeat-Expansion"] = "repeat expansion";
		de2en["uniparentale Disomie"] = "uniparental disomy";
		de2en["mosaik CNV"] = "mosaic CNV";
		de2en["nicht-detektierte kleine Variante (SNV/InDel)"] = "uncalled small variant (SNV/InDel)";
		de2en["nicht-detektierte CNV"] = "uncalled CNV";
		de2en["nicht-detektierte Strukturvariante"] = "uncalled structural variant";
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
		de2en["nach L&uuml;ckenschluss"] = "after closing gaps";
		de2en["Verbleibende L&uuml;cken nach L&uuml;ckenschluss"] = "gaps remaining";
		de2en["splicing effect validated by RNA dataset"] = "splicing effect validated by RNA dataset";
		de2en["no splicing effect found in RNA dataset"] = "no splicing effect found in RNA dataset";
		de2en["RNA dataset not usable"] = "RNA dataset not usable";
		de2en["Abdeckungsstatistik der RNA-Probe"] = "Coverage statistics of RNA sample";
		de2en["Abgedeckte Gene"] = "Covered genes";
		de2en["Anzahl der Reads"] = "Number of reads";
		de2en["Durchschnittliche Sequenziertiefe der Housekeeping-Gene"] = "Average sequencing depth of housekeeping genes";
		de2en["kb"] = "kb";
		de2en["Sofern vorhanden, werden in den nachfolgenden Tabellen erfasst: pathogene Varianten (Klasse 5)<sup>*</sup> und wahrscheinlich pathogene Varianten (Klasse 4)<sup>*</sup>, bei denen jeweils ein Zusammenhang mit der klinischen Fragestellung anzunehmen ist, sowie Varianten unklarer klinischer Signifikanz (Klasse 3)<sup>*</sup> f&uuml;r welche in Zusammenschau von Literatur und Klinik des Patienten ein Beitrag zur Symptomatik denkbar ist und f&uuml;r die gegebenenfalls eine weitere Einordnung der klinischen Relevanz durch Folgeuntersuchungen sinnvoll erscheint."]
		  = "If present, the following tables contain: likely pathogenic variants (class 4)<sup>*</sup> and pathogenic variants (class 5)<sup>*</sup>, for which a contribution to the clinical symptoms of the patient is conceivable, and variants of uncertain significance (class 3)<sup>*</sup>, for which a further evaluation of the clinical relevance by follow-up examinations may be useful.";
		de2en["Teilweise k&ouml;nnen - in Abh&auml;ngigkeit von der Art der genetischen Ver&auml;nderung, der Familienanamnese und der Klinik der Patientin/des Patienten - weiterf&uuml;hrende Untersuchungen eine &Auml;nderung der Klassifizierung bewirken."]
		  = "Depending on the type of genetic alteration, family history and clinical features of the patient further investigations might change the classification of variants.";
		de2en["Eine (unkommentierte) Liste aller detektierten Varianten kann bei Bedarf angefordert werden."] = "A (uncommented) list of all detected variants can be provided on request.";
		de2en["Bei konkreten differentialdiagnostischen Hinweisen auf eine konkrete Erkrankung k&ouml;nnen ggf. weiterf&uuml;hrende genetische Untersuchungen bzw. Untersuchungsmethoden indiziert sein."]
		  = "In case of a suspected clinical diagnosis genetic counseling is necessary to evaluate the indication/possibility of further genetic studies.";
		de2en["<sup>*</sup> F&uuml;r Informationen zur Klassifizierung von Varianten, siehe allgemeine Zusatzinformationen."] = "<sup>*</sup> For information on the classification of variants, see the general information.";
		de2en["kein &Uuml;berlappung mit Gen"] = "no gene overlap";
		de2en["Konnte nicht erstellt werden, weil keine Gene der Zielregion definiert wurden."] = "Could not be performed because no target region genes are definded.";
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

GapDetails GermlineReportGenerator::writeCoverageDetails(QTextStream& stream, const TargetRegionInfo& roi)
{
	//get low-coverage regions (precalculated or calculate on the fly)
	BedFile low_cov;
	try
	{
		//load gaps file
		BedFile gaps;
		gaps.load(data_.ps_lowcov, false, false);

		low_cov = GermlineReportGenerator::precalculatedGaps(gaps, roi.regions, data_.report_settings.min_depth, data_.processing_system_roi);

		//calculate mito coverage if necessary (mito is not part of the target region)
		BedFile mito_roi;
		for (int i=0; i<roi.regions.count(); ++i)
		{
			if (roi.regions[i].chr().isM()) mito_roi.append(roi.regions[i]);
		}
		if(!mito_roi.isEmpty())
		{
			low_cov.subtract(mito_roi);
			BedFile mito_gaps = data_.statistics_service.lowCoverage(mito_roi, data_.ps_bam, data_.report_settings.min_depth);
			low_cov.add(mito_gaps);
		}
	}
	catch(Exception e)
	{
		Log::warn("Low-coverage statistics needs to be calculated. Pre-calculated gap file cannot be used because: " + e.message());
		low_cov = data_.statistics_service.lowCoverage(roi.regions, data_.ps_bam, data_.report_settings.min_depth);
	}

	//print base data
	long long roi_bases = roi.regions.baseCount();
	long long gap_bases = low_cov.baseCount();
	double gap_percentage = 100.0 * gap_bases/roi_bases;
	stream << "<br />" << trans("Basen") << ": " << QString::number(roi_bases) << endl;
	stream << "<br />" << trans("Basen mit Tiefe &lt;") << data_.report_settings.min_depth << ": " << QString::number(gap_bases) << endl;
	stream << "<br />" << trans("Prozent L&uuml;cken") << ": " << QString::number(gap_percentage, 'f', 2) << "%" << endl;

	//group gaps by gene
	QMap<QByteArray, BedFile> gaps_by_gene;
	long long gap_bases_no_gene = 0;
	gapsByGene(low_cov, roi.genes, gaps_by_gene, gap_bases_no_gene);

	//print genes that have gaps and that have no gaps
	GeneSet complete_genes;
	QStringList incomplete_genes;
	foreach(const QByteArray& gene, roi.genes)
	{
		if (!gaps_by_gene.contains(gene))
		{
			complete_genes << gene;
		}
		else
		{
			incomplete_genes << gene + " <span style=\"font-size: 8pt;\">(" + QString::number(gaps_by_gene[gene].baseCount()) + ")</span>";
		}
	}
	stream << "<br />" << trans("Komplett abgedeckte Gene") << ": " << complete_genes.join(", ") << endl;
	stream << "<br />" << trans("Unvollst&auml;ndig abgedeckte Gene (fehlende Basen in bp)") << ": " << incomplete_genes.join(", ") << endl;

	//table gaps by gene
	stream << "<p>" << trans("Details Regionen mit Tiefe &lt;") << data_.report_settings.min_depth << ":" << endl;
	stream << "</p>" << endl;
	writeGapsByGeneTable(stream, gaps_by_gene, gap_bases_no_gene);

	//init gap closing
	ChromosomalIndex<BedFile> roi_idx(roi.regions);
	SqlQuery query = db_.getQuery();
	query.prepare("SELECT chr, start, end, status FROM gaps WHERE processed_sample_id='" + ps_id_ + "' AND status=:0 ORDER BY chr,start,end");
	BedFile gaps_closed;

	//closed by Sanger
	{
		int base_sum = 0;
		stream << "<br />" << trans("L&uuml;cken die mit Sanger-Sequenzierung geschlossen wurden:") << "<br />";
		stream << "<table>" << endl;
		stream << "<tr><td><b>" << trans("Gen") << "</b></td><td><b>" << trans("Basen") << "</b></td><td><b>" << trans("Chromosom") << "</b></td><td><b>" << trans("Koordinaten (hg38)") << "</b></td></tr>" << endl;
		query.bindValue(0, "closed");
		query.exec();
		while(query.next())
		{
			Chromosome chr = query.value("chr").toString();
			int start = query.value("start").toInt();
			int end = query.value("end").toInt();
			if (roi_idx.matchingIndex(chr, start, end)==-1) continue;

			base_sum += end-start+1;
			gaps_closed.append(BedLine(chr, start, end));

			stream << "<tr>" << endl;
			stream << "<td>" << db_.genesOverlapping(chr, start, end).join(", ") << "</td><td>" << QString::number(end-start+1) << "</td><td>" << chr.str() << "</td><td>" << QString::number(start) << "-" << QString::number(end) << "</td>";
			stream << "</tr>" << endl;
		}
		stream << "</table>" << endl;
		stream << trans("Basen gesamt:") << QString::number(base_sum);
	}

	//closed by visual inspection
	{
		int base_sum = 0;
		stream << "<p>" << trans("L&uuml;cken die mit visueller Inspektion der Rohdaten &uuml;berpr&uuml;ft wurden:") << "<br />";
		stream << "<table>" << endl;
		stream << "<tr><td><b>" << trans("Gen") << "</b></td><td><b>" << trans("Basen") << "</b></td><td><b>" << trans("Chromosom") << "</b></td><td><b>" << trans("Koordinaten (hg38)") << "</b></td></tr>" << endl;
		query.bindValue(0, "checked visually");
		query.exec();
		while(query.next())
		{
			Chromosome chr = query.value("chr").toString();
			int start = query.value("start").toInt();
			int end = query.value("end").toInt();
			if (roi_idx.matchingIndex(chr, start, end)==-1) continue;

			base_sum += end-start+1;
			gaps_closed.append(BedLine(chr, start, end));

			stream << "<tr>" << endl;
			stream << "<td>" << db_.genesOverlapping(chr, start, end).join(", ") << "</td><td>" << QString::number(end-start+1) << "</td><td>" << chr.str() << "</td><td>" << QString::number(start) << "-" << QString::number(end) << "</td>";
			stream << "</tr>" << endl;
		}
		stream << "</table>" << endl;
		stream << trans("Basen gesamt:") << QString::number(base_sum);
		stream << "</p>" << endl;
	}

	//print gaps that were not closed
	if (!gaps_closed.isEmpty())
	{
		//calcualte remaining gaps
		BedFile gaps_remaining = low_cov;
		gaps_closed.merge(); //argument of subtract needs to sorted and merged
		gaps_remaining.subtract(gaps_closed);
		gapsByGene(gaps_remaining, roi.genes, gaps_by_gene, gap_bases_no_gene);

		//write gap table after closing gaps
		stream << "<p>" << trans("Verbleibende L&uuml;cken nach L&uuml;ckenschluss") << endl;
		stream << "</p>" << endl;
		writeGapsByGeneTable(stream, gaps_by_gene, gap_bases_no_gene);

		//add gap percentage after closing gaps
		long long gap_bases_remaining = gaps_remaining.baseCount();
		gap_percentage = 100.0 * gap_bases_remaining/roi_bases;
		stream << "<p>";
		stream << trans("Basen mit Tiefe &lt;") << data_.report_settings.min_depth << " " << trans("nach L&uuml;ckenschluss") << ": " << QString::number(gap_bases_remaining) << endl;
		stream << "<br />" << trans("Prozent L&uuml;cken") << " " << trans("nach L&uuml;ckenschluss") << ": " << QString::number(gap_percentage, 'f', 2) << "%" << endl;
		stream << "</p>";
	}

	return GapDetails {gap_percentage, gaps_by_gene};
}

void GermlineReportGenerator::writeRNACoverageReport(QTextStream& stream)
{
	//get all related RNA
	QString sample_id = db_.sampleId(data_.ps);

	//get ids of all RNA processed samples corresponding to the current sample
	QList<int> rna_ps_ids;
	foreach (int rna_sample, db_.relatedSamples(sample_id.toInt(), "same sample", "RNA"))
	{
		rna_ps_ids << db_.getValuesInt("SELECT id FROM processed_sample WHERE quality!='bad' AND sample_id=:0", QString::number(rna_sample));
	}

	if (rna_ps_ids.size() > 0)
	{
		std::sort(rna_ps_ids.rbegin(), rna_ps_ids.rend());

		//get RNA QC
		QString avg_cov = "";
		QString avg_cov_housekeeping = "";
		QString covered_genes = "";
		QString read_count = "";
		QCCollection stats;
		try
		{
			stats = db_.getQCData(QString::number(rna_ps_ids.at(0)));
		}
		catch(...)
		{
		}

		for (int i=0; i<stats.count(); ++i)
		{
			if (stats[i].accession()=="QC:2000005") read_count = stats[i].toString();
			if (stats[i].accession()=="QC:2000025") avg_cov = stats[i].toString();
			if (stats[i].accession()=="QC:2000101") avg_cov_housekeeping = stats[i].toString();
			if (stats[i].accession()=="QC:2000109") covered_genes = stats[i].toString();
		}
		stream << endl;
		stream << "<p><b>" << trans("Abdeckungsstatistik der RNA-Probe") << "</b>" << endl;
		stream << "<br />" << trans("Anzahl der Reads") << ": " << QString::number((double) read_count.toInt()/1000000.0, 'f', 2) << " Mio" << endl;
		stream << "<br />" << trans("Durchschnittliche Sequenziertiefe") << ": " << avg_cov << endl;
		stream << "<br />" << trans("Durchschnittliche Sequenziertiefe der Housekeeping-Gene") << ": " << avg_cov_housekeeping << endl;
		stream << "<br />" << trans("Abgedeckte Gene") << ": " << covered_genes << endl;
		stream << "</p>" << endl;
	}

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

QString GermlineReportGenerator::formatCodingSplicing(const Variant& v)
{
	QStringList output;

	//get transcript-specific data of all relevant transcripts for all overlapping genes
	VariantHgvsAnnotator hgvs_annotator(genome_idx_);
	GeneSet genes = db_.genesOverlapping(v.chr(), v.start(), v.end(), 5000);
	foreach(const QByteArray& gene, genes)
	{
		int gene_id = db_.geneId(gene);
		foreach(const Transcript& trans, db_.releventTranscripts(gene_id))
		{
			try
			{
				//get RefSeq match of transcript if requested
				QString refseq;
				if (data_.report_settings.show_refseq_transcripts)
				{
					const QMap<QByteArray, QByteArrayList>& transcript_matches = NGSHelper::transcriptMatches(data_.build);
					foreach (const QByteArray& match, transcript_matches.value(trans.name()))
					{
						if (match.startsWith("NM_"))
						{
							refseq = "/"+match;
						}
					}
				}

				VariantConsequence consequence = hgvs_annotator.annotate(trans, v);
				output << gene + ":" + trans.nameWithVersion() + refseq + ":" + consequence.hgvs_c + ":" + consequence.hgvs_p;
			}
			catch(Exception& e)
			{
				output << e.message();
			}
		}
	}

	return output.join("<br />");
}

QString GermlineReportGenerator::convertOtherVariantType(const QString& type, bool xml)
{
	if(type == "RE") return (xml)? "repeat_expansion": "Repeat-Expansion";
	if(type == "UPD") return (xml)? "uniparental_disomy": "uniparentale Disomie";
	if(type == "mosaic CNV") return (xml)? "mosaic_cnv": "mosaik CNV";
	if(type == "uncalled small variant") return (xml)? "uncalled_small_variant": "nicht-detektierte kleine Variante (SNV/InDel)";
	if(type == "uncalled CNV") return (xml)? "uncalled_cnv": "nicht-detektierte CNV";
	if(type == "uncalled SV") return (xml)? "uncalled_sv": "nicht-detektierte Strukturvariante";
	THROW(ArgumentException, "Invalid variant type '" + type + "'!");
}

QMap<Chromosome, QString> GermlineReportGenerator::getChromosomeTable(GenomeBuild build)
{
	QStringList refseq;
	if (build == GenomeBuild::HG38)
	{
		// chromosomes 1-22:
		refseq << "NC_000001.11" << "NC_000002.12" << "NC_000003.12" << "NC_000004.12" << "NC_000005.10";
		refseq << "NC_000006.12" << "NC_000007.14" << "NC_000008.11" << "NC_000009.12" << "NC_000010.11";
		refseq << "NC_000011.10" << "NC_000012.12" << "NC_000013.11" << "NC_000014.9"  << "NC_000015.10";
		refseq << "NC_000016.10" << "NC_000017.11" << "NC_000018.10" << "NC_000019.10" << "NC_000020.11";
		refseq << "NC_000021.9"  << "NC_000022.11";
		// chromosomes y + x
		refseq << "NC_000024.10" << "NC_000023.11";
		// chrMT
		refseq << "NC_012920.1";
	}
	else if (build == GenomeBuild::HG19)
	{
		// chromosomes 1-22:
		refseq << "NC_000001.10" << "NC_000002.11" << "NC_000003.11" << "NC_000004.11" << "NC_000005.9";
		refseq << "NC_000006.11" << "NC_000007.13" << "NC_000008.10" << "NC_000009.11" << "NC_000010.10";
		refseq << "NC_000011.9"  << "NC_000012.11" << "NC_000013.10" << "NC_000014.8"  << "NC_000015.9";
		refseq << "NC_000016.9"  << "NC_000017.10" << "NC_000018.9"  << "NC_000019.9"  << "NC_000020.10";
		refseq << "NC_000021.8"  << "NC_000022.10";
		// chromosomes y + x
		refseq << "NC_000024.9" << "NC_000023.10";
		// chrMT
		refseq << "NC_012920.1";
	}
	//Order: chr1-22, chrY, chrX, chrMT
	QStringList chromosomes = db_.getEnum("variant", "chr");

	if (refseq.size() != chromosomes.size())
	{
		THROW(ProgrammingException, "Chromosomes and refseq don't have the same length.")
	}

	QMap<Chromosome, QString> map;
	for (int i=0; i < chromosomes.size(); i++)
	{
		map.insert(Chromosome(chromosomes[i]), refseq[i]);
	}

	return map;
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
	stream << "          <p>Genom: <span class='line'>" << buildToString(evaluation_sheet_data.build, true) << "</span></p>" << endl;
	QString kasp_text;
	try
	{
		KaspData kasp_data = db_.kaspData(ps_id_);
		if (kasp_data.random_error_prob>0.011)
		{
			kasp_text = "auff&auml;llig ("+QString::number(100.0*kasp_data.random_error_prob)+"%)";
		}
		else
		{
			kasp_text = "ok (" + QString::number(100.0*kasp_data.random_error_prob)+"%)";
		}
	}
	catch(DatabaseException& /*e*/) //KASP not done or invalid
	{
		kasp_text = trans("nicht durchgef&uuml;hrt");
	}
	stream << "          <p>KASP: <span class='line'>" << kasp_text << "</span></p>" << endl;
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
			int hpo_id = db_.phenotypeIdByAccession(info.disease_info.toUtf8(), false);
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

	OtherCausalVariant other_causal_var = data_.report_settings.report_config->otherCausalVariant();
	if (other_causal_var.isValid())
	{
		stream << "    <p><b>Sonstige kausale Varianten:</b>" << endl;
		stream << "      <table border='1'>" << endl;
		printVariantSheetRowHeaderOtherVariant(stream);
		printVariantSheetRowOtherVariant(stream, other_causal_var);
		stream << "      </table>" << endl;
		stream << "    </p>" << endl;
	}

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
			THROW(ProgrammingException, "Invalid germline report HTML file " + filename + " generated:\n" + validation_error);
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
	stream << "       <th>c.</th>" << endl;
	stream << "       <th>p.</th>" << endl;
	if (!causal)
	{
		stream << "       <th>Ausschlussgrund</th>" << endl;
	}
	stream << "       <th>gnomAD</th>" << endl;
	stream << "       <th style='white-space: nowrap'>NGSD hom/het</th>" << endl;
	stream << "       <th style='white-space: nowrap'>Kommentar 1. Auswerter</th>" << endl;
	stream << "       <th style='white-space: nowrap'>Kommentar 2. Auswerter</th>" << endl;
	stream << "       <th>Klasse</th>" << endl;
	stream << "       <th style='white-space: nowrap'>In Report</th>" << endl;
	stream << "       <th>RNA</th>" << endl;
	stream << "     </tr>" << endl;
}

void GermlineReportGenerator::printVariantSheetRow(QTextStream& stream, const ReportVariantConfiguration& conf)
{
	//get column indices
	Variant v = data_.variants[conf.variant_index];

	int i_genotype = data_.variants.getSampleHeader().infoByID(data_.ps).column_index;
	int i_class = data_.variants.annotationIndexByName("classification", true, true);
	int i_gnomad = data_.variants.annotationIndexByName("gnomAD", true, true);
	int i_ngsd_hom = data_.variants.annotationIndexByName("NGSD_hom", true, true);
	int i_ngsd_het = data_.variants.annotationIndexByName("NGSD_het", true, true);
	int i_filter = data_.variants.annotationIndexByName("filter", true, true);

	//manual curation
	if (conf.isManuallyCurated()) conf.updateVariant(v, genome_idx_, i_genotype);

	//get transcript-specific data of best transcript for all overlapping genes
	VariantHgvsAnnotator hgvs_annotator(genome_idx_);
	GeneSet genes = db_.genesOverlapping(v.chr(), v.start(), v.end(), 5000);
	QStringList types;
	QStringList hgvs_cs;
	QStringList hgvs_ps;
	foreach(const QByteArray& gene, genes)
	{
		int gene_id = db_.geneId(gene);
		Transcript trans = db_.bestTranscript(gene_id);
		if (trans.isValid())
		{
			try
			{
				VariantConsequence consequence = hgvs_annotator.annotate(trans, v);
				types << consequence.typesToString("&amp;");
				hgvs_cs << trans.nameWithVersion() + ":" + consequence.hgvs_c;
				hgvs_ps << trans.nameWithVersion() + ":" + consequence.hgvs_p;
			}
			catch(Exception& e)
			{
				types << e.message();
				hgvs_cs << e.message();
				hgvs_ps << e.message();
			}
		}
		else
		{
			types << "";
			hgvs_cs << "";
			hgvs_ps << "";
		}
	}

	//write line
	stream << "     <tr>" << endl;
	stream << "       <td>" << genes.join(", ") << "</td>" << endl;
	stream << "       <td>" << types.join(", ") << "</td>" << endl;
	QString genotype = v.annotations()[i_genotype];
	if (v.annotations()[i_filter].contains("mosaic")) genotype += " (mosaic)";
	stream << "       <td>" << genotype << "</td>" << endl;
	stream << "       <td style='white-space: nowrap'>" << v.toString(QChar(), 20) << (conf.isManuallyCurated() ? " (manually curated)" : "") << "</td>" << endl;
	stream << "       <td>" << conf.inheritance << "</td>" << endl;
	stream << "       <td>" << hgvs_cs.join(", ") << "</td>" << endl;
	stream << "       <td>" << hgvs_ps.join(", ") << "</td>" << endl;
	if (!conf.causal)
	{
		stream << "       <td>" << exclusionCriteria(conf) << "</td>" << endl;
	}
	stream << "       <td>" << v.annotations()[i_gnomad] << "</td>" << endl;
	stream << "       <td>" << v.annotations()[i_ngsd_hom] << " / " << v.annotations()[i_ngsd_het] << "</td>" << endl;
	stream << "       <td>" << conf.comments << "</td>" << endl;
	stream << "       <td>" << conf.comments2 << "</td>" << endl;
	stream << "       <td>" << v.annotations()[i_class] << "</td>" << endl;
	stream << "       <td>" << (conf.showInReport() ? "ja" : "nein") << " (" << conf.report_type << ")</td>" << endl;
	stream << "       <td>" << trans(conf.rna_info) << "</td>" << endl;
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
	stream << "       <th>RNA</th>" << endl;
	stream << "     </tr>" << endl;
}

void GermlineReportGenerator::printVariantSheetRowCnv(QTextStream& stream, const ReportVariantConfiguration& conf)
{
	CopyNumberVariant cnv = data_.cnvs[conf.variant_index];

	//manual curation
	if (conf.isManuallyCurated()) conf.updateCnv(cnv, data_.cnvs.annotationHeaders(), db_);

	stream << "     <tr>" << endl;
	stream << "       <td>" << cnv.toString() << (conf.isManuallyCurated() ? " (manually curated)" : "") << "</td>" << endl;
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
	stream << "       <td>" << trans(conf.rna_info) << "</td>" << endl;
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
	stream << "       <th>RNA</th>" << endl;
	stream << "     </tr>" << endl;
}

void GermlineReportGenerator::printVariantSheetRowSv(QTextStream& stream, const ReportVariantConfiguration& conf)
{
	BedpeLine sv = data_.svs[conf.variant_index];

	//manual curation
	if (conf.isManuallyCurated()) conf.updateSv(sv, data_.svs.annotationHeaders(), db_);

	BedFile affected_region = sv.affectedRegion(false);
	stream << "     <tr>" << endl;
	stream << "       <td>" << affected_region[0].toString(true);
	if (sv.type() == StructuralVariantType::BND) stream << " &lt;-&gt; " << affected_region[1].toString(true);
	if (conf.isManuallyCurated()) stream << " (manually curated)";
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
	stream << "       <td>" << trans(conf.rna_info) << "</td>" << endl;
	stream << "     </tr>" << endl;
}

void GermlineReportGenerator::printVariantSheetRowHeaderOtherVariant(QTextStream& stream)
{
	stream << "     <tr>" << endl;
	stream << "       <th>Variantentyp</th>" << endl;
	stream << "       <th>Regionen</th>" << endl;
	stream << "       <th>Gene</th>" << endl;
	stream << "       <th>Erbgang</th>" << endl;
	stream << "       <th>Kommentar</th>" << endl;
	stream << "       <th style='white-space: nowrap'>Kommentar 1. Auswerter</th>" << endl;
	stream << "       <th style='white-space: nowrap'>Kommentar 2. Auswerter</th>" << endl;
	stream << "     </tr>" << endl;
}

void GermlineReportGenerator::printVariantSheetRowOtherVariant(QTextStream& stream, OtherCausalVariant variant)
{
	stream << "     <tr>" << endl;
	stream << "       <td>" << convertOtherVariantType(variant.type) << "</td>" << endl;
	stream << "       <td>" << variant.coordinates << "</td>" << endl;
	stream << "       <td>" << variant.gene << "</td>" << endl;
	stream << "       <td>" << variant.inheritance << "</td>" << endl;
	stream << "       <td>" << variant.comment<< "</td>" << endl;
	stream << "       <td>" << variant.comment_reviewer1 << "</td>" << endl;
	stream << "       <td>" << variant.comment_reviewer2 << "</td>" << endl;
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

void GermlineReportGenerator::gapsByGene(const BedFile& low_cov, const GeneSet& roi_genes, QMap<QByteArray, BedFile>& gaps_by_gene, long long& gap_bases_no_gene)
{
	//clear
	gaps_by_gene.clear();
	gap_bases_no_gene = 0;

	//calculate
	for(int i=0; i<low_cov.count(); ++i)
	{
		const BedLine& line = low_cov[i];
		GeneSet genes = db_.genesOverlapping(line.chr(), line.start(), line.end(), 20); //extend by 20 to annotate splicing regions as well
		bool gene_match = false;
		foreach(const QByteArray& gene, genes)
		{
			if (roi_genes.contains(gene))
			{
				gaps_by_gene[gene].append(line);
				gene_match = true;
			}
		}
		if (gene_match==false)
		{
			gap_bases_no_gene += line.length();
		}
	}
}

void GermlineReportGenerator::writeGapsByGeneTable(QTextStream& stream, QMap<QByteArray, BedFile>& gaps_by_gene, long long& gap_bases_no_gene)
{
	stream << "<table>" << endl;
	stream << "<tr><td><b>" << trans("Gen") << "</b></td><td><b>" << trans("Basen") << "</b></td><td><b>" << trans("Chromosom") << "</b></td><td><b>" << trans("Koordinaten (hg38)") << "</b></td></tr>" << endl;
	for (auto it=gaps_by_gene.cbegin(); it!=gaps_by_gene.cend(); ++it)
	{
		stream << "<tr>" << endl;
		stream << "<td>" << endl;
		const BedFile& gaps = it.value();
		QString chr = gaps[0].chr().str();
		QStringList coords;
		for (int i=0; i<gaps.count(); ++i)
		{
			coords << QString::number(gaps[i].start()) + "-" + QString::number(gaps[i].end());
		}
		stream << it.key() << "</td><td>" << gaps.baseCount() << "</td><td>" << chr << "</td><td>" << coords.join(", ") << endl;

		stream << "</td>" << endl;
		stream << "</tr>" << endl;
	}
	if (gap_bases_no_gene>0)
	{
		stream << "<tr>" << endl;
		stream << "<td>" << trans("kein &Uuml;berlappung mit Gen") << "</td><td>" << gap_bases_no_gene << "</td><td>-</td><td>-</td>" << endl;
		stream << "</tr>" << endl;
	}
	stream << "</table>" << endl;

}
