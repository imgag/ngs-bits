QT_MAJOR = $$section(QT_VERSION, ., 0, 0)
contains(QT_MAJOR, 6) {
    DEST_DIR_PATH_PART = ../../../..    
} else {
    DEST_DIR_PATH_PART = ../..
}
