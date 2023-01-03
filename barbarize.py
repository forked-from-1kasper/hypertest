from sys import argv

def barbarize(s):
    for c in s:
        if ord(c) > 128:
            yield "u" + str(ord(c))
        else:
            yield c

for filename in argv[1:]:
    with open(filename, 'r') as fin:
        data = fin.read()

    with open(filename, 'w') as fout:
        for c in barbarize(data):
            fout.write(c)
