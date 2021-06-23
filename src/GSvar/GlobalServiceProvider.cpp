#include "GlobalServiceProvider.h"

#include "Settings.h"
#include "DatabaseServiceLocal.h"
#include "MainWindow.h"

GlobalServiceProvider::GlobalServiceProvider()
  : file_location_provider_()
  , database_service_()
{
	if (Settings::string("server_host",true).trimmed()!="" && Settings::string("server_port").trimmed()!="")
	{
		//TODO GSvarServer
	}
	else
	{
		database_service_ = QSharedPointer<DatabaseService>(new DatabaseServiceLocal());
	}
}

GlobalServiceProvider::~GlobalServiceProvider()
{
}

GlobalServiceProvider& GlobalServiceProvider::instance()
{
	static GlobalServiceProvider instance;

	return instance;
}

void GlobalServiceProvider::setFileLocationProvider(QSharedPointer<FileLocationProvider> file_location_provider)
{
	instance().file_location_provider_ = file_location_provider;
}

const FileLocationProvider& GlobalServiceProvider::fileLocationProvider()
{	
	if (instance().file_location_provider_.isNull())
	{
		THROW(ProgrammingException, "File location provider requested but not set!");
	}

	return *(instance().file_location_provider_);
}

void GlobalServiceProvider::clearFileLocationProvider()
{
	instance().file_location_provider_ = QSharedPointer<FileLocationProvider>();
}

const DatabaseService& GlobalServiceProvider::database()
{
	return *(instance().database_service_);
}

void GlobalServiceProvider::openProcessedSampleTab(QString processed_sample_name)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->openProcessedSampleTab(processed_sample_name);
		}
	}
}

void GlobalServiceProvider::openRunTab(QString run_name)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->openRunTab(run_name);
		}
	}
}

void GlobalServiceProvider::openGeneTab(QString symbol)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->openGeneTab(symbol);
		}
	}
}

void GlobalServiceProvider::openVariantTab(Variant variant)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->openVariantTab(variant);
		}
	}
}

void GlobalServiceProvider::openProjectTab(QString project_name)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->openProjectTab(project_name);
		}
	}
}

void GlobalServiceProvider::openProcessingSystemTab(QString system_short_name)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->openProcessingSystemTab(system_short_name);
		}
	}
}

void GlobalServiceProvider::gotoInIGV(QString region, bool init_if_not_done)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->executeIGVCommands(QStringList() << "goto " + region, init_if_not_done);
		}
	}
}

void GlobalServiceProvider::loadFileInIGV(QString filename, bool init_if_not_done)
{
	//normalize local files
	if (!filename.startsWith("http"))
	{
		filename = Helper::canonicalPath(filename);
	}

	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->executeIGVCommands(QStringList() << "load \"" + filename + "\"", init_if_not_done);
		}
	}
}

void GlobalServiceProvider::openGSvarViaNGSD(QString processed_sample_name, bool search_multi)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->openProcessedSampleFromNGSD(processed_sample_name, search_multi);
		}
	}
}
