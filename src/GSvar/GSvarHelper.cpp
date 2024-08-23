#include "GSvarHelper.h"
#include "LoginManager.h"
#include "NGSD.h"
#include "HttpHandler.h"
#include "Settings.h"
#include "SingleSampleAnalysisDialog.h"
#include "MultiSampleDialog.h"
#include "TrioDialog.h"
#include "SomaticDialog.h"
#include "Exceptions.h"
#include "ChainFileReader.h"
#include "Log.h"
#include "GlobalServiceProvider.h"
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>
#include <QBuffer>
#include <VariantHgvsAnnotator.h>
#include "ClientHelper.h"
#include "ApiCaller.h"

const GeneSet& GSvarHelper::impritingGenes()
{
	static GeneSet output;
	static bool initialized = false;

	if (!initialized)
	{
		const QMap<QByteArray, ImprintingInfo>& imprinting_genes = NGSHelper::imprintingGenes();
		auto it = imprinting_genes.begin();
		while(it!=imprinting_genes.end())
		{
			if (it.value().status=="imprinted")
			{
				output << it.key();
			}

			++it;
		}

		initialized = true;
	}

	return output;
}

const GeneSet& GSvarHelper::hi0Genes()
{
	static GeneSet output;
	static bool initialized = false;

	if (!initialized)
	{
		QStringList lines = Helper::loadTextFile(":/Resources/genes_actionable_hi0.tsv", true, '#', true);
		foreach(const QString& line, lines)
		{
			output << line.toUtf8();
		}

		initialized = true;
	}

	return output;
}

const GeneSet& GSvarHelper::genesWithPseudogene()
{
	static GeneSet output;
	static bool initialized = false;

	if (!initialized)
	{
		if (LoginManager::active())
		{
			NGSD db;
			QStringList genes = db.getValues("SELECT DISTINCT(g.symbol) FROM gene g, gene_pseudogene_relation gpr WHERE g.id=gpr.parent_gene_id");
			foreach(QString gene, genes)
			{
				output << gene.toUtf8();
			}

			initialized = true;
		}

	}

	return output;
}

const QMap<QByteArray, QByteArrayList>& GSvarHelper::preferredTranscripts(bool reload)
{
	static QMap<QByteArray, QByteArrayList> output;
    static bool initialized = false;

    if (!initialized || reload)
    {
		if (LoginManager::active())
		{
			NGSD db;
			output = db.getPreferredTranscripts();

			initialized = true;
		}
    }

	return output;
}

const QMap<QByteArray, QList<BedLine>> & GSvarHelper::specialRegions()
{
	static QMap<QByteArray, QList<BedLine>> output;
    static bool initialized = false;

    if (!initialized)
	{
        QString filename = GSvarHelper::applicationBaseName() + "_special_regions.tsv";
        QStringList lines = Helper::loadTextFile(filename, true, '#', true);
        foreach(const QString& line, lines)
        {
            QByteArrayList parts = line.toUtf8().split('\t');
            if (parts.count()>=2)
            {
                QByteArray gene = parts[0].trimmed();
                for (int i=1; i<parts.count(); ++i)
                {
					output[gene] << BedLine::fromString(parts[i]);
                }
            }
        }

        initialized = true;
    }

	return output;
}

QString GSvarHelper::applicationBaseName()
{
	return QCoreApplication::applicationDirPath() + QDir::separator() + QCoreApplication::applicationName();
}

GenomeBuild GSvarHelper::build()
{
	try
	{
		QString build_str = Settings::string("build");
		return stringToBuild(build_str);
	}
	catch(Exception& e)
	{
		Log::info("Genome build in GSvar.ini file is not valid: " + e.message());
	}

	return GenomeBuild::HG38; //fallback in case of exception
}

