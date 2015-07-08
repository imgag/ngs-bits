TEMPLATE = subdirs
SUBDIRS = cppCORE\
            cppCORE-TEST \
            cppXML \
            cppNGS \
            cppNGS-TEST

cppCORE-TEST.depends = cppCORE
cppNGS.depends = cppCORE cppXML
cppXML.depends = cppCORE
cppNGS-TEST.depends = cppNGS

OTHER_FILES += ToDos.txt
