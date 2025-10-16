#include "QueuingEngineExecutorProviderSge.h"
#include "Log.h"

QueuingEngineExecutorProviderSge::QueuingEngineExecutorProviderSge()
{
}

QString QueuingEngineExecutorProviderSge::getEngineName() const
{
    return "SGE";
}

QueuingEngineOutput QueuingEngineExecutorProviderSge::submitJob(int threads, QStringList queues, QStringList pipeline_args, QString project_folder, QString script, QString job_args, QString job_id, bool display_debug) const
{
    QueuingEngineOutput output;

    QString sge_out_base = PipelineSettings::dataFolder() + "/sge/megSAP_sge_job_" + job_id;
    QStringList qsub_args;
    qsub_args << "-V";
    if (display_debug) QTextStream(stdout) << "megSAP pipeline:\t " << script << QT_ENDL;
    if (script == "analyze_dragen.php") qsub_args << "-pe" << "smp" << "1";
    else qsub_args << "-pe" << "smp" << QString::number(threads);
    qsub_args << "-b" << "y";
    qsub_args << "-wd" << project_folder;
    qsub_args << "-m" << "n";
    qsub_args << "-e" << (sge_out_base + ".err");
    qsub_args << "-o" << (sge_out_base + ".out");
    qsub_args << "-q" << queues.join(",");
    qsub_args << "php";
    qsub_args << PipelineSettings::rootDir()+"/src/Pipelines/"+script;
    // QString job_args = job.args.simplified();
    if (!job_args.isEmpty())
    {
        qsub_args << job_args.split(' ');
    }
    qsub_args << pipeline_args;

    if (display_debug) QTextStream(stdout) << "SGE command:\t qsub " << qsub_args.join(" ") << QT_ENDL;

    output.command = "qsub";
    output.args = qsub_args;
    output.exit_code = Helper::executeCommand(output.command, qsub_args, &output.result);
    Log::info(output.command + " " + output.args.join(" "));
    return output;
}

QueuingEngineOutput QueuingEngineExecutorProviderSge::checkJobsForAllUsers() const
{
    QueuingEngineOutput output;
    output.command = "qstat";
    output.args = QStringList() << "-u" << "*";
    output.exit_code = Helper::executeCommand(output.command, output.args, &output.result);
    Log::info(output.command + " " + output.args.join(" "));
    return output;
}

QueuingEngineOutput QueuingEngineExecutorProviderSge::checkJobDetails(QString job_id) const
{
    QueuingEngineOutput output;
    output.command = "qstat";
    output.args = QStringList() << "-j" << job_id;
	output.exit_code = Helper::executeCommand(output.command, output.args, &output.result);
    Log::info(output.command + " " + output.args.join(" "));
    return output;
}

QueuingEngineOutput QueuingEngineExecutorProviderSge::checkCompletedJob(QString job_id) const
{
    QueuingEngineOutput output;
    output.command = "qacct";
    output.args = QStringList() << "-j" << job_id;
    output.exit_code = Helper::executeCommand(output.command, output.args, &output.result);
    Log::info(output.command + " " + output.args.join(" "));
    return output;
}

QueuingEngineOutput QueuingEngineExecutorProviderSge::deleteJob(QString job_id) const
{
    QueuingEngineOutput output;
    output.command = "qdel";
    output.args = QStringList() << job_id;
    output.exit_code = Helper::executeCommand(output.command, output.args, &output.result);
    Log::info(output.command + " " + output.args.join(" "));
    return output;
}
