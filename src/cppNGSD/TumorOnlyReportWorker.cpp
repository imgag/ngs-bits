#include <QCoreApplication>
#include <QFileInfo>
#include "TumorOnlyReportWorker.h"
#include "QCCollection.h"
#include "Exceptions.h"
#include "Statistics.h"
#include "LoginManager.h"
#include "RtfDocument.h"

TumorOnlyReportWorker::TumorOnlyReportWorker(const VariantList& variants, const TumorOnlyReportWorkerConfig& config)
	: config_(config)
	, variants_(variants)
	, db_(config.use_test_db)
{
	//set annotation indices
	i_co_sp_ = variants_.annotationIndexByName("coding_and_splicing");
	i_tum_af_ = variants_.annotationIndexByName("tumor_af");
	i_cgi_driver_statem_ = variants_.annotationIndexByName("CGI_driver_statement");
	i_ncg_oncogene_ = variants_.annotationIndexByName("ncg_oncogene");
	i_ncg_tsg_ = variants_.annotationIndexByName("ncg_tsg");
	i_germl_class_ = variants_.annotationIndexByName("classification");
	i_somatic_class_ = variants_.annotationIndexByName("somatic_classification");

	//Set up RTF file specifications
	doc_.setMargins(1134,1134,1134,1134);
	doc_.addColor(188,230,138);
	doc_.addColor(255,0,0);
	doc_.addColor(255,255,0);
	doc_.addColor(161,161,161);
	doc_.addColor(217,217,217);
}


void TumorOnlyReportWorker::checkAnnotation(const VariantList &variants)
{
	const QStringList anns = {"coding_and_splicing", "tumor_af", "tumor_dp", "gene", "variant_type", "CGI_driver_statement", "ncg_oncogene", "ncg_tsg", "classification", "somatic_classification"};

	for(const auto& ann : anns)
	{
		if( variants.annotationIndexByName(ann, true, false) < 0) THROW(FileParseException, "Could not find column " + ann + " for tumor only report in variant list.");
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

	//CGI classification
	if(var.annotations()[i_cgi_driver_statem_].contains("known")) out << "CGI: Treiber (bekannt)";
	else if(var.annotations()[i_cgi_driver_statem_].contains("predicted driver")) out << "CGI: Treiber (vorhergesagt)";

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
	int gene_id = db_.geneToApprovedID(gene);
	if (gene_id==-1) return "";
	gene = db_.geneSymbol(gene_id);

	//select transcripts
	QList<Transcript> transcripts;
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
			transcripts << db_.longestCodingTranscript(gene_id, Transcript::SOURCE::ENSEMBL, false, true);
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


QByteArray TumorOnlyReportWorker::cgiCancerTypeFromVariantList(const VariantList &variants)
{
	QStringList comments = variants.comments();
	foreach(QString comment,comments)
	{
		if(comment.startsWith("##CGI_CANCER_TYPE="))
		{
			QByteArray cancer_type = comment.mid(18).trimmed().toUtf8();
			if(!cancer_type.isEmpty()) return cancer_type;
			else return "n/a";
		}
	}
	return "n/a";
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
		row.addCell( { trans.hgvs_c + ", " + trans.hgvs_p, RtfText(trans.id).setFontSize(14).RtfCode()}, 2900 );
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
	QCCollection qc_mapping = db_.getQCData(db_.processedSampleId(config_.ps));

	RtfTable metadata;
	metadata.addRow( RtfTableRow( { RtfText("Allgemeine Informationen").setBold(true).setFontSize(16).RtfCode(), RtfText("Qualitätsparameter").setBold(true).setFontSize(16).RtfCode() }, {5000,4638}) );
	metadata.addRow( RtfTableRow( { "Datum:",QDate::currentDate().toString("dd.MM.yyyy").toUtf8(), "Coverage 100x:",  qc_mapping.value("QC:2000030",true).toString().toUtf8() + "\%"}, {2250,2750,2319,2319}) );
	metadata.addRow( RtfTableRow( { "Analysepipeline:", variants_.getPipeline().toUtf8(), "Coverage 500x:", qc_mapping.value("QC:2000032",true).toString().toUtf8() + "\%"} , {2250, 2750, 2319, 2319} ) );
	metadata.addRow( RtfTableRow( { "Auswertungssoftware:", QCoreApplication::applicationName().toUtf8() + " " + QCoreApplication::applicationVersion().toUtf8(), "Durchschnittliche Tiefe", qc_mapping.value("QC:2000025",true).toString().toUtf8() + "x"}, {2250,2750,2319,2319}) );
	metadata.addRow( RtfTableRow( { "CGI-Tumortyp:", cgiCancerTypeFromVariantList(variants_), "", ""} , {2250,2750,2319,2319} ) );

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
			Statistics::avgCoverage(low_cov, config_.bam_file, 1, false, true, 2);
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
