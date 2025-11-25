#include "BlatInitWorker.h"
#include "Log.h"
#include "HttpRequestHandler.h"
#include "ProxyDataService.h"
#include "Helper.h"
#include "Settings.h"
#include <QDir>
#include <QProcess>
#include <QCoreApplication>

BlatInitWorker::BlatInitWorker(int port, QString server_folder)
    : QRunnable()
    , port_(port)
    , server_folder_(server_folder)
{
    Log::info("Trying to initialize the BLAT server from the provided location '" + server_folder_ + "' at port " + QString::number(port_));
}

void BlatInitWorker::run()
{
    try
    {


        if (!prepareBlatServer())
        {
            Log::error("Could not initialize and start the BLAT server on port " + QString::number(port_));
            return;
        }

        QProcess *process = new QProcess();
        process->setProcessChannelMode(QProcess::MergedChannels);

        QString program = server_folder_ + "/gfServer";
        QStringList args = {"start", "localhost", QString::number(port_), "-stepSize=5", "-log=" + QCoreApplication::applicationDirPath() + "/blat/untrans.log", QCoreApplication::applicationDirPath() + "/blat/hg38.2bit"};

        QObject::connect(process, &QProcess::readyReadStandardOutput, [process]()
        {
            Log::info(QString::fromUtf8(process->readAllStandardOutput()));
        });

        QObject::connect(process, &QProcess::readyReadStandardError, [process]()
        {
            Log::error(QString::fromUtf8(process->readAllStandardError()));
        });

        #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QObject::connect(process, &QProcess::finished, [process](int exitCode, QProcess::ExitStatus status)
            {
                Log::info("Process finished with exit code " + QString::number(exitCode) + ", status: " + (status == QProcess::NormalExit ? "normal exit" : "crashed"));
            });
        #else
            Log::warn("The server has been built with Qt 5");
        #endif

        process->start(program, args);
        if (!process->waitForStarted())
        {
            Log::error("Failed to start BLAT server");
            return;
        }

        Log::info("BLAT server has been started on the port " + QString::number(port_) + " with the PID " + QString::number(process->processId()));
    }
    catch (FileAccessException& e)
    {
        Log::error("File access problem has been detected while initialazing the BLAT server: " + e.message());
    }
    catch (Exception& e)
    {
        Log::error("An error has been detected while initialazing the BLAT server: " + e.message());
    }
    catch (...)
    {
        Log::error("Could not initialize the BLAT server: unknown error");
        return;
    }
}

bool BlatInitWorker::prepareBlatServer()
{
    QStringList blat_files;
    blat_files << "https://hgdownload.soe.ucsc.edu/admin/exe/linux.x86_64/faToTwoBit";
    blat_files << "https://hgdownload.soe.ucsc.edu/admin/exe/linux.x86_64/blat/blat";
    blat_files << "https://hgdownload.soe.ucsc.edu/admin/exe/linux.x86_64/blat/gfServer";
    blat_files << "https://hgdownload.soe.ucsc.edu/admin/exe/linux.x86_64/blat/gfClient";

    QDir dir;
    if (!dir.exists(server_folder_))
    {
        if (dir.mkpath(server_folder_))
        {
            Log::info("Created BLAT folder: " + server_folder_);
        }
        else
        {
            Log::error("Could not create BLAT folder: " + server_folder_);
        }
    } else
    {
        Log::info("BLAT folder already exists: " + server_folder_);
    }

    for (QString current_url: blat_files)
    {
        QUrl url(current_url);
        QString current_file = server_folder_ + "/" + url.fileName();
        if (QFile::exists(current_file))
        {
            Log::info("Found " + current_file);
            continue;
        }

        Log::info("Downloading " + current_url + " into " + current_file);
        QSharedPointer<QFile> downloaded_file = Helper::openFileForWriting(current_file);

        QNetworkProxy proxy = QNetworkProxy::NoProxy;
        if(!ProxyDataService::isConnected())
        {
            const QNetworkProxy& proxy = ProxyDataService::getProxy();
            if(proxy.type() != QNetworkProxy::HttpProxy)
            {
                Log::error("No connection to the internet! Please check your proxy settings.");
                return false;
            }
            if(proxy.hostName().isEmpty() || (proxy.port() < 1))
            {
                Log::error("HTTP proxy without reqired host name or port provided!");
                return false;
            }

            //final check of the connection
            if(!ProxyDataService::isConnected())
            {
                Log::error("No connection to the internet! Please check your proxy settings.");
                return false;
            }
        }

        // set proxy for the file download, if needed
        proxy = ProxyDataService::getProxy();
        if (proxy!=QNetworkProxy::NoProxy)
        {
            Log::info("Using Proxy: " + proxy.hostName());
        }

        try
        {
            HttpHeaders add_headers;
            QByteArray file_content = HttpRequestHandler(proxy).get(current_url, add_headers).body;
            downloaded_file->write(file_content);
            downloaded_file->close();
            Log::info(current_file + " has been saved");
        }
        catch(Exception e)
        {
            Log::error("File download has failed: " + e.message());
            return false;
        }

        QFileDevice::Permissions permissions = downloaded_file->permissions();
        permissions |= QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther;
        bool ok = downloaded_file->setPermissions(permissions);
        if (!ok)
        {
            Log::error("Failed to set +x permission for " + current_file);
            return false;
        }
    }

    // check if hg38.2bit is present
    QString genome_file = server_folder_ + "/hg38.2bit";
    if (QFile::exists(genome_file))
    {
        Log::info("Found the genome " + genome_file);
    }
    else
    {
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);

        Log::info("Converting FA into 2BIT: " + Settings::string("reference_genome"));
        process.start(server_folder_ + "/faToTwoBit", QStringList() << Settings::string("reference_genome") << server_folder_ + "/hg38.2bit");

        bool success = process.waitForFinished(-1);
        if (!success || process.exitCode()>0)
        {
            Log::error("Conversion error: exit code " + QString::number(process.exitCode()) + ", " + process.readAll());
            return false;
        }
    }

    return true;
}
