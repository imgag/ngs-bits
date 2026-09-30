#!/usr/bin/env bash

set -euo pipefail

# ============================================================
# GSvar macOS DMG creation script
#
# Usage:
#   ./create_dmg.sh <Developer-ID> <FULL_BIN_PATH>
#
# Example:
#   ./create_dmg.sh \
#       "Developer ID Application: My Company (ABCDE12345)" \
#       "/Users/me/gsvar/bin"
# ============================================================

APP_NAME="GSvar.app"
APP_EXECUTABLE="${APP_NAME}/Contents/MacOS/GSvar"
FRAMEWORKS_DIR="${APP_NAME}/Contents/Frameworks"

# ------------------------------------------------------------
# Colors / formatting
# ------------------------------------------------------------

if [[ -t 1 ]]; then
    BOLD='\033[1m'
    GREEN='\033[0;32m'
    BLUE='\033[0;34m'
    YELLOW='\033[0;33m'
    RED='\033[0;31m'
    RESET='\033[0m'
else
    BOLD=''
    GREEN=''
    BLUE=''
    YELLOW=''
    RED=''
    RESET=''
fi

print_section()
{
    echo
    echo -e "${BLUE}${BOLD}============================================================${RESET}"
    echo -e "${BLUE}${BOLD} $1${RESET}"
    echo -e "${BLUE}${BOLD}============================================================${RESET}"
}

print_step()
{
    echo -e "${GREEN}▶${RESET} $1"
}

print_warning()
{
    echo -e "${YELLOW}⚠${RESET} $1"
}

print_error()
{
    echo -e "${RED}✖${RESET} $1" >&2
}

print_success()
{
    echo -e "${GREEN}${BOLD}✔ $1${RESET}"
}

# ------------------------------------------------------------
# Help
# ------------------------------------------------------------

show_help()
{
    cat << EOF
Usage:
    $(basename "$0") <Developer-ID> <FULL_BIN_PATH>

Create a signed macOS DMG for GSvar.

Arguments:
    Developer-ID
        The Apple Developer ID Application identity used for signing.

        Example:
        "Developer ID Application: My Company (ABCDE12345)"

    FULL_BIN_PATH
        Full path passed to macdeployqt's -libpath option.

        Example:
        "/Users/me/gsvar/bin"

Options:
    -h, --help
        Show this help page.

Example:
    $(basename "$0") \\
        "Developer ID Application: My Company (ABCDE12345)" \\
        "/Users/me/gsvar/bin"

The script performs the following steps:

    1. Validate the GSvar application and required files
    2. Fix dylib paths using install_name_tool
    3. Copy configuration/resources into the application
    4. Bundle required cpp* dylibs
    5. Remove extended attributes
    6. Sign the application
    7. Create the DMG using macdeployqt

EOF
}

# ------------------------------------------------------------
# Parse arguments
# ------------------------------------------------------------

