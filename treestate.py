#!/usr/bin/env python3
# Record the current state of a tree in the filesystem

import os
import sys
import json

DIR = 1
LINK = 2
FILE = 3

# Scan filesystem
def scanTreeState(path):
    if os.path.islink(path):
        yield [LINK, path ]
    elif os.path.isdir(path):
        yield [DIR, path]
        for leaf in os.listdir(path):
            p = os.path.join(path, leaf)
            for item in scanTreeState(p):
                yield item
    else:
        yield [FILE, path, os.path.getmtime(path)]

# Write the tree in JSON format
def writeTree(filename, tree):
    with open(filename, 'w') as f:
        json.dump(list(tree), f, indent=1)

def readTree(filename):
    with open(filename, "r") as f:
        if f == None:
            print(f"*** couldn't open {filename} ***")
        return json.load(f)

# Find updates in tree between states A and B
def findUpdates(a, b):
    a_map = {}

    # Build mapping of filename -> modified time in A
    for o in a:
        if o[0] == FILE:
            a_map[ o[1] ] = o[2]
        else:
            a_map[ o[1] ] = True

    # Find updated or new items in B
    for o in b:
        if o[1] in a_map:
            if o[0] == FILE and o[2] != a_map[ o[1] ]:
                print(o[1])

        elif o[0] == FILE or o[0] == LINK:
            print(o[1])


def help():
    print("Syntax:")
    print(f"  {sys.argv[0]} scan path stateFile")
    print(f"      Record the state of <path> in <stateFile>")
    print(f"  {sys.argv[0]} updates stateFile path")
    print(f"      Print the paths of objects in <path> which have updated since <stateFile>")
    exit()


# Process arguments
if len(sys.argv) < 2:
    help()

if sys.argv[1] == 'scan':
    if len(sys.argv) != 4:
        help()

    path = sys.argv[2]
    stateFile = sys.argv[3]

    tree = scanTreeState(path)
    writeTree(stateFile, tree)

elif sys.argv[1] == 'updates':
    if len(sys.argv) != 4:
        help()

    a = sys.argv[2]
    b = sys.argv[3]

    findUpdates(readTree(a), scanTreeState(b))

else:
    print(f"Unknown command: {sys.argv[1]}")
    help()

