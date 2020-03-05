#include "ReportWorker.h"
#include "Log.h"
#include "Helper.h"
#include "Exceptions.h"
#include "Statistics.h"
#include "Settings.h"
#include "BedFile.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "XmlHelper.h"
#include "NGSHelper.h"
#include "FilterCascade.h"
#include "GSvarHelper.h"
#include "LoginManager.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QCoreApplication>
#include <QXmlStreamWriter>
#include <QMessageBox>
#include <QDesktopServices>
#include <QApplication>



ReportWorker::ReportWorker(QString sample_name, QString file_bam, QString file_roi, const VariantList& variants, const CnvList& cnvs, const FilterCascade& filters, ReportSettings settings, QStringList log_files, QString file_rep)
	: WorkerBase("Report generation")
	, sample_name_(sample_name)
	, file_bam_(file_bam)
	, file_roi_(file_roi)
	, variants_(variants)
	, cnvs_(cnvs)
	, filters_(filters)
	, settings_(settings)
	, log_files_(log_files)
	, file_rep_(file_rep)
	, var_count_(variants_.count())
{
}

void ReportWorker::process()
{
	//load ROI if given
	if (file_roi_!="")
	{
		roi_.load(file_roi_);
		roi_.merge();

		//determine variant count (inside target region)
		FilterResult filter_result(variants_.count());
		FilterRegions::apply(variants_, roi_, filter_result);
		var_count_ = filter_result.countPassing();

		//load gene list file
		genes_ = GeneSet::createFromFile(file_roi_.left(file_roi_.size()-4) + "_genes.txt");
	}

	roi_stats_.clear();
	writeHTML();
}

QString ReportWorker::formatCodingSplicing(const QList<VariantTranscript>& transcripts)
{
    const QMap<QByteArray, QByteArrayList>& preferred_transcripts = GSvarHelper::preferredTranscripts();

	QList<QByteArray> output;
	QList<QByteArray> output_pt;

	foreach(const VariantTranscript& trans, transcripts)
	{
		QByteArray line = trans.gene + ":" + trans.id + ":" + trans.hgvs_c + ":" + trans.hgvs_p;

		output.append(line);

        if (preferred_transcripts.value(trans.gene).contains(trans.id))
		{
			output_pt.append(line);
		}
	}

	//return only preferred transcripts if present
	if (output_pt.count()>0)
	{
		output = output_pt;
	}

	return output.join("<br />");
}

QByteArray ReportWorker::formatGenotype(const QByteArray& gender, const QByteArray& genotype, const Variant& variant)
{
	//correct only hom variants on gonosomes outside the PAR for males
	if (gender!="male") return genotype;
	if (genotype!="hom") return genotype;
	if (!variant.chr().isGonosome()) return genotype;
	if (NGSHelper::pseudoAutosomalRegion("hg19").overlapsWith(variant.chr(), variant.start(), variant.end())) return genotype;

	return "hemi";
}

