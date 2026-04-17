#ifndef TRACKDATA_H
#define TRACKDATA_H

#include "SharedData.h"
#include "BamReader.h"

#include <QObject>
#include <QString>

enum TrackType
{
	BED,
	BAM_CRAM
};

struct TrackData
{
	QString file_path;
	QString name;

	TrackData(QString file_path, QString name)
		: file_path(file_path), name(name)
	{
	}
};

struct BedTrackData : public TrackData
{
	QSharedPointer<BedFile> bed_file;
};

/*this is used for connected BamAlignemnt and BamCoverage tracks*/
class BamTrackData : public QObject, public TrackData
{
	Q_OBJECT
public:
	BamTrackData(QString file_path, QString name)
		: TrackData(file_path, name)
	{
		connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(updateRegion()));
	}

	const QVector<BamAlignment>& getAlignments()
	{
		return alignments_;
	}

	const Sequence& getReferenceSeq()
	{
		return ref_seq_;
	}

	void setBamReader(QSharedPointer<BamReader> reader)
	{
		bam_reader_ = reader;
		updateRegion();
	}

signals:
	void onDataUpdate();

public slots:
	void updateRegion();

private:
	QSharedPointer<BamReader> bam_reader_;
	QVector<BamAlignment> alignments_;
	Sequence ref_seq_;

	void updateData();
};

#endif // TRACKDATA_H
