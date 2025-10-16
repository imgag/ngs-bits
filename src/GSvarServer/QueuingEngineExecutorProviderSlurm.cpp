#include "QueuingEngineExecutorProviderSlurm.h"
#include "Log.h"

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

	//Prepare sbatch arguments
    QStringList sbatch_args;
    if (display_debug) QTextStream(stdout) << "megSAP pipeline:\t " << script << QT_ENDL;
    if (script == "analyze_dragen.php") sbatch_args << "--cpus-per-task=1";
    else sbatch_args << "--cpus-per-task=" + QString::number(threads);
    sbatch_args << "-D" << project_folder;
    sbatch_args << "--mail-type=NONE";
    sbatch_args << "-e" << (slurm_out_base + ".err");
    sbatch_args << "-o" << (slurm_out_base + ".out");

	queues.removeAll("");
	if (!queues.isEmpty()) sbatch_args << "-p" << queues.join(",");

	// Build command line for wrapped job
	QString command = "php " + PipelineSettings::rootDir()+"/src/Pipelines/" + script;

    if (!job_args.isEmpty())
    {
		command += " " + job_args;
    }
	command += " " + pipeline_args.join(" ");

	// Create bash script for command
	QString command_sh = slurm_out_base + "_command.sh";

	QFile file(command_sh);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream out(&file);
		out << "#!/bin/sh\n";    // Shebang
		out << command << "\n";  // The actual command
		file.close();
		QFile::setPermissions(command_sh, QFileDevice::ExeUser | QFileDevice::ReadUser | QFileDevice::WriteUser |
										   QFileDevice::ReadGroup | QFileDevice::ReadOther);
	}
	else QTextStream(stderr) << "Failed to write command script: " << command_sh << "\n";

	sbatch_args << command_sh;

	// Debug output
    if (display_debug) QTextStream(stdout) << "Slurm command:\t sbatch " << sbatch_args.join(" ") << QT_ENDL;

	// Execute sbatch
    output.command = "sbatch";
    output.args = sbatch_args;
    output.exit_code = Helper::executeCommand(output.command, sbatch_args, &output.result);
	Log::info(output.command + " " + output.args.join(" "));
    return output;
}

QueuingEngineOutput QueuingEngineExecutorProviderSlurm::checkJobsForAllUsers() const
{
    QueuingEngineOutput output;
    output.command = "squeue";
    output.args = QStringList();
    output.exit_code = Helper::executeCommand(output.command, output.args, &output.result);
    Log::info(output.command + " " + output.args.join(" "));
    return output;
}

QueuingEngineOutput QueuingEngineExecutorProviderSlurm::checkJobDetails(QString job_id) const
{
    QueuingEngineOutput output;
    output.command = "squeue";
    output.args = QStringList() << "-j" << job_id;
    output.exit_code = Helper::executeCommand(output.command, output.args);
    Log::info(output.command + " " + output.args.join(" "));
    return output;
}

QueuingEngineOutput QueuingEngineExecutorProviderSlurm::checkCompletedJob(QString job_id) const
{
    QueuingEngineOutput output;
    output.command = "sacct";
    output.args = QStringList() << "-j" << job_id;
    output.exit_code = Helper::executeCommand(output.command, output.args, &output.result);
    Log::info(output.command + " " + output.args.join(" "));
    return output;
}

QueuingEngineOutput QueuingEngineExecutorProviderSlurm::deleteJob(QString job_id) const
{
    QueuingEngineOutput output;
    output.command = "scancel";
    output.args = QStringList() << job_id;
    output.exit_code = Helper::executeCommand(output.command, output.args, &output.result);
    Log::info(output.command + " " + output.args.join(" "));
    return output;
}
