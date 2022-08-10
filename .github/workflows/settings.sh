#!/bin/bash
mkdir ./projects_folder
touch ./bin/settings.ini
echo "ngsd_test_host = \"127.0.0.1\"" >> ./bin/settings.ini
echo "ngsd_test_port = 3306" >> ./bin/settings.ini
echo "ngsd_test_name = \"testdb\"" >> ./bin/settings.ini
echo "ngsd_test_user = \"testuser\"" >> ./bin/settings.ini
echo "ngsd_test_pass = \"password\"" >> ./bin/settings.ini
echo "projects_folder = \"projects_folder\"" >> ./bin/settings.ini
echo "data_folder = \"\"" >> ./bin/settings.ini
echo "url_lifetime = 5" >> ./bin/settings.ini
echo "server_host = \"127.0.0.1\"" >> ./bin/settings.ini
echo "http_server_port = 8080" >> ./bin/settings.ini
echo "https_server_port = 8443" >> ./bin/settings.ini
echo "ssl_certificate = \"$HOME/ssl/test-cert.crt\"" >> ./bin/settings.ini
echo "ssl_key = \"$HOME/ssl/test-key.key\"" >> ./bin/settings.ini
echo "liftover_hg19_hg38 = ../src/cppNGS-TEST/data_in/hg19ToHg38.over.chain.gz" >> ./bin/settings.ini
echo "liftover_hg38_hg19 = ../src/cppNGS-TEST/data_in/hg38ToHg19.over.chain.gz" >> ./bin/settings.ini
echo "session_duration = 36000" >> ./bin/settings.ini
