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
#include "VariantFilter.h"
#include "NGSHelper.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QCoreApplication>
#include <QXmlStreamWriter>
#include <QMessageBox>
#include <QDesktopServices>
#include <QApplication>



ReportWorker::ReportWorker(QString sample_name, QString file_bam, QString file_roi, const VariantList& variants, QMap<QString, QString> filters, QMap<QString, QStringList> preferred_transcripts, ReportSettings settings, QStringList log_files, QString file_rep)
	: WorkerBase("Report generation")
	, sample_name_(sample_name)
	, file_bam_(file_bam)
	, file_roi_(file_roi)
	, variants_(variants)
	, filters_(filters)
	, preferred_transcripts_(preferred_transcripts)
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
		VariantFilter filter(const_cast<VariantList&>(variants_));
		filter.flagByRegions(roi_);
		var_count_ = filter.countPassing();

		//load gene list file
		genes_ = GeneSet::createFromFile(file_roi_.left(file_roi_.size()-4) + "_genes.txt");
	}

	roi_stats_.clear();
	writeHTML();
}

QString ReportWorker::filterToGermanText(QString name, QString value)
{
	QString output;

	if (name=="classification")
	{
		output = "Keine Varianten mit Klasse &lt;" + value + " (siehe unten)";
	}
	else if (name=="maf")
	{
		output = "Keine Varianten mit einer Allelfrequenz &gt;" + value + " in &ouml;ffentlichen Datenbanken";
	}
	else if (name=="ihdb")
	{
		output = "Keine Varianten die intern h&auml;ufiger als " + value + "x mit dem selben Genotyp beobachtet wurden";
	}
	else if (name=="impact")
	{
		output = "Frameshift-, Nonsense-, Missense- und Splicingvarianten und synonyme Varianten (Impact: " + value + ")";
	}
	else if (name=="genotype")
	{
		output = "Varianten mit Genotyp: " + value;
	}
	else if (name!="keep_class_ge" && name!="keep_class_m")
	{
		output = "Filter: " + name + " Wert: " + value;
		Log::warn("Unknown filter name '" + name + "'. Using fallback format!");
	}

	return output.trimmed();
}

