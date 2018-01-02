apt-get update
apt-get -y install g++ git cmake python python-matplotlib qt5-default libqt5xmlpatterns5-dev libqt5sql5-mysql
make build_3rdparty build_libs_release build_tools_release build_gui_release
make -k test_lib test_tools