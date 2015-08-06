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
#include "NGSD.h"

ReportWorker::ReportWorker(QString sample_name, QMap<QString, QString> filters, const VariantList& variants, const QVector< QPair<int, bool> >& variants_selected, QMap<QString, QString> preferred_transcripts, QString outcome, QString file_roi, QString file_bam, bool var_details, QStringList log_files, QString file_rep)
	: WorkerBase("Report generation")
	, sample_name_(sample_name)
	, filters_(filters)
	, variants_(variants)
	, variants_selected_(variants_selected)
	, preferred_transcripts_(preferred_transcripts)
	, outcome_(outcome)
	, file_roi_(file_roi)
	, file_bam_(file_bam)
	, var_details_(var_details)
	, genes_()
	, log_files_(log_files)
	, file_rep_(file_rep)
	, roi_()
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

		VariantList tmp = variants_;
		tmp.filterByRegions(roi_);
		var_count_ = tmp.count();

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

		//remove duplicates
		int dups = genes_.removeDuplicates();
		if (dups!=0)
		{
			Log::warn("Gene list contains '" + filename + "' contains " + QString::number(dups) + " duplicates!");
		}
	}

	writeHTML();
	//disabled until it is needed
	//writeXML();
}

QString ReportWorker::filterToGermanText(QString name, QString value)
{
	QString output;

	if (name=="classification")
	{
		output = "Keine Varianten mit Klasse <" + value + " (siehe unten)";
	}
	else if (name=="maf")
	{
		output = "Keine Varianten mit einer Allelfrequenz >" + value + " in &ouml;ffentlichen Datenbanken";
	}
	else if (name=="ihdb")
	{
		output = "Keine Varianten die intern h&auml;ufiger als " + value + "x mit dem selben Genotyp beobachtet wurden";
	}
	else if (name=="keep_important")
	{
		output = "Varianten, die in &ouml;ffentlichen Datenbanken als pathogen klassifiziert wurden";
	}
	else if (name=="impact")
	{
		output = "Frameshift-, Nonsense-, Missense- und Splicingvarianten und synonyme Varianten (Impact: " + value + ")";
	}
	else if (name=="genotype")
	{
		output = "Varianten mit Genotyp: " + value;
	}
	else if (name=="quality")
	{
		output = "Varianten mit hoher Qualit&auml;t";
	}
	else if (name=="trio")
	{
		output = "Spezieller Filter basierend auf Trio-Sequenzierung";
	}
	else
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

void ReportWorker::writeHTML()
{
	QString temp_filename = Helper::tempFileName(".html");
	QScopedPointer<QFile> outfile(Helper::openFileForWriting(temp_filename));
	QTextStream stream(outfile.data());
	stream << "<html>" << endl;
	stream << "	<head>" << endl;
	stream << "	   <meta charset=\"utf-8\">" << endl;
	stream << "	   <style>" << endl;
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
	stream << "	font-size: 70%;" << endl;
	stream << "	text-align: left;" << endl;
	stream << "}" << endl;
	stream << "		-->" << endl;
	stream << "	   </style>" << endl;
	stream << "	</head>" << endl;

	stream << "	<body>" << endl;
	stream << "<h4>Technischer Report zur bioinformatischen Analyse</h4>" << endl;
	stream << "<p><b>Probe: " << sample_name_ << "</b> (" << NGSD().getExternalSampleName(sample_name_) << ")" << endl;
	stream << "<br>Prozessierungssystem: " << NGSD().getProcessingSystem(sample_name_, NGSD::LONG) << endl;
	stream << "<br>Genom-Build: " << NGSD().getGenomeBuild(sample_name_) << endl;
	stream << "<br>Datum: " << QDate::currentDate().toString("dd.MM.yyyy") << endl;
	stream << "<br>User: " << Helper::userName() << endl;
	stream << "<br>Analysesoftware: "  << QCoreApplication::applicationName() << " " << QCoreApplication::applicationVersion() << endl;
	if (file_roi_!="")
	{
		stream << "<p><b>Zielregion</b>" << endl;
		stream << "<br>Name: " << QFileInfo(file_roi_).baseName() << endl;
		stream << "<br>Regionen: " << roi_.count() << endl;
		stream << "<br>Basen: " << roi_.baseCount() << endl;
		if (!genes_.isEmpty())
		{
			stream << "<br>Angereicherte Gene (" << QString::number(genes_.count()) << "): " << genes_.join(", ") << endl;
		}
	}

	//get column indices
	int i_genotype = variants_.annotationIndexByName("genotype", true, true);
	int i_gene = variants_.annotationIndexByName("gene", true, true);
	int i_type = variants_.annotationIndexByName("variant_type", true, true);
	int i_co_sp = variants_.annotationIndexByName("coding_and_splicing", true, true);
	int i_dbsnp = variants_.annotationIndexByName("dbSNP", true, true);
	int i_1000g = variants_.annotationIndexByName("1000g", true, true);
	int i_exac = variants_.annotationIndexByName("ExAC", true, true);
	int i_esp = variants_.annotationIndexByName("ESP6500EA", true, true);
	int i_omim = variants_.annotationIndexByName("OMIM", true, true);
	int i_clinvar = variants_.annotationIndexByName("ClinVar", true, true);
	int i_hgmd = variants_.annotationIndexByName("HGMD", true, true);
	int i_class = variants_.annotationIndexByName("classification", true, true);
	int i_comment = variants_.annotationIndexByName("comment", true, true);

	//output: applied filters
	stream << "<p><b>Filterkriterien</b>" << endl;
	stream << "<br>Gefundene Varianten in Zielregion gesamt: " << var_count_ << endl;
	stream << "<br>Anzahl relevanter Varianten nach Filterung: " << variants_selected_.count() << endl;
	for(auto it = filters_.cbegin(); it!=filters_.cend(); ++it)
	{
		stream << "<br>&nbsp;&nbsp;&nbsp;&nbsp;- " << filterToGermanText(it.key(), it.value()) << endl;
	}

	//output: very important variants
	int important_variants = 0;
	for (int i=0; i<variants_selected_.count(); ++i)
	{
		important_variants += variants_selected_[i].second;
	}
	if (important_variants>0)
	{
		stream << "<p><b>Liste wichtiger Varianten</b>" << endl;
		for (int i=0; i<variants_selected_.count(); ++i)
		{
			if (!variants_selected_[i].second) continue;
			const Variant& v = variants_[variants_selected_[i].first];
			stream << "<table>" << endl;
			stream << "<tr><td><b>Variante:</b> " << v.chr().str() << ":" << v.start() << "-" << v.end() << " " << v.ref() << ">" << v.obs() << " <b>Gen:</b> " << v.annotations()[i_gene] << " <b>Typ:</b> " << v.annotations()[i_type] << " <b>VUS: </b>" << v.annotations()[i_class] << " <b>Vererbung:</b> ausfuellen </td></tr>" << endl;
			stream << "<tr><td><b>Details:</b> " << formatCodingSplicing(v.annotations()[i_co_sp]) << "</td></tr>" << endl;
			QString dbsnp = v.annotations()[i_dbsnp]; //because string is const
			stream << "<tr><td><b>Frequenz:</b> <b>1000g:</b> " << v.annotations()[i_1000g] << " <b>ExAC:</b> " << v.annotations()[i_exac] << " <b>ESP6500:</b> " << v.annotations()[i_esp] << " <b>dbSNP:</b> " << dbsnp.replace("[];", "") << "</td></tr>" << endl;
			stream << "<tr><td><b>OMIM:</b> " << v.annotations()[i_omim] << "</td></tr>" << endl;
			stream << "<tr><td><b>ClinVar:</b> " << v.annotations()[i_clinvar] << "</td></tr>" << endl;
			stream << "<tr><td><b>HGMD:</b> " << v.annotations()[i_hgmd] << "</td></tr>" << endl;
			stream << "<tr><td><b>Kommentar:</b> " << v.annotations()[i_comment] << "</td></tr>" << endl;
			stream << "</table>" << endl;
			stream << "<br>" << endl;
		}
	}

	//output: all rare variants
	stream << "<p><b>Liste relevanter Varianten nach Filterung</b>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><th>Gen</th><th>chr</th><th>start</th><th>end</th><th>ref</th><th>obs</th><th>Genotyp</th><th>Details</th><th>Klasse</th><th>Vererbung</th></tr>" << endl;
	for (int i=0; i<variants_selected_.count(); ++i)
	{
		const Variant& variant = variants_[variants_selected_[i].first];
		stream << "<tr>" << endl;
		stream << "<td>" << variant.annotations().at(i_gene) << "</td>" << endl;
		stream << "<td>" << endl;
		stream  << variant.chr().str() << "</td><td>" << variant.start() << "</td><td>" << variant.end() << "</td><td>" << variant.ref() << "</td><td>" << variant.obs() << "</td>";
		stream << "<td>" << variant.annotations().at(i_genotype) << "</td>" << endl;
		stream << "<td>" << formatCodingSplicing(variant.annotations().at(i_co_sp)).replace(", ", "<BR>") << "</td>" << endl;
		stream << "<td>" << variant.annotations().at(i_class) << "</td>" << endl;
		stream << "<td><font style=\"background-color: #FF0000\">&nbsp;&nbsp;&nbsp;</font></td>" << endl;
		stream << "</tr>" << endl;

		//OMIM and comment line
		QString omim = variant.annotations()[i_omim];
		QString comment = variant.annotations()[i_comment];
		if (comment!="" || omim!="")
		{
			QStringList parts;
			if (comment!="") parts << "<font style=\"background-color: #FF0000\">NGSD: " + comment + "</font>";
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
			stream << "<tr><td colspan=10>" << parts.join("<br>") << "</td></tr>" << endl;
		}
	}
	stream << "</table>" << endl;

	///classification explaination
	stream << "<p><b>Klassifikation von Varianten:</b>" << endl;
	stream << "<br>Die Klassifikation der Varianten erfolgt in Anlehnung an die Publikation von Plon et al. (Hum Mutat 2008)" << endl;
	stream << "<br><b>Klasse 5: Eindeutig pathogene Veränderung / Mutation:</b> Veränderung, die bereits in der Fachliteratur mit ausreichender Evidenz als krankheitsverursachend bezogen auf das vorliegende Krankheitsbild beschrieben wurde sowie als pathogen zu wertende Mutationstypen (i.d.R. Frameshift- bzw. Stoppmutationen)." << endl;
	stream << "<br><b>Klasse 4: Wahrscheinlich pathogene Veränderung:</b> DNA-Veränderung, die aufgrund ihrer Eigenschaften als sehr wahrscheinlich krankheitsverursachend zu werten ist." << endl;
	stream << "<br><b>Klasse 3: Variante unklarer Signifikanz (VUS) - Unklare Pathogenität:</b> Variante, bei der es unklar ist, ob eine krankheitsverursachende Wirkung besteht. Diese Varianten werden tabellarisch im technischen Report mitgeteilt." << endl;
	stream << "<br><b>Klasse 2: Sehr wahrscheinlich benigne Veränderungen:</b> Aufgrund der Häufigkeit in der Allgemeinbevölkerung oder der Lokalisation bzw. aufgrund von Angaben in der Literatur werden nicht mitgeteilt, können aber erfragt werden." << endl;
	stream << "<br><b>Klasse 1: Benigne Veränderungen:</b> Werden nicht mitgeteilt, können aber erfragt werden." << endl;

	///CNvs
	stream << "<p><b>Gefunden Copy-Number-Varianten</b>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><th>Koordianten</th><th>Exons</th><th>CopyNumbers</th><th>Details</th></tr>" << endl;
	stream << "<tr>" << endl;
	stream << "<td colspan=4><font style=\"background-color: #FF0000\">Abschnitt mit Daten aus dem Report fuellen oder loeschen!<br>Im Moment nur f&uuml;r X-Diagnostik relevant!</font></td>" << endl;
	stream << "</tr>" << endl;
	stream << "</table>" << endl;

	///output: coverage
	if (file_bam_!="")
	{
		//get statistics values
		QString avg_cov = "";
		QString perc_cov20 = "";
		QCCollection stats = Statistics::mapping(roi_, file_bam_);
		for (int i=0; i<stats.count(); ++i)
		{
			if (stats[i].name()=="target region read depth") avg_cov = stats[i].toString();
			if (stats[i].name()=="target region 20x percentage") perc_cov20 = stats[i].toString();
		}

		stream << "<p><b>Abdeckungsstatistik</b>" << endl;
		stream << "<br>Durchschnittliche Sequenziertiefe: " << avg_cov << endl;

		//calculate low coverage regions
		BedFile low_cov = Statistics::lowCoverage(roi_, file_bam_, 20);

		//annotate gene names
		QSet<QString> skipped_genes;
		BedFile gene_anno;
		gene_anno.load(Settings::string("kgxref"));
		gene_anno.extend(25); //extend to annotate splicing regions as well
		ChromosomalIndex<BedFile> gene_idx(gene_anno);
		for (int i=0; i<low_cov.count(); ++i)
		{
			QStringList genes;
			QVector<int> gene_indices = gene_idx.matchingIndices(low_cov[i].chr(), low_cov[i].start(), low_cov[i].end());
			foreach(int index, gene_indices)
			{
				QString current = gene_anno[index].annotations().at(0).toUpper();

				if (genes_.isEmpty() || genes_.contains(current))
				{
					if (!genes.contains(current)) genes.append(current);
				}
				else if (!genes_.isEmpty() && !genes_.contains(current))
				{
					skipped_genes.insert(current);
				}
			}
			low_cov[i].annotations().clear();
			low_cov[i].annotations().append(genes.join(", "));
		}

		//Log genes the were not in the gene list - if any
		if (!skipped_genes.isEmpty())
		{
			Log::info("Genes not in gene list: " + QStringList(skipped_genes.toList()).join(", "));
		}

		//group by gene name
		QMap<QString, QVector<BedLine> > grouped;
		for (int i=0; i<low_cov.count(); ++i)
		{
			QString genes = low_cov[i].annotations()[0];

			//skip non-gene regions
			// - remains of VEGA database in old HaloPlex designs
			// - SNPs for sample identification
			if (genes=="") continue;

			if (!grouped.contains(genes)) grouped.insert(genes, QVector<BedLine>());
			grouped[genes].append(low_cov[i]);
		}

		//output
		if (!genes_.isEmpty())
		{
			QStringList complete_genes;
			foreach(const QString& gene, genes_)
			{
				if (!grouped.contains(gene))
				{
					complete_genes << gene;
				}
			}
			stream << "<br>Komplett abgedeckte Gene: " << complete_genes.join(", ") << endl;
		}
		stream << "<br>Anteil Regionen mit Tiefe <20: " << QString::number(100.0-perc_cov20.toFloat(), 'f', 2) << "%" << endl;
		if (!genes_.isEmpty())
		{
			QStringList incomplete_genes;
			foreach(const QString& gene, genes_)
			{
				if (grouped.contains(gene))
				{
					const QVector<BedLine>& lines = grouped[gene];
					int missing_bases = 0;
					for (int i=0; i<lines.count(); ++i)
					{
						missing_bases += lines[i].length();
					}
					incomplete_genes << gene + " <font style=\"font-size: 80%;\">" + QString::number(missing_bases) + "</font> ";
				}
			}
			stream << "<br>Fehlende Basen in nicht komplett abgedeckten Genen: " << incomplete_genes.join(", ") << endl;
		}
		stream << "<br><font style=\"background-color: #FF0000\">Details Regionen mit Tiefe <20:</font>" << endl;
		stream << "<table>" << endl;
		stream << "<tr><th>Gen</th><th>Basen</th><th>Chromosom</th><th>Koordinaten_hg19</th></tr>" << endl;
		for (auto it=grouped.cbegin(); it!=grouped.cend(); ++it)
		{
			stream << "<tr>" << endl;
			stream << "<td>" << endl;
			QString chr;
			QString coords;
			int bases = 0;
			const QVector<BedLine>& lines = it.value();
			for (int i=0; i<lines.count(); ++i)
			{
				chr = lines[i].chr().str();
				bases += lines[i].length();
				if (coords!="") coords += ", ";
				coords += QString::number(lines[i].start()) + "-" + QString::number(lines[i].end());
			}
			stream << it.key() << "</td><td>" << bases << "</td><td>" << chr << "</td><td>" << coords << endl;

			stream << "</td>" << endl;
			stream << "</tr>" << endl;
		}
		stream << "</table>" << endl;
		stream << "	</body>" << endl;
		stream << "</html>" << endl;

		//additionally store low-coverage BED file
		low_cov.store(QString(file_rep_).replace(".html", "_lowcov.bed"));
	}

	//output variant details
	if(var_details_)
	{
		stream << "<p><b><font style=\"background-color: #FF0000\">Details zu Varianten (f&uuml;r interne Zwecke)</font>" << endl;
		stream << "<table>" << endl;
		stream << "<tr><th>chr</th><th>start</th><th>end</th><th>ref</th><th>obs</th>";
		for (int i=0; i<variants_.annotations().count(); ++i)
		{
			stream << "<th>" << variants_.annotations()[i].name() << "</th>";
		}
		stream << "</tr>";
		for (int i=0; i<variants_selected_.count(); ++i)
		{
			const Variant& variant = variants_[variants_selected_[i].first];
			stream << "<tr><td>" << variant.chr().str() << "</td><td>" << variant.start() << "</td><td>" << variant.end() << "</td><td>" << variant.ref() << "</td><td>" << variant.obs() << "</td>";
			for (int j=0; j<variants_.annotations().count(); ++j)
			{
				QString anno = variant.annotations().at(j);
				bool ok = false;
				anno.toDouble(&ok);
				if (ok) anno.replace(".", ",");
				stream << "<td><nobr>" << anno.toHtmlEscaped() << "</nobr></td>";
			}
			stream << "</tr>" << endl;
		}
		stream << "</table>" << endl;
	}

	//collect and display important tool versions
	stream << "<p><b><font style=\"background-color: #FF0000\">Details zu Analysetools (f&uuml;r interne Zwecke)</font>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><th>tool</th><th>version</th><th>parameters</th></tr>";
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
				stream << "<tr><td>" << tool << "</td><td>" << version << "</td><td>" << params << "</td></tr>" << endl;
			}
		}
	}
	stream << "</table>" << endl;

	//close stream
	outfile->close();

	//copy temp file to output folder
	if (QFile(file_rep_).exists() && !QFile(file_rep_).remove())
	{
		THROW(FileAccessException, "Could not remove previous HTML report: " + file_rep_);
	}
	if (!QFile::rename(temp_filename, file_rep_))
	{
		THROW(FileAccessException, "Could not copy HTML report from temporary file " + temp_filename + " to " + file_rep_ + " !");
	}

	//copy report to archive folder
	QString file_rep_copy = "M:/Diagnostik/GSvarReportsArchive/" + QFileInfo(file_rep_).fileName();
	if (QFile(file_rep_copy).exists() && !QFile::remove(file_rep_copy))
	{
		THROW(FileAccessException, "Could not remove previous HTML report in archive folder: " + file_rep_copy);
	}
	if (!QFile::copy(file_rep_, file_rep_copy))
	{
		THROW(FileAccessException, "Could not copy HTML report to archive folder: " + file_rep_copy);
	}
}