QString ReportWorker::formatCodingSplicing(const QList<VariantTranscript>& transcripts)
{
	QList<QByteArray> output;
	QList<QByteArray> output_pt;

	foreach(const VariantTranscript& trans, transcripts)
	{
		QByteArray line = trans.gene + ":" + trans.id + ":" + trans.hgvs_c + ":" + trans.hgvs_p;

		output.append(line);

		if (preferred_transcripts_.value(trans.gene).contains(trans.id))
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

QString ReportWorker::inheritance(QString gene_info, bool color)
{
	QStringList output;
	foreach(QString gene, gene_info.split(","))
	{
		//extract inheritance info
		QString inheritance;
		QStringList parts = gene.replace('(',' ').replace(')',' ').split(' ');
		foreach(QString part, parts)
		{
			if (part.startsWith("inh=")) inheritance = part.mid(4);
		}

		if (color && inheritance=="n/a")
		{
			inheritance = "<span style=\"background-color: #FF0000\">n/a</span>";
		}
		output << inheritance;
	}
	return output.join(",");
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
	stream << "<p><b>Abdeckungsstatistik</b>" << endl;
	stream << "<br />Durchschnittliche Sequenziertiefe: " << avg_cov << endl;

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
			stream << "<br />Komplett abgedeckte Gene: " << complete_genes.join(", ") << endl;
		}
		QString gap_perc = QString::number(100.0*low_cov.baseCount()/roi.baseCount(), 'f', 2);
		if (output!=nullptr) output->insert("gap_percentage", gap_perc);
		stream << "<br />Anteil Regionen mit Tiefe &lt;" << min_cov << ": " << gap_perc << "%" << endl;
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
			stream << "<br />Fehlende Basen in nicht komplett abgedeckten Genen: " << incomplete_genes.join(", ") << endl;
		}

		stream << "</p>" << endl;
		stream << "<p>Details Regionen mit Tiefe &lt;" << min_cov << ":" << endl;
		stream << "</p>" << endl;
		stream << "<table>" << endl;
		stream << "<tr><td><b>Gen</b></td><td><b>L&uuml;cken</b></td><td><b>Chromosom</b></td><td><b>Koordinaten (hg19)</b></td></tr>" << endl;
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
	stream << "<p><b>Abdeckungsstatistik f&uuml;r CCDS " << ext_string << "</b></p>" << endl;
	if (gap_table) stream << "<table>";
	if (gap_table) stream << "<tr><td><b>Gen</b></td><td><b>Transcript</b></td><td><b>Gr&ouml;&szlig;e</b></td><td><b>L&uuml;cken</b></td><td><b>Chromosom</b></td><td><b>Koordinaten (hg19)</b></td></tr>";
	QMap<QByteArray, int> gap_count;
	long long bases_overall = 0;
	long long bases_sequenced = 0;
	GeneSet genes_noncoding;
	foreach(const QByteArray& gene, genes)
	{
		int gene_id = db.geneToApprovedID(gene);

		//approved gene symbol
		QByteArray symbol = db.geneSymbol(gene_id);

		//longest coding transcript
		Transcript transcript = db.longestCodingTranscript(gene_id, Transcript::CCDS, true);
		if (!transcript.isValid())
		{
			genes_noncoding.insert(gene);
			transcript = db.longestCodingTranscript(gene_id, Transcript::CCDS, true, true);
		}

		//gaps
		QString message;
		BedFile roi = transcript.regions();
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
		stream << "<br>Warning: Using the longest *non-coding* transcript for the genes " << genes_noncoding.join(", ") << " since no coding transcripts for GRCh37 is defined!";
	}

	//overall statistics
	stream << "<p>CCDS " << ext_string << "gesamt: " << bases_overall << endl;
	stream << "<br />CCDS " << ext_string << "mit Tiefe &ge;" << min_cov << ": " << bases_sequenced << " (" << QString::number(100.0 * bases_sequenced / bases_overall, 'f', 2)<< "%)" << endl;
	long long gaps = bases_overall - bases_sequenced;
	stream << "<br />CCDS " << ext_string << "mit Tiefe &lt;" << min_cov << ": " << gaps << " (" << QString::number(100.0 * gaps / bases_overall, 'f', 2)<< "%)" << endl;
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
		stream << "Komplett abgedeckte Gene: " << genes_complete.join(", ") << endl;
		stream << "<br />Fehlende Basen in nicht komplett abgedeckten Genen: " << genes_incomplete.join(", ") << endl;
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
	QString processed_sample_id = db.processedSampleId(bam_file);
	ProcessingSystemData system_data = db.getProcessingSystemData(processed_sample_id, true);
	if (system_data.type=="WGS")
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
	ProcessingSystemData system_data = db.getProcessingSystemData(db.processedSampleId(bam_file), true);

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

	//get data from database
	SampleData sample_data = db_.getSampleData(db_.sampleId(sample_name_));
	QString processed_sample_id = db_.processedSampleId(sample_name_);
	ProcessedSampleData processed_sample_data = db_.getProcessedSampleData(processed_sample_id);
	ProcessingSystemData system_data = db_.getProcessingSystemData(processed_sample_id, true);

	stream << "<h4>Technischer Report zur bioinformatischen Analyse</h4>" << endl;
	stream << "<p><b>Probe: " << sample_name_ << "</b> (" << sample_data.name_external << ")" << endl;
	stream << "<br />Prozessierungssystem: " << processed_sample_data.processing_system << endl;
	stream << "<br />Referenzgenom: " << system_data.genome << endl;
	stream << "<br />Datum: " << QDate::currentDate().toString("dd.MM.yyyy") << endl;
	stream << "<br />Benutzer: " << Helper::userName() << endl;
	stream << "<br />Analysesoftware: "  << QCoreApplication::applicationName() << " " << QCoreApplication::applicationVersion() << endl;	
	stream << "<br />KASP-Ergebnis: " << db_.getQCData(processed_sample_id).value("kasp").asString() << endl;
	stream << "</p>" << endl;

	//get column indices
	int i_genotype = variants_.annotationIndexByName("genotype", true, false); //optional because of tumor samples
	int i_geneinfo = variants_.annotationIndexByName("gene_info", true, true);
	int i_gene = variants_.annotationIndexByName("gene", true, true);
	int i_co_sp = variants_.annotationIndexByName("coding_and_splicing", true, true);
	int i_omim = variants_.annotationIndexByName("OMIM", true, true);
	int i_class = variants_.annotationIndexByName("classification", true, true);
	int i_comment = variants_.annotationIndexByName("comment", true, true);
	int i_exac = variants_.annotationIndexByName("ExAC", true, true);
	int i_gnomad = variants_.annotationIndexByName("gnomAD", true, true);

	//get tumor specific column indices
	bool tumor = (i_genotype==-1);
	int i_tumor_af = std::max(variants_.annotationIndexByName("tumor_maf", true, false), variants_.annotationIndexByName("tumor_af", true, false));

	//output: applied filters
	stream << "<p><b>Filterkriterien</b>" << endl;
	stream << "<br />Gefundene Varianten in Zielregion gesamt: " << var_count_ << endl;
	stream << "<br />Anzahl Varianten nach automatischer Filterung: " << settings_.variants_selected.count() << endl;
	for(auto it = filters_.cbegin(); it!=filters_.cend(); ++it)
	{
		QString text = filterToGermanText(it.key(), it.value());
		if (text!="")
		{
			stream << "<br />&nbsp;&nbsp;&nbsp;&nbsp;- " << text << endl;
		}
	}
	stream << "</p>" << endl;

	//output: all rare variants
	stream << "<p><b>Liste priorisierter Varianten</b>" << endl;
	stream << "</p>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><td><b>Gen</b></td><td><b>Variante</b></td><td><b>" << (tumor ? "Allelfrequenz" : "Genotyp") << "</b></td><td><b>Details</b></td><td><b>Klasse</b></td><td><b>Vererbung</b></td><td><b>ExAC</b></td><td><b>gnomAD</b></td></tr>" << endl;
	for (int i=0; i<settings_.variants_selected.count(); ++i)
	{
		const Variant& variant = variants_[settings_.variants_selected[i]];
		QByteArray genes = variant.annotations()[i_gene];
		stream << "<tr>" << endl;
		stream << "<td>" << genes << "</td>" << endl;
		stream << "<td>" << endl;
		stream  << variant.chr().str() << ":" << variant.start() << "&nbsp;" << variant.ref() << "&nbsp;&gt;&nbsp;" << variant.obs() << "</td>";
		stream << "<td>" << (tumor ? variant.annotations().at(i_tumor_af) : variant.annotations().at(i_genotype)) << "</td>" << endl;
		stream << "<td>" << formatCodingSplicing(variant.transcriptAnnotations(i_co_sp)) << "</td>" << endl;
		stream << "<td>" << variant.annotations().at(i_class) << "</td>" << endl;
		stream << "<td>" << inheritance(variant.annotations()[i_geneinfo]) << "</td>" << endl;
		stream << "<td>" << variant.annotations().at(i_exac) << "</td>" << endl;
		stream << "<td>" << variant.annotations().at(i_gnomad) << "</td>" << endl;
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
			stream << "<tr><td colspan=\"8\">" << parts.join("<br />") << "</td></tr>" << endl;
		}
	}
	stream << "</table>" << endl;

	stream << "<p>F&uuml;r Informationen zur Klassifizierung von Varianten, siehe allgemeine Zusatzinformationen." << endl;
	stream << "</p>" << endl;

	stream << "<p>Teilweise k&ouml;nnen bei Varianten unklarer Signifikanz (Klasse 3) -  in Abh&auml;ngigkeit von der Art der genetischen Ver&auml;nderung, der Familienanamnese und der Klinik des/der Patienten - weiterf&uuml;hrende Untersuchungen eine &Auml;nderung der Klassifizierung bewirken. Bei konkreten differentialdiagnostischen Hinweisen auf eine entsprechende Erkrankung ist eine humangenetische Mitbeurteilung erforderlich, zur Beurteilung ob erweiterte genetische Untersuchungen zielf&uuml;hrend w&auml;ren." << endl;
	stream << "</p>" << endl;

	///classification explaination
	if (settings_.show_class_details)
	{
		stream << "<p><b>Klassifikation von Varianten:</b>" << endl;
		stream << "<br />Die Klassifikation der Varianten erfolgt in Anlehnung an die Publikation von Plon et al. (Hum Mutat 2008)" << endl;
		stream << "<br /><b>Klasse 5: Eindeutig pathogene Ver&auml;nderung / Mutation:</b> Ver&auml;nderung, die bereits in der Fachliteratur mit ausreichender Evidenz als krankheitsverursachend bezogen auf das vorliegende Krankheitsbild beschrieben wurde sowie als pathogen zu wertende Mutationstypen (i.d.R. Frameshift- bzw. Stoppmutationen)." << endl;
		stream << "<br /><b>Klasse 4: Wahrscheinlich pathogene Ver&auml;nderung:</b> DNA-Ver&auml;nderung, die aufgrund ihrer Eigenschaften als sehr wahrscheinlich krankheitsverursachend zu werten ist." << endl;
		stream << "<br /><b>Klasse 3: Variante unklarer Signifikanz (VUS) - Unklare Pathogenit&auml;t:</b> Variante, bei der es unklar ist, ob eine krankheitsverursachende Wirkung besteht. Diese Varianten werden tabellarisch im technischen Report mitgeteilt." << endl;
		stream << "<br /><b>Klasse 2: Sehr wahrscheinlich benigne Ver&auml;nderungen:</b> Aufgrund der H&auml;ufigkeit in der Allgemeinbev&ouml;lkerung oder der Lokalisation bzw. aufgrund von Angaben in der Literatur sehr wahrscheinlich benigne. Werden nicht mitgeteilt, k&ouml;nnen aber erfragt werden." << endl;
		stream << "<br /><b>Klasse 1: Benigne Ver&auml;nderungen:</b> Werden nicht mitgeteilt, k&ouml;nnen aber erfragt werden." << endl;
		stream << "<p>F&uuml;r Informationen zur Klassifizierung von Varianten, siehe alllgemeine Zusatzinformationen." << endl;
		stream << "</p>" << endl;
	}

	///Target region statistics
	if (file_roi_!="")
	{
		stream << "<p><b>Zielregion</b>" << endl;
		stream << "<br /><span style=\"font-size: 80%;\">Die Zielregion umfasst mindestens die CCDS (\"consensus coding sequence\") unten genannter Gene &plusmn;20 Basen flankierender intronischer Sequenz, kann aber auch zus&auml;tzliche Exons und/oder flankierende Basen beinhalten." << endl;
		stream << "<br />Name: " << QFileInfo(file_roi_).fileName().replace(".bed", "") << endl;
		if (!genes_.isEmpty())
		{
			stream << "<br />Ausgewertete Gene (" << QString::number(genes_.count()) << "): " << genes_.join(", ") << endl;
		}
		stream << "</p>" << endl;
	}

	///low-coverage analysis
	if (settings_.show_coverage_details && file_bam_!="")
	{
		writeCoverageReport(stream, file_bam_, file_roi_, roi_, genes_, settings_.min_depth, db_, settings_.recalculate_avg_depth, &roi_stats_, settings_.roi_low_cov);

		writeCoverageReportCCDS(stream, file_bam_, genes_, settings_.min_depth, 0, db_, &roi_stats_, false, false);

		writeCoverageReportCCDS(stream, file_bam_, genes_, settings_.min_depth, 5, db_, nullptr, true, true);
	}

	//collect and display important tool versions
	if (settings_.show_tool_details)
	{
		stream << "<p><b>Details zu Analysetools</b>" << endl;
		stream << "</p>" << endl;
		stream << "<table>" << endl;
		stream << "<tr><td><b>tool</b></td><td><b>version</b></td><td><b>parameters</b></td></tr>";
		QStringList whitelist;
		whitelist << "SeqPurge" << "samblaster" << "/bwa" << "samtools" << "VcfLeftNormalize" <<  "freebayes" << "abra2" << "vcflib" << "SnpEff"; //current
		whitelist << "varFilter" << "stampy.py" << "BamLeftAlign" << "GenomeAnalysisTK" << "VcfSplitMultiallelic" << "picard-tools"; //legacy
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


	validateAndCopyReport(temp_filename, file_rep_,true,true);

	//write XML file to transfer folder
	QString gsvar_variant_transfer = Settings::string("gsvar_variant_transfer");
	if (gsvar_variant_transfer!="")
	{
		writeXML(gsvar_variant_transfer + "/" + QFileInfo(file_rep_).fileName().replace(".html", ".xml"));
	}
}

