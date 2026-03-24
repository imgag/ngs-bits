#ifndef GENOMEDATA_H
#define GENOMEDATA_H

#include "cppVISUAL_global.h"
#include "Transcript.h"
#include "FastaFileIndex.h"
#include "ChromosomalIndex.h"
#include "BedFile.h"

#include <QSize>

//Helper struct for globally used settings
struct CPPVISUALSHARED_EXPORT GlobalSettings
{
	//Width of the label area on the left of panels
	int label_width  = 165;
	//If only GENCODE primary are shown in GenePanel (also relevant for search)
	bool show_only_primary = true;
	//Minimum number of bases to show
	int min_window_size = 40;
	//How many bases genes/transcripts are padded with, e.g. after search
	int transcript_padding = 2000;
};

//Singleton for data shared by all widgets for visualization.
//Also emits signals if the data changes, so that widgets can react accordingly.
class CPPVISUALSHARED_EXPORT SharedData
	: public QObject
{
	Q_OBJECT

public:
	//Returns the genome index
	static const FastaFileIndex& genome()
	{
		return instance()->genome_index_;
	}

	//Return transcripts
	static const TranscriptList& transcripts()
	{
		return instance()->transcripts_;
	}
	//Sets transcripts and creates/updates indices
	static void setTranscripts(const TranscriptList& transcripts);

	//Returns indices of transcripts in a region
	static QVector<int> transcriptsInRegion(const Chromosome& chr, int start, int end)
	{
		return  instance()->transcripts_index_.matchingIndices(chr, start, end);
	}

	//Return the global settings
	static const GlobalSettings& settings()
	{
		return  instance()->settings_;
	}
	static void setSettings(const GlobalSettings& settings)
	{
		instance()->settings_ = settings;
		emit instance()->settingsChanged();
	}

	//Return the currently displayed region
	static const BedLine& region()
	{
		return  instance()->region_;
	}
	static void setRegion(const Chromosome& chr, int start, int end);

	//Returns the size of a character
	static const QSize& characterSize()
	{
		return  instance()->char_size_;
	}

	static void displayError(QString msg)
	{
		emit instance()->displayErrorReq(msg);
	}

	//Returns the instance (creates it on first call). This method is public only to connect signal/slots. For all other purposes, use other methods.
	static SharedData* instance();

signals:
	void transcriptsChanged();
	void settingsChanged();
	void regionChanged();
	void displayErrorReq(QString);

protected:
	explicit SharedData(QObject* parent = nullptr);
	Q_DISABLE_COPY(SharedData)
	QSize determineCharacterSize();

	FastaFileIndex genome_index_;
	TranscriptList transcripts_;
	ChromosomalIndex<TranscriptList> transcripts_index_;
	GlobalSettings settings_;
	BedLine region_;
	QSize char_size_;
};



#endif // GENOMEDATA_H