void GSvarHelper::colorGeneItem(QTableWidgetItem* item, const GeneSet& genes)
{
	//init
	static const GeneSet& imprinting_genes = impritingGenes();
	static const GeneSet& hi0_genes = hi0Genes();
	static const GeneSet& pseudogene_genes = genesWithPseudogene();

	QStringList messages;

	GeneSet intersect = genes.intersect(imprinting_genes);
	foreach(const QByteArray& gene, intersect)
	{
		messages << (gene + ": Imprinting gene");
	}

	intersect = genes.intersect(hi0_genes);
	foreach(const QByteArray& gene, intersect)
	{
		messages << (gene + ": No evidence for haploinsufficiency");
	}

	intersect = genes.intersect(pseudogene_genes);
	foreach(const QByteArray& gene, intersect)
	{
		messages << (gene + ": Has pseudogene(s)");
	}

	//mark gene
	if (!messages.isEmpty())
	{
		messages.sort();
		item->setBackgroundColor(Qt::yellow);
		item->setToolTip(messages.join('\n'));
	}
}

bool GSvarHelper::colorQcItem(QTableWidgetItem* item, const QString& accession, const QString& sys_type, const QString& gender)
{
	//init
	static QColor orange = QColor(255,150,0,125);
	static QColor red = QColor(255,0,0,125);

	//check that value is numeric
	bool ok = false;
	double value = item->text().toDouble(&ok);
	if (!ok) return false;

	//determine color
	QColor* color = nullptr;
	if (accession=="QC:2000014") //known variants %
	{
		if (value<95) color = &orange;
		if (value<90) color = &red;
	}
	else if (accession=="QC:2000025") //avg depth
	{
		if (sys_type=="WGS")
		{
			if (value<35) color = &orange;
			if (value<30) color = &red;
		}
		else if (sys_type=="lrGS")
		{
			if (value<30) color = &orange;
			if (value<20) color = &red;
		}
		else
		{
			if (value<80) color = &orange;
			if (value<50) color = &red;
		}
	}
	else if (accession=="QC:2000027") //cov 20x
	{
		if (sys_type=="WGS")
		{
			if (value<99) color = &orange;
			if (value<95) color = &red;
		}
		else
		{
			if (value<95) color = &orange;
			if (value<90) color = &red;
		}
	}
	else if (accession=="QC:2000051") //AF deviation
	{
		if (value>3) color = &orange;
		if (value>6) color = &red;
	}
	else if(accession=="QC:2000045") //known somatic variants percentage
	{
		if (value>4) color = &orange;
		if (value>5) color = &red;
	}
	else if(accession=="QC:2000139") //chrY/chrX read ratio
	{
		if (gender=="female" && value>0.02) color = &orange;
	}
	else if (accession=="QC:2000023") //insert size
	{
		if (value<190) color = &orange;
		if (value<150) color = &red;
	}
	else if (accession=="QC:2000113") //CNV count
	{
		if (value<1) color = &red;
	}
	else if (accession=="QC:2000024") //duplicate %
	{
		if (value>25) color = &orange;
		if (value>35) color = &red;
	}
	else if (accession=="QC:2000021") //on target %
	{
		if (value<50) color = &orange;
		if (value<25) color = &red;
	}
	else if (accession=="QC:2000071") //target region read depth 2-fold duplication
	{
		if (value<1000) color = &orange;
		if (value<500) color = &red;
	}
	else if (accession=="QC:2000083") //cfDNA-tumor correlation
	{
		if (value<0.9) color = &orange;
		if (value<0.75) color = &red;
	}

	//set color
	if (color!=nullptr)
	{
		item->setBackgroundColor(*color);
	}

	return color!=nullptr;
}

void GSvarHelper::limitLines(QLabel* label, QString text, int max_lines)
{
	QStringList lines = text.split("\n");
	if (lines.count()<max_lines)
	{
		label->setText(text);
	}
	else
	{
		while(lines.count()>max_lines) lines.removeLast();
		lines.append("<i>[text truncated - see tooltip/edit dialog for complete text]</i>");
		label->setText(lines.join("<br>"));
		label->setToolTip(text);
	}
}

