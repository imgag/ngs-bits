# Running a development server

### Setting up a GSvar server for testing (on Ubuntu)

First you need a unused port on the development server. You can either select a random 5-digit port and check with `netstat -lntu` if the port is already used.
Or you can use the following 1-liner to get a free port from the kernel:

    > python -c 'import socket; s=socket.socket(); s.bind(("", 0)); print(s.getsockname()[1]); s.close()' 

Additionally you have to provide a certificate. If you do not have a valid certificate for your machine you can create a self-signed one:

    > openssl genrsa -out GSvarServer.key 2048   
    > openssl req -new -key GSvarServer.key -out GSvarServer.csr   
    > openssl x509 -signkey GSvarServer.key -in GSvarServer.csr -req -days 365 -out GSvarServer.crt

If you use self-signed certificates IGV and libcurl will not work.(See possible fix [below](#trust-self-signed-certificates-on-ubuntu))

Next you have to create a `GSvarServer.ini` in the `bin`folder:

    > cp bin/GSvarServer.ini.example bin/GSvarServer.ini

And perform the [configuration](../configuration.md).

Next step is to build the server:

    > make build_libs_release build_server_release

If NGSD database does not exist, you need to import it from a file or initialize an empty one. For local development and debugging we recommend running a MariaDB instance inside a Docker container. You can use the [following instructions](../../install_ngsd.md) to do this.

And now run the server:

    > ./bin/GSvarServer

Now you can adapt the [configuration](../../GSvar/configuration.md) in your client and connect to the server.

### Trust self-signed certificates on Ubuntu

For the development and testing purposes it is possible to run a local instance of GSvar Server. However, if you are using self-signed certificates, you will have to make them trusted (otherwise IGV and libcurl will not be able to verify them):

Install CA certificates package:

    > sudo apt-get install ca-certificates

Copy your self-signed certificate to this location:

    > sudo cp YOUR_CERTIFICATE.crt /usr/local/share/ca-certificates

Update the list of certificate authorities:

    > sudo update-ca-certificates

## Local development environment

You are going to need a SSL certificate and a key for the server to support HTTPS protocol. For the development purposes self-signed ones will be sufficient:

    > openssl req -new -newkey rsa:4096 -days 365 -nodes -x509 -subj "/C=DE/ST=BW/L=Tuebingen/O=test-certificate/CN=localhost" -keyout ~/test-key.key -out ~/test-cert.crt

GSvarServer requires a MySQL/MariaDB database.

## Qt debug statements

Depending on the Qt installation, you may have disabled debug statements by default. To turn them on, follow these steps:

- Open qtlogging.ini in /etc/xdg/QtProject/ (create a new empty file, if it does not exist)
- Add (or modify accordingly) the following config

[Rules]  
*.debug=true  
qt.*.debug=false

--

[back to main page](../index.md)