void ReportWorker::writeCoverageReport(QTextStream& stream, QString bam_file, QString roi_file, const BedFile& roi, const GeneSet& genes, int min_cov,  NGSD& db, bool calculate_depth, QMap<QString, QString>* output, bool gene_and_gap_details)
{
	//get target region coverages (from NGSD or calculate)
	QString avg_cov = "";
	QCCollection stats;
	if (isProcessingSystemTargetFile(bam_file, roi_file, db) || !calculate_depth)
	{
		try
		{
			QString processed_sample_id = db.processedSampleId(bam_file);
			stats = db.getQCData(processed_sample_id);
		}
		catch(...)
		{
		}
	}
	if (stats.count()==0)
	{
		Log::warn("Target region depth from NGSD cannot be used because ROI is not the processing system target region! Recalculating...");
		stats = Statistics::mapping(roi, bam_file);
	}
	for (int i=0; i<stats.count(); ++i)
	{
		if (stats[i].accession()=="QC:2000025") avg_cov = stats[i].toString();
	}
	stream << "<p><b>" << trans("Abdeckungsstatistik") << "</b>" << endl;
	stream << "<br />" << trans("Durchschnittliche Sequenziertiefe") << ": " << avg_cov << endl;
	stream << "</p>" << endl;

	if (gene_and_gap_details)
	{
		//calculate low-coverage regions
		QString message;
		BedFile low_cov = precalculatedGaps(bam_file, roi, min_cov, db, message);
		if (!message.isEmpty())
		{
			Log::warn("Low-coverage statistics needs to be calculated. Pre-calulated gap file cannot be used because: " + message);
			low_cov = Statistics::lowCoverage(roi, bam_file, min_cov);
		}

		//annotate low-coverage regions with gene names
		for(int i=0; i<low_cov.count(); ++i)
		{
			BedLine& line = low_cov[i];
			GeneSet genes = db.genesOverlapping(line.chr(), line.start(), line.end(), 20); //extend by 20 to annotate splicing regions as well
			line.annotations().append(genes.join(", "));
		}

		//group by gene name
		QHash<QByteArray, BedFile> grouped;
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
		if (!genes.isEmpty())
		{
			QStringList complete_genes;
			foreach(const QByteArray& gene, genes)
			{
				if (!grouped.contains(gene))
				{
					complete_genes << gene;
				}
			}
			stream << "<br />" << trans("Komplett abgedeckte Gene") << ": " << complete_genes.join(", ") << endl;
		}
		QString gap_perc = QString::number(100.0*low_cov.baseCount()/roi.baseCount(), 'f', 2);
		if (output!=nullptr) output->insert("gap_percentage", gap_perc);
		stream << "<br />" << trans("Anteil Regionen mit Tiefe &lt;") << min_cov << ": " << gap_perc << "%" << endl;
		if (!genes.isEmpty())
		{
			QStringList incomplete_genes;
			foreach(const QByteArray& gene, genes)
			{
				if (grouped.contains(gene))
				{
					incomplete_genes << gene + " <span style=\"font-size: 80%;\">" + QString::number(grouped[gene].baseCount()) + "</span> ";
				}
			}
			stream << "<br />" << trans("Fehlende Basen in nicht komplett abgedeckten Genen") << ": " << incomplete_genes.join(", ") << endl;
		}

		stream << "<p>" << trans("Details Regionen mit Tiefe &lt;") << min_cov << ":" << endl;
		stream << "</p>" << endl;
		stream << "<table>" << endl;
		stream << "<tr><td><b>" << trans("Gen") << "</b></td><td><b>" << trans("L&uuml;cken") << "</b></td><td><b>" << trans("Chromosom") << "</b></td><td><b>" << trans("Koordinaten (hg19)") << "</b></td></tr>" << endl;
		for (auto it=grouped.cbegin(); it!=grouped.cend(); ++it)
		{
			stream << "<tr>" << endl;
			stream << "<td>" << endl;
			const BedFile& gaps = it.value();
			QString chr = gaps[0].chr().strNormalized(true);;
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

void ReportWorker::writeCoverageReportCCDS(QTextStream& stream, QString bam_file, const GeneSet& genes, int min_cov, int extend, NGSD& db, QMap<QString, QString>* output, bool gap_table, bool gene_details)
{
	QString ext_string = (extend==0 ? "" : " +-" + QString::number(extend) + " ");
	stream << "<p><b>" << trans("Abdeckungsstatistik f&uuml;r CCDS") << " " << ext_string << "</b></p>" << endl;
	if (gap_table) stream << "<p><table>";
	if (gap_table) stream << "<tr><td><b>" << trans("Gen") << "</b></td><td><b>" << trans("Transcript") << "</b></td><td><b>" << trans("Gr&ouml;&szlig;e") << "</b></td><td><b>" << trans("L&uuml;cken") << "</b></td><td><b>" << trans("Chromosom") << "</b></td><td><b>" << trans("Koordinaten (hg19)") << "</b></td></tr>";
	QMap<QByteArray, int> gap_count;
	long long bases_overall = 0;
	long long bases_sequenced = 0;
	GeneSet genes_noncoding;
	GeneSet genes_notranscript;
	foreach(const QByteArray& gene, genes)
	{
		int gene_id = db.geneToApprovedID(gene);

		//approved gene symbol
		QByteArray symbol = db.geneSymbol(gene_id);

		//longest coding transcript
		Transcript transcript = db.longestCodingTranscript(gene_id, Transcript::CCDS, true);
		if (!transcript.isValid())
		{
			transcript = db.longestCodingTranscript(gene_id, Transcript::CCDS, true, true);
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
		QString message;
		BedFile roi = transcript.codingRegions();
		if (extend>0)
		{
			roi.extend(extend);
			roi.merge();
		}
		BedFile gaps = precalculatedGaps(bam_file, roi, min_cov, db, message);
		if (!message.isEmpty())
		{
			Log::warn("Low-coverage statistics for transcript " + transcript.name() + " needs to be calculated. Pre-calulated gap file cannot be used because: " + message);
			gaps = Statistics::lowCoverage(roi, bam_file, min_cov);
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
	if (gap_table) stream << "</table>";

	//show warning if non-coding transcripts had to be used
	if (!genes_noncoding.isEmpty())
	{
		stream << "<br>Warning: Using the longest *non-coding* transcript for genes " << genes_noncoding.join(", ") << " (no coding transcripts for GRCh37 defined)";
	}
	if (!genes_notranscript.isEmpty())
	{
		stream << "<br>Warning: No transcript defined for genes " << genes_notranscript.join(", ");
	}

	//overall statistics
	stream << "<p>CCDS " << ext_string << trans("gesamt") << ": " << bases_overall << endl;
	stream << "<br />CCDS " << ext_string << trans("mit Tiefe") << " &ge;" << min_cov << ": " << bases_sequenced << " (" << QString::number(100.0 * bases_sequenced / bases_overall, 'f', 2)<< "%)" << endl;
	long long gaps = bases_overall - bases_sequenced;
	stream << "<br />CCDS " << ext_string << trans("mit Tiefe") << " &lt;" << min_cov << ": " << gaps << " (" << QString::number(100.0 * gaps / bases_overall, 'f', 2)<< "%)" << endl;
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
				genes_incomplete << it.key() + " <span style=\"font-size: 80%;\">" + QByteArray::number(it.value()) + "</span> ";
			}
		}
		stream << "<p>";
		stream << trans("Komplett abgedeckte Gene") << ": " << genes_complete.join(", ") << endl;
		stream << "<br />" << trans("Fehlende Basen in nicht komplett abgedeckten Genen") << ": " << genes_incomplete.join(", ") << endl;
		stream << "</p>";
	}

	if (output!=nullptr) output->insert("ccds_sequenced", QString::number(bases_sequenced));
}

BedFile ReportWorker::precalculatedGaps(QString bam_file, const BedFile& roi, int min_cov, NGSD& db, QString& message)
{
	message.clear();

	//check depth cutoff
	if (min_cov!=20)
	{
		message = "Depth cutoff is not 20!";
		return BedFile();
	}

	//find low-coverage file
	QString dir = QFileInfo(bam_file).absolutePath();
	QStringList low_cov_files = Helper::findFiles(dir, "*_lowcov.bed", false);
	if(low_cov_files.count()!=1)
	{
		message = "Low-coverage file does not exist in " + dir;
		return BedFile();
	}
	QString low_cov_file = low_cov_files[0];

	//load low-coverage file
	BedFile gaps;
	gaps.load(low_cov_file);

	//For WGS there is nothing more to check
	ProcessingSystemData system_data = db.getProcessingSystemData(db.processingSystemIdFromProcessedSample(bam_file), true);
	if (system_data.type=="WGS" || system_data.type=="WGS (shallow)")
	{
		return gaps;
	}

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
	if (regions<0 || bases<0)
	{
		message = "Low-coverage file header does not contain target region statistics: " + low_cov_file;
		return BedFile();
	}

	//compare statistics to current processing system
	if (system_data.target_file=="")
	{
		message = "Processing system target file not defined in NGSD!";
		return BedFile();
	}
	BedFile sys;
	sys.load(system_data.target_file);
	sys.merge();
	if (sys.count()!=regions || sys.baseCount()!=bases)
	{
		message = "Low-coverage file is outdated. It does not match processing system target region: " + low_cov_file;
		return BedFile();
	}

	//calculate gaps inside target region
	gaps.intersect(roi);

	//add target region bases not covered by processing system target file
	BedFile uncovered(roi);
	uncovered.subtract(sys);
	gaps.add(uncovered);
	gaps.merge();

	return gaps;
}

bool ReportWorker::isProcessingSystemTargetFile(QString bam_file, QString roi_file, NGSD& db)
{
	ProcessingSystemData system_data = db.getProcessingSystemData(db.processingSystemIdFromProcessedSample(bam_file), true);

	return Helper::canonicalPath(system_data.target_file) == Helper::canonicalPath(roi_file);
}

void ReportWorker::writeHtmlHeader(QTextStream& stream, QString sample_name)
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
	stream << "	font-family: sans-serif;" << endl;
	stream << "	font-size: 70%;" << endl;
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
	stream << "	font-size: 100%;" << endl;
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

void ReportWorker::writeHtmlFooter(QTextStream& stream)
{
	stream << "	</body>" << endl;
	stream << "</html>" << endl;
}

void ReportWorker::writeHTML()
{
	QString temp_filename = Helper::tempFileName(".html");
	QSharedPointer<QFile> outfile = Helper::openFileForWriting(temp_filename);
	QTextStream stream(outfile.data());
	writeHtmlHeader(stream, sample_name_);

	//get trio data
	bool is_trio = variants_.type() == GERMLINE_TRIO;
	SampleInfo info_father;
	SampleInfo info_mother;
	if (is_trio)
	{
		info_father = variants_.getSampleHeader().infoByStatus(false, "male");
		info_mother = variants_.getSampleHeader().infoByStatus(false, "female");
	}

	//get data from database
	QString sample_id = db_.sampleId(sample_name_);
	SampleData sample_data = db_.getSampleData(sample_id);
	QString processed_sample_id = db_.processedSampleId(sample_name_);
	ProcessedSampleData processed_sample_data = db_.getProcessedSampleData(processed_sample_id);
	ProcessingSystemData system_data = db_.getProcessingSystemData(db_.processingSystemIdFromProcessedSample(sample_name_), true);

	//report header (meta information)
	stream << "<h4>" << trans("Technischer Report zur bioinformatischen Analyse") << "</h4>" << endl;

	stream << "<p>" << endl;
	stream << "<b>" << trans("Probe") << ": " << sample_name_ << "</b> (" << sample_data.name_external << ")" << endl;
	if (is_trio)
	{
		stream << "<br />" << endl;
		stream << "<br />" << trans("Vater") << ": "  << info_father.id << endl;
		stream << "<br />" << trans("Mutter") << ": "  << info_mother.id << endl;
	}
	stream << "<br />" << endl;
	stream << "<br />" << trans("Geschlecht") << ": " << processed_sample_data.gender << endl;
	stream << "<br />" << trans("Prozessierungssystem") << ": " << processed_sample_data.processing_system << endl;
	stream << "<br />" << trans("Prozessierungssystem-Typ") << ": " << processed_sample_data.processing_system_type << endl;
	stream << "<br />" << trans("Referenzgenom") << ": " << system_data.genome << endl;
	stream << "<br />" << trans("Datum") << ": " << QDate::currentDate().toString("dd.MM.yyyy") << endl;
	stream << "<br />" << trans("Benutzer") << ": " << LoginManager::user() << endl;
	stream << "<br />" << trans("Analysepipeline") << ": "  << variants_.getPipeline() << endl;
	stream << "<br />" << trans("Auswertungssoftware") << ": "  << QCoreApplication::applicationName() << " " << QCoreApplication::applicationVersion() << endl;
	stream << "<br />" << trans("KASP-Ergebnis") << ": " << db_.getQCData(processed_sample_id).value("kasp").asString() << endl;
	stream << "</p>" << endl;

	///Phenotype information
	stream << "<p><b>" << trans("Ph&auml;notyp") << "</b>" << endl;
	QList<SampleDiseaseInfo> info = db_.getSampleDiseaseInfo(sample_id, "ICD10 code");
	foreach(const SampleDiseaseInfo& entry, info)
	{
		stream << "<br />ICD10: " << entry.disease_info << endl;
	}
	info = db_.getSampleDiseaseInfo(sample_id, "HPO term id");
	foreach(const SampleDiseaseInfo& entry, info)
	{
		stream << "<br />HPO: " << entry.disease_info << " (" << db_.phenotypeByAccession(entry.disease_info.toLatin1(), false).name() << ")" << endl;
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
	if (file_roi_!="")
	{
		stream << "<p><b>" << trans("Zielregion") << "</b>" << endl;
		stream << "<br /><span style=\"font-size: 80%;\">" << trans("Die Zielregion umfasst mindestens die CCDS (\"consensus coding sequence\") unten genannter Gene &plusmn;20 Basen flankierender intronischer Sequenz, kann aber auch zus&auml;tzliche Exons und/oder flankierende Basen beinhalten.") << endl;
		stream << "<br />" << trans("Name") << ": " << QFileInfo(file_roi_).fileName().replace(".bed", "") << endl;
		if (!genes_.isEmpty())
		{
			stream << "<br />" << trans("Ausgewertete Gene") << " (" << QString::number(genes_.count()) << "): " << genes_.join(", ") << endl;
		}
		stream << "</span></p>" << endl;
	}

	//get column indices
	int i_genotype = variants_.getSampleHeader().infoByStatus(true).column_index;
	int i_gene = variants_.annotationIndexByName("gene", true, true);
	int i_co_sp = variants_.annotationIndexByName("coding_and_splicing", true, true);
	int i_omim = variants_.annotationIndexByName("OMIM", true, true);
	int i_class = variants_.annotationIndexByName("classification", true, true);
	int i_comment = variants_.annotationIndexByName("comment", true, true);
	int i_kg = variants_.annotationIndexByName("1000G", true, true);
	int i_gnomad = variants_.annotationIndexByName("gnomAD", true, true);

	//output: applied filters
	stream << "<p><b>" << trans("Filterkriterien") << " " << "</b>" << endl;
	for(int i=0; i<filters_.count(); ++i)
	{
		stream << "<br />&nbsp;&nbsp;&nbsp;&nbsp;- " << filters_[i]->toText() << endl;
	}
	stream << "<br />" << trans("Gefundene Varianten in Zielregion gesamt") << ": " << var_count_ << endl;
	int selected_var_count = 0;
	foreach(int index, settings_.report_config.variantIndices(VariantType::SNVS_INDELS, true, settings_.report_type))
	{
		const Variant & variant = variants_[index];
		if (file_roi_=="" || roi_.overlapsWith(variant.chr(), variant.start(), variant.end()))
		{
			++selected_var_count;
		}
	}
	stream << "<br />" << trans("Anzahl Varianten ausgew&auml;hlt f&uuml;r Report") << ": " << selected_var_count << endl;
	int selected_cnv_count = 0;
	foreach(int index, settings_.report_config.variantIndices(VariantType::CNVS, true, settings_.report_type))
	{
		const CopyNumberVariant& cnv = cnvs_[index];
		if (file_roi_=="" || roi_.overlapsWith(cnv.chr(), cnv.start(), cnv.end()))
		{
			++selected_cnv_count;
		}
	}
	stream << "<br />" << trans("Anzahl CNVs ausgew&auml;hlt f&uuml;r Report") << ": " << selected_cnv_count << endl;
	stream << "</p>" << endl;

	//output: selected variants
	stream << "<p><b>" << trans("Varianten nach klinischer Interpretation im Kontext der Fragestellung") << "</b>" << endl;
	stream << "</p>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><td><b>" << trans("Variante") << "</b></td><td><b>" << trans("Genotyp") << "</b></td>";
	if (is_trio)
	{
		stream << "<td><b>" << trans("Vater") << "</b></td>";
		stream << "<td><b>" << trans("Mutter") << "</b></td>";
	}
	stream << "<td><b>" << trans("Gen(e)") << "</b></td><td><b>" << trans("Details") << "</b></td><td><b>" << trans("Klasse") << "</b></td><td><b>" << trans("Vererbung") << "</b></td><td><b>1000g</b></td><td><b>gnomAD</b></td></tr>" << endl;

	foreach(const ReportVariantConfiguration& var_conf, settings_.report_config.variantConfig())
	{
		if (var_conf.variant_type!=VariantType::SNVS_INDELS) continue;
		if (!var_conf.showInReport()) continue;
		if (var_conf.report_type!=settings_.report_type) continue;
		const Variant& variant = variants_[var_conf.variant_index];
		if (file_roi_!="" && !roi_.overlapsWith(variant.chr(), variant.start(), variant.end())) continue;

		stream << "<tr>" << endl;
		stream << "<td>" << endl;
		stream  << variant.chr().str() << ":" << variant.start() << "&nbsp;" << variant.ref() << "&nbsp;&gt;&nbsp;" << variant.obs() << "</td>";
		QString geno = formatGenotype(processed_sample_data.gender.toLatin1(), variant.annotations().at(i_genotype), variant);
		if (var_conf.de_novo) geno += " (de-novo)";
		if (var_conf.mosaic) geno += " (mosaic)";
		if (var_conf.comp_het) geno += " (comp-het)";
		stream << "<td>" << geno << "</td>" << endl;
		if (is_trio)
		{
			stream << "<td>" << formatGenotype("male", variant.annotations().at(info_father.column_index), variant) << "</td>";
			stream << "<td>" << formatGenotype("female", variant.annotations().at(info_mother.column_index), variant) << "</td>";
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
	stream << "<br>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><td><b>" << trans("CNV") << "</b></td><td><b>" << trans("Regionen") << "</b></td><td><b>" << trans("CN") << "</b></td><td><b>" << trans("Gen(e)") << "</b><td><b>" << trans("Klasse") << "</b></td><td><b>" << trans("Vererbung") << "</b></td></td></tr>" << endl;

	foreach(const ReportVariantConfiguration& var_conf, settings_.report_config.variantConfig())
	{
		if (var_conf.variant_type!=VariantType::CNVS) continue;
		if (!var_conf.showInReport()) continue;
		if (var_conf.report_type!=settings_.report_type) continue;
		const CopyNumberVariant& cnv = cnvs_[var_conf.variant_index];
		if (file_roi_!="" && !roi_.overlapsWith(cnv.chr(), cnv.start(), cnv.end())) continue;

		stream << "<tr>" << endl;
		stream << "<td>" << cnv.toString() << "</td>" << endl;
		stream << "<td>" << std::max(1, cnv.regions()) << "</td>" << endl; //trio CNV lists don't contain number of regions > fix

		QString cn = QString::number(cnv.copyNumber(cnvs_.annotationHeaders()));
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

	stream << "<p>" << trans("F&uuml;r Informationen zur Klassifizierung von Varianten, siehe allgemeine Zusatzinformationen.") << endl;
	stream << "</p>" << endl;

	stream << "<p>" << trans("Teilweise k&ouml;nnen bei Varianten unklarer Signifikanz (Klasse 3) -  in Abh&auml;ngigkeit von der Art der genetischen Ver&auml;nderung, der Familienanamnese und der Klinik des/der Patienten - weiterf&uuml;hrende Untersuchungen eine &Auml;nderung der Klassifizierung bewirken. Bei konkreten differentialdiagnostischen Hinweisen auf eine entsprechende Erkrankung ist eine humangenetische Mitbeurteilung erforderlich, zur Beurteilung ob erweiterte genetische Untersuchungen zielf&uuml;hrend w&auml;ren.") << endl;
	stream << "</p>" << endl;

	///classification explaination
	if (settings_.show_class_details)
	{
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
	if (settings_.show_coverage_details && file_bam_!="")
	{
		writeCoverageReport(stream, file_bam_, file_roi_, roi_, genes_, settings_.min_depth, db_, settings_.recalculate_avg_depth, &roi_stats_, settings_.roi_low_cov);

		writeCoverageReportCCDS(stream, file_bam_, genes_, settings_.min_depth, 0, db_, &roi_stats_, false, false);

		writeCoverageReportCCDS(stream, file_bam_, genes_, settings_.min_depth, 5, db_, nullptr, true, true);
	}

	//OMIM table
	if (settings_.show_omim_table)
	{
		stream << "<p><b>" << trans("OMIM Gene und Phenotypen") << "</b>" << endl;
		stream << "</p>" << endl;
		stream << "<table>" << endl;
		stream << "<tr><td><b>" << trans("Gen") << "</b></td><td><b>" << trans("Gen MIM") << "</b></td><td><b>" << trans("Phenotyp") << "</b></td><td><b>" << trans("Phenotyp MIM") << "</b></td></tr>" << endl;
		foreach(QByteArray gene, genes_)
		{
			OmimInfo omim_info = db_.omimInfo(gene);
			if (!omim_info.gene_symbol.isEmpty())
			{
				QStringList names;
				QStringList accessions;
				foreach(const Phenotype& p, omim_info.phenotypes)
				{
					names << p.name();
					accessions << p.accession();
				}

				stream << "<tr><td>" << omim_info.gene_symbol << "</td><td>" << omim_info.mim << "</td><td>" << names.join("<br>")<< "</td><td>" << accessions.join("<br>")<< "</td></tr>";
			}
		}
		stream << "</table>" << endl;
	}

	//collect and display important tool versions
	if (settings_.show_tool_details)
	{
		stream << "<p><b>" << trans("Details zu Programmen der Analysepipeline") << "</b>" << endl;
		stream << "</p>" << endl;
		stream << "<table>" << endl;
		stream << "<tr><td><b>" << trans("Tool") << "</b></td><td><b>" << trans("Version") << "</b></td><td><b>" << trans("Parameter") << "</b></td></tr>";
		QStringList whitelist;
		whitelist << "SeqPurge" << "samblaster" << "/bwa" << "samtools" << "VcfLeftNormalize" <<  "freebayes" << "abra2" << "vcflib" << "ensembl-vep"; //current
		log_files_.sort();
		foreach(QString file, log_files_)
		{
			QStringList lines = Helper::loadTextFile(file);
			for(int i=0; i<lines.count(); ++i)
			{
				QString& line = lines[i];

				if (line.contains("Calling external tool")
					|| line.contains("command 1")
					|| line.contains("command 2")
					|| line.contains("command 3")
					|| line.contains("command 4")
					|| line.contains("command 5")
					|| line.contains("command 6")
					|| line.contains("command 7")
					|| line.contains("command 8")
					|| line.contains("command 9")
					|| line.contains("command 10")
					|| line.contains("command 11")
					|| line.contains("command 12")
					|| line.contains("command 13")
					|| line.contains("command 14")
					|| line.contains("command 15"))
				{
					//check if tool is whitelisted
					QString match = "";
					foreach(QString entry, whitelist)
					{
						if (line.contains(entry))
						{
							match = entry;
							break;
						}
					}
					if (match=="") continue;

					//extract version and parameters
					if (i+2>=lines.count()) continue;
					QString tool;
					if (line.contains("Calling external tool"))
					{
						tool = line.split("\t")[2].trimmed().mid(23, -1);
					}
					else
					{
						tool = line.split("\t")[2].trimmed().mid(13, -1);
					}
					tool.replace("/mnt/share/opt/", "[bin]/");
					QString version = lines[i+1].split("\t")[2].trimmed().mid(13);
					QString params = lines[i+2].split("\t")[2].trimmed().mid(13);
					params.replace(QRegExp("/tmp/[^ ]+"), "[file]");
					params.replace(QRegExp("\\./[^ ]+"), "[file]");
					params.replace("/mnt/share/data/", "[data]/");
					params.replace("//", "/");

					//output
					stream << "<tr><td>" << tool.toHtmlEscaped() << "</td><td>" << version.toHtmlEscaped() << "</td><td>" << params.toHtmlEscaped() << "</td></tr>" << endl;
				}
			}
		}
		stream << "</table>" << endl;
	}

	//close stream
	writeHtmlFooter(stream);
	outfile->close();


	validateAndCopyReport(temp_filename, file_rep_, true, false);

	//write XML file to transfer folder
	QString gsvar_variant_transfer = Settings::string("gsvar_variant_transfer");
	if (gsvar_variant_transfer!="")
	{
		QString xml_file = gsvar_variant_transfer + "/" + QFileInfo(file_rep_).fileName().replace(".html", ".xml");
		writeXML(xml_file, file_rep_);
	}
}

void ReportWorker::validateAndCopyReport(QString from, QString to, bool put_to_archive, bool is_rtf)
{
	//validate written file (HTML only)
	if (!is_rtf)
	{
		QString validation_error = XmlHelper::isValidXml(from);
		if (validation_error!="")
		{
			Log::warn("Generated HTML report at " + from + " is not well-formed: " + validation_error);
		}
	}

	if (QFile::exists(to) && !QFile(to).remove())
	{
		THROW(FileAccessException,"Could not remove previous " + QString(is_rtf ? "RTF" : "HTML") + " report: " + to);
	}
	if (!QFile::rename(from, to))
	{
		THROW(FileAccessException,"Could not move " + QString(is_rtf ? "RTF" : "HTML") + " report from temporary file " + from + " to " + to + " !");
	}

	//copy report to archive folder
	if(put_to_archive)
	{
		QString archive_folder = Settings::string("gsvar_report_archive");
		if (archive_folder!="")
		{
			QString file_rep_copy = archive_folder + "\\" + QFileInfo(to).fileName();
			if (QFile::exists(file_rep_copy) && !QFile::remove(file_rep_copy))
			{
				THROW(FileAccessException, "Could not remove previous " + QString(is_rtf ? "RTF" : "HTML") + " report in archive folder: " + file_rep_copy);
			}
			if (!QFile::copy(to, file_rep_copy))
			{
				THROW(FileAccessException, "Could not copy " + QString(is_rtf ? "RTF" : "HTML") + " report to archive folder: " + file_rep_copy);
			}
		}
	}
}

QByteArray ReportWorker::inheritance(const QByteArray& gene_info)
{
	QByteArrayList output;
	foreach(QByteArray gene, gene_info.split(','))
	{
		//extract inheritance info
		QByteArray inheritance;
		QByteArrayList parts = gene.replace('(',' ').replace(')',' ').split(' ');
		foreach(QByteArray part, parts)
		{
			if (part.startsWith("inh=")) inheritance = part.mid(4);
		}

		output << inheritance;
	}
	return output.join(",");
}

QString ReportWorker::trans(const QString& text) const
{
	if (settings_.language=="german")
	{
		return text;
	}
	else if (settings_.language=="english")
	{
		QHash<QString, QString> de2en;
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
		de2en["Varianten nach klinischer Interpretation im Kontext der Fragestellung"] = "List of prioritized variants";
		de2en["Vererbung"] = "Inheritance";
		de2en["Klasse"] = "Class";
		de2en["Details"] = "Details";
		de2en["Genotyp"] = "Genotype";
		de2en["Variante"] = "Variant";
		de2en["Gen"] = "Gene";
		de2en["F&uuml;r Informationen zur Klassifizierung von Varianten, siehe allgemeine Zusatzinformationen."] = "For further information regarding the classification see Additional Information.";
		de2en["Teilweise k&ouml;nnen bei Varianten unklarer Signifikanz (Klasse 3) -  in Abh&auml;ngigkeit von der Art der genetischen Ver&auml;nderung, der Familienanamnese und der Klinik des/der Patienten - weiterf&uuml;hrende Untersuchungen eine &Auml;nderung der Klassifizierung bewirken. Bei konkreten differentialdiagnostischen Hinweisen auf eine entsprechende Erkrankung ist eine humangenetische Mitbeurteilung erforderlich, zur Beurteilung ob erweiterte genetische Untersuchungen zielf&uuml;hrend w&auml;ren."] = "TODO";
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
		de2en["Komplett abgedeckte Gene"] = "Genes without gaps";
		de2en["Anteil Regionen mit Tiefe &lt;"] = "Percentage of regions with depth &lt;";
		de2en["Fehlende Basen in nicht komplett abgedeckten Genen"] = "Number of missing bases for genes with gaps";
		de2en["Details Regionen mit Tiefe &lt;"] = "Details regions with depth &lt;";
		de2en["Koordinaten (hg19)"] = "Coordinates (hg19)";
		de2en["Chromosom"] = "Chromosome";
		de2en["L&uuml;cken"] = "Gaps";
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

		if (!de2en.contains(text))
		{
			Log::warn("Could not translate to " + settings_.language + ": '" + text + "'");
		}

		return de2en[text];
	}

	THROW(ProgrammingException, "Unsupported language '" + settings_.language + "'!");
}

void ReportWorker::writeXML(QString outfile_name, QString report_document)
{
	QSharedPointer<QFile> outfile = Helper::openFileForWriting(outfile_name);

	QXmlStreamWriter w(outfile.data());
	w.setAutoFormatting(true);
	w.writeStartDocument();

	//element DiagnosticNgsReport
	w.writeStartElement("DiagnosticNgsReport");
	w.writeAttribute("version", "3");
	w.writeAttribute("type", settings_.report_type);

	//element ReportGeneration
	w.writeStartElement("ReportGeneration");
	w.writeAttribute("date", QDate::currentDate().toString("yyyy-MM-dd"));
	w.writeAttribute("user_name", LoginManager::user());
	w.writeAttribute("software", QCoreApplication::applicationName() + " " + QCoreApplication::applicationVersion());
	w.writeAttribute("outcome", settings_.diag_status.outcome);
	w.writeEndElement();

	//element Sample
	w.writeStartElement("Sample");
	w.writeAttribute("name", sample_name_);

	SampleData sample_data = db_.getSampleData(db_.sampleId(sample_name_));
	w.writeAttribute("name_external", sample_data.name_external);
	QString ps_id = db_.processedSampleId(sample_name_);
	ProcessedSampleData processed_sample_data = db_.getProcessedSampleData(ps_id);
	w.writeAttribute("processing_system", processed_sample_data.processing_system);
	w.writeAttribute("processing_system_type", processed_sample_data.processing_system_type);
	w.writeEndElement();

	//element TargetRegion (optional)
	if (file_roi_!="")
	{
		BedFile roi;
		roi.load(file_roi_);
		roi.merge();

		w.writeStartElement("TargetRegion");
		w.writeAttribute("name", QFileInfo(file_roi_).fileName().replace(".bed", ""));
		w.writeAttribute("regions", QString::number(roi.count()));
		w.writeAttribute("bases", QString::number(roi.baseCount()));
		QString gap_percentage = roi_stats_["gap_percentage"]; //cached from HTML report
		if (!gap_percentage.isEmpty())
		{
			w.writeAttribute("gap_percentage", gap_percentage);
		}
		QString ccds_sequenced = roi_stats_["ccds_sequenced"]; //cached from HTML report
		if (!ccds_sequenced.isEmpty())
		{
			w.writeAttribute("ccds_bases_sequenced", ccds_sequenced);
		}

		//contained genes
		foreach(const QString& gene, genes_)
		{
			w.writeStartElement("Gene");
			w.writeAttribute("name", gene);
			w.writeEndElement();
		}

		w.writeEndElement();
	}

	//element VariantList
	w.writeStartElement("VariantList");
	w.writeAttribute("overall_number", QString::number(variants_.count()));
	w.writeAttribute("genome_build", "hg19");

	//element Variant
	int geno_idx = variants_.getSampleHeader().infoByStatus(true).column_index;
	foreach(const ReportVariantConfiguration& var_conf, settings_.report_config.variantConfig())
	{
		if (var_conf.variant_type!=VariantType::SNVS_INDELS) continue;
		if (!var_conf.showInReport()) continue;
		if (var_conf.report_type!=settings_.report_type) continue;

		const Variant& variant = variants_[var_conf.variant_index];
		w.writeStartElement("Variant");
		w.writeAttribute("chr", variant.chr().str());
		w.writeAttribute("start", QString::number(variant.start()));
		w.writeAttribute("end", QString::number(variant.end()));
		w.writeAttribute("ref", variant.ref());
		w.writeAttribute("obs", variant.obs());
		w.writeAttribute("genotype", formatGenotype(processed_sample_data.gender.toLatin1(), variant.annotations()[geno_idx], variant));
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
		int i_co_sp = variants_.annotationIndexByName("coding_and_splicing", true, false);
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
				QString omim_gene_id = db_.getValue("SELECT id FROM omim_gene WHERE gene=:0", true, gene).toString();
				if (omim_gene_id!="")
				{
					//gene
					w.writeStartElement("GeneDiseaseInformation");
					w.writeAttribute("gene", gene);
					w.writeAttribute("source", "OMIM gene");
					w.writeAttribute("identifier", db_.getValue("SELECT mim FROM omim_gene WHERE id=" + omim_gene_id).toString());
					w.writeAttribute("name", gene);
					w.writeEndElement();

					//phenotypes
					QStringList phenos = db_.getValues("SELECT phenotype FROM omim_phenotype WHERE omim_gene_id=" + omim_gene_id);
					foreach(QString pheno, phenos)
					{
						w.writeStartElement("GeneDiseaseInformation");
						w.writeAttribute("gene", gene);
						w.writeAttribute("source", "OMIM phenotype");
						w.writeAttribute("identifier", "[see name]");
						w.writeAttribute("name", pheno);
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
	bool no_cnv_calling = cnvs_.caller()==CnvCallerType::INVALID;
	w.writeStartElement("CnvList");
	w.writeAttribute("cnv_caller", no_cnv_calling ? "NONE" :  cnvs_.callerAsString());
	w.writeAttribute("overall_number", QString::number(cnvs_.count()));
	w.writeAttribute("genome_build", "hg19");
	QString cnv_callset_id = db_.getValue("SELECT id FROM cnv_callset WHERE processed_sample_id=" + ps_id, true).toString();
	if (no_cnv_calling) cnv_callset_id = "-1";
	QString cnv_calling_quality = db_.getValue("SELECT quality FROM cnv_callset WHERE id=" + cnv_callset_id, true).toString();
	if (cnv_calling_quality.trimmed()=="") cnv_calling_quality="n/a";
	w.writeAttribute("quality", cnv_calling_quality);
	if(cnvs_.caller()==CnvCallerType::CLINCNV && !cnv_callset_id.isEmpty())
	{
		QHash<QString, QString> qc_metrics = db_.cnvCallsetMetrics(cnv_callset_id.toInt());
		w.writeAttribute("number_of_iterations", qc_metrics["number of iterations"]);
		w.writeAttribute("number_of_hq_cnvs", qc_metrics["high-quality cnvs"]);
	}

	foreach(const ReportVariantConfiguration& var_conf, settings_.report_config.variantConfig())
	{
		if (var_conf.variant_type!=VariantType::CNVS) continue;
		if (!var_conf.showInReport()) continue;
		if (var_conf.report_type!=settings_.report_type) continue;

		const CopyNumberVariant& cnv = cnvs_[var_conf.variant_index];

		//element Cnv
		w.writeStartElement("Cnv");
		w.writeAttribute("chr", cnv.chr().str());
		w.writeAttribute("start", QString::number(cnv.start()));
		w.writeAttribute("end", QString::number(cnv.end()));
		w.writeAttribute("start_band", NGSHelper::cytoBand(cnv.chr(), cnv.start()));
		w.writeAttribute("end_band", NGSHelper::cytoBand(cnv.chr(), cnv.end()));
		int cn = cnv.copyNumber(cnvs_.annotationHeaders());
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
		w.writeAttribute("url", "http://dgv.tcag.ca/gb2/gbrowse/dgv2_hg19/?name=" + cnv.toString());
		w.writeAttribute("type", "DGV");
		w.writeEndElement();
		w.writeStartElement("ExternalLink");
		w.writeAttribute("url", "https://genome.ucsc.edu/cgi-bin/hgTracks?db=hg19&position=" + cnv.toString());
		w.writeAttribute("type", "UCSC");
		w.writeEndElement();

		w.writeEndElement();
	}
	w.writeEndElement();

	//element ReportDocument
	w.writeStartElement("ReportDocument");
	QString format = QFileInfo(report_document).suffix().toUpper();
	w.writeAttribute("format", format);
	QByteArray base64_data = "";
	QFile file(report_document);
	file.open(QIODevice::ReadOnly);
	base64_data = file.readAll().toBase64();
	file.close();
	w.writeCharacters(base64_data);
	w.writeEndElement();

	w.writeEndDocument();
	outfile->close();

	//validate written XML file
	QString xml_error = XmlHelper::isValidXml(outfile_name, "://Resources/DiagnosticReport_v3.xsd");
	if (xml_error!="")
	{
		THROW(ProgrammingException, "ReportWorker::storeXML produced an invalid XML file: " + xml_error);
	}
}
