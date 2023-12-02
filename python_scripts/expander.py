import click
import re
from pathlib import Path


def expand_includes(src_path):
    lines = src_path.read_text().splitlines(True)
    src_dir = src_path.parent

    i = 0
    while i < len(lines):
        match = re.match(r'#include "(.*)"', lines[i])
        if match:
            included_file = src_dir / match.group(1)
            if included_file.exists():
                lines[i : i + 1] = (
                    ["// Begin of included file: " + str(included_file) + "\n"]
                    + expand_includes(included_file)
                    + ["\n// End of included file: " + str(included_file) + "\n"]
                )
            else:
                print(f"Warning: {included_file} does not exist")
                i += 1
        else:
            i += 1

    return lines


@click.command()
@click.argument("src_path", type=click.Path(exists=True))
@click.argument("dst_path", type=click.Path())
def main(src_path, dst_path):
    src_path = Path(src_path)
    dst_path = Path(dst_path)

    expanded_lines = expand_includes(src_path)

    output = "// This file is generated by expand_includes.py\n" + "".join(
        expanded_lines
    )
    dst_path.write_text(output)


if __name__ == "__main__":
    main()