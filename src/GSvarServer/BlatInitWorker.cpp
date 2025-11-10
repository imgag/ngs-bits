#include "BlatInitWorker.h"
#include "Log.h"
#include "HttpRequestHandler.h"
#include "ProxyDataService.h"
#include "Helper.h"
#include "Settings.h"
#include <QDir>
#include <QProcess>
#include <QCoreApplication>

BlatInitWorker::BlatInitWorker(int port)
    : QRunnable()
    , port_(port)
{
    Log::info("Trying to initialize the BLAT server at port " + QString::number(port_));
}

void BlatInitWorker::run()
{
    if (!prepareBlatServer())
    {
        Log::error("Could not initialize and start the BLAT server on port " + QString::number(port_));
        return;
    }

    QProcess *process = new QProcess();
    process->setProcessChannelMode(QProcess::MergedChannels);   

    QString program = QCoreApplication::applicationDirPath() + "/blat/gfServer";
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

bool BlatInitWorker::prepareBlatServer()
{
    QStringList blat_files;
    blat_files << "https://hgdownload.soe.ucsc.edu/admin/exe/linux.x86_64/faToTwoBit";
    blat_files << "https://hgdownload.soe.ucsc.edu/admin/exe/linux.x86_64/blat/blat";
    blat_files << "https://hgdownload.soe.ucsc.edu/admin/exe/linux.x86_64/blat/gfServer";
    blat_files << "https://hgdownload.soe.ucsc.edu/admin/exe/linux.x86_64/blat/gfClient";

    QString blat_folder = QCoreApplication::applicationDirPath() + "/blat";
    QDir dir;
    if (!dir.exists(blat_folder))
    {
        if (dir.mkpath(blat_folder))
        {
            Log::info("Created BLAT folder: " + blat_folder);
        }
        else
        {
            Log::error("Could not create BLAT folder: " + blat_folder);
            return false;
        }
    } else
    {
        Log::info("BLAT folder already exists: " + blat_folder);
    }

    for (QString current_url: blat_files)
    {
        QUrl url(current_url);
        QString current_file = blat_folder + "/" + url.fileName();
        if (QFile::exists(current_file))
        {
            Log::info("Found " + current_file);
            continue;
        }

        Log::info("Downloading " + current_url);
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
    QString genome_file = QCoreApplication::applicationDirPath() + "/blat/hg38.2bit";
    if (QFile::exists(genome_file))
    {
        Log::info("Found the genome " + genome_file);
    }
    else
    {
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);

        Log::info("Converting FA into 2BIT: " + Settings::string("reference_genome"));
        process.start(QCoreApplication::applicationDirPath() + "/blat/faToTwoBit", QStringList() << Settings::string("reference_genome") << QCoreApplication::applicationDirPath() + "/blat/hg38.2bit");

        bool success = process.waitForFinished(-1);
        if (!success || process.exitCode()>0)
        {
            Log::error("Conversion error: exit code " + QString::number(process.exitCode()) + ", " + process.readAll());
            return false;
        }
    }

    return true;
}