void ReportWorker::validateAndCopyReport(QString from, QString to,bool put_to_archive,bool is_rtf)
{
	//validate written HTML file
	QString validation_error = XmlHelper::isValidXml(from);
	if (validation_error!="" && !is_rtf)
	{
		Log::warn("Generated report at " + from + " is not well-formed: " + validation_error);
	}

	if (QFile::exists(to) && !QFile(to).remove())
	{
		if(is_rtf)
		{
			THROW(FileAccessException,"Could not remove previous RTF report: " + to);
		}
		else
		{
			THROW(FileAccessException, "Could not remove previous HTML report: " + to);
		}
	}
	if (!QFile::rename(from, to))
	{
		if(is_rtf)
		{
			THROW(FileAccessException,"Could not copy RTF report from temporary file " + from + " to " + to + " !");
		}
		else
		{
			THROW(FileAccessException, "Could not copy HTML report from temporary file " + from + " to " + to + " !");
		}
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
				if(is_rtf)
				{
					THROW(FileAccessException, "Could not remove previous RTF report in archive folder: " + file_rep_copy);
				}
				else
				{
					THROW(FileAccessException, "Could not remove previous HTML report in archive folder: " + file_rep_copy);
				}
			}
			if (!QFile::copy(to, file_rep_copy))
			{
				if(is_rtf)
				{
					THROW(FileAccessException, "Could not copy RTF report to archive folder: " + file_rep_copy);
				}
				else
				{
					THROW(FileAccessException, "Could not copy HTML report to archive folder: " + file_rep_copy);

				}
			}
		}
	}
}

