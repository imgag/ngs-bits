#include "GenomeVisualizationWidget.h"
#include "TrackGroupManager.h"
#include "ParameterEditor.h"
#include "Settings.h"
#include "SharedData.h"
#include "BedTrack.h"
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
