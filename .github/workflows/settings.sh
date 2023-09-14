#!/bin/bash
mkdir ./projects_folder
touch ./bin/settings.ini
echo "ngsd_host = \"127.0.0.1\"" >> ./bin/settings.ini
echo "ngsd_port = 3306" >> ./bin/settings.ini
echo "ngsd_name = \"test_db\"" >> ./bin/settings.ini
echo "ngsd_user = \"test_user\"" >> ./bin/settings.ini
echo "ngsd_pass = \"password\"" >> ./bin/settings.ini
echo "ngsd_test_host = \"127.0.0.1\"" >> ./bin/settings.ini
echo "ngsd_test_port = 3306" >> ./bin/settings.ini
echo "ngsd_test_name = \"test_db\"" >> ./bin/settings.ini
echo "ngsd_test_user = \"test_user\"" >> ./bin/settings.ini
echo "ngsd_test_pass = \"password\"" >> ./bin/settings.ini
echo "projects_folder = \"projects_folder\"" >> ./bin/settings.ini
echo "data_folder = \"\"" >> ./bin/settings.ini
echo "liftover_hg19_hg38 = ../src/cppNGS-TEST/data_in/hg19ToHg38.over.chain.gz" >> ./bin/settings.ini
echo "liftover_hg38_hg19 = ../src/cppNGS-TEST/data_in/hg38ToHg19.over.chain.gz" >> ./bin/settings.ini
echo "threads = 1" >> ./bin/settings.ini
echo "server_port = 8443" >> ./bin/settings.ini

touch ./bin/GSvarServer-TEST.ini
echo "url_lifetime = 5" >> ./bin/GSvarServer-TEST.ini
echo "server_host = \"localhost\"" >> ./bin/GSvarServer-TEST.ini
echo "server_port = 8443" >> ./bin/GSvarServer-TEST.ini
echo "server_root = \"./bin\"" >> ./bin/GSvarServer-TEST.ini
echo "allow_folder_listing = true" >> ./bin/GSvarServer-TEST.ini
echo "ssl_certificate = \"$HOME/ssl/test-cert.crt\"" >> ./bin/GSvarServer-TEST.ini
echo "ssl_key = \"$HOME/ssl/test-key.key\"" >> ./bin/GSvarServer-TEST.ini
echo "session_duration = 36000" >> ./bin/GSvarServer-TEST.ini
echo "test_mode = true" >> ./bin/GSvarServer-TEST.ini
cp ./bin/GSvarServer-TEST.ini ./bin/GSvarServer.ini

sed -i '/CRYPT/d' src/cppCORE/cppCORE.pro
echo 'DEFINES += "CRYPT_KEY=\\\"0xf0a0c1ba2b7b7a82\\\""' >> ./src/cppCORE/cppCORE.pro
