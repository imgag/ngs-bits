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

struct BamAlignmentWrapper
{
	QString id; // for hashing
	BamAlignment alignment;

	BamAlignmentWrapper(BamAlignment aln)
		: id(makeId(aln)), alignment(aln)
	{
	}

	BamAlignmentWrapper(BamAlignment&& aln)
		: id(makeId(aln)), alignment(std::move(aln))
	{
	}

	bool operator==(const BamAlignmentWrapper& other) const
	{
		return id == other.id;
	}

	QString makeId(const BamAlignment& al)
	{
		return QString("%1_%2_%3_%4")
			.arg(al.name())
			.arg(al.start())
			.arg(al.isReverseStrand())
			.arg(al.cigarDataAsString());
	}
};

inline size_t qHash(const BamAlignmentWrapper& key, size_t seed = 0)
{
	return qHash(key.id, seed);
}

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

	const QVector<BamAlignmentWrapper>& getAlignments()
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
	QVector<BamAlignmentWrapper> alignments_;
	Sequence ref_seq_;

	void updateData();
};

#endif // TRACKDATA_H
