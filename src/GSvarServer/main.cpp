#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QCommandLineParser>
#include "HttpsServer.h"
#include "ServerHelper.h"
#include "EndpointController.h"
#include "EndpointHandler.h"

int log_level = 3;

QFile gsvar_server_log_file("gsvar-server-log.txt");

void interceptLogMessage(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
	QString time_stamp = QDate::currentDate().toString("dd/MM/yyyy") + " " + QTime::currentTime().toString("hh:mm:ss:zzz");
	QString log_statement = "";
	int msg_level = 0;
	switch (type) {
		case QtCriticalMsg:
			msg_level = 0;
			log_statement = QString("%1 - [Critical] %2").arg(time_stamp, msg);
			break;
		case QtFatalMsg:
			msg_level = 0;
			log_statement = QString("%1 - [Fatal] %2").arg(time_stamp, msg);
			break;
		case QtInfoMsg:
			msg_level = 1;
			log_statement = QString("%1 - [Info] %2").arg(time_stamp, msg);
			break;
		case QtWarningMsg:
			msg_level = 2;
			log_statement = QString("%1 - [Warning] %2").arg(time_stamp, msg);
			break;
		case QtDebugMsg:
		default:
			msg_level = 3;
			log_statement = QString("%1 - [Debug] %2").arg(time_stamp, msg);
	}

	// Log levels:
	// 0: only critical and fatal
	// 1: += info
	// 2: += warning
	// 3: += debug
	if (msg_level <= log_level)
	{
		printf("%s", qUtf8Printable(log_statement.replace("\"", "")));
		printf("\n");

		QTextStream out_stream(&gsvar_server_log_file);
		out_stream.setCodec("UTF-8");
		out_stream.setGenerateByteOrderMark(false);
		out_stream << log_statement << endl;
	}

	if (type == QtFatalMsg)
	{
		abort();
	}
}

