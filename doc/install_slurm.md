# Install and Configure Slurm Queuing Engine for GSvarServer

## Base Installation

To install and configure **Slurm**, follow the instructions in the official documentation:
[https://slurm.schedmd.com/quickstart_admin.html](https://slurm.schedmd.com/quickstart_admin.html)

Below is an example of a **minimal `slurm.conf`** configuration file suitable for use with **GSvarServer**.

```bash
# General settings
ClusterName=slurm-cluster
SlurmctldHost=localhost
MpiDefault=none
ReturnToService=2
SlurmctldPort=6817
SlurmdPort=6818
SlurmdSpoolDir=/var/spool/slurmd
SlurmUser=slurm
StateSaveLocation=/var/spool/slurmctld
SwitchType=switch/none
TaskPlugin=task/none
ProctrackType=proctrack/linuxproc
SchedulerType=sched/backfill

# Define compute resources
NodeName=localhost Sockets=1 CoresPerSocket=48 ThreadsPerCore=2 RealMemory=257408
PartitionName=example_partition Nodes=localhost Default=YES MaxTime=INFINITE State=UP

# Log location for slurmctld
SlurmctldLogFile=/var/log/slurm/slurmctld.log
```

---

## Enable Slurm Accounting Storage

To allow **GSvarServer** to retrieve all required job status information, Slurm accounting must be enabled.

Job accounting data is stored in a **SlurmDBD** (Slurm Database Daemon).
Although Slurm can store accounting data in text files, **GSvarServer** has only been tested with a SlurmDBD setup.

For full details, refer to:
[https://slurm.schedmd.com/accounting.html](https://slurm.schedmd.com/accounting.html)

---

### Quick Setup Guide

1. **Install dependencies**

   Ensure the following are installed:

   * **MariaDB/MySQL**
   * **SlurmDBD**
   * **MUNGE**

2. **Create a database and user**

   ```sql
   CREATE DATABASE slurm_acct_db;
   CREATE USER 'slurm'@'localhost' IDENTIFIED BY 'StrongPasswordHere';
   GRANT ALL PRIVILEGES ON slurm_acct_db.* TO 'slurm'@'localhost';
   FLUSH PRIVILEGES;
   EXIT;
   ```

3. **Create the SlurmDBD configuration file**
   Location: `<sysconfdir>/slurmdbd.conf`

   Example:

   ```bash
   # SlurmDBD configuration file

   AuthType=auth/munge
   DbdHost=localhost
   DbdPort=6819

   SlurmUser=slurm
   StorageType=accounting_storage/mysql
   StorageHost=localhost
   StoragePort=3306
   StorageUser=slurm
   StoragePass=StrongPasswordHere
   StorageLoc=slurm_acct_db

   PidFile=/var/run/slurmdbd/slurmdbd.pid
   LogFile=/var/log/slurm/slurmdbd.log
   ```

4. **Fix ownership and permissions**

   ```bash
   sudo chown slurm: /etc/slurm/slurmdbd.conf
   sudo chmod 600 /etc/slurm/slurmdbd.conf
   ```

5. **Enable accounting in `<sysconfdir>/slurm.conf`**

   Add the following lines:

   ```bash
   # Enable Accounting
   JobAcctGatherType=jobacct_gather/linux
   JobAcctGatherFrequency=30
   AccountingStorageType=accounting_storage/slurmdbd
   AccountingStorageHost=localhost
   AccountingStoragePort=6819
   ```

6. **Start and enable the services**

   ```bash
   sudo systemctl enable slurmdbd
   sudo systemctl start slurmdbd
   sudo systemctl status slurmdbd

   sudo systemctl restart slurmctld
   ```

7. **Check logs**

   ```bash
   sudo tail -f /var/log/slurm/slurmdbd.log
   sudo tail -f /var/log/slurm/slurmctld.log
   ```

8. **Test job accounting**

   ```bash
   sbatch --wrap="whoami"
   sacct
   ```

9. **Set Slurm as default queuing engine in megSAP**

    Set the following line to "slurm" in your megSAP settings.ini

    ```bash
    queuing_engine = "slurm"
    ```