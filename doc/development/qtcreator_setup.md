# QtCreator setup

## General settings
There are a few things we have to set in the QtCreator options just once:

 * `Tools -> Options -> Environment -> Interface -> Language: English`
 * `Tools -> Options -> C++ -> Code Style -> Current Settings: MedGen`  
    Import `QtCreatorCodingStyle.xml` if the MedGen style does not exist
 * `Tools -> Options -> C++ -> File Naming -> Lower case file names: off`
 * `Help -> About Plugins... -> Utilities -> Todo : on`

## Project settings
For each project file we have to set the following:
	
 * `Projects -> Code Style -> Current Settings: MedGen`
 * `Projects -> Build & Run -> Build -> Make arguments: -j5`


[Back to main page](index.md)