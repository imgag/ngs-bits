#include "GenomeVisualizationWidget.h"
#include "TrackGroupManager.h"
#include "ParameterEditor.h"
#include "Settings.h"
#include "SharedData.h"
#include "BedTrack.h"
#include "FileLoader.h"
#include "IgvTrack.h"
#include "BamAlignmentTrack.h"
#include "BamCoverageTrack.h"
#include "BamTrackDataManager.h"
#include "BamWriter.h"
#include <QScopeGuard>
#include <QDialog>
#include <QFontMetrics>
#include <QImage>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QDir>
#include <QUuid>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtTest>

class GenomeVisualizationWidget_Test : public QObject
{
	Q_OBJECT

private:
	QDir data_{QDir::currentPath() + "/test-data-" + QUuid::createUuid().toString(QUuid::WithoutBraces)};

	void writeFile(const QString& name, const QByteArray& contents)
	{
		QFile file(data_.filePath(name));
		QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.errorString()));
		QCOMPARE(file.write(contents), contents.size());
	}

	void openEditor(TrackWidget* track)
	{
		emit track->trackSelected(track);
		for (QWidget* parent = track->parentWidget(); parent; parent = parent->parentWidget())
		{
			if (auto* viewer = qobject_cast<GenomeVisualizationWidget*>(parent))
			{
				if (viewer->findChild<QDockWidget*>("settings_dock")->isHidden())
				{
					viewer->findChild<QToolButton*>("settings_btn")->click();
				}
				return;
			}
		}
		QFAIL("Track is not inside a genome visualization widget");
	}

