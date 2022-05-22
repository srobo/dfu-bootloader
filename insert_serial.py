#!/usr/bin/env python3
import argparse
from pathlib import Path


def pad_serial(serial_num, length=16):
    ""
    # Trim and pad to one below length to allow for a null terminator
    serial_num_truncated = serial_num[:length - 1]
    serial_num_padded = serial_num_truncated.ljust(length - 1, '\0')

    return serial_num_padded + '\0'


def insert_bin_serial(data, serial_num, addr):
    ""
    data_array = bytearray(data)  # make the data mutable

    serial_bytes = serial_num.encode('ascii')
    length = len(serial_bytes)

    # insert serial using slicing
    data_array[addr:(addr + length)] = serial_bytes

    return bytes(data_array)


def ExistingFile(val):
    """ To be used as an argparse argument type
        Tests the file exists without opening it to avoid issues with files
        not being closed: https://bugs.python.org/issue13824
    """
    f = Path(val)
    try:
        if not f.is_file():
            raise argparse.ArgumentTypeError(f'File not found {val}')
    except PermissionError as e:
        raise argparse.ArgumentTypeError(e)

    return f


def ValidPath(val):
    """ To be used as an argparse argument type
        Tests the filepath exists without creating it to avoid issues with
        files not being closed: https://bugs.python.org/issue13824
    """
    f = Path(val)
    try:
        if not f.parent.is_dir():
            raise argparse.ArgumentTypeError(f'Path does not exist {val}')
    except PermissionError as e:
        raise argparse.ArgumentTypeError(e)

    return f


def main():
    parser = argparse.ArgumentParser(
        description="Insert a serial number into a pre-compiled binary"
    )
    parser.add_argument(
        '-a', '--address', type=int, default=0x1FE0,
        help=(
            "The address offset from the start of the file to place the "
            "serial number (defaults to 0x1FE0)"
        )
    )
    parser.add_argument(
        '-l', '--length', type=int, default=16,
        help="The length of the serial number placeholder, including the null terminator (defaults to 16)"
    )
    parser.add_argument('serial_num', help="The value to set the serial number to.")
    parser.add_argument(
        'infile', type=ExistingFile,
        help="The template file to insert the serial number into.",
    )
    parser.add_argument(
        'outfile', type=ValidPath,
        help="The file to write the output to.",
    )

    args = parser.parse_args()

    data = args.infile.read_bytes()
    serial_num = pad_serial(args.serial_num, args.length)
    new_data = insert_bin_serial(data, serial_num, args.address)

    args.outfile.write_bytes(new_data)


if __name__ == "__main__":
    main()
