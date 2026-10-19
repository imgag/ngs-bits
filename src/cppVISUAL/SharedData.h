#ifndef GENOMEDATA_H
#define GENOMEDATA_H

#include "cppVISUAL_global.h"
#include "Transcript.h"
#include "FastaFileIndex.h"
#include "ChromosomalIndex.h"
#include "BedFile.h"
#include "ParameterList.h"

#include <QXmlStreamWriter>
#include <QDomDocument>

//Fast read-only parameters access; populated and maintained by ParameterList::link().
struct CPPVISUALSHARED_EXPORT GlobalSettings : public QObject
{
	Q_OBJECT
	Q_PROPERTY(int label_width MEMBER label_width)
	Q_PROPERTY(int min_window_size MEMBER min_window_size)
	Q_PROPERTY(int transcript_padding MEMBER transcript_padding)
	Q_PROPERTY(int bam_max_visibility_region MEMBER bam_max_visibility_region)
	Q_PROPERTY(double bam_insert_size_lower_percentile MEMBER bam_insert_size_lower_percentile)
	Q_PROPERTY(double bam_insert_size_upper_percentile MEMBER bam_insert_size_upper_percentile)

public:
	int label_width{};
	int min_window_size{};
	int transcript_padding{};
	int bam_max_visibility_region{};
	double bam_insert_size_lower_percentile{};
	double bam_insert_size_upper_percentile{};
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
	//Sets transcripts and creates/updates indices. Do this after initializing GenomeVisualizationWidget!
	static void setTranscripts(const TranscriptList& transcripts);

	//Returns indices of transcripts in a region
	static QVector<int> transcriptsInRegion(const Chromosome& chr, int start, int end)
	{
		return  instance()->transcripts_index_.matchingIndices(chr, start, end);
	}

	//Returns editable global parameters. Changes synchronize the fast settings and emit settingsChanged.
	static ParameterList& parameters() { return instance()->settings_; }

	//Returns fast read-only access to global settings
	static const GlobalSettings& settings()
	{
		return instance()->fast_settings_;
	}

	//Writes one Settings element per global parameter, inside the General element.
	static void writeSettingsToXml(QXmlStreamWriter& writer);
	//Requires all global settings and applies them atomically after validation.
	static void loadSettingsFromXml(const QDomElement& general);

	//Return the currently displayed region
	static const BedLine& region()
	{
		return  instance()->region_;
	}
	static void setRegion(const Chromosome& chr, int start, int end);

	//Returns the instance (creates it on first call). This method is public only to connect signal/slots. For all other purposes, use other methods.
	static SharedData* instance();

	//Updates genomic coordinate in status bar
	static void mouseMoved(int x, int width);

signals:
	void transcriptsChanged();
	void settingsChanged();
	void regionChanged();
	void bookmarksChanged();
	void updateGenomicCoordinate(QString);

protected:
	explicit SharedData(QObject* parent = nullptr);
	Q_DISABLE_COPY(SharedData)

	FastaFileIndex genome_index_;
	TranscriptList transcripts_;
	ChromosomalIndex<TranscriptList> transcripts_index_;
	ParameterList settings_;
	GlobalSettings fast_settings_;
	BedLine region_;

private:
	static QList<Parameter> getParameters();
	static QHash<QByteArray, QVariant> getParameterDefaults();
	void updateSettingsConstraints();
};



#endif // GENOMEDATA_H
