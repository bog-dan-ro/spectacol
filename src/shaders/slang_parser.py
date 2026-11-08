#!/bin/python3

import os
import re
import sys

def read_file_with_includes(filename, included_files=None):
    if included_files is None:
        included_files = set()

    full_text = []
    base_dir = os.path.dirname(filename)

    if filename in included_files:
        return ""  # Avoid circular includes
    included_files.add(filename)

    try:
        with open(filename, "r") as f:
            for line in f:
                include_match = re.match(r'#include\s+"(.+?)"', line)
                optional_include_match = re.match(r'#pragma\s+include_optional\s+"(.+?)"', line)

                if include_match:
                    include_path = os.path.join(base_dir, include_match.group(1))
                    full_text.append(read_file_with_includes(include_path, included_files))
                elif optional_include_match:
                    include_path = os.path.join(base_dir, optional_include_match.group(1))
                    try:
                        full_text.append(read_file_with_includes(include_path, included_files))
                    except FileNotFoundError:
                        print(f"[warning] optional include missing: {include_path}")
                        pass
                else:
                    full_text.append(line)
    except FileNotFoundError:
        raise FileNotFoundError(f"File not found: {filename}")

    return ''.join(full_text)

def parse_slang_file(filename):
    content = read_file_with_includes(filename)
    lines = content.splitlines()

    common_lines = []
    stages = {}
    current_stage = None
    stage_encountered = False

    for line in lines:
        stage_match = re.match(r'\s*#pragma\s+stage\s+(\w+)', line)
        if stage_match:
            current_stage = stage_match.group(1)
            stage_encountered = True
            if current_stage not in stages:
                stages[current_stage] = []
        elif not stage_encountered:
            common_lines.append(line)
        else:
            if current_stage:
                stages.setdefault(current_stage, []).append(line)

    if not stages:
        stages["fragment"] = lines

    for stage in stages:
        stages[stage] = common_lines + stages[stage]

    return stages

def write_stage_files(original_filename, stages):
    base_dir = os.path.dirname(original_filename)
    base_name = os.path.basename(original_filename)
    base_name_no_ext = base_name.rsplit(".", 1)[0]

    for stage, lines in stages.items():
        ext = {
            "vertex": "vert",
            "fragment": "frag",
            "frag": "frag",
            "vert": "vert"
        }.get(stage, stage)

        output_file = os.path.join(base_dir, f"{base_name_no_ext}.slang.{ext}")
        with open(output_file, "w") as f:
            f.write('\n'.join(lines) + '\n')
        print(f"Wrote: {output_file}")

def process_slang_file(filepath):
    try:
        stages = parse_slang_file(filepath)
        write_stage_files(filepath, stages)
    except Exception as e:
        print(f"Error processing {filepath}: {e}")

def process_all_slang_files(directory):
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(".slang"):
                process_slang_file(os.path.join(root, file))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("No file or folder given. Processing all .slang files recursively in current directory.")
        process_all_slang_files(os.getcwd())
    else:
        input_path = sys.argv[1]
        if os.path.isdir(input_path):
            process_all_slang_files(input_path)
        elif os.path.isfile(input_path):
            process_slang_file(input_path)
        else:
            print(f"{input_path} is not a valid file or directory.")
