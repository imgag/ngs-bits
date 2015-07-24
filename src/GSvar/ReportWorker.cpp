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

ReportWorker::ReportWorker(QString sample_name, QString sample_name_external, QStringList filters, const VariantList& variants, const QVector< QPair<int, bool> >& variants_selected, QString outcome, QString file_roi, QString file_bam, bool var_details, QStringList log_files, QString file_rep)
	: WorkerBase("Report generation")
	, sample_name_(sample_name)
	, sample_name_external_(sample_name_external)
	, filters_(filters)
	, variants_(variants)
	, variants_selected_(variants_selected)
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

void ReportWorker::writeHTML()
{
	QString temp_filename = Helper::tempFileName(".html");
	QScopedPointer<QFile> outfile(Helper::openFileForWriting(temp_filename));
	QTextStream stream(outfile.data());
	stream << "<html>" << endl;
	stream << "	<head>" << endl;
	stream << "	   <style>" << endl;
	stream << "		<!--" << endl;
	stream << "body" << endl;
	stream << "{" << endl;
	stream << "	font-family: sans-serif;" << endl;
	stream << "	font-size: 65%;" << endl;
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
	stream << "	font-size: 60%;" << endl;
	stream << "	text-align: left;" << endl;
	stream << "}" << endl;
	stream << "		-->" << endl;
	stream << "	   </style>" << endl;
	stream << "	</head>" << endl;
	stream << "	<body>" << endl;
	stream << "<h4>Technischer Report zur bioinformatischen Analyse</h4>" << endl;
	stream << "<p><b>Probe: " << sample_name_ << "</b> (" << sample_name_external_ << ")" << endl;
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
			stream << "<br>Gene (" << QString::number(genes_.count()) << "): " << genes_.join(", ") << endl;
		}
	}

	//output: very important variants
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
	stream << "<p><b>Liste wichtiger Varianten</b>" << endl;
	for (int i=0; i<variants_selected_.count(); ++i)
	{
		if (!variants_selected_[i].second) continue;
		const Variant& v = variants_[variants_selected_[i].first];
		stream << "<table>" << endl;
		stream << "<tr><td><b>Variante:</b> " << v.chr().str() << ":" << v.start() << "-" << v.end() << " " << v.ref() << ">" << v.obs() << " <b>Gen:</b> " << v.annotations()[i_gene] << " <b>Typ:</b> " << v.annotations()[i_type] << " <b>VUS: </b>" << v.annotations()[i_class] << " <b>Vererbung:</b> ausfuellen </td></tr>" << endl;
		QString co_sp = v.annotations()[i_co_sp];
		stream << "<tr><td><b>Details:</b> " << co_sp.replace(",", ", ") << "</td></tr>" << endl;
		QString dbsnp = v.annotations()[i_dbsnp]; //because string is const
		stream << "<tr><td><b>Frequenz:</b> <b>1000g:</b> " << v.annotations()[i_1000g] << " <b>ExAC:</b> " << v.annotations()[i_exac] << " <b>ESP6500:</b> " << v.annotations()[i_esp] << " <b>dbSNP:</b> " << dbsnp.replace("[];", "") << "</td></tr>" << endl;
		stream << "<tr><td><b>OMIM:</b> " << v.annotations()[i_omim] << "</td></tr>" << endl;
		stream << "<tr><td><b>ClinVar:</b> " << v.annotations()[i_clinvar] << "</td></tr>" << endl;
		stream << "<tr><td><b>HGMD:</b> " << v.annotations()[i_hgmd] << "</td></tr>" << endl;
		stream << "<tr><td><b>Kommentar:</b> " << v.annotations()[i_comment] << "</td></tr>" << endl;
		stream << "</table>" << endl;
		stream << "<br>" << endl;
	}

	//output: all variants
	QStringList anno_cols;
	anno_cols << "genotype" << "gene" << "quality" << "variant_type" << "coding_and_splicing";
	stream << "<p><b>Liste aller seltenen Varianten</b>" << endl;
	stream << "<br>Gefundene Varianten in Zielregion gesamt: " << var_count_ << endl;
	stream << "<br>Verwendete Filter: " << filters_.join(";").toHtmlEscaped() << endl;
	stream << "<br>Anzahl relevante Varianten: " << variants_selected_.count() << endl;
	stream << "<table>" << endl;
	stream << "<tr><th>chr</th><th>start</th><th>end</th><th>ref</th><th>obs</th><th>" << anno_cols.join("</th><th>") << "</th></tr>" << endl;
	for (int i=0; i<variants_selected_.count(); ++i)
	{
		stream << "<tr>" << endl;
		stream << "<td>" << endl;
		const Variant& variant = variants_[variants_selected_[i].first];
		stream  << variant.chr().str() << "</td><td>" << variant.start() << "</td><td>" << variant.end() << "</td><td>" << variant.ref() << "</td><td>" << variant.obs();
		for (int j=0; j<anno_cols.count(); ++j)
		{
			QString col = anno_cols[j];
			int index = variants_.annotationIndexByName(col, true, true);
			QString value = variant.annotations().at(index);
			if (col=="quality") value.replace(";", " ");
			if (col=="coding_and_splicing") value.replace(",", ", ");
			stream << "</td><td>" << value.toHtmlEscaped();
		}
		stream << "</td>" << endl;
		stream << "</tr>" << endl;
	}
	stream << "</table>" << endl;

	//output: comments
	stream << "<p><b>Kommentare zu Varianten</b>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><th>Chr.</th><th>Start</th><th>Gen</th><th>Kommentar</th></tr>" << endl;
	for (int i=0; i<variants_selected_.count(); ++i)
	{
		const Variant& variant = variants_[variants_selected_[i].first];
		if (variant.annotations()[i_comment]!="")
		{
			stream << "<tr>" << endl;
			stream << "<td>" << endl;
			stream << variant.chr().str() << "</td><td>" << variant.start() << "</td><td>" << variant.annotations()[i_gene] << "</td><td>" << variant.annotations()[i_comment] << endl;
			stream << "</td>" << endl;
			stream << "</tr>" << endl;
		}
	}
	stream << "</table>" << endl;

	//output: OMIM
	stream << "<p><b>OMIM-Informationen zu Varianten</b>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><th>Chr.</th><th>Start</th><th>OMIM-ID</th><th>OMIM-Details</th></tr>" << endl;
	for (int i=0; i<variants_selected_.count(); ++i)
	{
		const Variant& variant = variants_[variants_selected_[i].first];
		QString omim_value = variant.annotations()[i_omim];
		if (omim_value!="")
		{
			QStringList parts = omim_value.append(" ").split("]; ");
			foreach(QString part, parts)
			{
				if (part.count()<10) continue;

				stream << "<tr>" << endl;
				stream << "<td>" << endl;
				stream << variant.chr().str() << "</td><td>" << variant.start() << "</td><td>" << part.left(6) << "</td><td>" << part.mid(8) << endl;
				stream << "</td>" << endl;
				stream << "</tr>" << endl;
			}
		}
	}
	stream << "</table>" << endl;

	stream << "<p><b>Gefunden Copy-Number-Varianten</b>" << endl;
	stream << "<table>" << endl;
	stream << "<tr><th>Koordianten</th><th>Exons</th><th>CopyNumbers</th><th>Details</th></tr>" << endl;
	stream << "<tr>" << endl;
	stream << "<td colspan=4>Abschnitt mit Daten aus dem Report fuellen oder loeschen!</td>";
	stream << "</tr>" << endl;
	stream << "<tr>" << endl;
	stream << "<td colspan=4>Im Moment nur f&uuml;r X-Diagnostik relevant!</td>" << endl;
	stream << "</tr>" << endl;
	stream << "</table>" << endl;

	//output: coverage
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
			stream << "<br>Komplett abgedeckte Gene (" << QString::number(complete_genes.count()) << "/" << QString::number(genes_.count()) << "): " << complete_genes.join(", ") << endl;
		}
		stream << "<br>Anteil Regionen mit Tiefe kleiner 20: " << QString::number(100.0-perc_cov20.toFloat(), 'f', 2) << "%" << endl;
		stream << "<br>Details Regionen mit Tiefe kleiner 20:" << endl;
		stream << "<table>" << endl;
		stream << "<tr><th>Gen</th><th>Basen</th><th>Chromosom</th><th>Koordinaten_hg19</th></tr>" << endl;
		QMap<QString, QVector<BedLine> >::Iterator it = grouped.begin();
		while(it!=grouped.end())
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

			++it;
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
		stream << "<p><b>Details zu Varianten (f&uuml;r interne Zwecke)" << endl;
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
	stream << "<p><b>Details zu Analysetools (f&uuml;r interne Zwecke)" << endl;
	stream << "<table>" << endl;
	stream << "<tr><th>tool</th><th>version</th><th>parameters</th></tr>";
	QStringList whitelist;
	whitelist << "SeqPurge" << "samblaster" << "/bwa" << "samtools" << "VcfLeftAlign" <<  "freebayes" << "abra" << "vcflib" << "SnpEff"; //current
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
	if (!QFile(file_rep_).remove())
	{
		THROW(FileAccessException, "Could not remove previous HTML report: " + file_rep_);
	}
	if (!QFile::rename(temp_filename, file_rep_))
	{
		THROW(FileAccessException, "Could not copy HTML report from temporary file " + temp_filename + " to " + file_rep_ + " !");
	}

	//copy report to archive folder
	QString file_rep_copy = "M:/Diagnostik/GSvarReportsArchive/" + QFileInfo(file_rep_).fileName();
	if (!QFile::remove(file_rep_copy))
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
	w.writeAttribute("name_external", sample_name_external_);
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
	foreach(QString part, filters_)
	{
		w.writeStartElement("AppliedFilter");
		w.writeAttribute("name", part);
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
