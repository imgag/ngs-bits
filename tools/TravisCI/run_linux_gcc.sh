apt-get update
apt-get -y install make g++ git python python-matplotlib qt5-default libqt5xmlpatterns5-dev libqt5sql5-mysql libbz2-dev liblzma-dev cmake

make build_3rdparty build_libs_release build_tools_release build_gui_release
make -k test_lib test_tools
