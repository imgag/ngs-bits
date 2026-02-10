# Install and Configure Sun Grid Engine (SGE) for GSvarServer

## Base Installation

Install Sun Grid Engine using your distribution packages (e.g. gridengine, sge, or uge), or follow your vendor’s installation guide. More information can be found [here](https://gridscheduler.sourceforge.net/htmlman/manuals.html)

After installation, ensure the following SGE components are running:

* __qmaster__

* __execd__ (on compute nodes)

Verify with:
```
qstat -g c
```

## Minimal SGE Configuration

GSvarServer requires standard job submission and accounting support.

Key requirements:

* A working queue

* At least one execution host

* Accounting enabled (default in SGE)

Example checks:
```
qconf -sql        # list queues
qconf -sel        # list execution hosts
```

## Enable and Verify Accounting

Ensure accounting is enabled:
```
qconf -sconf | grep accounting
```

If disabled, enable it:
```
qconf -mconf
accounting=true
```

Restart SGE services after changes.

## Test Job Submission and Accounting

Submit a test job:
```
qsub -b y whoami
```

Verify job history:
```
qacct
```

GSvarServer uses qstat and qacct to retrieve job status and runtime information.

## Set the following line to "sge" in your megSAP settings.ini

```bash
queuing_engine = "sge"
```

Restart GSvarServer after updating the configuration.

## Notes

* No external database is required for SGE accounting.

* Ensure GSvarServer runs as a user with permission to execute qsub, qstat, and qacct.

--

[back to main page](index.md)