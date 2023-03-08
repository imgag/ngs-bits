# Checklist for a new ngs-bits release
1. ## Notify the users about a potential downtime (optional)
- Create/edit the `GSvarServer_notification.json` file in `GSvarServer-current` folder. The text saved in this file will be shown to the users of GSvar desktop application. If the file is changed, another (updated) notification will be sent to the users.
2. ## Deploy server
- SSH to the server
- Check out the latest version from the repository
- Run `make build_3rdparty build_libs_release build_server_release` to build the server
- Stop the currently running (if there is one) server `sudo /usr/sbin/service gsvar stop`. You can always check the status of the server by executing `sudo /usr/sbin/service gsvar status`
- Remove the symlink to the folder with the server binary `unlink GSvarServer-current`
- Run the deployment process `make deploy_server_nobuild`
- Start the new server `sudo /usr/sbin/service gsvar start`
3. ## Deploy new version of client
- Build GSvar for Windows
- Create a new folder at Q:\AH\Apps\NGS-Tools-Public\GSvar_app and call it YEAR_MONTH_DAY of the build
- Copy GSvar.exe to this folder alnong with all its dependencies (e.g. DLL files, database drivers, config files, etc.)
- Update the GSvar.bat file in Q:\AH\Apps\NGS-Tools-Public\. The entry inside the file should point to the newly created version of the app
- Create/edit the `GSvarServer_client.json` file in `GSvarServer-current` folder. The file should include a JSON object with the following fields (the `date` field is optional):
    ``` 
    {
        "version": GITHUB_RELEASE_TAG_WITHOUT_HASH,
        "message": MESSAGE_FOR_THE_USER,
        "date": UNIX_TIME_IN_SECONDS
    }
    ```
4. ## Test on virtual machine *vswmg-fr-vpn-01*
This VM allows to test some functionality available to the users outside our institution. 