void ReportWorker::writeXML()
{
	QString outfile_name = QString(file_rep_).replace(".html", ".xml");
	QScopedPointer<QFile> outfile(Helper::openFileForWriting(outfile_name));

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
	w.writeAttribute("name_external", NGSD().getExternalSampleName(sample_name_));
	w.writeEndElement();

	//element TargetRegion (optional)
	if (file_roi_!="")
	{
		BedFile roi;
		roi.load(file_roi_);
		roi.merge();

		w.writeStartElement("TargetRegion");
		w.writeAttribute("name", QFileInfo(file_roi_).baseName());
		w.writeAttribute("regions", QString::number(roi.count()));
		w.writeAttribute("bases", QString::number(roi.baseCount()));
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
	int geno_idx = variants_.annotationIndexByName("genotype", true, true);
	int comment_idx = variants_.annotationIndexByName("comment", true, true);
	for (int i=0; i<variants_selected_.count(); ++i)
	{
		const Variant& v = variants_[variants_selected_[i].first];
		w.writeStartElement("Variant");
		w.writeAttribute("chr", v.chr().str());
		w.writeAttribute("start", QString::number(v.start()));
		w.writeAttribute("end", QString::number(v.end()));
		w.writeAttribute("ref", v.ref());
		w.writeAttribute("obs", v.obs());
		w.writeAttribute("genotype", v.annotations()[geno_idx]);
		w.writeAttribute("comment", v.annotations()[comment_idx]);

		//element Annotation
		for (int i=0; i<v.annotations().count(); ++i)
		{
			if (i==geno_idx) continue;
			if (i==comment_idx) continue;

			w.writeStartElement("Annotation");
			w.writeAttribute("name", variants_.annotations()[i].name());
			w.writeAttribute("value", v.annotations()[i]);
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
