#!/usr/bin/env python3
import os
import sys

def second_newest_directory(path):
    dirs = []
    for entry in os.scandir(path):
        if (
            entry.is_dir(follow_symlinks=False)
            and not entry.is_symlink()
            and entry.name != "GSvarServer-current"
            and entry.name != "default-settings"
            and entry.name.startswith("GSvarServer-")
        ):
            dirs.append((entry.name, entry.stat().st_mtime))

    if len(dirs) < 2:
        return None

    # Sort by modification time (newest first)
    dirs.sort(key=lambda x: x[1], reverse=True)

    return dirs[1][0]


if __name__ == "__main__":
    directory = sys.argv[1] if len(sys.argv) > 1 else "."

    prev_version = second_newest_directory(directory)
    if prev_version:
        print("Previous installation found at: " + prev_version)

        current_link = "/opt/GSvarServer/GSvarServer-current"        
        if os.path.islink(current_link) or os.path.exists(current_link):
            print("Found a symbolic link: " + current_link)

            try:
                target = os.readlink(current_link)
                print(f"symlink: {current_link} -> {target}")
            except OSError as e:
                print(f"symlink: {current_link} -> [error reading target: {e}]")
            
            os.remove(current_link)
            os.symlink(prev_version.rstrip("/"), current_link)
            print("Rolled back to the previous version at: " + prev_version)
    else:
        print("No previous installations found", file=sys.stderr)
        sys.exit(1)
