# Checklist for a new ngs-bits release

## 1. Deploy server

- Notify the users about a potential downtime (optional): Edit `/mnt/storage2/GSvarServer/GSvarServer-current/GSvarServer_notification.json`. The text in this file will be shown to the users of GSvar desktop application. If the file is changed, another (updated) notification will be sent to the users.
- SSH to SRV011
- Pull the latest version from the repository
- Run `make build_3rdparty build_libs_release build_server_release` to build the server (do not forget to set the encryption key at `./src/cppCORE/CRYPT_KEY.txt`)
- Stop the currently running (if there is one) server `sudo /usr/sbin/service gsvar stop`. You can always check the status of the server by executing `sudo /usr/sbin/service gsvar status`
- Run the deployment process `make deploy_server_nobuild`
- Start the new server `sudo /usr/sbin/service gsvar start`
- Check the log records: `sudo journalctl -u gsvar.service -n 100`

## 2. Deploy new version of client

- Build GSvar for Windows:
  - GIT Sync (main repo and sub-modules)
  - clear cppCORE build
  - run QMake on cppCORE
  - build
- Create a new folder at Q:\AH\Apps\NGS-Tools-Public\ngs-bits_clientserver\ and call it according to the Git tag of the build: YYYY_MM-commit
- Copy GSvar.exe to this folder along with all its dependencies (e.g. DLL files, database drivers, config files, etc.)
- Test on virtual machine *vswmg-fr-vpn-01*
- Update `Q:\AH\Apps\NGS-Tools\current\the GSvar.bat`. The entry inside the file should point to the newly created version of the app
- Create/edit the `GSvarServer_client.json` file in `GSvarServer-current` folder to notify the user of the new version. The file should include a JSON object with the following fields (the `date` field is optional):
    ``` 
    {
        "version": GITHUB_RELEASE_TAG_WITHOUT_HASH,
        "message": MESSAGE_FOR_THE_USER,
        "date": UNIX_TIME_IN_SECONDS
    }
    ```
