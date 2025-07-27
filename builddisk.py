import os
import re
import argparse
import sys
import random
import string
from pathlib import Path
from d64 import DiskImage

def is_valid_agi_game(path):
    required_files = ['LOGDIR', 'PICDIR', 'SNDDIR', 'VIEWDIR', 'WORDS.TOK', 'OBJECT']
    files = [f.upper() for f in os.listdir(path)]
    return all(req in files for req in required_files)

def get_game_name(path):
    return re.sub(r'[^A-Za-z0-9_]', '_', os.path.basename(os.path.abspath(path)))

def add_files_to_disk_image(src_path, base_disk_image_path, output_path, autoboot=False, verbose=False):
    try:
        if (verbose):
            print(f"Adding files from {src_path} to disk image {output_path}...")
        with DiskImage(base_disk_image_path) as source_d:
            if (verbose):
                print(f"Base disk image opened: {base_disk_image_path}")
            disk_name = output_path.split('.')[0].upper().encode('ascii', 'strict')
            if (verbose):
                print(f"Disk name set to: {disk_name.decode('ascii')}")
            disk_id = ''.join(random.choices(string.ascii_uppercase + string.digits, k=2))
            if (verbose):
                print(f"Disk ID generated: {disk_id}")
            DiskImage.create('d81', Path(output_path), disk_name, disk_id.encode('ascii', 'strict'))
            with DiskImage(Path(output_path), mode='w') as dest_d:
                if (verbose):
                    print(f"New disk image created: {output_path}")
                for file in source_d.iterdir():
                    with file.open() as source_f:
                        if (autoboot and file.name == b'AGI.C65' and file.entry.file_type == 'PRG'):
                            target_name = b'AUTOBOOT.C65' 
                        else:
                            target_name = file.name
                        with dest_d.path(target_name).open('w', ftype=file.entry.file_type) as dest_f:
                            if (verbose):
                                print(f"Copying file {file.name.decode('ascii')} to disk image as {target_name.decode('ascii')}:{file.entry.file_type}...")
                            dest_f.write(source_f.read())
                for filename in os.listdir(src_path):
                    file_path = os.path.join(src_path, filename)
                    if os.path.isfile(file_path):
                        with open(file_path, 'rb') as f:
                            data = f.read()
                        if (len(filename) > 16):
                            print(f"Warning: Filename '{filename}' is longer than 16 characters, failed creating image.")
                            return False
                        cbm_filename = filename[:16].upper().encode('ascii', 'strict')
                        with dest_d.path(cbm_filename).open('w', ftype='seq') as dest_f:
                            if (verbose):
                                print(f"Importing file {filename} to disk image as {cbm_filename.decode('ascii')}:SEQ...")
                            dest_f.write(data)
        return True
    except ValueError as e:
        print(f"Invalid character in filename.")
        return False

def main():
    parser = argparse.ArgumentParser(description="Build a MEGA65 AGI game disk image from a folder of AGI files.")
    parser.add_argument("path", help="Path to the AGI game directory")
    parser.add_argument("-v", "--verbose", action="store_true", help="Enable verbose output")
    parser.add_argument("-a", "--autoboot", action="store_true", help="Create an autostart disk image")
    args = parser.parse_args()

    game_path = args.path
    if not os.path.isdir(game_path):
        print(f"Error: {game_path} is not a directory.")
        sys.exit(1)

    if (len(game_path) > 16):
        print(f"Warning: Game path '{game_path}' is longer than 16 characters, failed creating image.")
        sys.exit(1)

    if not is_valid_agi_game(game_path):
        print("The specified path does not appear to be a valid Sierra AGI game.")
        sys.exit(1)

    game_name = get_game_name(game_path)
    output_image = f"{game_name}.d81"
    base_disk_image = "mega65-agi.d81"

    if not os.path.isfile(base_disk_image):
        print(f"Base disk image '{base_disk_image}' not found.")
        sys.exit(1)

    if (add_files_to_disk_image(game_path, base_disk_image, output_image, autoboot=args.autoboot, verbose=args.verbose)):
        print(f"Game disk image created: {output_image}")
        print(f"Transfer the image to your MEGA65 using m65connect or mega65_ftp.")

if __name__ == "__main__":
    main()