BedLine GSvarHelper::liftOver(const Chromosome& chr, int start, int end, bool hg19_to_hg38)
{
	//special handling of chrMT (they are the same for GRCh37 and GRCh38)
	if (chr.strNormalized(true)=="chrMT") return BedLine(chr, start, end);

	//cache chain file readers to speed up multiple calls
	static QHash<QString, QSharedPointer<ChainFileReader>> chain_reader_cache;
	QString chain = hg19_to_hg38 ? "hg19_hg38" : "hg38_hg19";
	if (!chain_reader_cache.contains(chain))
	{
		QString filepath = Settings::string("liftover_" + chain, true).trimmed();
		if (filepath.isEmpty()) THROW(ProgrammingException, "No chain file specified in settings.ini. Please inform the bioinformatics team!");
		chain_reader_cache.insert(chain, QSharedPointer<ChainFileReader>(new ChainFileReader(filepath, 0.05)));
	}

	//lift region
	BedLine region = chain_reader_cache[chain]->lift(chr, start, end);

	if (!region.isValid()) THROW(ArgumentException, "genomic coordinate lift-over failed: Lifted region is not a valid region");

	return region;
}

Variant GSvarHelper::liftOverVariant(const Variant& v, bool hg19_to_hg38)
{
	//load genome indices (static to do it only once)
	static FastaFileIndex genome_index(Settings::string("reference_genome"));
	static FastaFileIndex genome_index_hg19(Settings::string("reference_genome_hg19"));

	//lift-over coordinates
	BedLine coords_new = liftOver(v.chr(), v.start(), v.end(), hg19_to_hg38);
	if (v.chr().isNonSpecial() && !coords_new.chr().isNonSpecial())
	{
		THROW(ArgumentException, "Chromosome changed to special chromosome: "+v.chr().strNormalized(true)+" > "+coords_new.chr().strNormalized(true));
	}

	//init new variant
	Variant v2;
	v2.setChr(coords_new.chr());
	v2.setStart(coords_new.start());
	v2.setEnd(coords_new.end());

	//check sequence context is the same
	Sequence context_old;
	Sequence context_new;
	int context_length = 10 + v.ref().length();
	if (hg19_to_hg38)
	{
		context_old = genome_index_hg19.seq(v.chr(), v.start()-5, context_length);
		context_new = genome_index.seq(v2.chr(), v2.start()-5, context_length);
	}
	else
	{
		context_old = genome_index.seq(v.chr(), v.start()-5, context_length);
		context_new = genome_index_hg19.seq(v2.chr(), v2.start()-5, context_length);
	}
	if (context_old==context_new)
	{
		v2.setRef(v.ref());
		v2.setObs(v.obs());
	}
	else //check if strand changed, e.g. in NIPA1, GDF2, ANKRD35, TPTE, ...
	{
		context_new.reverseComplement();
		if (context_old==context_new)
		{
			Sequence ref = v.ref();
			if (ref!="-") ref.reverseComplement();
			v2.setRef(ref);

			Sequence obs = v.obs();
			if (obs!="-") obs.reverseComplement();
			v2.setObs(obs);
		}
		else
		{
			context_new.reverseComplement();
			THROW(ArgumentException, "Sequence context of variant changed: "+context_old+" > "+context_new);
		}
	}

	return v2;
}

QString GSvarHelper::gnomADLink(const Variant& v, bool open_in_v4)
{
	FastaFileIndex idx(Settings::string("reference_genome"));
	return "https://gnomad.broadinstitute.org/variant/" + v.toGnomAD(idx) + "?dataset=" + (open_in_v4 ? "gnomad_r4" : "gnomad_r3");
}

QString GSvarHelper::allOfUsLink(const Variant& v)
{
	FastaFileIndex idx(Settings::string("reference_genome"));
	return "https://databrowser.researchallofus.org/variants/" + v.toGnomAD(idx);
}


QString GSvarHelper::clinVarSearchLink(const Variant& v, GenomeBuild build)
{
	return "https://www.ncbi.nlm.nih.gov/clinvar/?term=" + v.chr().strNormalized(false)+"[chr]+AND+" + QString::number(v.start()) + "%3A" + QString::number(v.end()) + (build==GenomeBuild::HG38? "[chrpos38]" : "[chrpos37]");
}

