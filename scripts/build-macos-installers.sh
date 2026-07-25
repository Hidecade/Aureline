#!/usr/bin/env bash
set -euo pipefail
export COPYFILE_DISABLE=1

configuration="Release"
build_directory="build/macos-release"
skip_build=0
application_sign_identity="${AURELINE_APPLICATION_SIGN_IDENTITY:-}"
installer_sign_identity="${AURELINE_INSTALLER_SIGN_IDENTITY:-}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --configuration) configuration="$2"; shift 2 ;;
        --build-directory) build_directory="$2"; shift 2 ;;
        --skip-build) skip_build=1; shift ;;
        --application-sign-identity) application_sign_identity="$2"; shift 2 ;;
        --installer-sign-identity) installer_sign_identity="$2"; shift 2 ;;
        -h|--help)
            echo "Usage: $0 [--configuration Release] [--build-directory build/macos-release] [--skip-build] [--application-sign-identity IDENTITY] [--installer-sign-identity IDENTITY]"
            exit 0
            ;;
        *) echo "Unknown argument: $1" >&2; exit 2 ;;
    esac
done

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
version="$(sed -nE 's/^project\(Aureline VERSION ([0-9.]+).*/\1/p' "$repo_root/CMakeLists.txt")"
if [[ -z "$version" ]]; then
    echo "Could not read the Aureline version from CMakeLists.txt" >&2
    exit 1
fi

build_root="$repo_root/$build_directory"
dist_directory="$repo_root/dist"
stage_root="$build_root/package-stage"
if [[ "$skip_build" -eq 0 ]]; then
    cmake -S "$repo_root" -B "$build_root" \
        -DCMAKE_BUILD_TYPE="$configuration" \
        -DAURELINE_BUILD_STANDALONE=ON \
        -DAURELINE_BUILD_PLUGINS=ON
    cmake --build "$build_root" --config "$configuration" --target \
        Aureline_Plugin_Standalone Aureline_Plugin_VST3 Aureline_Plugin_AU
fi

plugin_artifact_root="$build_root/Aureline_Plugin_artefacts"
if [[ -d "$plugin_artifact_root/$configuration" ]]; then
    plugin_artifact_root="$plugin_artifact_root/$configuration"
fi
standalone_artifact="$plugin_artifact_root/Standalone/Aureline.app"
vst3_artifact="$plugin_artifact_root/VST3/Aureline.vst3"
au_artifact="$plugin_artifact_root/AU/Aureline.component"

for required in "$standalone_artifact" "$vst3_artifact" "$au_artifact"; do
    if [[ ! -e "$required" ]]; then
        echo "Required artifact was not found: $required" >&2
        exit 1
    fi
done

mkdir -p "$dist_directory"
rm -rf "$stage_root"
app_stage="$stage_root/standalone/Applications"
vst3_stage="$stage_root/vst3/Library/Audio/Plug-Ins/VST3"
au_stage="$stage_root/au/Library/Audio/Plug-Ins/Components"
mkdir -p "$app_stage" "$vst3_stage" "$au_stage"
ditto --norsrc --noextattr "$standalone_artifact" "$app_stage/Aureline.app"
ditto --norsrc --noextattr "$vst3_artifact" "$vst3_stage/Aureline.vst3"
ditto --norsrc --noextattr "$au_artifact" "$au_stage/Aureline.component"
xattr -cr "$stage_root"

sign_bundle() {
    local bundle="$1"
    if [[ -n "$application_sign_identity" ]]; then
        codesign --force --deep --options runtime --timestamp \
            --sign "$application_sign_identity" "$bundle"
    else
        codesign --force --deep --sign - "$bundle"
    fi
}
sign_bundle "$app_stage/Aureline.app"
sign_bundle "$vst3_stage/Aureline.vst3"
sign_bundle "$au_stage/Aureline.component"

create_package() {
    local kind="$1"
    local identifier="$2"
    local output="$3"
    local components="$stage_root/$kind-components.plist"
    pkgbuild --analyze --root "$stage_root/$kind" "$components"
    if ! /usr/libexec/PlistBuddy -c "Set :0:BundleIsRelocatable false" "$components" 2>/dev/null; then
        /usr/libexec/PlistBuddy -c "Add :0:BundleIsRelocatable bool false" "$components"
    fi
    pkgbuild --root "$stage_root/$kind" \
        --component-plist "$components" \
        --identifier "$identifier" \
        --version "$version" \
        --install-location / \
        "$output"
}

standalone_pkg="$dist_directory/Aureline-Standalone-$version-macOS.pkg"
vst3_pkg="$dist_directory/Aureline-VST3-$version-macOS.pkg"
au_pkg="$dist_directory/Aureline-AU-$version-macOS.pkg"
create_package standalone jp.hidecade.aureline.standalone "$standalone_pkg"
create_package vst3 jp.hidecade.aureline.vst3 "$vst3_pkg"
create_package au jp.hidecade.aureline.au "$au_pkg"

if [[ -n "$installer_sign_identity" ]]; then
    for package in "$standalone_pkg" "$vst3_pkg" "$au_pkg"; do
        signed="$stage_root/$(basename "$package")"
        productsign --sign "$installer_sign_identity" "$package" "$signed"
        mv "$signed" "$package"
    done
fi

echo "Installers created in $dist_directory"
