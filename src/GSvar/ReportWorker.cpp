#include "ReportWorker.h"
#include "Log.h"
#include "Helper.h"
#include "Exceptions.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include "Statistics.h"
#include "Settings.h"
#include "BedFile.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include <QCoreApplication>
#include <QXmlStreamWriter>
#include "XmlHelper.h"
#include "VariantFilter.h"

ReportWorker::ReportWorker(QString sample_name, QMap<QString, QString> filters, const VariantList& variants, const QList<int>& variants_selected, QMap<QString, QString> preferred_transcripts, QString outcome, QString file_roi, QString file_bam, int min_cov, QStringList log_files, QString file_rep)
	: WorkerBase("Report generation")
	, sample_name_(sample_name)
	, filters_(filters)
	, variants_(variants)
	, variants_selected_(variants_selected)
	, preferred_transcripts_(preferred_transcripts)
	, outcome_(outcome)
	, file_roi_(file_roi)
	, file_bam_(file_bam)
	, min_cov_(min_cov)
	, genes_()
	, log_files_(log_files)
	, file_rep_(file_rep)
	, roi_()
	, var_count_(variants_.count())
	, db_()
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
		QString filename = file_roi_.mid(0, file_roi_.length()-4) + "_genes.txt";
		if (QFile::exists(filename))
		{
			genes_ = Helper::loadTextFile(filename, true, '#', true);
			for(int i=0; i<genes_.count(); ++i)
			{
				genes_[i] = genes_[i].toUpper();
			}
			genes_.sort();
		}

		//remove duplicates from gene list
		int dups = genes_.removeDuplicates();
		if (dups!=0)
		{
			Log::warn("Gene list contains '" + filename + "' contains " + QString::number(dups) + " duplicates!");
		}
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

