TEMPLATE = subdirs
SUBDIRS = cppCORE \
    cppCORE-TEST

cppCORE-TEST.depends = cppCORE

OTHER_FILES += ToDos.txt
