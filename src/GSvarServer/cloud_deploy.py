#!/usr/bin/env python3
import sys
import os
import shutil
import subprocess
from pathlib import Path

# prints out an error and exits with code 1
def exitWithError(msg):
    print(f"Error: {msg}")
    sys.exit(1)

# runs an external command
def run(cmd, check=True):
    print(f"> {cmd}")
    result = subprocess.run(cmd, shell=True)
    if check and result.returncode != 0:
        sys.exit(result.returncode)

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 cloud_deploy.py <SERVER_DEP_PATH>")
        sys.exit(1)

    SERVER_DEP_PATH = sys.argv[1].rstrip("/") + "/"

    # ---- Checks ----
    if not Path("/opt/GSvarServer/").exists():
        exitWithError("/opt/GSvarServer/ is missing, you need to crete the folder first!")

    if not Path("./bin/GSvarServer").exists():
        exitWithError("bin/GSvarServer is missing, you need to build the executable first!")

    if not Path("/opt/GSvarServer/default-settings").exists():
        exitWithError("/opt/GSvarServer/default-settings is missing, you need to create a folder for default settings first!")

    if not Path("/opt/GSvarServer/default-settings/GSvarServer.ini").exists():
        exitWithError("/opt/GSvarServer/default-settings/GSvarServer.ini is missing, you need to create a file to store default settings first!")

    print("#Check configuration files")
    if not Path("/opt/GSvarServer/GSvarServer-current/GSvarServer.ini").exists():
        print("Could not find any previous installations of the server, nothing to check")
    else:
        print("Checking the config from the previous installation")
        run("diff /opt/GSvarServer/GSvarServer-current/GSvarServer.ini /opt/GSvarServer/default-settings/GSvarServer.ini -s")
    print()

    # ---- Clean up source ----
    print("#Clean up source")
    print("./bin/...")
    shutil.rmtree("./bin/out", ignore_errors=True)

    bin_path = Path("./bin")
    for p in bin_path.glob("*-TEST"):
        if p.is_dir():
            shutil.rmtree(p, ignore_errors=True)
        else:
            p.unlink(missing_ok=True)

    print()

    # ---- Deploy binaries ----
    print("#Deploy binaries")
    print(SERVER_DEP_PATH)
    os.makedirs(SERVER_DEP_PATH, exist_ok=True)

    for root, dirs, files in os.walk("./bin"):
        for name in files:
            src = Path(root) / name
            s = str(src)
            if "settings" in s or "blat" in s:
                continue
            dest = Path(SERVER_DEP_PATH) / src.name
            shutil.copy2(src, dest)

    print()

    # ---- Create new symbolic link ----
    print("#Create a new link")
    current_link = "/opt/GSvarServer/GSvarServer-current"
    print(current_link)
    if os.path.islink(current_link) or os.path.exists(current_link):
        os.remove(current_link)
    os.symlink(SERVER_DEP_PATH.rstrip("/"), current_link)

    print()

    # ---- Create empty log ----
    print("#Create an empty log file")
    print(Path(SERVER_DEP_PATH, "GSvarServer.log"))
    Path(SERVER_DEP_PATH, "GSvarServer.log").touch(exist_ok=True)

    print()

    # ---- Deploy settings ----
    print("#Deploy settings")
    print(Path(SERVER_DEP_PATH, "GSvarServer.ini"))
    shutil.copy2(
        "/opt/GSvarServer/default-settings/GSvarServer.ini",
        Path(SERVER_DEP_PATH, "GSvarServer.ini")
    )

    print("\nDeployment completed successfully.\n")

if __name__ == "__main__":
    main()
