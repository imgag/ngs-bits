apt-get update
apt-get -y install g++ git cmake python python-matplotlib qt5-default libqt5xmlpatterns5-dev libqt5sql5-mysql
make build_3rdparty build_tools_release 
make test_lib 
make test_tools