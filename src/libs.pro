TEMPLATE = subdirs
SUBDIRS = cppCORE\
	cppXML \
	cppNGS \
    cppCORE-TEST \
    cppNGS-TEST \
    cppGUI \
    cppNGSD \

cppCORE-TEST.depends = cppCORE
cppXML.depends = cppCORE
cppNGS.depends = cppCORE
cppNGS.depends = cppXML
cppNGS-TEST.depends = cppNGS
cppGUI.depends = cppCORE
cppGUI.depends = cppXML
cppNGSD.depends = cppCORE
cppNGSD.depends = cppNGS

OTHER_FILES += \
	../ToDos.txt

