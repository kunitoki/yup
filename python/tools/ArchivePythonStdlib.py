import os
import stat
import shutil
import hashlib
import zipfile
from pathlib import Path
from argparse import ArgumentParser


def file_hash(file):
    h = hashlib.md5()
    with open(file, "rb") as f:
        h.update(f.read())
    return h.hexdigest()


def should_exclude(path, name, exclude_patterns):
    for pattern in exclude_patterns:
        if name == pattern:
            return True

        if pattern.startswith('**/'):
            pattern_name = pattern[3:]
            if name == pattern_name:
                return True

        if pattern.startswith('**/*.'):
            ext = pattern[4:]  # Remove **/*
            if name.endswith(ext):
                return True

        if '*' in pattern and not pattern.startswith('**/'):
            import fnmatch
            if fnmatch.fnmatch(name, pattern):
                return True

    return False


def copy_filtered_tree(src, dst, exclude_patterns):
    if not os.path.exists(dst):
        os.makedirs(dst)

    for item in os.listdir(src):
        src_path = os.path.join(src, item)
        dst_path = os.path.join(dst, item)

        if should_exclude(src_path, item, exclude_patterns):
            continue

        if os.path.isdir(src_path):
            copy_filtered_tree(src_path, dst_path, exclude_patterns)
        else:
            shutil.copy2(src_path, dst_path)


def clean_duplicate_libraries(directory):
    lib_files = {}
    for root, _, files in os.walk(directory):
        for file in files:
            file_path = os.path.join(root, file)

            if not any(ext in file for ext in ['.dylib', '.dll', '.so', '.a', '.lib']):
                continue

            base_name = file.split('.')[0]

            if '.dylib' in file:    lib_type = 'dylib'
            elif '.dll' in file:    lib_type = 'dll'
            elif '.so' in file:     lib_type = 'so'
            elif '.a' in file:      lib_type = 'static'
            elif '.lib' in file:    lib_type = 'lib'
            else:                   continue

            key = (base_name, lib_type)
            if key not in lib_files:
                lib_files[key] = file_path
            else:
                os.remove(file_path)


def make_archive(file, directory):
    archived_files = []
    for dirname, _, files in os.walk(directory):
        for filename in files:
            path = os.path.join(dirname, filename)
            archived_files.append((path, os.path.relpath(path, directory)))

    with zipfile.ZipFile(file, "w") as zf:
        for path, archive_path in sorted(archived_files):
            permission = 0o555 if os.access(path, os.X_OK) else 0o444

            zip_info = zipfile.ZipInfo.from_file(path, archive_path)
            zip_info.date_time = (1999, 1, 1, 0, 0, 0)
            zip_info.external_attr = (stat.S_IFREG | permission) << 16

            with open(path, "rb") as fp:
                zf.writestr(zip_info, fp.read(), compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)


