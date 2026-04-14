#include "FileLoader.h"
#include "BedTrack.h"
#include "BamAlignmentTrack.h"
#include "BamCoverageTrack.h"
#include "GenomeVisualizationWidget.h"

TrackWidgetList FileLoader::loadTracks(QString file_path, QWidget* parent)
{
	if (file_path.endsWith(".bed")) return loadBedFileTracks(file_path, parent);
	if (file_path.endsWith(".bam") || file_path.endsWith(".cram")) return loadBamFileTracks(file_path, parent);

	GenomeVisualizationWidget::displayError(file_path + ": Unsupported file type.");
	return TrackWidgetList();
}

TrackWidgetList FileLoader::loadBedFileTracks(QString file_path, QWidget* parent)
{
	TrackWidgetList out;
	const QFileInfo info(file_path);

	BedTrack* bed_track = BedTrack::createTrack(parent, file_path, info.fileName());
	if (bed_track) out << bed_track;

	return out;
}

TrackWidgetList FileLoader::loadBamFileTracks(QString file_path, QWidget* parent)
{
	TrackWidgetList out;
	QFileInfo info(file_path);
	QSharedPointer<BamReader> reader = loadBamFile(file_path);
	if (reader)
	{
		BamAlignmentTrack* align_track = new BamAlignmentTrack(parent, file_path, info.fileName());
		BamCoverageTrack* cov_track	   = new BamCoverageTrack(parent, file_path, info.fileName() + " Coverage");
		QSharedPointer<BamTrackData> track_data = QSharedPointer<BamTrackData>::create(file_path, info.fileName());
		cov_track->setTrackData(track_data);
		align_track->setTrackData(track_data);
		track_data->setBamReader(reader);
		out << cov_track << align_track;
	}
	return out;
}

QSharedPointer<BedFile> FileLoader::loadBedFile(QString file_path)
{
	const QFileInfo info(file_path);
	if (!info.isFile())
	{
		GenomeVisualizationWidget::displayError(file_path + " not found");
		return nullptr;
	}
	try
	{
		QSharedPointer<BedFile> bedfile = QSharedPointer<BedFile>::create();
		bedfile->load(file_path);
		bedfile->sort();
		return bedfile;
	}
	catch (const FileParseException& e)
	{
		GenomeVisualizationWidget::displayError(e.message());
		return nullptr;
	}
}

QSharedPointer<BamReader> FileLoader::loadBamFile(QString file_path)
{
	const QFileInfo info(file_path);
	if (!info.isFile())
	{
		GenomeVisualizationWidget::displayError(file_path + " not found");
		return nullptr;
	}
	try
	{
		QSharedPointer<BamReader> reader = QSharedPointer<BamReader>::create(file_path);
		return reader;
	}
	catch (const FileParseException& e)
	{
		GenomeVisualizationWidget::displayError(e.message());
		return nullptr;
	}
}
