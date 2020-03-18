#include "GSvarHelper.h"
#include "Helper.h"
#include <QCoreApplication>

GeneSet GSvarHelper::imprinting_genes_ = GeneSet();
GeneSet GSvarHelper::hi0_genes_ = GeneSet();
QMap<QByteArray, QByteArrayList> GSvarHelper::preferred_transcripts_ = QMap<QByteArray, QByteArrayList>();
QMap<QByteArray, QList<BedLine>> GSvarHelper::special_regions_ = QMap<QByteArray, QList<BedLine>>();

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
        preferred_transcripts_.clear();

        QString filename = GSvarHelper::applicationBaseName() + "_preferred_transcripts.tsv";
        QStringList lines = Helper::loadTextFile(filename, true, '#', true);
        foreach(const QString& line, lines)
        {
			QByteArrayList parts = line.toLatin1().replace(',', '\t').split('\t');
            if (parts.count()>=2)
            {
                QByteArray gene = parts[0].trimmed();
                for (int i=1; i<parts.count(); ++i)
                {
                    preferred_transcripts_[gene] << parts[i].trimmed();
                }
            }
        }

        initialized = true;
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

QString GSvarHelper::applicationBaseName()
{
	return QCoreApplication::applicationDirPath() + "/" + QCoreApplication::applicationName().replace(".exe","");
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
