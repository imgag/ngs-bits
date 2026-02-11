# Using a custom queuing engine via HTTP API

## HTTP API server implementation guidelines

We have created `QueuingEngineControllerGeneric` class to allow working with a remote queuing engine via a simple HTTP API. Using this class, you can send commands to the server defined in `qe_api_base_url` configuration parameter. In addition to that, you need to set `qe_secure_token`, an authentication token for the API server (issued and validated by the very same server), to be able to access the API. Such a server should have an endpoint that accepts **POST** requests containing specific JSON objects. 4 actions are supported at the moment: submit, update, check, and delete. For each of them there is a predefined JSON object format:

1. `submit` (**POST** request) - Submits a new job
    
    ```
    {
        "action": "submit",
        "token": "security token to allow the action",
        "threads": number of threads to be used for running the analyisis,
        "queues": [list of queues],
        "script": pipeline script,
        "pipeline_args": [list of command line arguments for the pipeline script],
        "working_directory": folder where the sample is stored
    }
    ```
    __returns__ JSON object 
    ```
    {
        "result": "some text on succes or error message on failure",
        "qe_job_id": "text job identifier used by the queuing engine",
        "cmd_exit_code": command submition exit code (0 means success, -1 means failure)
    }
    ```

    __http status code__ 200 - success

2. `update` (**POST** request) - Updates the status of a running job
    ```
    {
        "action": "update",
        "token": "security token to allow the action",
        "qe_job_id": queuing engine job id,
        "qe_job_queue": queuing engine job queue  
    }
    ```
    __returns__ JSON object 
    ```
    {
        "result": "some text on succes or error message on failure",
        "status": should be "queued/running" on successful start,
		"queue", "queue identifier",
        "qe_job_id": "text job identifier used by the queuing engine",
        "cmd_exit_code": command submition exit code (0 means success, -1 means failure)
    }
    ```
    
    __http status code__ 200 - success

3. `check` (**POST** request) - Performs job accounting after completion
    ```
    {
        "action": "check",
        "token": "security token to allow the action",
        "qe_job_id": queuing engine job id,
        "stdout_stderr": standard output that contains error messages (if there were errors)
    }
    ```
    __returns__ JSON object 
    ```
    {
        "result": "some text on succes or error message on failure",
        "qe_exit_code": queuing engine job execution exit code (0 means success, -1 means failure),
        "qe_job_id": "text job identifier used by the queuing engine",
        "cmd_exit_code": command submition exit code (0 means success, -1 means failure)
    }
    ```

    __http status code__ 200 - success

4. `delete` (**POST** request) - Deletes a job
    ```
    {
        "action": "delete",
        "token": "security token to allow the action",
        "qe_job_id": queuing engine job id,
        "qe_job_type": queuing engine job type (single sample/trio/multi sample/somatic),        
    }
    ```
    __returns__ JSON object 
    ```
    {
        "result": "some text on succes or error message on failure",
        "qe_job_id": "text job identifier used by the queuing engine",
        "cmd_exit_code": command submition exit code (0 means success, -1 means failure)
    }
    ```

    __http status code__ 200 - success
     
Please pay attention to the HTTP codes and to error messages returned by the endpoint. It will help to process the output correctly.

## Set your custom queuing engine as the default one in megSAP

Set the following line to "generic" in your megSAP settings.ini

```bash
queuing_engine = "generic"
```
    

--

[back to main page](../index.md)
