#include <QCoreApplication>
#include <QFileInfo>
#include "TumorOnlyReportWorker.h"
#include "QCCollection.h"
#include "Exceptions.h"
#include "Statistics.h"
#include "LoginManager.h"
#include "RtfDocument.h"
#include "XmlHelper.h"
#include "Settings.h"

TumorOnlyReportWorker::TumorOnlyReportWorker(const VariantList& variants, const TumorOnlyReportWorkerConfig& config)
	: config_(config)
	, variants_(variants)
	, db_(config.use_test_db)
{
	//set annotation indices
	i_co_sp_ = variants_.annotationIndexByName("coding_and_splicing");
	i_tum_af_ = variants_.annotationIndexByName("tumor_af");
	i_tum_dp_ = variants_.annotationIndexByName("tumor_dp");
	i_gene_ = variants_.annotationIndexByName("gene");
	i_ncg_oncogene_ = variants_.annotationIndexByName("ncg_oncogene");
	i_ncg_tsg_ = variants_.annotationIndexByName("ncg_tsg");
	i_germl_class_ = variants_.annotationIndexByName("classification");
	i_somatic_class_ = variants_.annotationIndexByName("somatic_classification");

	//Set up RTF file specifications
	doc_.addColor(188,230,138);
	doc_.addColor(255,0,0);
	doc_.addColor(255,255,0);
	doc_.addColor(161,161,161);
	doc_.addColor(217,217,217);
}


void TumorOnlyReportWorker::checkAnnotation(const VariantList &variants)
{
	const QStringList anns = {"coding_and_splicing", "tumor_af", "tumor_dp", "gene", "variant_type", "ncg_oncogene", "ncg_tsg", "classification", "somatic_classification"};

	for(const auto& ann : anns)
	{
		if( variants.annotationIndexByName(ann, true, false) < 0) THROW(FileParseException, "Could not find column " + ann + " for tumor only report in variant list.");
	}
}

