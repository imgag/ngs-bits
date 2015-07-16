TEMPLATE = subdirs

#Library targets and depdendencies
SUBDIRS = cppCORE\
        cppXML \
        cppNGS \
        cppGUI \
        cppNGSD

cppXML.depends = cppCORE
cppNGS.depends = cppCORE cppXML
cppGUI.depends = cppCORE cppXML
cppNGSD.depends = cppCORE
cppNGSD.depends = cppNGS

SUBDIRS += GSvar
GSvar.depends = cppGUI cppNGSD

#other stuff
OTHER_FILES += ToDos.txt

