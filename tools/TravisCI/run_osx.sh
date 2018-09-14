brew update
brew install qt lzlib
brew link qt --force
sudo pip install matplotlib

make build_3rdparty build_libs_release build_tools_release build_gui_release
make -k test_lib test_tools
