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

## Integrating Codex as AI coding assistent into QtCreator

To use Codex in QtCreator, follow these instructions.

1. Install [Node.js](https://nodejs.org/dist/v24.21.0/node-v24.21.0-x64.msi)

	If `npm` is not in the path of new CMD windows, add `C:\Program Files\nodejs\` to the PATH environment variable.

1. Install Codex CLI:

	> powershell -ExecutionPolicy ByPass -c "irm https://chatgpt.com/codex/install.ps1 | iex"

	During the installation, you have to log into your account, determine the sandbox and give a few preferences.

1. Install [QtCreator 20.0 or higher](https://www.qt.io/development/offline-installers) (ACP/MCP protocol is available in QtCreator starting from version 20)

1. Enable ACP/MCP in `Help → About Plugins`

1. Add Codex as ACP server: `Edit → Preferences → AI → ACP servers → Add`
	
	Select `Codex`.

1. Enable QtCreator MCP server at `Edit → Preferences → AI → ACP servers → QtCreator MCP server`

1. Open AI panel: `Tools → ACP client → Show agentic AI chat in side panel`

[Back to main page](index.md)