QString GSvarHelper::localRoiFolder()
{
	QStringList default_paths = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
	if(default_paths.isEmpty())
	{
		THROW(Exception, "No local application data path was found!");
	}

	QString local_roi_folder = default_paths[0] + QDir::separator() + "target_regions" + QDir::separator();
	if(Helper::mkdir(local_roi_folder)==-1)
	{
		THROW(ProgrammingException, "Could not create application target region folder '" + local_roi_folder + "'!");
	}

	return local_roi_folder;
}

bool GSvarHelper::queueSampleAnalysis(AnalysisType type, const QList<AnalysisJobSample>& samples, QWidget *parent)
{
	if (!LoginManager::active())
	{
		QMessageBox::warning(parent, "No access to the NGSD", "You need access to the NGSD to queue a anlysis!");
		return false;
	}

	//init NGSD
	NGSD db;

	if (type==GERMLINE_SINGLESAMPLE || type==CFDNA)
	{
		SingleSampleAnalysisDialog dlg(parent);
		if (samples.size() > 0) dlg.setSamples(samples);
		if (dlg.exec()==QDialog::Accepted && dlg.samples().size() > 0)
		{
			foreach(const AnalysisJobSample& sample,  dlg.samples())
			{
				db.queueAnalysis("single sample", dlg.highPriority(), dlg.arguments(), QList<AnalysisJobSample>() << sample);
			}
			return true;
		}
	}
	else if (type==GERMLINE_MULTISAMPLE)
	{
		MultiSampleDialog dlg(parent);
		dlg.setSamples(samples);
		if (dlg.exec()==QDialog::Accepted)
		{
			db.queueAnalysis("multi sample", dlg.highPriority(), dlg.arguments(), dlg.samples());
			return true;
		}
	}
	else if (type==GERMLINE_TRIO)
	{
		TrioDialog dlg(parent);
		dlg.setSamples(samples);
		if (dlg.exec()==QDialog::Accepted)
		{
			foreach(auto sample, dlg.samples()) qDebug() << "Sample: " << sample.name;

			NGSD().queueAnalysis("trio", dlg.highPriority(), dlg.arguments(), dlg.samples());
			return true;
		}
	}
	else if (type==SOMATIC_PAIR || type==SOMATIC_SINGLESAMPLE)
	{
		SomaticDialog dlg(parent);
		dlg.setSamples(samples);

		if (dlg.exec()==QDialog::Accepted)
		{
			db.queueAnalysis("somatic", dlg.highPriority(), dlg.arguments(), dlg.samples());
			return true;
		}
	}
	else
	{
		THROW(NotImplementedException, "Invalid analysis type")
	}

	return false;
}

QString GSvarHelper::specialGenes(const GeneSet& genes)
{
	GeneSet special_genes;
	special_genes << "ACTA2" << "BRAF" << "BRCA1" << "BRCA2" << "CFTR" << "CNBP" << "COL3A1" << "DMPK" << "F8" << "FBN1" << "FMR1" << "GJB2" << "GJB6" << "HTT" << "KRAS" << "MLH1" << "MSH2" << "MSH6" << "MYH11" << "MYLK" << "PMS2" << "PTPN11" << "RAF1" << "RIT1" << "SMAD3" << "SMN1" << "SMN2" << "SOS1" << "TGFB2" << "TGFBR1" << "TGFBR2";
	GeneSet inter = genes.intersect(special_genes);

	//output
	if (inter.isEmpty()) return "";

	return "Some genes (" + inter.join(", ") + ") require 'indikationsspezifische Abrechnung'!";
}

