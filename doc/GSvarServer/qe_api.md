# Using a custom queuing engine via HTTP API

## HTTP API server implementation guidelines

In addition to Slurm and SGE, other queuing engines are support using a generic HTTP API.
To start analysis jobs, get the status of the jobs and cancel jobs, the GSvar server uses the API definded by the settings entry `qe_api_base_url`.  
In addition to `qe_api_base_url`, you need to set `qe_secure_token`, an authentication token for the API server (issued and validated by the API), to be able to access the API.  
The API must accept **POST** requests containing specific JSON objects for each of the three actions: submit, update and delete.  
For each action there is a predefined JSON object format:

1. `submit` (**POST** request) - Submits a new job
    
    ```
    {
        "action": "submit",
        "token": security token for authentication,
        "threads": number of threads to be used for running the analysis,
        "queues": [list of queues the analysis can run in],
        "script": megSAP pipeline script,
        "script_args": [list of command line arguments for the pipeline script],
        "working_directory": working directory the script is executed in
    }
    ```
    __returns__ JSON object 
    ```
    {
        "result": log output of the attempt to start the job,
        "qe_job_id": queuing engine job id (max 10 characters),
        "exit_code": numeric job exit code (0 means job was started. Everything else means there was an error while starting the job)
    }
    ```

    __http status code__ 200 - always return 200. Failure is detected from 'exit_code'.

2. `update` (**POST** request) - Used to get the status of a started job and write it into NGSD
    ```
    {
        "action": "update",
        "token": security token for authentication,
        "qe_job_id": queuing engine job id,
    }
    ```
    __returns__ JSON object 
    ```
    {
        "result": log output of the job,
        "status": must be one of 'started' (job is started and still running), 'finished' (job is successfully finished) or 'error' (job failed),
        "queue", "queue identifier where the job is running or was running (can be empty if not running yet)",
    }
    ```
    
    __http status code__ 200 - always return 200. Failure is detected from 'exit_code'.

4. `delete` (**POST** request) - Deletes a job
    ```
    {
        "action": "delete",
        "token": security token for authentication,
        "qe_job_id": queuing engine job id
    }
    ```
    __returns__ JSON object 
    ```
    {
        "result":  log output of the attempt to delete the job,
        "exit_code": numeric job exit code (0 means the job was deleted. Everything else means there was an error why trying to delete the job)
    }
    ```

    __http status code__ 200 - always return 200. Failure is detected from 'exit_code'.


## Set your custom queuing engine as the default one in megSAP

Set the following line to "generic" in your megSAP settings.ini

```bash
queuing_engine = "generic"
```
    

--

[back to main page](../index.md)