QString ReportWorker::formatCodingSplicing(QByteArray text)
{
	QList<QByteArray> transcripts = text.split(',');
	for (int i=0; i<transcripts.count(); ++i)
	{
		QList<QByteArray> parts = transcripts[i].split(':');
		if (parts.count()<7) THROW(ProgrammingException, "Could not split 'coding_and_splicing' transcript information to 7 parts: " + transcripts[i]);

		QByteArray gene = parts[0].trimmed();
		QByteArray trans = parts[1].trimmed();
		QByteArray output = gene + ":" + trans + ":" + parts[5].trimmed() + ":" + parts[6].trimmed();

		//return only preferred transcript if we know it
		QString pt = preferred_transcripts_.value(gene, "");
		if (pt!="" && QString(trans).startsWith(pt)) return output;

		transcripts[i] = output;
	}
	return transcripts.join(", ");
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

BedFile ReportWorker::writeCoverageReport(QTextStream& stream, QString bam_file, QString roi_file, const BedFile& roi, QStringList genes, int min_cov,  NGSD& db, QMap<QString, QString>* output)
{
	//get target region coverages (from NGSD or calculate)
	QString avg_cov = "";
	QCCollection stats;
	if (isProcessingSystemTargetFile(bam_file, roi_file, db))
	{
		try
		{
			QCCollection tmp = db.getQCData(bam_file);
			stats = tmp;
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
		QStringList genes = db.genesOverlapping(line.chr(), line.start(), line.end(), 20); //extend by 20 to annotate splicing regions as well
		line.annotations().append(genes.join(", "));
	}

	//group by gene name
	QHash<QString, BedFile> grouped;
	for (int i=0; i<low_cov.count(); ++i)
	{
		QStringList genes = low_cov[i].annotations()[0].split(",");
		foreach(QString gene, genes)
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
		foreach(const QString& gene, genes)
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
		foreach(const QString& gene, genes)
		{
			if (grouped.contains(gene))
			{
				incomplete_genes << gene + " <span style=\"font-size: 80%;\">" + QString::number(grouped[gene].baseCount()) + "</span> ";
			}
		}
		stream << "<br />Fehlende Basen in nicht komplett abgedeckten Genen: " << incomplete_genes.join(", ") << endl;
	}
	stream << "</p>" << endl;
	stream << "<p><span style=\"background-color: #FF0000\">Details Regionen mit Tiefe &lt;" << min_cov << ":</span>" << endl;
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

	return low_cov;
}

void ReportWorker::writeCoverageReportCCDS(QTextStream& stream, QString bam_file, QStringList genes, int min_cov, NGSD& db, QMap<QString, QString>* output)
{
	stream << "<p><b>Abdeckungsstatistik f&uuml;r CCDS</b></p>" << endl;
	stream << "<table>";
	stream << "<tr><td><b>Gen</b></td><td><b>Transcript</b></td><td><b>Gr&ouml;&szlig;e</b></td><td><b>L&uuml;cken</b></td><td><b>Chromosom</b></td><td><b>Koordinaten (hg19)</b></td></tr>";
	long long bases_overall = 0;
	long long bases_sequenced = 0;
	foreach(const QString& gene, genes)
	{
		int gene_id = db.geneToApprovedID(gene);

		//approved gene symbol
		QString symbol = db.geneSymbol(gene_id);

		//longest coding transcript
		Transcript transcript = db.longestCodingTranscript(gene_id, Transcript::CCDS);
		if (!transcript.isValid()) //fallback to REFSEQ when no CCDS transcript is defined for the gene
		{
			transcript = db.longestCodingTranscript(gene_id, Transcript::REFSEQ);
		}
		if (!transcript.isValid())
		{
			Log::warn("Low-coverage statistics for gene " + symbol + " cannot be calculated: No coding transcript found in CCDS/RefSeq!");
			continue;
		}

		//gaps
		QString message;
		BedFile gaps = precalculatedGaps(bam_file, transcript.regions(), min_cov, db, message);
		if (!message.isEmpty())
		{
			Log::warn("Low-coverage statistics for transcript " + transcript.name() + " needs to be calculated. Pre-calulated gap file cannot be used because: " + message);
			gaps = Statistics::lowCoverage(transcript.regions(), bam_file, min_cov);
		}

		long long bases_transcipt = transcript.regions().baseCount();
		long long bases_gaps = gaps.baseCount();
		QStringList coords;
		for (int i=0; i<gaps.count(); ++i)
		{
			coords << QString::number(gaps[i].start()) + "-" + QString::number(gaps[i].end());
		}
		stream << "<tr><td>" + symbol + "</td><td>" << transcript.name() << "</td><td>" << bases_transcipt << "</td><td>" << bases_gaps << "</td><td>" << transcript.regions()[0].chr().strNormalized(true) << "</td><td>" << coords.join(", ") << "</td></tr>";
		bases_overall += bases_transcipt;
		bases_sequenced += bases_transcipt - bases_gaps;
	}
	stream << "</table>";
	stream << "<p>CCDS-Basen gesamt: " << bases_overall << endl;
	stream << "<br />CCDS-Basen sequenziert: " << bases_sequenced << " (" << QString::number(100.0 * bases_sequenced / bases_overall, 'f', 2)<< "%)" << endl;
	stream << "</p>" << endl;

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
	QString sys_type = db.getProcessingSystem(bam_file, NGSD::TYPE);
	if (sys_type=="WGS")
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
	QString sys_file = db.getProcessingSystem(bam_file, NGSD::FILE);
	if (sys_file=="")
	{
		message = "Processing system target file not defined in NGSD!";
		return BedFile();
	}
	BedFile sys;
	sys.load(sys_file);
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
	QString sys_file = db.getProcessingSystem(bam_file, NGSD::FILE);

	return Helper::canonicalPath(sys_file) == Helper::canonicalPath(roi_file);
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

	stream << "<h4>Technischer Report zur bioinformatischen Analyse</h4>" << endl;
	stream << "<p><b>Probe: " << sample_name_ << "</b> (" << db_.getExternalSampleName(sample_name_) << ")" << endl;
	stream << "<br />Prozessierungssystem: " << db_.getProcessingSystem(sample_name_, NGSD::LONG) << endl;
	stream << "<br />Genom-Build: " << db_.getGenomeBuild(sample_name_) << endl;
	stream << "<br />Datum: " << QDate::currentDate().toString("dd.MM.yyyy") << endl;
	stream << "<br />User: " << Helper::userName() << endl;
	stream << "<br />Analysesoftware: "  << QCoreApplication::applicationName() << " " << QCoreApplication::applicationVersion() << endl;
	stream << "</p>" << endl;

	//get column indices
	int i_genotype = variants_.annotationIndexByName("genotype", true, false); //optional because of tumor samples
	int i_geneinfo = variants_.annotationIndexByName("gene_info", true, true);
	int i_gene = variants_.annotationIndexByName("gene", true, true);
	int i_co_sp = variants_.annotationIndexByName("coding_and_splicing", true, true);
	int i_omim = variants_.annotationIndexByName("OMIM", true, true);
	int i_class = variants_.annotationIndexByName("classification", true, true);
	int i_comment = variants_.annotationIndexByName("comment", true, true);

	//get tumor specific column indices
	bool tumor = (i_genotype==-1);
	int i_tumor_af = std::max(variants_.annotationIndexByName("tumor_maf", true, false), variants_.annotationIndexByName("tumor_af", true, false));

	//output: applied filters
	stream << "<p><b>Filterkriterien</b>" << endl;
	stream << "<br />Gefundene Varianten in Zielregion gesamt: " << var_count_ << endl;
	stream << "<br />Anzahl relevanter Varianten nach Filterung: " << variants_selected_.count() << endl;
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
	stream << "<p><b>Liste gefilterter Varianten</b>" << endl;
	stream << "</p>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><td><b>Gen</b></td><td><b>chr</b></td><td><b>start</b></td><td><b>end</b></td><td><b>ref</b></td><td><b>obs</b></td><td><b>" << (tumor ? "Allelfrequenz" : "Genotyp") << "</b></td><td><b>Details</b></td><td><b>Klasse</b></td><td><b>Vererbung</b></td></tr>" << endl;
	for (int i=0; i<variants_selected_.count(); ++i)
	{
		const Variant& variant = variants_[variants_selected_[i]];
		QByteArray genes = variant.annotations()[i_gene];
		stream << "<tr>" << endl;
		stream << "<td>" << genes << "</td>" << endl;
		stream << "<td>" << endl;
		stream  << variant.chr().str() << "</td><td>" << variant.start() << "</td><td>" << variant.end() << "</td><td>" << variant.ref() << "</td><td>" << variant.obs() << "</td>";
		stream << "<td>" << (tumor ? variant.annotations().at(i_tumor_af) : variant.annotations().at(i_genotype)) << "</td>" << endl;
		stream << "<td>" << formatCodingSplicing(variant.annotations().at(i_co_sp)).replace(", ", "<br />") << "</td>" << endl;
		stream << "<td>" << variant.annotations().at(i_class) << "</td>" << endl;
		stream << "<td>" << inheritance(variant.annotations()[i_geneinfo]) << "</td>" << endl;
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
			stream << "<tr><td colspan=\"10\">" << parts.join("<br />") << "</td></tr>" << endl;
		}
	}
	stream << "</table>" << endl;

	stream << "<p>Teilweise k&ouml;nnen bei Varianten unklarer Signifikanz (Klasse 3) -  in Abh&auml;ngigkeit von der Art der genetischen Ver&auml;nderung, der Familienanamnese und der Klinik des/der Patienten - weiterf&uuml;hrende Untersuchungen eine &Auml;nderung der Klassifizierung bewirken. Bei konkreten differentialdiagnostischen Hinweisen auf eine entsprechende Erkrankung ist eine humangenetische Mitbeurteilung erforderlich, zur Beurteilung ob erweiterte genetische Untersuchungen zielf&uuml;hrend w&auml;ren." << endl;
	stream << "</p>" << endl;

	///classification explaination
	stream << "<p><b>Klassifikation von Varianten:</b>" << endl;
	stream << "<br />Die Klassifikation der Varianten erfolgt in Anlehnung an die Publikation von Plon et al. (Hum Mutat 2008)" << endl;
	stream << "<br /><b>Klasse 5: Eindeutig pathogene Ver&auml;nderung / Mutation:</b> Ver&auml;nderung, die bereits in der Fachliteratur mit ausreichender Evidenz als krankheitsverursachend bezogen auf das vorliegende Krankheitsbild beschrieben wurde sowie als pathogen zu wertende Mutationstypen (i.d.R. Frameshift- bzw. Stoppmutationen)." << endl;
	stream << "<br /><b>Klasse 4: Wahrscheinlich pathogene Ver&auml;nderung:</b> DNA-Ver&auml;nderung, die aufgrund ihrer Eigenschaften als sehr wahrscheinlich krankheitsverursachend zu werten ist." << endl;
	stream << "<br /><b>Klasse 3: Variante unklarer Signifikanz (VUS) - Unklare Pathogenit&auml;t:</b> Variante, bei der es unklar ist, ob eine krankheitsverursachende Wirkung besteht. Diese Varianten werden tabellarisch im technischen Report mitgeteilt." << endl;
	stream << "<br /><b>Klasse 2: Sehr wahrscheinlich benigne Ver&auml;nderungen:</b> Aufgrund der H&auml;ufigkeit in der Allgemeinbev&ouml;lkerung oder der Lokalisation bzw. aufgrund von Angaben in der Literatur sehr wahrscheinlich benigne. Werden nicht mitgeteilt, k&ouml;nnen aber erfragt werden." << endl;
	stream << "<br /><b>Klasse 1: Benigne Ver&auml;nderungen:</b> Werden nicht mitgeteilt, k&ouml;nnen aber erfragt werden." << endl;
	stream << "</p>" << endl;

	///CNVs
	stream << "<p><b>Gefunden Copy-Number-Varianten</b>" << endl;
	stream << "</p>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><td><b>Koordianten</b></td><td><b>Exons</b></td><td><b>CopyNumbers</b></td><td><b>Details</b></td></tr>" << endl;
	stream << "<tr>" << endl;
	stream << "<td colspan=\"4\"><span style=\"background-color: #FF0000\">Abschnitt mit Daten aus dem Report fuellen oder loeschen!<br />Im Moment nur f&uuml;r X-Diagnostik relevant!</span></td>" << endl;
	stream << "</tr>" << endl;
	stream << "</table>" << endl;

	///Target region statistics
	if (file_roi_!="")
	{
		stream << "<p><b>Zielregion</b>" << endl;
		stream << "<br />Name: " << QFileInfo(file_roi_).fileName().replace(".bed", "") << endl;
		stream << "<br />Regionen: " << roi_.count() << endl;
		stream << "<br />Basen: " << roi_.baseCount() << endl;
		if (!genes_.isEmpty())
		{
			stream << "<br />Angereicherte Gene (" << QString::number(genes_.count()) << "): " << genes_.join(", ") << endl;
		}
		stream << "</p>" << endl;
	}

	///low-coverage analysis
	if (file_bam_!="")
	{
		BedFile low_cov = writeCoverageReport(stream, file_bam_, file_roi_, roi_, genes_, min_cov_, db_, &roi_stats_);

		writeCoverageReportCCDS(stream, file_bam_, genes_, min_cov_, db_, &roi_stats_);

		//additionally store low-coverage BED file
		low_cov.store(QString(file_rep_).replace(".html", "_lowcov.bed"));
	}

	//collect and display important tool versions
	stream << "<p><b><span style=\"background-color: #FF0000\">Details zu Analysetools (f&uuml;r interne Zwecke)</span></b>" << endl;
	stream << "</p>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><td><b>tool</b></td><td><b>version</b></td><td><b>parameters</b></td></tr>";
	QStringList whitelist;
	whitelist << "SeqPurge" << "samblaster" << "/bwa" << "samtools" << "VcfLeftAlign" <<  "freebayes" << "abra.jar" << "vcflib" << "SnpEff"; //current
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

	//close stream
	writeHtmlFooter(stream);
	outfile->close();

	//validate written HTML file
	QString validation_error = XmlHelper::isValidXml(temp_filename);
	if (validation_error!="")
	{
		Log::warn("Generated report is not well-formed: " + validation_error);
	}

	//copy temp file to output folder
	if (QFile::exists(file_rep_) && !QFile(file_rep_).remove())
	{
		THROW(FileAccessException, "Could not remove previous HTML report: " + file_rep_);
	}
	if (!QFile::rename(temp_filename, file_rep_))
	{
		THROW(FileAccessException, "Could not copy HTML report from temporary file " + temp_filename + " to " + file_rep_ + " !");
	}

	//copy report to archive folder
	QString archive_folder = Settings::string("gsvar_report_archive");
	if (archive_folder!="")
	{
		QString file_rep_copy = archive_folder + "\\" + QFileInfo(file_rep_).fileName();
		if (QFile::exists(file_rep_copy) && !QFile::remove(file_rep_copy))
		{
			THROW(FileAccessException, "Could not remove previous HTML report in archive folder: " + file_rep_copy);
		}
		if (!QFile::copy(file_rep_, file_rep_copy))
		{
			THROW(FileAccessException, "Could not copy HTML report to archive folder: " + file_rep_copy);
		}
	}

	//write XML file to transfer folder
	QString gsvar_variant_transfer = Settings::string("gsvar_variant_transfer");
	if (gsvar_variant_transfer!="")
	{
		writeXML(gsvar_variant_transfer + "/" + QFileInfo(file_rep_).fileName().replace(".html", ".xml"));
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
	w.writeAttribute("outcome", outcome_);
	w.writeEndElement();

	//element Sample
	w.writeStartElement("Sample");
	w.writeAttribute("name", sample_name_);
	w.writeAttribute("name_external", db_.getExternalSampleName(sample_name_));
	w.writeAttribute("processing_system", db_.getProcessingSystem(sample_name_, NGSD::LONG));
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
	for (int i=0; i<variants_selected_.count(); ++i)
	{
		const Variant& v = variants_[variants_selected_[i]];
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
