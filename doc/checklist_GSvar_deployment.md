# Checklist for a new ngs-bits release

1. ## Deploy server
- SSH to the server
- Check out the latest version from the repository
- Run `make build_3rdparty build_libs_release build_server_release` to build the server
- Stop the currently running (if there is one) server `sudo /usr/sbin/service gsvar stop`. You can always check the status of the server by executing `sudo /usr/sbin/service gsvar status`
- Remove the symlink to the folder with the server binary `unlink GSvarServer-current`
- Run the deployment process `make deploy_server_nobuild`
- Start the new server `sudo /usr/sbin/service gsvar start`
1. ## Deploy new version of client
1. ## Test on virtual machine *vswmg-fr-vpn-01*
