#include "Application.h"
#include "Exceptions.h"
#include "ToolBase.h"
#include "GUIHelper.h"

Application::Application(int& argc, char** argv)
	: QApplication(argc, argv)
{
	QCoreApplication::setApplicationVersion(ToolBase::version());
}

bool Application::notify(QObject* rec, QEvent* ev)
{
	try
	{
		return QApplication::notify(rec,ev);
	}
	catch(Exception& e)
	{
		QMap<QString, QString> add;
		add.insert("file", e.file());
		add.insert("line", QString::number(e.line()));
		GUIHelper::showMessage("Uncaught cppCORE exception", e.message(), add);
		exit(1);
	}
	catch(std::exception& e)
	{
		GUIHelper::showMessage("Uncaught std::exception", e.what());
		exit(1);
	}
	catch(...)
	{
		GUIHelper::showMessage("Uncaught unknown exception", "Unknown exception type!");
	}

	return false;
}
