TEMPLATE = subdirs

#Library targets and depdendencies
SUBDIRS = cppCORE\
        cppXML \
        cppNGS \
        cppGUI \
        cppNGSD \
        cppVISUAL

cppXML.depends = cppCORE
cppNGS.depends = cppCORE cppXML
cppGUI.depends = cppCORE cppXML
cppNGSD.depends = cppCORE cppXML cppNGS
cppVISUAL.depends = cppCORE cppXML cppNGS cppGUI

SUBDIRS += GSvar
GSvar.depends = cppCORE cppXML cppNGS cppGUI cppNGSD cppVISUAL
