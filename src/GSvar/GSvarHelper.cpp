#include "GSvarHelper.h"
#include "LoginManager.h"
#include "NGSD.h"
#include "HttpHandler.h"
#include "Settings.h"
#include <QDir>
#include <QStandardPaths>

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

const QMap<QByteArray, QByteArrayList>& GSvarHelper::transcriptMatches()
{
	static QMap<QByteArray, QByteArrayList> output;
	static bool initialized = false;

	if (!initialized)
	{
		QStringList lines = Helper::loadTextFile(":/Resources/hg19_ensembl_transcript_matches.tsv", true, '#', true);
		foreach(const QString& line, lines)
		{
			QByteArrayList parts = line.toLatin1().split('\t');
			if (parts.count()>=2)
			{
				QByteArray enst = parts[0];
				QByteArray match = parts[1];
				output[enst] << match;
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

BedLine GSvarHelper::liftOver(const Chromosome& chr, int start, int end)
{
	//call lift-over webservice
	QString base_url = Settings::string("liftover_webservice");
	HttpHandler handler(HttpRequestHandler::ProxyType::NONE);
	QString output = handler.get(base_url + "?chr=" + chr.strNormalized(true) + "&start=" + QString::number(start) + "&end=" + QString::number(end));

	//handle error from webservice
	if (output.contains("ERROR")) THROW(ArgumentException, "GSvarHelper::liftOver: " + output);

	//convert output to region
	BedLine region = BedLine::fromString(output);
	if (!region.isValid()) THROW(ArgumentException, "GSvarHelper::liftOver: Could not convert output '" + output + "' to region");

	return region;
}

QString GSvarHelper::gnomaADLink(const Variant& v)
{
	QString url = "http://gnomad.broadinstitute.org/variant/" + v.chr().strNormalized(false) + "-";

	if (v.obs()=="-") //deletion
	{
		int pos = v.start()-1;
		FastaFileIndex idx(Settings::string("reference_genome"));
		QString base = idx.seq(v.chr(), pos, 1);
		url += QString::number(pos) + "-" + base + v.ref() + "-" + base;
	}
	else if (v.ref()=="-") //insertion
	{
		int pos = v.start();
		FastaFileIndex idx(Settings::string("reference_genome"));
		QString base = idx.seq(v.chr(), pos, 1);
		url += QString::number(v.start()) + "-" + base + "-" + base + v.obs();
	}
	else //snv
	{
		url += QString::number(v.start()) + "-" + v.ref() + "-" + v.obs();
	}

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
