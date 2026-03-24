import os
from collections import deque

INSTANCE_DIR = "tiny02"

files_in_instance_dir = os.listdir(INSTANCE_DIR)

for file in files_in_instance_dir:
    if file.endswith(".nw"):
        with open(os.path.join(INSTANCE_DIR, file), "r") as f:
            lines = f.readlines()
            highest_depths = []
            trees = 0
            leaves = 0
            for line in lines:
                bracket_stack = deque()
                max_depth = 0
                if line.startswith("#p"):
                  parts = line.split()
                  trees = int(parts[1])
                  leaves = int(parts[2])
                elif line.startswith("#"):
                    continue
                if line.startswith("("):
                  for i in line:
                      if i == "(":
                          bracket_stack.append(i)
                      elif i == ")":
                          if bracket_stack:
                              bracket_stack.pop()
                      max_depth = max(max_depth, len(bracket_stack))
                  highest_depths.append(max_depth)
            # print(f"File: {file}, Trees: {trees}, Leaves: {leaves}, Depths: {highest_depths}", Max: {max(highest_depths)}, Min: {min(highest_depths)}, Avg: {sum(highest_depths) / len(highest_depths)}")
            print(file, trees, leaves, max(highest_depths), min(highest_depths), sum(highest_depths) / len(highest_depths), sep=",")