void ReportWorker::writeXML(QString outfile_name)
{
	QSharedPointer<QFile> outfile = Helper::openFileForWriting(outfile_name);

	QXmlStreamWriter w(outfile.data());
	w.setAutoFormatting(true);
	w.writeStartDocument();

	//element DiagnosticNgsReport
	w.writeStartElement("DiagnosticNgsReport");
	w.writeAttribute("version", "1");

	//element ReportGeneration
	w.writeStartElement("ReportGeneration");
	w.writeAttribute("date", QDate::currentDate().toString("yyyy-MM-dd"));
	w.writeAttribute("user_name", Helper::userName());
	w.writeAttribute("software", QCoreApplication::applicationName() + " " + QCoreApplication::applicationVersion());
	w.writeAttribute("outcome", settings_.diag_status.outcome);
	w.writeEndElement();

	//element Sample
	w.writeStartElement("Sample");
	w.writeAttribute("name", sample_name_);

	SampleData sample_data = db_.getSampleData(db_.sampleId(sample_name_));
	w.writeAttribute("name_external", sample_data.name_external);
	ProcessedSampleData processed_sample_data = db_.getProcessedSampleData(db_.processedSampleId(sample_name_));
	w.writeAttribute("processing_system", processed_sample_data.processing_system);
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

	//element AppliedFilter
	for(auto it = filters_.cbegin(); it!=filters_.cend(); ++it)
	{
		w.writeStartElement("AppliedFilter");
		w.writeAttribute("name", it.key() + ":" + it.value());
		w.writeEndElement();
	}

	//element Variant
	int geno_idx = variants_.annotationIndexByName("genotype", true, false);
	int comment_idx = variants_.annotationIndexByName("comment", true, true);
	int geneinfo_idx = variants_.annotationIndexByName("gene_info", true, false);
	for (int i=0; i<settings_.variants_selected.count(); ++i)
	{
		const Variant& v = variants_[settings_.variants_selected[i]];
		w.writeStartElement("Variant");
		w.writeAttribute("chr", v.chr().str());
		w.writeAttribute("start", QString::number(v.start()));
		w.writeAttribute("end", QString::number(v.end()));
		w.writeAttribute("ref", v.ref());
		w.writeAttribute("obs", v.obs());
		w.writeAttribute("genotype", (geno_idx==-1 ? "n/a" : v.annotations()[geno_idx]));
		w.writeAttribute("comment", v.annotations()[comment_idx]);

		//element Annotation
		for (int i=0; i<v.annotations().count(); ++i)
		{
			if (i==geno_idx) continue;
			if (i==comment_idx) continue;
			if (i==geneinfo_idx) continue;

			w.writeStartElement("Annotation");
			QString name = variants_.annotations()[i].name();
			w.writeAttribute("name", name);
			QByteArray value = v.annotations()[i];
			if (name=="gene") value += " (" + inheritance(v.annotations()[geneinfo_idx], false) +")";
			w.writeAttribute("value", value);
			w.writeEndElement();
		}

		//element TranscriptInformation
		int i_co_sp = variants_.annotationIndexByName("coding_and_splicing", true, false);
		if (i_co_sp!=-1)
		{
			foreach(const VariantTranscript& trans, v.transcriptAnnotations(i_co_sp))
			{
				w.writeStartElement("TranscriptInformation");
				w.writeAttribute("gene", trans.gene);
				w.writeAttribute("transcript_id", trans.id);
				w.writeAttribute("hgvs_c", trans.hgvs_c);
				w.writeAttribute("hgvs_p", trans.hgvs_p);
				w.writeEndElement();
			}
		}

		//end of variant
		w.writeEndElement();
	}
	w.writeEndDocument();
	outfile->close();

	//validate written XML file
	QString xml_error = XmlHelper::isValidXml(outfile_name, "://Resources/DiagnosticReport_v1.xsd");
	if (xml_error!="")
	{
		THROW(ProgrammingException, "ReportWorker::storeXML produced an invalid XML file: " + xml_error);
	}
}