int main(int argc, char **argv)
{
	gsvar_server_log_file.open(QIODevice::WriteOnly | QIODevice::Append);

	QCoreApplication app(argc, argv);

	QCommandLineParser parser;
	parser.setApplicationDescription("GSvar file server");
	parser.addHelpOption();
	parser.addVersionOption();
	QCommandLineOption serverPortOption(QStringList() << "p" << "port",
			QCoreApplication::translate("main", "Server port number"),
			QCoreApplication::translate("main", "port"));
	parser.addOption(serverPortOption);
	QCommandLineOption logLevelOption(QStringList() << "l" << "log",
			QCoreApplication::translate("main", "Log level"),
			QCoreApplication::translate("main", "logging"));
	parser.addOption(logLevelOption);
	parser.process(app);
	QString port = parser.value(serverPortOption);
	QString log_level_option = parser.value(logLevelOption);

	if (!log_level_option.isEmpty())
	{
		qInfo().noquote() << "Log level parameter has been provided through the command line arguments:" + log_level_option;
		log_level = log_level_option.toInt();
	}
	else {
		qInfo().noquote() << "Using log level from the application settings:" + QString::number(ServerHelper::getNumSettingsValue("log_level"));
		log_level = ServerHelper::getNumSettingsValue("log_level");
	}

	qInstallMessageHandler(interceptLogMessage);

	EndpointManager::appendEndpoint(Endpoint{
						"",
						QMap<QString, ParamProps>{},
						RequestMethod::GET,
						ContentType::TEXT_HTML,
						"Index page with general information",
						&EndpointHandler::serveIndexPage
					});
	EndpointManager::appendEndpoint(Endpoint{
						"favicon.ico",
						QMap<QString, ParamProps>{},
						RequestMethod::GET,
						ContentType::IMAGE_PNG,
						"Favicon to avoid warnings from the browser",
						&EndpointHandler::serveFavicon
					});
	EndpointManager::appendEndpoint(Endpoint{
						"info",
						QMap<QString, ParamProps>{},
						RequestMethod::GET,
						ContentType::APPLICATION_JSON,
						"General information about this API",
						&EndpointHandler::serveApiInfo
					});
	EndpointManager::appendEndpoint(Endpoint{
						"static",
						QMap<QString, ParamProps>{
						   {"filename", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::PATH_PARAM, true, "Name of the file to be served"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_HTML,
						"Static content served from the server root folder (defined in the config file)",
						&EndpointController::serveStaticFromServerRoot
				   });

	EndpointManager::appendEndpoint(Endpoint{
						"static",
						QMap<QString, ParamProps>{
						   {"filename", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::PATH_PARAM, false, "Name of the file to be served"}}
						},
						RequestMethod::HEAD,
						ContentType::TEXT_HTML,
						"Size of the static content served from the server root folder (defined in the config file)",
						&EndpointController::serveStaticFromServerRoot
				   });

	EndpointManager::appendEndpoint(Endpoint{
						"cache",
						QMap<QString, ParamProps>{
						   {"filename", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::PATH_PARAM, false, "Name of the file to be served"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_HTML,
						"Static content served from the server cache",
						&EndpointController::serveStaticFileFromCache
				   });
	EndpointManager::appendEndpoint(Endpoint{
						"temp",
						QMap<QString, ParamProps>{
						   {"id", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::PATH_PARAM, false, "Unique id pointing to a file"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_HTML,
						"Static file served via secure temporary URL",
						&EndpointController::serveStaticForTempUrl
				   });

	EndpointManager::appendEndpoint(Endpoint{
						"temp",
						QMap<QString, ParamProps>{
							{"id", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::PATH_PARAM, false, "Unique id pointing to a folder"}},
							{"filename", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::PATH_PARAM, true, "Filename in a folder with a temporary URL"}}
						},
						RequestMethod::HEAD,
						ContentType::TEXT_HTML,
						"Size of the static file served via secure temporary URL",
						&EndpointController::serveStaticForTempUrl
				   });

	EndpointManager::appendEndpoint(Endpoint{
						"help",
						QMap<QString, ParamProps>{
						   {"endpoint", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::PATH_PARAM, true,
							"Endpoint path the help is requested for. Help for all endpoints wiil be provided, if this parameter is ommited"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_HTML,
						"Help page on the usage of the endpoints",
						&EndpointController::serveEndpointHelp
					});

	EndpointManager::appendEndpoint(Endpoint{
						"file_location",
						QMap<QString, ParamProps> {
						   {"ps", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::GET_URL_PARAM, false, "Sample id"}},
						   {"type", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::GET_URL_PARAM, true, "Format of the requested file(s)"}},
						   {"path", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::GET_URL_PARAM, true, "Returns an absolute path on the server, if set to 'absolute'"}},
						   {"locus", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::GET_URL_PARAM, true, "Locus for repeat expansion image"}},
						   {"return_if_missing", ParamProps{ParamProps::ParamType::INTEGER, ParamProps::ParamCategory::GET_URL_PARAM, true, "Return file info, if the file is missing"}}
						},
						RequestMethod::GET,
						ContentType::APPLICATION_JSON,
						"Retrieve file location information for specific file types",
						&EndpointHandler::locateFileByType
					});

	EndpointManager::appendEndpoint(Endpoint{
						"project_file",
						QMap<QString, ParamProps> {
						   {"ps", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::GET_URL_PARAM, false, "Sample id"}},
						   {"multi", ParamProps{ParamProps::ParamType::INTEGER, ParamProps::ParamCategory::GET_URL_PARAM, true, "Perform multi search"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_PLAIN,
						"Temporary URL leading to a specific project file (GSvar file)",
						&EndpointHandler::locateProjectFile
					});

	EndpointManager::appendEndpoint(Endpoint{
						"file_info",
						QMap<QString, ParamProps> {
						   {"file", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::GET_URL_PARAM, false, "Filename with its full path"}}
						},
						RequestMethod::GET,
						ContentType::APPLICATION_JSON,
						"Detailed information about a specific file",
						&EndpointController::getFileInfo
					});


	EndpointManager::appendEndpoint(Endpoint{
						"ps_regions",
						QMap<QString, ParamProps> {
						   {"sys_id", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::GET_URL_PARAM, false, "Processing system id"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_PLAIN,
						"Processing system regions",
						&EndpointHandler::getProcessingSystemRegions
					});

	EndpointManager::appendEndpoint(Endpoint{
						"ps_amplicons",
						QMap<QString, ParamProps> {
						   {"sys_id", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::GET_URL_PARAM, false, "Processing system id"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_PLAIN,
						"Processing system amplicons",
						&EndpointHandler::getProcessingSystemAmplicons
					});

	EndpointManager::appendEndpoint(Endpoint{
						"ps_genes",
						QMap<QString, ParamProps> {
						   {"sys_id", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::GET_URL_PARAM, false, "Processing system id"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_PLAIN,
						"Processing system genes",
						&EndpointHandler::getProcessingSystemGenes
					});

	EndpointManager::appendEndpoint(Endpoint{
						"login",
						QMap<QString, ParamProps>{
							{"name", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::POST_URL_ENCODED, false, "User name"}},
							{"password", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::POST_URL_ENCODED, false, "Password"}}
						},
						RequestMethod::POST,
						ContentType::TEXT_PLAIN,
						"Secure token generation, the token will be used to access protected resources and to perform  certain API calls",
						&EndpointHandler::performLogin
					});
	EndpointManager::appendEndpoint(Endpoint{
						"logout",
						QMap<QString, ParamProps>{
							{"token", ParamProps{ParamProps::ParamType::STRING, ParamProps::ParamCategory::POST_URL_ENCODED, false, "Secure token received after a successful login"}}
						},
						RequestMethod::POST,
						ContentType::TEXT_PLAIN,
						"Secure token invalidation, after this step the token cannot longer be used",
						&EndpointHandler::performLogout
					});

	int port_number = ServerHelper::getNumSettingsValue("server_port");

	if (!port.isEmpty())
	{
		qInfo() << "Server port has been provided through the command line arguments:" + port;
		port_number = port.toInt();
	}
	else {
		qInfo() << "Using port number from the application settings";
	}

	HttpsServer sslserver(port_number);
	return app.exec();
}