CfdnaDiseaseCourseTable GSvarHelper::cfdnaTable(const QString& tumor_ps_name, QStringList& errors, bool throw_if_fails)
{
	if (!LoginManager::active())
	{
		THROW(ArgumentException, "No access to the NGSD! NGSD access is required to create table.");
	}

	//init NGSD
	NGSD db;
	CfdnaDiseaseCourseTable table;


	// get all related cfDNA processed sample ids
	//same samples
	int sample_id = db.sampleId(tumor_ps_name).toInt();
	QSet<int> same_sample_ids = db.relatedSamples(sample_id, "same sample");
	same_sample_ids << sample_id; // add current sample id
	// get all related cfDNA samples
	QSet<int> cf_dna_sample_ids;
	foreach (int cur_sample_id, same_sample_ids)
	{
		cf_dna_sample_ids.unite(db.relatedSamples(cur_sample_id, "tumor-cfDNA"));
	}
	// get corresponding processed sample
	QStringList cf_dna_ps_ids;
	foreach (int cf_dna_sample, cf_dna_sample_ids)
	{
		//ignore merged samples
		cf_dna_ps_ids << db.getValues("SELECT ps.id FROM processed_sample ps WHERE sample_id=:0 AND ps.id NOT IN (SELECT processed_sample_id FROM merged_processed_samples)", QString::number(cf_dna_sample));
	}

	//make sure every cfDNA has the same processing system
	// get processing systems from cfDNA samples
	QSet<QString> processing_systems;
	foreach (const QString& cf_dna_ps_id, cf_dna_ps_ids)
	{
		processing_systems.insert(db.getProcessingSystemData(db.processingSystemIdFromProcessedSample(db.processedSampleName(cf_dna_ps_id))).name_short);
	}
	if (processing_systems.size() > 1)
	{
		THROW(ArgumentException, "Multiple processing systems used for cfDNA analysis. Cannot compare samples!");
	}
	QString system_name = processing_systems.toList().at(0);


	// load cfDNA panel
	QList<CfdnaPanelInfo> cfdna_panels = db.cfdnaPanelInfo(db.processedSampleId(tumor_ps_name), db.processingSystemId(system_name));
	if (cfdna_panels.size() < 1)
	{
		THROW(ArgumentException, "No matchin cfDNA panel for sample " + tumor_ps_name + " found in NGSD!");
	}
	CfdnaPanelInfo cfdna_panel_info  = cfdna_panels.at(0);
	VcfFile cfdna_panel = db.cfdnaPanelVcf(cfdna_panel_info.id);


	// get header infos
	table.tumor_sample.name = tumor_ps_name;
	table.tumor_sample.ps_id = db.processedSampleId(tumor_ps_name);
	table.tumor_sample.received_date = QDate::fromString(db.getSampleData(db.sampleId(tumor_ps_name)).received, "dd.MM.yyyy");

	TsvFile dummy_mrd_file;
	dummy_mrd_file.addHeader("MRD_log10");
	dummy_mrd_file.addHeader("MRD_pval");
	dummy_mrd_file.addHeader("SUM_DP");
	dummy_mrd_file.addHeader("SUM_ALT");
	dummy_mrd_file.addHeader("Mean_AF");
	dummy_mrd_file.addHeader("Median_AF");
	dummy_mrd_file.addRow(QStringList() << "file not found" << "" << "" << "" << "" << "");

	//load cfDNA VCFs
	QMap<QString, QMap<QString,VcfLine>> cfdna_variants;
	QMap<QString, TsvFile> mrd_files;
	foreach (const QString& ps_id, cf_dna_ps_ids)
	{
		//store date/name for sorting
		CfdnaDiseaseCourseTable::PSInfo cfdna_sample;
		cfdna_sample.name = db.processedSampleName(ps_id);
		cfdna_sample.ps_id = ps_id;
		SampleData sample_data = db.getSampleData(db.sampleId(cfdna_sample.name));
		cfdna_sample.received_date = QDate::fromString(sample_data.received, "dd.MM.yyyy");

		if (sample_data.order_date != "")
		{
			cfdna_sample.order_date = QDate::fromString(sample_data.order_date,  "dd.MM.yyyy");
		}

		if (sample_data.sampling_date != "")
		{
			cfdna_sample.sampling_date = QDate::fromString(sample_data.sampling_date, "dd.MM.yyyy");
		}

		table.cfdna_samples << cfdna_sample;


		//load VCFs
		FileLocation cfdna_fl = GlobalServiceProvider::database().processedSamplePath(ps_id, PathType::VCF_CF_DNA);
		if(cfdna_fl.exists)
		{
			VcfFile vcf;
			vcf.load(cfdna_fl.filename);
			QMap<QString,VcfLine> current_cfdna_variants;
			for (int i=0; i<vcf.count(); ++i)
			{
				const VcfLine& vcf_line = vcf[i];
				current_cfdna_variants.insert(vcf_line.toString(), vcf_line);
			}
			cfdna_variants.insert(cfdna_sample.name, current_cfdna_variants);
		}
		else
		{
			if (throw_if_fails) THROW(FileAccessException, "cfDNA file '" + cfdna_fl.filename + "' doesn't exist!");

			errors << "cfDNA file '" + cfdna_fl.filename + "' doesn't exist!";
			//cfdna_variants.insert(cfdna_sample.name, QMap<QString,VcfLine>());
		}


		//load MRD
		FileLocation mrd_file_fl = GlobalServiceProvider::database().processedSamplePath(ps_id, PathType::MRD_CF_DNA);
		if (!mrd_file_fl.exists)
		{
			if (throw_if_fails) THROW(FileAccessException, "Could not find cfDNA MRD file for processed Sample " + cfdna_sample.name + "! ");

			errors << "Could not find cfDNA MRD file for processed Sample " + cfdna_sample.name + "! ";

			mrd_files.insert(cfdna_sample.name, dummy_mrd_file);
		}
		else
		{
			// load mrd table
			TsvFile mrd_file;
			mrd_file.load(mrd_file_fl.filename);

			//check for correct table format
			if(mrd_file.headers() != QStringList() << "MRD_log10" << "MRD_pval" << "SUM_DP" << "SUM_ALT" << "Mean_AF" << "Median_AF")
			{
				if(throw_if_fails) THROW(ArgumentException,  "Invalid MRD file format! Header doesn't match in MRD file for processed Sample " + cfdna_sample.name + "! ");
				errors << "Invalid MRD file format! Header doesn't match in MRD file for processed Sample " + cfdna_sample.name + "! ";
				mrd_files.insert(cfdna_sample.name, dummy_mrd_file);

			}
			else
			{
				mrd_files.insert(cfdna_sample.name, mrd_file);
			}
		}
	}
	//sort cfDNA samples
	std::sort(table.cfdna_samples.begin(), table.cfdna_samples.end());


	//collapse to table struct
	for (int i=0; i<cfdna_panel.count(); i++)
	{
		CfdnaDiseaseCourseTable::CfdnaDiseaseCourseTableLine line;

		line.tumor_vcf_line = cfdna_panel[i];
		QString var_str = line.tumor_vcf_line.toString();

		foreach (const auto& cfdna_sample, table.cfdna_samples)
		{
			CfdnaDiseaseCourseTable::CfdnaDiseaseCourseTableCfdnaEntry cfdna_entry;

			if(cfdna_variants.contains(cfdna_sample.name))
			{
				if(cfdna_variants.value(cfdna_sample.name).contains(var_str))
				{
					const VcfLine& cfdna_variant = cfdna_variants.value(cfdna_sample.name).value(var_str);

					// check umiVar VCF
					QStringList missing_keys;
					if (!cfdna_variant.formatKeys().contains("M_AF")) missing_keys << "M_AF";
					if (!cfdna_variant.formatKeys().contains("M_AC")) missing_keys << "M_AC";
					if (!cfdna_variant.formatKeys().contains("M_REF")) missing_keys << "M_REF";
					if (!cfdna_variant.formatKeys().contains("Pval")) missing_keys << "Pval";
					// old umiVar format
					if (missing_keys.size() > 0)
						THROW(FileParseException, "Keys '" + missing_keys.join("', '") + "'.\n Maybe sample '" + cfdna_sample.name + "' was analyzed with an old version of umiVar. Please redo the VC!");

					//parse VCF line
					cfdna_entry.multi_af = Helper::toDouble(cfdna_variant.formatValueFromSample("M_AF"), "M_AF", QString::number(i));
					cfdna_entry.multi_alt = Helper::toDouble(cfdna_variant.formatValueFromSample("M_AC"), "M_AC", QString::number(i));
					cfdna_entry.multi_ref = Helper::toDouble(cfdna_variant.formatValueFromSample("M_REF"), "M_REF", QString::number(i));
					cfdna_entry.p_value = Helper::toDouble(cfdna_variant.formatValueFromSample("Pval"), "Pval", QString::number(i));

				}
				else
				{
					//variant not called in cfDNA
					cfdna_entry.multi_af = 0.0;
					cfdna_entry.multi_alt = 0;
					cfdna_entry.multi_ref = 0;
					cfdna_entry.p_value = 1.0;
				}
			}
			else
			{
				//fallback if file-not-found
				cfdna_entry.multi_af = std::numeric_limits<double>::quiet_NaN();
				cfdna_entry.multi_alt = -1;
				cfdna_entry.multi_ref = -1;
				cfdna_entry.p_value = std::numeric_limits<double>::quiet_NaN();
			}

			line.cfdna_columns << cfdna_entry;
		}

		table.lines << line;
	}

	//add mrd files
	foreach (const auto& cfdna_sample, table.cfdna_samples)
	{
		table.mrd_tables << mrd_files.value(cfdna_sample.name);
	}

	return table;
}