void TumorOnlyReportWorker::writeXML(QString filename, bool test)
{
	QSharedPointer<QFile> outfile = Helper::openFileForWriting(filename);

	QXmlStreamWriter w(outfile.data());
	w.writeStartDocument();
	w.setAutoFormatting(true);

	w.writeStartElement("DiagnosticNgsReport");
	w.writeAttribute("version", "1");
	w.writeAttribute("genome_build", buildToString( config_.build, true) );


	w.writeStartElement("ReportGeneration");
	w.writeAttribute("date", (test ? "2022-01-30" : QDate::currentDate().toString(Qt::ISODate)) );
	w.writeAttribute("user_name", (test ? "ahtest1" : LoginManager::userLogin() ) );
	w.writeAttribute("software",  (test ? "cppNGSD-TEST-CASE" : QCoreApplication::applicationName()+ " " + QCoreApplication::applicationVersion()) );
	w.writeEndElement();


	//sample
	w.writeStartElement("Sample");
	w.writeAttribute("name", config_.ps_data.name);
	w.writeAttribute("processing_system", config_.sys.name);
	w.writeAttribute("processing_system_type", config_.sys.type);
	w.writeAttribute("comments", config_.ps_data.comments);

	//QC data
	QCCollection qc_data = db_.getQCData(db_.processedSampleId(config_.ps_data.name));
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

	w.writeStartElement("AnalysisPipeline");
	w.writeAttribute("name", "megSAP");
	w.writeAttribute("version", variants_.getPipeline().replace("megSAP","").trimmed() );
	w.writeEndElement();


	w.writeStartElement("TargetRegion");
	w.writeAttribute("name", config_.roi.name);
	for(int i=0; i<config_.roi.regions.count(); ++i)
	{
		const auto& line = config_.roi.regions[i];
		w.writeStartElement("Region");
		w.writeAttribute( "chr", line.chr().str() );
		w.writeAttribute( "start", QString::number( line.start() ) );
		w.writeAttribute( "end", QString::number( line.end() ) );
		w.writeEndElement();
	}

	//group gaps by gene
	QMap<QByteArray, BedFile> gaps_by_gene;
	if(QFileInfo::exists(config_.low_coverage_file))
	{
		BedFile low_cov;
		low_cov.load(config_.low_coverage_file);
		low_cov.intersect(config_.roi.regions);
		for(int i=0; i<low_cov.count(); ++i)
		{
			const BedLine& line = low_cov[i];
			GeneSet genes = db_.genesOverlapping(line.chr(), line.start(), line.end(), 20); //extend by 20 to annotate splicing regions as well
            for (const QByteArray& gene : genes)
			{
				gaps_by_gene[gene].append(line);
			}
		}
	}

	for(QByteArray gene : config_.roi.genes)
	{
		GeneInfo gene_info = db_.geneInfo(gene);
		if(gene_info.symbol.isEmpty()) continue;
		if(gene_info.hgnc_id.isEmpty()) continue;
		gene = gene_info.symbol.toUtf8();

		w.writeStartElement("Gene");
		w.writeAttribute("name", gene);
		w.writeAttribute("id", gene_info.hgnc_id);
		int gene_id = db_.geneId(gene);
		Transcript transcript = db_.bestTranscript(gene_id);
		w.writeAttribute("bases", QString::number(transcript.regions().baseCount()));

		//omim info
		QList<OmimInfo> omim_infos = db_.omimInfo(gene);
        for (const OmimInfo& omim_info : omim_infos)
		{
            for (const Phenotype& pheno : omim_info.phenotypes)
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
		const BedFile& gaps = gaps_by_gene[gene];
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


	w.writeStartElement("VariantList");


	for(int i=0; i<variants_.count(); ++i)
	{
		const Variant& var = variants_[i];

		if(!config_.filter_result.passing(i)) continue;

		w.writeStartElement("Variant");

		w.writeAttribute("chr", var.chr().str());
		w.writeAttribute("start", QString::number(var.start()) );
		w.writeAttribute("end", QString::number(var.end()) );
		w.writeAttribute("ref", var.ref());
		w.writeAttribute("obs", var.obs());
		w.writeAttribute("allele_frequency", var.annotations()[i_tum_af_]);
		w.writeAttribute("depth", var.annotations()[i_tum_dp_]);
		if( !var.annotations()[i_germl_class_].isEmpty() ) w.writeAttribute("germline_class" , var.annotations()[i_germl_class_] );
		if( !var.annotations()[i_somatic_class_].isEmpty() ) w.writeAttribute("somatic_class", var.annotations()[i_somatic_class_] );
		QByteArrayList genes = var.annotations()[i_gene_].split(',');
		QByteArrayList oncogenes = var.annotations()[i_ncg_oncogene_].split(',');
		QByteArrayList tsg = var.annotations()[i_ncg_tsg_].split(',');

		for(int j=0; j < genes.count(); ++j)
		{
			GeneInfo gene_info = db_.geneInfo(genes[j]);
			if(gene_info.symbol.isEmpty()) continue;
			if(gene_info.hgnc_id.isEmpty()) continue; //genes that have been withdrawn or cannot be mapped to a unique approved symbol
			w.writeStartElement("Gene");
			w.writeAttribute("name", gene_info.symbol);
			w.writeAttribute("id", gene_info.hgnc_id);
			if(tsg[j].contains("1"))
			{
				w.writeStartElement("IsTumorSuppressor");
				w.writeAttribute("source", "Network of Cancer Genes");
				w.writeAttribute("source_version", "6.0");
				w.writeEndElement();
			}
			if(oncogenes[j].contains("1"))
			{
				w.writeStartElement("IsOncoGene");
				w.writeAttribute("source", "Network of Cancer Genes");
				w.writeAttribute("source_version", "6.0");
				w.writeEndElement();
			}
			w.writeEndElement();
		}


		//Elements transcript information
		for(int i=0; i < var.transcriptAnnotations(i_co_sp_).count(); ++i )
		{

			const auto trans = var.transcriptAnnotations(i_co_sp_)[i];
			w.writeStartElement("TranscriptInformation");
			w.writeAttribute("transcript_id", QString(trans.id));
			w.writeAttribute("gene", QString(trans.gene) );
			w.writeAttribute("type", QString(trans.type) );
			w.writeAttribute("hgvs_c", QString(trans.hgvs_c) );
			w.writeAttribute("hgvs_p", QString(trans.hgvs_p) );
			w.writeAttribute("exon", QString(trans.exon) ) ;
			w.writeAttribute("variant_type", QString(trans.type) );

			bool is_main_transcript = config_.preferred_transcripts.contains(trans.gene) && config_.preferred_transcripts.value(trans.gene).contains(trans.idWithoutVersion());
			w.writeAttribute("main_transcript", is_main_transcript ? "true" : "false");
			w.writeEndElement();
		}
		w.writeEndElement();
	}
	w.writeEndElement();


	//Element ReportDocument
	w.writeStartElement("ReportDocument");
	w.writeAttribute("format", "RTF");
	w.writeEndElement();

	//end element DiagnosticNgsReport
	w.writeEndDocument();

	w.writeEndDocument();
	outfile->close();

	//validate written XML file
	QString xml_error = XmlHelper::isValidXml(filename, ":/resources/TumorOnlyNGSReport_v1.xsd");
	if (xml_error!="")
	{
		THROW(ProgrammingException, "Invalid tumor only report XML file " + filename + " generated:\n" + xml_error);
	}
}

QByteArray TumorOnlyReportWorker::variantDescription(const Variant &var)
{

	QByteArrayList out;

	//NCG gene classification
	if(var.annotations()[i_ncg_tsg_].contains("1")) out << "TSG";
	if(var.annotations()[i_ncg_oncogene_].contains("1")) out << "Onkogen";

	//germline in-house classification
	if(var.annotations()[i_germl_class_] == "4" || var.annotations()[i_germl_class_] == "5") out << "Keimbahn: Klasse " + var.annotations()[i_germl_class_];

	//somatic in-house classification
	if(!var.annotations()[i_somatic_class_].isEmpty() && var.annotations()[i_somatic_class_] != "n/a")
	{
		out << "Somatik: " +  trans(var.annotations()[i_somatic_class_]);
	}

	return out.join(", \\line\n");
}

QByteArray TumorOnlyReportWorker::trans(QByteArray english)
{
	static QHash<QByteArray, QByteArray> en2de;

	en2de["activating"] = "aktivierend";
	en2de["likely_activating"] = "wahrscheinlich aktivierend";
	en2de["inactivating"] = "inaktivierend";
	en2de["likely_inactivating"] = "wahrscheinlich inaktivierend";
	en2de["unclear"] = "unklar";
	en2de["test_dependent"] = "testabhängig";

	if(!en2de.contains(english)) return english;

	return en2de[english];
}


QByteArray TumorOnlyReportWorker::exonNumber(QByteArray gene, int start, int end)
{
	//get approved gene name
	int gene_id = db_.geneId(gene);
	if (gene_id==-1) return "";
	gene = db_.geneSymbol(gene_id);

	//select transcripts
	TranscriptList transcripts;
	try
	{
		if(config_.preferred_transcripts.contains(gene))
		{
			for(QByteArray preferred_trans : config_.preferred_transcripts.value(gene))
			{
				transcripts << db_.transcript(db_.transcriptId(preferred_trans));
			}
		}
		else //fallback to longest coding transcript
		{
			transcripts << db_.bestTranscript(gene_id);
		}
	}
	catch(Exception)
	{
		return "";
	}

	//calculate exon number
	QByteArrayList out;
	for(const Transcript& trans : transcripts)
	{
		int exon_number = trans.exonNumber(start, end);
		if(exon_number<=0) continue;
		out << trans.name() + " (exon " + QByteArray::number(exon_number) + "/" + QByteArray::number(trans.regions().count()) + ")";
	}
	return out.join(",\\line\n");
}

void TumorOnlyReportWorker::writeRtf(QByteArray file_path)
{

	//Write SNV table
	RtfTable snv_table;
	for(int i=0; i<variants_.count(); ++i)
	{
		if(!config_.filter_result.passing(i)) continue;

		RtfTableRow row;
		VariantTranscript trans = variants_[i].transcriptAnnotations(i_co_sp_).first();
		for(const VariantTranscript& tmp_trans : variants_[i].transcriptAnnotations(i_co_sp_))
		{
			if(config_.preferred_transcripts.value(tmp_trans.gene).contains(tmp_trans.idWithoutVersion()))
			{
				trans = tmp_trans;
				break;
			}
		}

		row.addCell( 1000, trans.gene , RtfParagraph().setItalic(true) );
		row.addCell( 2900, QByteArrayList() << trans.hgvs_c + ", " + trans.hgvs_p << RtfText(trans.id).setFontSize(14).RtfCode());
		row.addCell( 1700, trans.type.replace("_variant", "").replace("&", ", ") );
		row.addCell( 900, QByteArray::number(variants_[i].annotations()[i_tum_af_].toDouble(),'f',2) );
		row.addCell( 3138, variantDescription(variants_[i]) );

		snv_table.addRow(row);
	}

	//sort SNVs by gene symbol
	snv_table.sortByCol(0);

	//SNV table headline
	snv_table.prependRow(RtfTableRow({"Gen","Veränderung","Typ","Anteil","Beschreibung"},{1000,2900,1700,900,3138},RtfParagraph().setBold(true).setHorizontalAlignment("c")).setHeader());
	snv_table.prependRow(RtfTableRow("Punktmutationen (SNVS) und kleine Insertionen/Deletionen (INDELs)",doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setBackgroundColor(5).setHeader());

	snv_table.setUniqueBorder(1,"brdrhair");

	doc_.addPart( snv_table.RtfCode() );
	doc_.addPart( RtfParagraph("").RtfCode() );


	//Create table with additional report data
	QCCollection qc_mapping = db_.getQCData(db_.processedSampleId(config_.ps_data.name));

	RtfTable metadata;
	metadata.addRow( RtfTableRow( { RtfText("Allgemeine Informationen").setBold(true).setFontSize(16).RtfCode(), RtfText("Qualitätsparameter").setBold(true).setFontSize(16).RtfCode() }, {5000,4638}) );
	metadata.addRow( RtfTableRow( { "Datum:",QDate::currentDate().toString("dd.MM.yyyy").toUtf8(), "Coverage 100x:",  qc_mapping.value("QC:2000030",true).toString().toUtf8() + "\%"}, {2250,2750,2319,2319}) );
	metadata.addRow( RtfTableRow( { "Analysepipeline:", variants_.getPipeline().toUtf8(), "Coverage 500x:", qc_mapping.value("QC:2000032",true).toString().toUtf8() + "\%"} , {2250, 2750, 2319, 2319} ) );
	metadata.addRow( RtfTableRow( { "Auswertungssoftware:", QCoreApplication::applicationName().toUtf8() + " " + QCoreApplication::applicationVersion().toUtf8(), "Durchschnittliche Tiefe", qc_mapping.value("QC:2000025",true).toString().toUtf8() + "x"}, {2250,2750,2319,2319}) );

	metadata.setUniqueFontSize(16);

	doc_.addPart(metadata.RtfCode());
	doc_.addPart(RtfParagraph("").RtfCode());


	//gap statistics if target file exists
	if(config_.roi.isValid() && QFileInfo::exists(config_.low_coverage_file))
	{
		doc_.addPart(RtfParagraph("Statistik:").setBold(true).setSpaceAfter(45).setSpaceBefore(45).setFontSize(16).RtfCode());
		RtfTable table;
		table.addRow( RtfTableRow( {"Zielregion:", config_.roi.name.toUtf8()} , {1700,7938} ) );

		if(!config_.roi.genes.isEmpty())
		{
			table.addRow( RtfTableRow({"Zielregion Gene (" + QByteArray::number(config_.roi.genes.count())+"):", config_.roi.genes.join(", ")} , {1700,7938}) );
		}
		table.addRow( RtfTableRow({"Zielregion Region:",QByteArray::number(config_.roi.regions.count())} , {1700,7938}) );
		table.addRow( RtfTableRow({"Zielregion Basen:",QByteArray::number(config_.roi.regions.baseCount())} , {1700,7938}) );


		//Low coverage statistics of target region
		BedFile low_cov;
		low_cov.load(config_.low_coverage_file);
		low_cov.intersect(config_.roi.regions);

		table.addRow( RtfTableRow( {"Lücken Regionen:",QByteArray::number(low_cov.count())}, {1700,7938}) );
		table.addRow( RtfTableRow( {"Lücken Basen:",QByteArray::number(low_cov.baseCount()) + " (" + QByteArray::number(100.0 * low_cov.baseCount()/config_.roi.regions.baseCount(), 'f', 2) + "%)"},{1700,7938}) );

		table.setUniqueFontSize(16);
		doc_.addPart( table.RtfCode() );
		doc_.addPart(RtfParagraph("").RtfCode());


		//Add coverage per gap as annotation to low cov file
		if(config_.include_coverage_per_gap)
		{
			QString ref_file = Settings::string("reference_genome");
			Statistics::avgCoverage(low_cov, config_.bam_file, 1, config_.threads, 2, ref_file);
		}

		//Find genes with gaps
		QVector<QByteArray> genes;
		QVector<QByteArray> exons;


		//block summary of gaps that overlap an exon, to be printed after gap statistics in table
		QMultiMap<QString, QString> block_summary;

		for(int i=0; i<low_cov.count(); ++i)
		{
			const BedLine& line = low_cov[i];

			QStringList tmp_genes = db_.genesOverlapping(line.chr(), line.start(), line.end()).toStringList();

			genes.append( tmp_genes.join(", ").toUtf8() );

			if(config_.include_exon_number_per_gap)
			{
				QStringList tmp_exons;
				for(const auto& tmp_gene : tmp_genes)
				{
					QByteArray exon = exonNumber(tmp_gene.toUtf8() , line.start(), line.end());
					if(exon != "")
					{
						tmp_exons << exon;
						block_summary.insert(tmp_gene, line.toString(true));
					}
				}
				exons.append( tmp_exons.join(", ").toUtf8() );
			}

		}
		//Write each gaps
		RtfTable detailed_gaps;
		for(int i=0; i< low_cov.count(); ++i)
		{
			RtfTableRow row;
			if(!genes.isEmpty())
			{
				row.addCell(2000, genes[i], RtfParagraph().setItalic(true));
			}
			else
			{
				row.addCell(2000,"NA");
			}

			RtfSourceCode pos  = low_cov[i].chr().strNormalized(true) + ":" + QByteArray::number(low_cov[i].start()) + "-" + QByteArray::number(low_cov[i].end());
			if(!exons.isEmpty() && !exons[i].isEmpty()) pos += RtfText("\\line\n" + exons[i]).setFontSize(14).RtfCode();

			row.addCell(3500, pos);
			if(config_.include_coverage_per_gap) row.addCell(4138, low_cov[i].annotations().last() + "x");

			detailed_gaps.addRow(row);
		}

		//sort by gene symbol, then by coordinate column
		detailed_gaps.sortbyCols({0,1});

		//add header
		if(low_cov.count()>0)
		{
			detailed_gaps.prependRow(RtfTableRow({"Gen", "Lücke"}, {2000,3500}, RtfParagraph().setBold(true) ).setHeader() );
			if(config_.include_coverage_per_gap) detailed_gaps.first().addCell(4138, "Coverage", RtfParagraph().setBold(true) );
		}

		detailed_gaps.setUniqueFontSize(16);
		doc_.addPart(detailed_gaps.RtfCode());

		//add block summary of exon gaps
		if(!block_summary.isEmpty())
		{
			QList<RtfSourceCode> block_text;
			for( const auto& gene : block_summary.uniqueKeys() )
			{
				block_text << RtfText(gene.toUtf8()).setItalic(true).setFontSize(16).RtfCode() + ": " + block_summary.values(gene).join(", ").toUtf8();
			}
			doc_.addPart(RtfParagraph("").RtfCode());
			doc_.addPart(RtfParagraph(block_text.join("; ")).setFontSize(16).RtfCode());
		}
	}

	doc_.save(file_path);
}
