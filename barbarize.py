from sys import argv
import os

prefix = "barbarized"
os.makedirs(prefix, exist_ok=True)

def barbarize(s):
    for c in s:
        if ord(c) > 128:
            yield "u" + str(ord(c))
        else:
            yield c

for filename in argv[1:]:
    print(filename)

    with open(filename, 'r') as fin:
        data = fin.read()

    outfile = os.path.join(prefix, filename)
    os.makedirs(os.path.dirname(outfile), exist_ok=True)

    with open(outfile, 'w') as fout:
        for c in barbarize(data):
            fout.write(c)
