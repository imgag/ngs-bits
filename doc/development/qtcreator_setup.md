# QtCreator setup

## General settings
There are a few things we have to set in the QtCreator options just once:

 * `Edit -> Preferences -> Environment -> Interface -> Language: English`
 * `Edit -> Preferences -> C++ -> Code Style -> Custom Settings`: Import `QtCreatorCodingStyle.clang-format` (from ngs-bits/doc/development/)
 * `Edit -> Preferences -> C++ -> File Naming -> Lower case file names: off`


## Disable plugin debugging

Change the entry `QT_DEBUG_PLUGINS` in `Project -> Run Settings -> Environment` from 1 to 0.

## Change settings location

If you are in a Windows domain in wich the AppData folder is synced between PCs, e.g. at UKT, this can lead to strange behaviour of QtCreator.  
Change the QtCreator settings location by using the following command line argument:

`QtCreator -settingspath [PATH]`


[Back to main page](index.md)
