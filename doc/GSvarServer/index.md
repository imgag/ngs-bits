# GSvarServer

The GSvar server is the backend for the GSvar client.

It has to following functionality:

* It has a built-in web server to provide access to sample/analysis files to the GSvar client and IGV (so the Linux file system must not be mounted on Windows PCs the run GSvar). 
* It can perform calculation on the data, without transfering the data to the client, e.g. determining low-coverage regions.
* It start and monitors analysis job queuing on the HPC infrastructure (via SGE).

## Dependencies

The server does not add any code dependencies to the project.  
It as the same dependencies as the rest of ngs-bits.

To allow IGV to read data from the GSvar server, a valid certificate is for the server is needed (a self-signed certificate is not enouth).

The GSvar server needs a own MySQL/MariaDB database for transient data (user sessions, temporary URLs, etc.).

## Build

To build the server, the following steps have to be executed

> make build_libs_release build_server_release

## Configuration

The server is configurable via the GSVarServer.ini file located at the `./bin` folder together with all the rest config files.  
For details see the [configuration](configuration.md).

## Running

The following command starts the server (if you are located at the root of the repository):
> ./bin/GSvarServer

You can force the server to ignore the port provided in the config file by using `p` argument and setting your own value:
> ./bin/GSvarServer -p=8443

GSvar server is intended to be used by the GSvar client. However, you can also access its Web UI through any browser at:
> https://[HOST_NAME]:[PORT_NUMBER]

It is actually a good way to check, if the server is running. Web UI also provides an extensive help page describing all its endpoints:
> https://[HOST_NAME]:[PORT_NUMBER]/help


## FAQ

### How do I start a development instance of the GSvarServer?

Please see [Running a development server](development_instance.md).

### Is it possible to run the GSvar server on AWS?

Yes, it is possible. Please see the [AWS documentation](run_gsvar_in_aws_cloud.md).