QList<QStringList> GSvarHelper::annotateCodingAndSplicing(const VcfLine& variant, GeneSet& genes, bool add_flags, int offset)
{
	if (!LoginManager::active()) THROW(ArgumentException, "No access to the NGSD! You need access to the NGSD for annotation!");

	QList<QStringList> annotations;
	genes.clear();

	//get all transcripts containing the variant
	TranscriptList transcripts  = NGSD().transcriptsOverlapping(variant.chr(), variant.start(), variant.end(), offset);
	transcripts.sortByRelevance();

	//annotate consequence for each transcript
	FastaFileIndex genome_idx(Settings::string("reference_genome"));
	VariantHgvsAnnotator hgvs_annotator(genome_idx);
	foreach(const Transcript& trans, transcripts)
	{
		VariantConsequence consequence = hgvs_annotator.annotate(trans, variant);

		QStringList entry;
		entry << trans.gene() << trans.nameWithVersion() << consequence.typesToString() << consequence.hgvs_c << consequence.hgvs_p;
		genes << trans.gene();

		if(add_flags)
		{
			//flags for important transcripts
			QStringList flags = trans.flags(true);
			entry += flags;
		}

		annotations << entry;
	}

	return annotations;
}

QString GSvarHelper::appPathForTemplate(QString path)
{
    QString app_path_auto_template = "[APP_PATH_AUTO]";
    QString app_path_windows_template = "[APP_PATH_WINDOWS]";
    QString app_path_unix_template = "[APP_PATH_UNIX]";

    QString windows_sep = "\\";
    QString unix_sep = "/";

    QString win_app_path = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
    QString unix_app_path = QCoreApplication::applicationDirPath();

    if (!win_app_path.endsWith(windows_sep)) win_app_path = win_app_path + windows_sep;
    if (!unix_app_path.endsWith(unix_sep)) unix_app_path = unix_app_path + unix_sep;

    path = path.replace(app_path_windows_template, win_app_path);
    path = path.replace(app_path_unix_template, unix_app_path);

    if (path.contains(app_path_auto_template))
    {
        if (Helper::isWindows())
        {
            path = path.replace(unix_sep, windows_sep);
            path = path.replace(app_path_auto_template, win_app_path);
        }
        else
        {
            path = path.replace(windows_sep, unix_sep);
            path = path.replace(app_path_auto_template, unix_app_path);
        }
    }

    return path;
}