private slots:
	void initTestCase()
	{
		QVERIFY(QDir().mkpath(data_.path()));
		writeFile("reference.fa", ">chr22\n" + QByteArray(100, 'A') + "\n");
		writeFile("reference.fa.fai", "chr22\t100\t7\t100\t101\n");
		writeFile("settings.ini", "reference_genome=" + data_.filePath("reference.fa").toUtf8() + "\n");
		Settings::setSettingsOverride(data_.filePath("settings.ini"));
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		writeFile("first.bed", "chr22\t10\t20\n");
		writeFile("second.bed", "chr22\t30\t40\n");
	}

	void igvNumericRecordsAndMetadata()
	{
		const QByteArray headers = "#track name=numeric graphtype=BAR_CHART viewlimits=-2:3 maxheightpixels=80\nChromosome\tStart\tEnd\tFeature\tSignal\n";
		writeFile("numeric.igv", headers
			+ "chr22\t30\t40\tfeature\t0.123456789012345\n"
			+ "chr22\t20\t25\tfeature\tinvalid\n"
			+ "chr22\t15\t18\tfeature\n"
			+ "chr22\t12\t14\n"
			+ "chr22\t0\t10\tfeature\t-1.5\n"
			+ "chr2\t0\t5\tfeature\t2.5\n");
		auto file = FileLoader::loadIgvFile(data_.filePath("numeric.igv"));
		QVERIFY(file);
		QCOMPARE(file->count(), 3);
		QVERIFY(file->isSorted());
		QCOMPARE(file->headers().count(), 2);
		QCOMPARE(file->headers()[0], headers.split('\n')[0]);
		QCOMPARE((*file)[0].chr(), Chromosome("chr2"));
		QCOMPARE((*file)[1].start(), 1);
		QCOMPARE((*file)[1].end(), 10);
		QCOMPARE((*file)[1].value(), -1.5);
		QCOMPARE((*file)[2].value(), 0.123456789012345);
		ChromosomalIndex<IgvFile> index(*file);
		QCOMPARE(index.matchingIndices(Chromosome("chr22"), 10, 10), QVector<int>({1}));
		QVERIFY(index.matchingIndices(Chromosome("chr22"), 11, 30).isEmpty());
		QCOMPARE(index.matchingIndices(Chromosome("chr22"), 31, 40), QVector<int>({2}));

		QScopedPointer<IgvTrack> track(IgvTrack::createTrack(nullptr, data_.filePath("numeric.igv")));
		QVERIFY(track);
		QCOMPARE(track->parameters().getString("draw mode"), QByteArray("bar chart"));
		QCOMPARE(track->parameters().getDouble("min"), -2.0);
		QCOMPARE(track->parameters().getDouble("max"), 3.0);
		QCOMPARE(track->sizeHint().height(), 80);

		writeFile("empty.igv", headers + "chr22\t0\t10\tfeature\tinvalid\n");
		auto empty = FileLoader::loadIgvFile(data_.filePath("empty.igv"));
		QVERIFY(empty);
		QCOMPARE(empty->count(), 0);
		ChromosomalIndex<IgvFile> empty_index(*empty);
		QVERIFY(empty_index.matchingIndices(Chromosome("chr22"), 1, 10).isEmpty());
	}

	void igvPopupUsesRenderedPositionAndBarHeight()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		IgvTrack track(nullptr, "", "");
		track.resize(SharedData::settings().label_width + 4 + 160, 100);
		track.setIgvFile(QSharedPointer<IgvFile>::create(
			QVector<IgvFileLine>{IgvFileLine(Chromosome("chr22"), 10, 70, 0.5)}, QByteArrayList{}));
		track.show();
		const int x0 = SharedData::settings().label_width + 2;
		const QPoint marker(x0 + 79, 50);
		for (const QByteArray& mode : QByteArrayList{"points", "line plot", "heatmap", "bar chart"})
		{
			track.parameters().setString("draw mode", mode);
			QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, QPoint(x0 + 28, 60));
			QVERIFY(!track.findChild<QDialog*>());
			if (mode == "line plot") continue; // A single record has no line segment.
			QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, marker);
			auto* popup = track.findChild<QDialog*>();
			QVERIFY(popup);
			delete popup;
		}
		track.parameters().setString("draw mode", "bar chart");
		track.setIgvFile(QSharedPointer<IgvFile>::create(
			QVector<IgvFileLine>{IgvFileLine(Chromosome("chr22"), 10, 70, 2.0)}, QByteArrayList{}));
		QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, QPoint(marker.x(), 1));
		QVERIFY(!track.findChild<QDialog*>());
		QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, QPoint(marker.x(), 5));
		QVERIFY(track.findChild<QDialog*>());
		delete track.findChild<QDialog*>();

		QSignalSpy selected(&track, &TrackWidget::trackSelected);
		QTest::mouseClick(&track, Qt::RightButton, Qt::NoModifier, marker);
		QCOMPARE(selected.count(), 1);
		selected.clear();
		QTest::mousePress(&track, Qt::LeftButton, Qt::NoModifier, marker);
		QTest::mouseRelease(&track, Qt::LeftButton, Qt::NoModifier, marker + QPoint(20, 0));
		QCOMPARE(selected.count(), 1);
		QVERIFY(!track.findChild<QDialog*>());
	}

	void igvPlotClipsLinesAndPreservesFractionalCells()
	{
		SharedData::setRegion(Chromosome("chr22"), 20, 59);
		IgvTrack track(nullptr, "", "");
		track.resize(SharedData::settings().label_width + 4 + 60, 100);
		track.setIgvFile(QSharedPointer<IgvFile>::create(
			QVector<IgvFileLine>{IgvFileLine(Chromosome("chr22"), 10, 10, 0.5), IgvFileLine(Chromosome("chr22"), 70, 70, 0.5)}, QByteArrayList{}));
		track.parameters().setString("draw mode", "line plot");
		QImage image(track.size(), QImage::Format_ARGB32);
		track.render(&image);
		const int x0 = SharedData::settings().label_width + 2;
		QCOMPARE(image.pixelColor(x0 - 1, 50), QColor(Qt::white));
		QCOMPARE(image.pixelColor(x0 + 60, 50), QColor(Qt::white));
		QCOMPARE(image.pixelColor(x0 + 30, 50), QColor(Qt::blue));

		track.setIgvFile(QSharedPointer<IgvFile>::create(
			QVector<IgvFileLine>{IgvFileLine(Chromosome("chr22"), 27, 27, 1.0)}, QByteArrayList{}));
		for (const QByteArray& mode : QByteArrayList{"heatmap", "bar chart"})
		{
			track.parameters().setString("draw mode", mode);
			track.render(&image);
			// The cell spans x0+10.5 to x0+12: its fractional right edge colors this pixel.
			QColor edge = image.pixelColor(x0 + 12, 30);
			QVERIFY(edge.blue() > edge.red());
		}
	}

	void bamVisibilityRangeIsSharedAndControlsLoading()
	{
		const GlobalSettings original_settings = SharedData::settings();
		const auto restore_settings = qScopeGuard([&]() { SharedData::setSettings(original_settings); });
		GlobalSettings settings = original_settings;
		settings.bam_max_region_len = 40;
		SharedData::setSettings(settings);
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		writeFile("visibility.sam", "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\nread\t0\tchr22\t10\t60\t10M\t=\t10\t0\tAAAAAAAAAA\tIIIIIIIIII\n");
		{
			BamReader reader(data_.filePath("visibility.sam"));
			BamWriter writer(data_.filePath("visibility.bam"));
			writer.writeHeader(reader);
			BamAlignment alignment;
			while (reader.getNextAlignment(alignment)) writer.writeAlignment(alignment);
		}
		QCOMPARE(sam_index_build(data_.filePath("visibility.bam").toUtf8().constData(), 0), 0);
		const QString relative_path = QDir::current().relativeFilePath(data_.filePath("visibility.bam"));
		QScopedPointer<BamAlignmentTrack> alignment(BamAlignmentTrack::createTrack(nullptr, relative_path, "alignments"));
		QScopedPointer<BamCoverageTrack> coverage(BamCoverageTrack::createTrack(nullptr, data_.filePath("visibility.bam"), "coverage"));
		QVERIFY(alignment);
		QVERIFY(coverage);
		auto data = BamTrackDataManager::getOrCreate(relative_path);
		QCOMPARE(data, BamTrackDataManager::getOrCreate(data_.filePath("visibility.bam")));
		QCOMPARE(alignment->parameters().getInt("visibility range"), 40);
		QCOMPARE(coverage->parameters().getInt("visibility range"), 40);
		QVERIFY(data->getAlignments().isEmpty());
		QVERIFY_THROWS_EXCEPTION(ArgumentException, alignment->parameters().setInt("visibility range", 0));

		QSignalSpy changes(data.get(), &BamTrackData::visibilityRangeChanged);
		alignment->parameters().setInt("visibility range", 100);
		QCOMPARE(coverage->parameters().getInt("visibility range"), 100);
		QCOMPARE(data->getAlignments().count(), 1);
		QCOMPARE(changes.count(), 1);
		coverage->parameters().setInt("visibility range", 20);
		QCOMPARE(alignment->parameters().getInt("visibility range"), 20);
		QVERIFY(data->getAlignments().isEmpty());
		coverage->parameters().setInt("visibility range", 100);
		QCOMPARE(alignment->parameters().getInt("visibility range"), 100);
		QCOMPARE(data->getAlignments().count(), 1);
		QImage image(coverage->size(), QImage::Format_ARGB32);
		coverage->render(&image);

		QByteArray xml;
		QXmlStreamWriter writer(&xml);
		alignment->writeToXml(writer);
		QDomDocument doc;
		QVERIFY(doc.setContent(xml));
		coverage->parameters().setInt("visibility range", 20);
		QStringList errors;
		QScopedPointer<TrackWidget> restored(TrackWidget::fromXml(doc.documentElement(), nullptr, errors));
		QVERIFY(restored);
		QVERIFY(errors.isEmpty());
		QCOMPARE(restored->parameters().getInt("visibility range"), 100);
		QCOMPARE(coverage->parameters().getInt("visibility range"), 100);

		settings.bam_max_region_len = 200;
		SharedData::setSettings(settings);
		QCOMPARE(alignment->parameters().getInt("visibility range"), 100);
		BamCoverageTrack other(nullptr, "", "other");
		QCOMPARE(other.parameters().getInt("visibility range"), 200);
		other.parameters().setInt("visibility range", 60);
		QCOMPARE(coverage->parameters().getInt("visibility range"), 100);
	}

	void bamPhaseGroupingAndHitTesting()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		writeFile("phase.sam", "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\n"
			"unassigned\t0\tchr22\t10\t60\t10M\t=\t10\t0\tAAAAAAAAAA\tIIIIIIIIII\n"
			"phase2\t0\tchr22\t10\t60\t10M\t=\t10\t0\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:2\n"
			"phase1\t0\tchr22\t10\t60\t10M\t=\t10\t0\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:1\n"
			"unsupported\t0\tchr22\t40\t60\t10M\t=\t40\t0\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:3\n");
		{
			BamReader reader(data_.filePath("phase.sam"));
			BamWriter writer(data_.filePath("phase.bam"));
			writer.writeHeader(reader);
			BamAlignment al;
			while (reader.getNextAlignment(al)) writer.writeAlignment(al);
		}
		QCOMPARE(sam_index_build(data_.filePath("phase.bam").toUtf8().constData(), 0), 0);
		QScopedPointer<BamAlignmentTrack> track(BamAlignmentTrack::createTrack(nullptr, data_.filePath("phase.bam"), ""));
		QVERIFY(track);
		QCOMPARE(track->parameters().getString("group by"), QByteArray("none"));
		QVERIFY_THROWS_EXCEPTION(ArgumentException, track->parameters().setString("group by", "other"));
		track->parameters().setString("color by", "none");
		track->resize(SharedData::settings().label_width + 4 + 320, 200);
		track->show();
		const int x0 = SharedData::settings().label_width + 2;
		auto checkRead = [&](int row, int pos, const QString& name)
		{
			const QPoint point(x0 + (pos - 1) * 4 + 2, row * 14 + 6);
			QImage image(track->size(), QImage::Format_ARGB32);
			track->render(&image);
			QVERIFY(image.pixelColor(point) != QColor(Qt::white));
			QTest::mouseClick(track.get(), Qt::LeftButton, Qt::NoModifier, point);
			auto* popup = track->findChild<QDialog*>();
			QVERIFY(popup);
			QVERIFY(popup->findChild<QLabel*>()->text().contains("<b>" + name + "</b>"));
			delete popup;
		};
		for (bool pairs : {false, true})
		{
			track->parameters().setBool("view as pairs", pairs);
			track->parameters().setString("group by", "phase");
			checkRead(0, 15, "phase1");
			checkRead(2, 15, "phase2");
			checkRead(4, 15, "unassigned");
			checkRead(4, 45, "unsupported");
			QTest::mouseClick(track.get(), Qt::LeftButton, Qt::NoModifier, QPoint(x0 + 58, 20));
			QVERIFY(!track->findChild<QDialog*>());
			track->parameters().setString("group by", "none");
			checkRead(0, 15, "unassigned");
			checkRead(1, 15, "phase2");
			checkRead(2, 15, "phase1");
		}

		writeFile("phase-pairs.sam", "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\n"
			"assigned-mate\t65\tchr22\t10\t60\t10M\t=\t30\t30\tAAAAAAAAAA\tIIIIIIIIII\n"
			"phase2\t0\tchr22\t10\t60\t10M\t=\t10\t0\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:2\n"
			"conflict\t65\tchr22\t10\t60\t10M\t=\t30\t30\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:1\n"
			"assigned-mate\t129\tchr22\t30\t60\t10M\t=\t10\t-30\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:1\n"
			"conflict\t129\tchr22\t30\t60\t10M\t=\t10\t-30\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:2\n");
		{
			BamReader reader(data_.filePath("phase-pairs.sam"));
			BamWriter writer(data_.filePath("phase-pairs.bam"));
			writer.writeHeader(reader);
			BamAlignment al;
			while (reader.getNextAlignment(al)) writer.writeAlignment(al);
		}
		QCOMPARE(sam_index_build(data_.filePath("phase-pairs.bam").toUtf8().constData(), 0), 0);
		track.reset(BamAlignmentTrack::createTrack(nullptr, data_.filePath("phase-pairs.bam"), ""));
		QVERIFY(track);
		track->parameters().setString("color by", "none");
		track->parameters().setString("group by", "phase");
		track->resize(SharedData::settings().label_width + 4 + 320, 200);
		track->show();
		checkRead(0, 15, "assigned-mate");
		checkRead(0, 35, "assigned-mate");
		checkRead(2, 15, "phase2");
		checkRead(4, 15, "conflict");
		checkRead(4, 35, "conflict");
	}

	void startupEmbeddedInMainWindow()
	{
		QMainWindow window;
		auto* central = new QWidget(&window);
		auto* layout = new QVBoxLayout(central);
		auto* viewer = new GenomeVisualizationWidget(central);
		layout->addWidget(viewer);
		window.setCentralWidget(central);
		auto* dock_host = viewer->findChild<QMainWindow*>("dock_window");
		QVERIFY(dock_host);
		QVERIFY(!dock_host->isWindow());
		window.show();
		QCoreApplication::processEvents();
		QVERIFY(viewer->isVisible());
		QVERIFY(dock_host->isVisibleTo(viewer));
		QVERIFY(viewer->findChild<QLineEdit*>("search")->isVisibleTo(viewer));
		QVERIFY(viewer->findChild<TrackGroupManager*>()->isVisibleTo(viewer));
		QVERIFY(dock_host->width() > 0);
		QVERIFY(dock_host->height() > 0);
		viewer->findChild<QToolButton*>("settings_btn")->click();
		auto* placeholder = viewer->findChild<QLabel*>("settings_placeholder");
		QVERIFY(placeholder->isVisible());
		QCOMPARE(placeholder->text(), QString("Select a track to show settings"));
		viewer->loadFile(data_.filePath("first.bed"));
		openEditor(viewer->findChild<TrackWidget*>());
		QCoreApplication::processEvents();
		auto* dock = viewer->findChild<QDockWidget*>("settings_dock");
		QVERIFY(dock->isVisibleTo(viewer));
		dock->close();
		QCoreApplication::processEvents();
		QVERIFY(dock->isHidden());
		QVERIFY(viewer->findChild<TrackGroupManager*>()->isVisibleTo(viewer));
	}

	void trackSelectionAndCollapse()
	{
		GenomeVisualizationWidget viewer(nullptr);
		viewer.show();
		QCoreApplication::processEvents();
		auto* dock = viewer.findChild<QDockWidget*>("settings_dock");
		QVERIFY(dock);
		QVERIFY(dock->isHidden());
		viewer.loadFile(data_.filePath("first.bed"));
		viewer.loadFile(data_.filePath("second.bed"));
		auto tracks = viewer.findChildren<TrackWidget*>();
		QCOMPARE(tracks.size(), 2);
		auto* first = tracks[0];
		auto* second = tracks[1];
		second->parameters().setString("color", "red");
		openEditor(first);
		QVERIFY(!dock->isHidden());
		QCOMPARE(dock->windowTitle(), QString("Track settings"));
		auto* editor = dock->findChild<ParameterEditor*>();
		QVERIFY(editor);
		QCOMPARE(editor->findChild<QLabel*>("title")->text(), "Track: " + first->displayName());
		editor->findChild<QComboBox*>("color")->setCurrentText("green");
		QVERIFY(editor->store());
		QCOMPARE(first->parameters().getString("color"), QByteArray("green"));
		QCOMPARE(second->parameters().getString("color"), QByteArray("red"));
		editor->findChild<QCheckBox*>("auto_apply")->setChecked(true);
		QTest::mouseClick(second, Qt::LeftButton, Qt::NoModifier, QPoint(2, 2));
		QCOMPARE(dock->findChild<ParameterEditor*>(), editor);
		QCOMPARE(editor->findChild<QLabel*>("title")->text(), "Track: " + second->displayName());
		QCOMPARE(editor->findChild<QComboBox*>("color")->currentText(), QString("red"));
		QVERIFY(editor->findChild<QCheckBox*>("auto_apply")->isChecked());
		editor->findChild<QComboBox*>("color")->setCurrentText("yellow");
		QCOMPARE(second->parameters().getString("color"), QByteArray("yellow"));
		QCOMPARE(first->parameters().getString("color"), QByteArray("green"));
		dock->close();
		QVERIFY(dock->isHidden());
		openEditor(first);
		QVERIFY(!dock->isHidden());
		viewer.findChild<QToolButton*>("settings_btn")->click();
		QVERIFY(dock->isHidden());
		viewer.findChild<QToolButton*>("settings_btn")->click();
		QVERIFY(!dock->isHidden());
	}

	void removedTrackAndClearedSession()
	{
		GenomeVisualizationWidget viewer(nullptr);
		viewer.loadFile(data_.filePath("first.bed"));
		auto* track = viewer.findChild<TrackWidget*>();
		openEditor(track);
		emit track->trackDeleted();
		QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
		auto* dock = viewer.findChild<QDockWidget*>("settings_dock");
		QVERIFY(!dock->findChild<ParameterEditor*>());
		QVERIFY(!dock->isHidden());
		QVERIFY(!viewer.findChild<QLabel*>("settings_placeholder")->isHidden());
		viewer.loadFile(data_.filePath("second.bed"));
		openEditor(viewer.findChild<TrackWidget*>());
		viewer.clearSession();
		QVERIFY(!dock->findChild<ParameterEditor*>());
		QCOMPARE(dock->windowTitle(), QString("Track settings"));
	}

	void bedTrackVisibleRowsAndHitTesting()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		auto bed = QSharedPointer<BedFile>::create();
		bed->append(BedLine(Chromosome("chr22"), 10, 20, {"first"}));
		bed->append(BedLine(Chromosome("chr22"), 20, 30, {"second"}));
		BedTrack track(nullptr, "", "BED");
		track.setBedFile(bed);
		QFont region_font = track.font();
		region_font.setPointSize(8);
		const int row_height = 10 + QFontMetrics(region_font).height() + 2;
		QCOMPARE(track.sizeHint().height(), 2 * row_height);
		track.resize(600, track.sizeHint().height());
		track.show();
		QCoreApplication::processEvents();
		const int x = SharedData::settings().label_width + 2 + int(24.5 * (600 - SharedData::settings().label_width - 4) / 80);
		QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, QPoint(x, row_height + 5));
		auto* popup = track.findChild<QDialog*>();
		QVERIFY(popup);
		const QString text = popup->findChild<QLabel*>()->text();
		QVERIFY(text.contains("second"));
		QVERIFY(!text.contains("first"));
		delete popup;
		track.parameters().setBool("show region names", false);
		QCOMPARE(track.sizeHint().height(), 25);
		SharedData::setRegion(Chromosome("chr22"), 50, 100);
		QCOMPARE(track.sizeHint().height(), 25);
		track.parameters().setBool("show region names", true);
		QCOMPARE(track.sizeHint().height(), 25);
	}

	void bedTrackInvalidInputAndEmptyPopup()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		BedTrack track(nullptr, "", "BED");
		track.parameters().setString("draw mode", "collapsed");
		track.resize(600, 25);
		track.show();
		QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, QPoint(300, 5));
		QVERIFY(!track.findChild<QDialog*>());
		QVERIFY_THROWS_EXCEPTION(ArgumentException, track.setBedFile({}));
		auto bed = QSharedPointer<BedFile>::create();
		bed->append(BedLine(Chromosome("chr22"), 10, 20));
		track.setBedFile(bed);
		auto unsorted = QSharedPointer<BedFile>::create();
		unsorted->append(BedLine(Chromosome("chr22"), 30, 40));
		unsorted->append(BedLine(Chromosome("chr22"), 10, 20));
		QVERIFY_THROWS_EXCEPTION(ArgumentException, track.setBedFile(unsorted));
		//The previous data must remain usable after rejected input.
		QImage image(track.size(), QImage::Format_ARGB32);
		track.render(&image);
		track.setMinimumWidth(0);
		track.resize(SharedData::settings().label_width + 4, 25);
		QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, QPoint(SharedData::settings().label_width + 2, 5));
		QVERIFY(!track.findChild<QDialog*>());
	}
};

QTEST_MAIN(GenomeVisualizationWidget_Test)
#include "GenomeVisualizationWidget_Test.moc"
