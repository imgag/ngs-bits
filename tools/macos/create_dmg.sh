#!/bin/bash

set -e

# ============================================================
# GSvar macOS DMG creation script
# ============================================================

APP="GSvar.app"
EXECUTABLE="$APP/Contents/MacOS/GSvar"
FRAMEWORKS="$APP/Contents/Frameworks"


# ============================================================
# Functions
# ============================================================

print_section()
{
    echo
    echo "============================================================"
    echo " $1"
    echo "============================================================"
}

print_step()
{
    echo "  -> $1"
}


show_help()
{
    cat << EOF
Usage:
    $(basename "$0") <Developer-ID> <Full_path_to_bin> [BUILD]

Create a macOS DMG for GSvar.

Arguments:
    Developer-ID
        The Developer ID Application identity used for code signing.

        Example:
        "Developer ID Application: NAME (xxxxx)"

    Full_path_to_bin
        Full path to the "ngs-bits/bin" directory containing GSvar.app and the
        required libraries, resources (including the certificate chain file), configs, etc.

        The script changes into this directory before executing
        the packaging commands.

        Example:
        "/Users/name/GSvar/bin"
        
    BUILD
        Optional. Set to "true" to build the release libraries
        and GUI before creating the DMG.

        Default:
        false

        Example:
        true

Options:
    -h, --help
        Show this help page.

Finding your Developer ID:
    Run the following command to list available code signing identities:

        security find-identity -v -p codesigning

Example:
    $(basename "$0") \\
        "Developer ID Application: NAME (xxxxx)" \\
        "/Users/name/GSvar/bin"

EOF
}


# ============================================================
# Command line arguments
# ============================================================

if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    show_help
    exit 0
fi

if [[ $# -lt 2 || $# -gt 3 ]]; then
    echo "Error: expected 2 or 3 arguments."
    echo
    show_help
    exit 1
fi

DEVELOPER_ID="$1"
FULL_BIN_PATH="$2"
BUILD="${3:-false}"

# ============================================================
# Validate BUILD argument
# ============================================================

if [[ "$BUILD" != "true" && "$BUILD" != "false" ]]; then
    echo "Error: BUILD must be either 'true' or 'false'."
    echo
    show_help
    exit 1
fi

# ============================================================
# Change to application directory
# ============================================================

if [[ ! -d "$FULL_BIN_PATH" ]]; then
    echo "Error: directory does not exist:"
    echo "  $FULL_BIN_PATH"
    exit 1
fi

cd "$FULL_BIN_PATH"

# ============================================================
# Build release version
# ============================================================

if [[ "$BUILD" == "true" ]]; then

    print_section "Building release version"

    print_step "Running: make -C "$(dirname "$FULL_BIN_PATH")" build_libs_release build_gui_release"

    make -C "$(dirname "$FULL_BIN_PATH")" build_libs_release build_gui_release

    print_step "Release build completed."

fi

# ============================================================
# Check required files
# ============================================================

print_section "Checking files"

if [[ ! -d "$APP" ]]; then
    echo "Error: $APP not found in:"
    echo "  $FULL_BIN_PATH"
    exit 1
fi

if [[ ! -f "$EXECUTABLE" ]]; then
    echo "Error: GSvar executable not found:"
    echo "  $EXECUTABLE"
    exit 1
fi

print_step "Working directory: $(pwd)"
print_step "Developer ID: $DEVELOPER_ID"
echo "Build: $BUILD"

echo
echo "Files look OK."


# ============================================================
# Fix dynamic library paths
# ============================================================

print_section "Fixing dynamic library paths"

print_step "libcppCORE"
install_name_tool -change \
    libcppCORE.1.dylib \
    @executable_path/../Frameworks/libcppCORE.1.dylib \
    GSvar.app/Contents/MacOS/GSvar

print_step "libcppXML"
install_name_tool -change \
    libcppXML.1.dylib \
    @executable_path/../Frameworks/libcppXML.1.dylib \
    GSvar.app/Contents/MacOS/GSvar

print_step "libcppNGS"
install_name_tool -change \
    libcppNGS.1.dylib \
    @executable_path/../Frameworks/libcppNGS.1.dylib \
    GSvar.app/Contents/MacOS/GSvar

print_step "libcppGUI"
install_name_tool -change \
    libcppGUI.1.dylib \
    @executable_path/../Frameworks/libcppGUI.1.dylib \
    GSvar.app/Contents/MacOS/GSvar

print_step "libcppNGSD"
install_name_tool -change \
    libcppNGSD.1.dylib \
    @executable_path/../Frameworks/libcppNGSD.1.dylib \
    GSvar.app/Contents/MacOS/GSvar

print_step "libcppVISUAL"
install_name_tool -change \
    libcppVISUAL.1.dylib \
    @executable_path/../Frameworks/libcppVISUAL.1.dylib \
    GSvar.app/Contents/MacOS/GSvar


# ============================================================
# Copy application resources
# ============================================================

print_section "Copying application resources"

print_step "INI files"
cp *.ini GSvar.app/Contents/MacOS/

print_step "XML files"
cp *.xml GSvar.app/Contents/MacOS/

print_step "TSV files"
cp *.tsv GSvar.app/Contents/MacOS/

print_step "PEM files"
cp *.pem GSvar.app/Contents/MacOS/

print_step "genomes directory"
cp -r genomes GSvar.app/Contents/MacOS/


# ============================================================
# Bundle libraries
# ============================================================

print_section "Bundling libraries"

mkdir -p GSvar.app/Contents/Frameworks

print_step "libcppCORE"
cp libcppCORE.1.dylib GSvar.app/Contents/Frameworks/

print_step "libcppXML"
cp libcppXML.1.dylib GSvar.app/Contents/Frameworks/

print_step "libcppNGS"
cp libcppNGS.1.dylib GSvar.app/Contents/Frameworks/

print_step "libcppGUI"
cp libcppGUI.1.dylib GSvar.app/Contents/Frameworks/

print_step "libcppNGSD"
cp libcppNGSD.1.dylib GSvar.app/Contents/Frameworks/

print_step "libcppVISUAL"
cp libcppVISUAL.1.dylib GSvar.app/Contents/Frameworks/


# ============================================================
# Remove extended attributes
# ============================================================

print_section "Removing extended attributes"

print_step "Cleaning GSvar.app"
xattr -cr GSvar.app


# ============================================================
# Code signing
# ============================================================

print_section "Code signing"

print_step "Signing GSvar.app"
print_step "Developer ID: $DEVELOPER_ID"

codesign \
    --deep \
    --force \
    --verify \
    --verbose \
    --sign "$DEVELOPER_ID" \
    GSvar.app


# ============================================================
# Create DMG
# ============================================================

print_section "Creating DMG"

print_step "Running macdeployqt"

macdeployqt \
    GSvar.app \
    -dmg \
    -verbose=2 \
    -libpath="$FULL_BIN_PATH"


# ============================================================
# Finished
# ============================================================

print_section "Done"

echo "GSvar DMG creation completed successfully."
echo
echo "Output directory:"
echo "  $FULL_BIN_PATH"
echo
