#include "FileLoader.h"
#include "BafTrack.h"
#include "BedTrack.h"
#include "BamAlignmentTrack.h"
#include "BamCoverageTrack.h"
#include "GenomeVisualizationWidget.h"

TrackWidgetList FileLoader::loadTracks(QString file_path, QWidget* parent)
{
	if (file_path.endsWith(".bed")) return loadBedFileTracks(file_path, parent);
	if (file_path.endsWith(".bam") || file_path.endsWith(".cram")) return loadBamFileTracks(file_path, parent);
	if (file_path.endsWith(".baf") || file_path.endsWith(".igv")) return loadBafFileTracks(file_path, parent);

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
	BamAlignmentTrack* align_track = BamAlignmentTrack::createTrack(parent, file_path, info.fileName());
	BamCoverageTrack* cov_track	   = BamCoverageTrack::createTrack(parent, file_path, info.fileName() + " Coverage");
	if (cov_track) out << cov_track;
	if (align_track) out << align_track;

	return out;
}

TrackWidgetList FileLoader::loadBafFileTracks(QString file_path, QWidget* parent)
{
	TrackWidgetList out;
	QFileInfo info(file_path);
	BafTrack* baf_track = BafTrack::createTrack(parent, file_path, info.fileName());
	if (baf_track) out << baf_track;
	return out;
}

QSharedPointer<BedFile> FileLoader::loadBedFile(QString file_path)
{
	// static QHash<QString, QWeakPointer<BedFile>> cache;

	const QFileInfo info(file_path);

	const QString abs_path = info.absoluteFilePath();

	// if (cache.contains(abs_path) && !reload) return cache[abs_path];

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
		// cache[abs_path] = bedfile;
		return bedfile;
	}
	catch (const FileParseException& e)
	{
		GenomeVisualizationWidget::displayError(e.message());
		return nullptr;
	}
}

QSharedPointer<BedFile> FileLoader::loadBafFile(QString file_path)
{

	const QFileInfo info(file_path);

	const QString abs_path = info.absoluteFilePath();

	if (!info.isFile())
	{
		GenomeVisualizationWidget::displayError(file_path + " not found");
		return nullptr;
	}
	try
	{
		QSharedPointer<BedFile> bedfile = QSharedPointer<BedFile>::create();
		bedfile->load(file_path);

		bool contains_baf_column = false;
		foreach (const QByteArray& header, bedfile->headers())
		{
			contains_baf_column |= (header.contains("BAF") && !header.startsWith('#'));
		}

		if (!contains_baf_column)
		{
			GenomeVisualizationWidget::displayError(file_path + " does not contain BAF column");
			return nullptr;
		}

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
	// /*TODO it might be good to have this*/ static QHash<QString, QWeakPointer<BamReader>> cache;

	const QFileInfo info(file_path);

	const QString abs_path = info.absoluteFilePath();

	// if (cache.contains(abs_path) && !reload)
	// {
	// 	auto existing = cache[abs_path].lock();
	// 	if (existing) return existing;
	// }

	if (!info.isFile())
	{
		GenomeVisualizationWidget::displayError(file_path + " not found");
		return nullptr;
	}
	try
	{
		QSharedPointer<BamReader> reader = QSharedPointer<BamReader>::create(file_path);
		// cache[abs_path] = reader.toWeakRef();
		return reader;
	}
	catch (const FileParseException& e)
	{
		GenomeVisualizationWidget::displayError(e.message());
		return nullptr;
	}
}