if [[ $# -eq 1 && ("$1" == "-h" || "$1" == "--help") ]]; then
    show_help
    exit 0
fi

if [[ $# -ne 2 ]]; then
    print_error "Expected 2 arguments, but received $#."
    echo
    show_help
    exit 1
fi

DEVELOPER_ID="$1"
FULL_BIN_PATH="$2"

# ------------------------------------------------------------
# Validation
# ------------------------------------------------------------

print_section "Checking environment"

print_step "Developer ID: ${DEVELOPER_ID}"
print_step "Library path: ${FULL_BIN_PATH}"

if [[ ! -d "${APP_NAME}" ]]; then
    print_error "${APP_NAME} not found in the current directory."
    exit 1
fi

if [[ ! -f "${APP_EXECUTABLE}" ]]; then
    print_error "GSvar executable not found: ${APP_EXECUTABLE}"
    exit 1
fi

if [[ ! -d "${FULL_BIN_PATH}" ]]; then
    print_error "Library path does not exist: ${FULL_BIN_PATH}"
    exit 1
fi

for command in install_name_tool cp xattr codesign macdeployqt; do
    if ! command -v "${command}" >/dev/null 2>&1; then
        print_error "Required command not found: ${command}"
        exit 1
    fi
done

print_success "Environment looks good."

# ------------------------------------------------------------
# Fix dylib references
# ------------------------------------------------------------

print_section "Fixing dynamic library paths"

declare -A DYLIBS=(
    ["libcppCORE.1.dylib"]="libcppCORE.1.dylib"
    ["libcppXML.1.dylib"]="libcppXML.1.dylib"
    ["libcppNGS.1.dylib"]="libcppNGS.1.dylib"
    ["libcppGUI.1.dylib"]="libcppGUI.1.dylib"
    ["libcppNGSD.1.dylib"]="libcppNGSD.1.dylib"
    ["libcppVISUAL.1.dylib"]="libcppVISUAL.1.dylib"
)

for dylib in "${!DYLIBS[@]}"; do
    print_step "Updating ${dylib}"

    install_name_tool \
        -change "${dylib}" \
        "@executable_path/../Frameworks/${dylib}" \
        "${APP_EXECUTABLE}"
done

print_success "Dynamic library paths updated."

# ------------------------------------------------------------
# Copy application resources
# ------------------------------------------------------------

print_section "Copying application resources"

copy_files()
{
    local extension="$1"

    shopt -s nullglob
    local files=( *."${extension}" )
    shopt -u nullglob

    if [[ ${#files[@]} -eq 0 ]]; then
        print_warning "No *.${extension} files found."
        return
    fi

    for file in "${files[@]}"; do
        print_step "Copying ${file}"
        cp "${file}" "${APP_NAME}/Contents/MacOS/"
    done
}

copy_files "ini"
copy_files "xml"
copy_files "tsv"
copy_files "pem"

if [[ -d "genomes" ]]; then
    print_step "Copying genomes/"
    cp -r genomes "${APP_NAME}/Contents/MacOS/"
else
    print_warning "genomes/ directory not found."
fi

print_success "Application resources copied."

# ------------------------------------------------------------
# Bundle libraries
# ------------------------------------------------------------

print_section "Bundling GSvar libraries"

mkdir -p "${FRAMEWORKS_DIR}"

for dylib in "${!DYLIBS[@]}"; do
    if [[ ! -f "${dylib}" ]]; then
        print_error "Required library not found: ${dylib}"
        exit 1
    fi

    print_step "Bundling ${dylib}"
    cp "${dylib}" "${FRAMEWORKS_DIR}/"
done

print_success "GSvar libraries bundled."

# ------------------------------------------------------------
# Remove extended attributes
# ------------------------------------------------------------

print_section "Cleaning extended attributes"

print_step "Removing extended attributes from ${APP_NAME}"

xattr -cr "${APP_NAME}"

print_success "Extended attributes removed."

# ------------------------------------------------------------
# Code signing
# ------------------------------------------------------------

print_section "Code signing application"

print_step "Signing ${APP_NAME}"
print_step "Identity: ${DEVELOPER_ID}"

codesign \
    --deep \
    --force \
    --verify \
    --verbose \
    --sign "${DEVELOPER_ID}" \
    "${APP_NAME}"

print_success "Application signed successfully."

# ------------------------------------------------------------
# DMG creation
# ------------------------------------------------------------

print_section "Creating DMG"

print_step "Running macdeployqt"
print_step "Library path: ${FULL_BIN_PATH}"

macdeployqt \
    "${APP_NAME}" \
    -dmg \
    -verbose=2 \
    -libpath="${FULL_BIN_PATH}"

print_success "DMG created successfully."

# ------------------------------------------------------------
# Final output
# ------------------------------------------------------------

print_section "Finished"

echo -e "${GREEN}${BOLD}GSvar macOS distribution package is ready.${RESET}"
echo

shopt -s nullglob
DMG_FILES=( *.dmg )
shopt -u nullglob

if [[ ${#DMG_FILES[@]} -gt 0 ]]; then
    for dmg in "${DMG_FILES[@]}"; do
        echo "DMG: ${dmg}"
    done
fi

echo
