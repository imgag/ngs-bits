#include "GenomeVisualizationWidget.h"
#include "TrackGroupManager.h"
#include "ParameterEditor.h"
#include "Settings.h"
#include "SharedData.h"
#include <QCheckBox>
#include <QDockWidget>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
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
		QMenu menu;
		track->populateContextMenu(menu, QPoint(1, 1));
		for (QAction* action : menu.actions())
		{
			if (action->text() == "Edit track settings...")
			{
				action->trigger();
				return;
			}
		}
		QFAIL("Missing settings context menu entry");
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
		viewer->loadFile(data_.filePath("first.bed"));
		openEditor(viewer->findChild<TrackWidget*>());
		QCoreApplication::processEvents();
		auto* dock = viewer->findChild<QDockWidget*>("settings_dock");
		QVERIFY(dock->isVisibleTo(viewer));
		QVERIFY(viewer->findChild<QToolButton*>("settings_btn")->isChecked());
		dock->close();
		QCoreApplication::processEvents();
		QVERIFY(!viewer->findChild<QToolButton*>("settings_btn")->isChecked());
		QVERIFY(viewer->findChild<TrackGroupManager*>()->isVisibleTo(viewer));
	}

	void contextMenuSelectionAndCollapse()
	{
		GenomeVisualizationWidget viewer(nullptr);
		auto* dock = viewer.findChild<QDockWidget*>("settings_dock");
		QVERIFY(dock);
		QVERIFY(dock->isHidden());
		viewer.loadFile(data_.filePath("first.bed"));
		viewer.loadFile(data_.filePath("second.bed"));
		auto tracks = viewer.findChildren<TrackWidget*>();
		QCOMPARE(tracks.size(), 2);
		auto* first = tracks[0];
		auto* second = tracks[1];
		second->parameters().setInt("color", 123);
		openEditor(first);
		QVERIFY(!dock->isHidden());
		QVERIFY(dock->windowTitle().contains(first->displayName()));
		auto* editor = dock->findChild<ParameterEditor*>();
		QVERIFY(editor);
		editor->findChild<QLineEdit*>("color")->setText("456");
		QVERIFY(editor->store());
		QCOMPARE(first->parameters().getInt("color"), 456);
		QCOMPARE(second->parameters().getInt("color"), 123);
		editor->findChild<QCheckBox*>("auto_apply")->setChecked(true);
		QTest::mouseClick(second, Qt::LeftButton, Qt::NoModifier, QPoint(2, 2));
		QCOMPARE(dock->findChild<ParameterEditor*>(), editor);
		QVERIFY(dock->windowTitle().contains(second->displayName()));
		QCOMPARE(editor->findChild<QLineEdit*>("color")->text(), QString("123"));
		QVERIFY(editor->findChild<QCheckBox*>("auto_apply")->isChecked());
		editor->findChild<QLineEdit*>("color")->setText("789");
		QCOMPARE(second->parameters().getInt("color"), 789);
		QCOMPARE(first->parameters().getInt("color"), 456);
		dock->close();
		QVERIFY(dock->isHidden());
		openEditor(first);
		QVERIFY(!dock->isHidden());
		viewer.findChild<QToolButton*>("settings_btn")->setChecked(true);
		viewer.findChild<QToolButton*>("settings_btn")->setChecked(false);
		QVERIFY(dock->isHidden());
		viewer.findChild<QToolButton*>("settings_btn")->setChecked(true);
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
		QVERIFY(!viewer.findChild<QLabel*>("settings_placeholder")->isHidden());
		viewer.loadFile(data_.filePath("second.bed"));
		openEditor(viewer.findChild<TrackWidget*>());
		viewer.clearSession();
		QVERIFY(!dock->findChild<ParameterEditor*>());
		QCOMPARE(dock->windowTitle(), QString("Track settings"));
	}
};

QTEST_MAIN(GenomeVisualizationWidget_Test)
#include "GenomeVisualizationWidget_Test.moc"
