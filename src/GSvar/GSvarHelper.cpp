#include "GSvarHelper.h"
#include "LoginManager.h"
#include "NGSD.h"
#include "HttpHandler.h"
#include "Settings.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

GeneSet GSvarHelper::imprinting_genes_ = GeneSet();
GeneSet GSvarHelper::hi0_genes_ = GeneSet();
QMap<QByteArray, QByteArrayList> GSvarHelper::preferred_transcripts_ = QMap<QByteArray, QByteArrayList>();
QMap<QByteArray, QList<BedLine>> GSvarHelper::special_regions_ = QMap<QByteArray, QList<BedLine>>();
QMap<QByteArray, QByteArrayList> GSvarHelper::transcript_matches_ = QMap<QByteArray, QByteArrayList>();

const GeneSet& GSvarHelper::impritingGenes()
{
	static bool initialized = false;

	if (!initialized)
	{
        imprinting_genes_.clear();

		//load imprinting gene list
		QStringList lines = Helper::loadTextFile(":/Resources/imprinting_genes.tsv", true, '#', true);
		foreach(const QString& line, lines)
		{
			QStringList parts = line.split("\t");
			if (parts.count()==2)
			{
				imprinting_genes_ << parts[0].toLatin1();
			}
		}

		initialized = true;
	}

	return imprinting_genes_;
}

const GeneSet& GSvarHelper::hi0Genes()
{
	static bool initialized = false;

	if (!initialized)
	{
		hi0_genes_.clear();

		//load imprinting gene list
		QStringList lines = Helper::loadTextFile(":/Resources/genes_actionable_hi0.tsv", true, '#', true);
		foreach(const QString& line, lines)
		{
			hi0_genes_ << line.toLatin1();
		}

		initialized = true;
	}

	return hi0_genes_;
}

const QMap<QByteArray, QByteArrayList>& GSvarHelper::preferredTranscripts(bool reload)
{
    static bool initialized = false;

    if (!initialized || reload)
    {
		if (LoginManager::active())
		{
			NGSD db;
			preferred_transcripts_ = db.getPreferredTranscripts();

			initialized = true;
		}

    }

    return preferred_transcripts_;
}

const QMap<QByteArray, QList<BedLine>> & GSvarHelper::specialRegions()
{
    static bool initialized = false;

    if (!initialized)
    {
        special_regions_.clear();

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
                    special_regions_[gene] << BedLine::fromString(parts[i]);
                }
            }
        }

        initialized = true;
    }

	return special_regions_;
}

const QMap<QByteArray, QByteArrayList>& GSvarHelper::transcriptMatches()
{
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
				transcript_matches_[enst] << match;
			}
		}

		initialized = true;
	}

	return transcript_matches_;
}

QString GSvarHelper::applicationBaseName()
{
	return QCoreApplication::applicationDirPath() + QDir::separator() + QCoreApplication::applicationName();
}

void GSvarHelper::colorGeneItem(QTableWidgetItem* item, const GeneSet& genes)
{
	static const GeneSet& imprinting_genes = impritingGenes();
	static const GeneSet& hi0_genes = hi0Genes();

	bool hit_imprinting = imprinting_genes.intersectsWith(genes);
	bool hit_hi0 = hi0_genes.intersectsWith(genes);
	if (hit_imprinting && hit_hi0)
	{
		item->setBackgroundColor(Qt::green);
		item->setToolTip("Imprinting gene\nNo evidence for haploinsufficiency");
	}
	else if (hit_imprinting)
	{
		item->setBackgroundColor(Qt::yellow);
		item->setToolTip("Imprinting gene");
	}
	else if (hit_hi0)
	{
		item->setBackgroundColor(Qt::cyan);
		item->setToolTip("No evidence for haploinsufficiency");
	}
}

QString GSvarHelper::getEvidenceFile(const QString& bam_file)
{
	if (!bam_file.endsWith(".bam", Qt::CaseInsensitive))
	{
		THROW(ArgumentException, "Invalid BAM file path \"" + bam_file + "\"!");
	}
	QFileInfo bam_file_info(bam_file);
	QDir evidence_dir(bam_file_info.absolutePath() + "/manta_evid/");
	QString ps_name = bam_file_info.fileName().left(bam_file_info.fileName().length() - 4);
	return evidence_dir.absoluteFilePath(ps_name + "_manta_evidence.bam");
}

BedLine GSvarHelper::liftOver(const Chromosome& chr, int start, int end)
{
	//call lift-over webservice
	QString base_url = Settings::string("liftover_webservice");
	HttpHandler handler(HttpHandler::NONE);
	QString output = handler.get(base_url + "?chr=" + chr.strNormalized(true) + "&start=" + QString::number(start) + "&end=" + QString::number(end));

	//handle error from webservice
	if (output.contains("ERROR")) THROW(ArgumentException, "GSvarHelper::liftOver: " + output);

	//convert output to region
	BedLine region = BedLine::fromString(output);
	if (!region.isValid()) THROW(ArgumentException, "GSvarHelper::liftOver: Could not convert output '" + output + "' to region");

	return region;
}
