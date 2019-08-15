#include "GSvarHelper.h"
#include "Helper.h"

GeneSet GSvarHelper::imprinting_genes_ = GeneSet();
QMap<QString, QStringList> GSvarHelper::preferred_transcripts_ = QMap<QString, QStringList>();

const GeneSet& GSvarHelper::impritingGenes()
{
	static bool initialized = false;
	if (!initialized)
	{
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

const QMap<QString,QStringList>& GSvarHelper::preferredTranscripts()
{
	static bool initialized = false;
	if (!initialized)
	{
		//TODO
		initialized = true;
	}
	return preferred_transcripts_;
}