if __name__ == "__main__":
    print(f"-- YUP -- Starting python standard lib archiving tool...")

    parser = ArgumentParser()
    parser.add_argument("-r", "--root-folder", type=Path, help="Path to the python base folder.")
    parser.add_argument("-o", "--output-folder", type=Path, help="Path to the output folder.")
    parser.add_argument("-M", "--version-major", type=int, help="Major version number (integer).")
    parser.add_argument("-m", "--version-minor", type=int, help="Minor version number (integer).")
    parser.add_argument("-x", "--exclude-patterns", type=str, default=None, help="Excluded patterns (semicolon separated list).")

    args = parser.parse_args()

    version = f"{args.version_major}.{args.version_minor}"
    version_nodot = f"{args.version_major}{args.version_minor}"
    python_folder_name = f"python{version}"

    final_location: Path = args.output_folder / "python"
    final_archive = args.output_folder / f"python{version_nodot}.zip"
    temp_archive = args.output_folder / f"temp{version_nodot}.zip"

    base_python: Path = args.root_folder

    base_patterns = [
        "*.pyc",
        "__pycache__",
        "__phello__",
        "_tk*",
        "_test*",
        "_xxtestfuzz*",
        "config-3*",
        "idlelib",
        "pkgconfig",
        "pydoc_data",
        "test",
        "*tcl*",
        "*tdbc*",
        "*tk*",
        "Tk*",
        "turtledemo",
        ".DS_Store",
        "EXTERNALLY-MANAGED",
        "LICENSE.txt",
    ]

    if args.exclude_patterns:
        custom_patterns = [x.strip() for x in args.exclude_patterns.replace('"', '').split(";")]
        base_patterns += custom_patterns

    print(f"-- YUP -- Cleaning up {final_location}...")
    if final_location.exists():
        shutil.rmtree(final_location)

    final_location.mkdir(parents=True, exist_ok=True)

    # Copy lib folder structure
    lib_src = base_python / "lib"
    if lib_src.exists():
        lib_dst = final_location / "lib"
        lib_dst.mkdir(parents=True, exist_ok=True)

        # Copy python version folder
        python_lib_src = lib_src / python_folder_name
        if python_lib_src.exists():
            python_lib_dst = lib_dst / python_folder_name
            print(f"-- YUP -- Copying library from {python_lib_src} to {python_lib_dst}...")
            copy_filtered_tree(python_lib_src, python_lib_dst, base_patterns)

            # Create site-packages directory
            site_packages = python_lib_dst / "site-packages"
            site_packages.mkdir(parents=True, exist_ok=True)
        else:
            print(f"-- YUP -- Warning: Python library path {python_lib_src} does not exist")

        # Copy dynamic libraries from lib root (e.g., libpython3.13.dylib)
        print(f"-- YUP -- Copying dynamic libraries from {lib_src}...")
        for item in lib_src.iterdir():
            if item.is_file():
                if any(ext in item.name for ext in ['.dylib', '.dll', '.so', '.a', '.lib']):
                    if not should_exclude(str(item), item.name, base_patterns):
                        shutil.copy2(item, lib_dst / item.name)
    else:
        print(f"-- YUP -- Warning: Library path {lib_src} does not exist")

    # Copy bin folder
    bin_src = base_python / "bin"
    if bin_src.exists():
        bin_dst = final_location / "bin"
        bin_dst.mkdir(parents=True, exist_ok=True)

        print(f"-- YUP -- Copying binaries from {bin_src} to {bin_dst}...")

        # Copy python executables and symlinks (deduplicate with set)
        executables = list(set([
            "python3",
            "python",
            f"python{version}",
            f"python{args.version_major}",
            f"python{args.version_major}.{args.version_minor}",
        ]))

        for executable in executables:
            exe_path = bin_src / executable
            dst_path = bin_dst / executable

            if exe_path.exists():
                # Skip if destination already exists
                if dst_path.exists():
                    continue

                if exe_path.is_symlink():
                    # Preserve symlinks
                    link_target = os.readlink(exe_path)
                    os.symlink(link_target, dst_path)
                else:
                    shutil.copy2(exe_path, dst_path)
    else:
        print(f"-- YUP -- Warning: Binary path {bin_src} does not exist")

    # Copy include folder
    include_src = base_python / "include"
    if include_src.exists():
        include_dst = final_location / "include"
        include_dst.mkdir(parents=True, exist_ok=True)

        print(f"-- YUP -- Copying include files from {include_src} to {include_dst}...")

        # Copy the python version include folder
        python_include_src = include_src / python_folder_name
        if python_include_src.exists():
            python_include_dst = include_dst / python_folder_name
            shutil.copytree(python_include_src, python_include_dst, dirs_exist_ok=True)
        else:
            # Try copying the whole include folder
            for item in include_src.iterdir():
                if item.is_dir():
                    shutil.copytree(item, include_dst / item.name, dirs_exist_ok=True)
                else:
                    shutil.copy2(item, include_dst / item.name)
    else:
        print(f"-- YUP -- Warning: Include path {include_src} does not exist")

    # Clean up duplicate libraries
    print(f"-- YUP -- Cleaning up duplicate libraries...")
    clean_duplicate_libraries(final_location)

    # Create archive from final_location contents (not including the python/ wrapper)
    print(f"-- YUP -- Making archive {temp_archive} to {final_archive}...")
    if os.path.exists(final_archive):
        make_archive(temp_archive, final_location)
        if file_hash(temp_archive) != file_hash(final_archive):
            shutil.copy(temp_archive, final_archive)
            os.remove(temp_archive)
            print("-- YUP -- Archive updated")
        else:
            os.remove(temp_archive)
            print("-- YUP -- Archive unchanged")
    else:
        make_archive(final_archive, final_location)
        print("-- YUP -- Archive created")

    # Clean up temporary directory
    print(f"-- YUP -- Cleaning up {final_location}...")
    shutil.rmtree(final_location)
