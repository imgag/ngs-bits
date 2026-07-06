#include "FileLoader.h"
#include "IgvTrack.h"
#include "BedTrack.h"
#include "BamAlignmentTrack.h"
#include "BamCoverageTrack.h"
#include "GenomeVisualizationWidget.h"

TrackWidgetList FileLoader::loadTracks(QString file_path, QWidget* parent)
{
	if (file_path.endsWith(".bed")) return loadBedFileTracks(file_path, parent);
	if (file_path.endsWith(".bam") || file_path.endsWith(".cram")) return loadBamFileTracks(file_path, parent);
	if (file_path.endsWith(".igv")) return loadIgvFileTracks(file_path, parent);

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
	BamCoverageTrack* cov_track	   = BamCoverageTrack::createTrack(parent, file_path, info.fileName());
	if (cov_track) out << cov_track;
	if (align_track) out << align_track;

	return out;
}

TrackWidgetList FileLoader::loadIgvFileTracks(QString file_path, QWidget* parent)
{
	TrackWidgetList out;
	QFileInfo info(file_path);
	IgvTrack* baf_track = IgvTrack::createTrack(parent, file_path, "");
	if (baf_track) out << baf_track;
	return out;
}

QSharedPointer<BedFile> FileLoader::loadBedFile(QString file_path)
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
		bedfile->sort();
		return bedfile;
	}
	catch (const Exception& e)
	{
		GenomeVisualizationWidget::displayError(e.message());
		return nullptr;
	}
}

QSharedPointer<BedFile> FileLoader::loadIgvFile(QString file_path)
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

		if (!isValidIgvFile(bedfile))
		{
			GenomeVisualizationWidget::displayError(file_path + " is not a valid IGV file (header does not contain 5 columns)");
			return nullptr;
		}

		bedfile->sort();
		return bedfile;
	}
	catch (const Exception& e)
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
	catch (const Exception& e)
	{
		GenomeVisualizationWidget::displayError(e.message());
		return nullptr;
	}
}

//TODO: this should check more things, i.e. if the 5th column has numeric values
// if the 4th column is Features
bool FileLoader::isValidIgvFile(QSharedPointer<BedFile> bed_file)
{
	if (!bed_file) return false; // redundant check but avoids accidental crashes

	bool is_valid = true;

	foreach (const QByteArray& header, bed_file->headers())
	{
		if (header.startsWith("#")) continue;
		QList<QByteArray> columns = header.split('\t');

		if (columns.count() < 5) is_valid = false;
	}

	return is_valid & !bed_file->headers().empty();
}
