#include "QueuingEngineExecutorProviderSlurm.h"

QueuingEngineExecutorProviderSlurm::QueuingEngineExecutorProviderSlurm()
{

}

QString QueuingEngineExecutorProviderSlurm::getEngineName() const
{
    return "SLURM";
}

QueuingEngineOutput QueuingEngineExecutorProviderSlurm::submitJob(int threads, QStringList queues, QStringList pipeline_args, QString project_folder, QString script, QString job_args, QString job_id, bool display_debug) const
{
    QueuingEngineOutput output;

    QString slurm_out_base = PipelineSettings::dataFolder() + "/slurm/megSAP_slurm_job_" + job_id;
    QStringList sbatch_args;

    if (display_debug) QTextStream(stdout) << "megSAP pipeline:\t " << script << QT_ENDL;
    if (script == "analyze_dragen.php") sbatch_args << "--cpus-per-task=1";
    else sbatch_args << "--cpus-per-task=" + QString::number(threads);
    sbatch_args << "-D" << project_folder;
    sbatch_args << "--mail-type=NONE";
    sbatch_args << "-e" << (slurm_out_base + ".err");
    sbatch_args << "-o" << (slurm_out_base + ".out");
    sbatch_args << "-p" << queues.join(",");
    sbatch_args << "php";
    sbatch_args << PipelineSettings::rootDir()+"/src/Pipelines/"+script;

    if (!job_args.isEmpty())
    {
        sbatch_args << job_args.split(' ');
    }
    sbatch_args << pipeline_args;

    if (display_debug) QTextStream(stdout) << "Slurm command:\t sbatch " << sbatch_args.join(" ") << QT_ENDL;

    output.command = "sbatch";
    output.args = sbatch_args;
    output.exit_code = Helper::executeCommand(output.command, sbatch_args, &output.result);

    return output;
}

QueuingEngineOutput QueuingEngineExecutorProviderSlurm::checkJobsForAllUsers() const
{
    QueuingEngineOutput output;
    output.command = "squeue";
    output.args = QStringList();
    output.exit_code = Helper::executeCommand(output.command, output.args, &output.result);

    return output;
}

QueuingEngineOutput QueuingEngineExecutorProviderSlurm::checkJobDetails(QString job_id) const
{
    QueuingEngineOutput output;
    output.command = "squeue";
    output.args = QStringList() << "-j" << job_id;
    output.exit_code = Helper::executeCommand(output.command, output.args);

    return output;
}

QueuingEngineOutput QueuingEngineExecutorProviderSlurm::checkCompletedJob(QString job_id) const
{
    QueuingEngineOutput output;
    output.command = "sacct";
    output.args = QStringList() << "-j" << job_id;
    output.exit_code = Helper::executeCommand(output.command, output.args, &output.result);

    return output;
}

QueuingEngineOutput QueuingEngineExecutorProviderSlurm::deleteJob(QString job_id) const
{
    QueuingEngineOutput output;
    output.command = "scancel";
    output.args = QStringList() << job_id;
    output.exit_code = Helper::executeCommand(output.command, output.args, &output.result);

    return output;
}
