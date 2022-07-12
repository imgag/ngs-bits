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
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>
#include <QBuffer>

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
			output << line.toLatin1();
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
				output << gene.toLatin1();
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
            QByteArrayList parts = line.toLatin1().split('\t');
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

void GSvarHelper::limitLines(QLabel* label, QString text, QString sep, int max_lines)
{
	QStringList lines = text.split(sep);
	if (lines.count()<max_lines)
	{
		label->setText(text);
	}
	else
	{
		while(lines.count()>max_lines) lines.removeLast();
		lines.append("...");
		label->setText(lines.join(sep));
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

QString GSvarHelper::gnomADLink(Variant v, GenomeBuild build)
{
	QString url = "https://gnomad.broadinstitute.org/variant/" + v.chr().strNormalized(false) + "-";

	if (v.obs()!="-" && v.ref()!="-") //SNV
	{
		url += QString::number(v.start()) + "-" + v.ref() + "-" + v.obs();
	}
	else
	{
		QString genome_key = "reference_genome";
		if (GSvarHelper::build()==GenomeBuild::HG38 && build==GenomeBuild::HG19) //GSvar for HG38, but link for HG19 > use special genome key
		{
			genome_key = "reference_genome_hg19";
		}
		FastaFileIndex idx(Settings::string(genome_key));

		if (v.obs()=="-") //deletion
		{
			int pos = v.start()-1;
			QString base = idx.seq(v.chr(), pos, 1);
			url += QString::number(pos) + "-" + base + v.ref() + "-" + base;
		}
		else if (v.ref()=="-") //insertion
		{
			int pos = v.start();
			QString base = idx.seq(v.chr(), pos, 1);
			url += QString::number(v.start()) + "-" + base + "-" + base + v.obs();
		}
	}

	//genome build
	if (build==GenomeBuild::HG38) url += "?dataset=gnomad_r3";

	return url;
}

QString GSvarHelper::localRoiFolder()
{
	QStringList default_paths = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
	if(default_paths.isEmpty())
	{
		THROW(Exception, "No local application data path was found!");
	}

	QString local_roi_folder = default_paths[0] + QDir::separator() + "target_regions" + QDir::separator();
	if(!QFile::exists(local_roi_folder) && !QDir().mkpath(local_roi_folder))
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


bool GSvarHelper::colorMaxEntScan(QString anno, QList<double>& percentages, QList<double>& abs_values)
{
	double max_relevant_change = 0;
	foreach (const QString value, anno.split(','))
	{
		QStringList parts = value.split('>');

		if (parts.count() == 2)
		{
			double percent_change;
			double base = parts[0].toDouble();
			double new_value = parts[1].toDouble();
			double abs_change = std::abs(base-new_value);
			abs_values.append(abs_change);

			// calculate percentage change:
			if (base != 0)
			{
				if (base > 0)
				{
					percent_change = (new_value - base) / base;
				} else {
					percent_change = (base - new_value) / base;
				}
			}
			percent_change = std::abs(percent_change);
			percentages.append(percent_change);

			//Don't color if absChange smaller than 0.5
			if ((abs_change > 0.5) && percent_change > max_relevant_change)
			{
				max_relevant_change = percent_change;
			}
		}
	}

	if (max_relevant_change >= 0.15)
	{
		return true;
	}
	else
	{
		return false;
	}
}
