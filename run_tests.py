from sys import stdout, exit
from os import listdir, remove
from os.path import splitext
import subprocess as sp

COMPILER = "c++"
# -Wno-missing-braces because clang false positive
CXXFLAGS="-std=c++11 -Wall -Wextra -Wno-missing-braces -I."

nsuccess = 0
nfailed  = 0

for fname in listdir("./tests"):
    (modname, modext) = splitext(fname)

    if modext != ".cpp":
        continue

    pc = sp.Popen("%s tests/%s -o tests/%s %s"
        % (COMPILER, fname, modname, CXXFLAGS), shell = True,
            stdout = sp.PIPE, stderr = sp.STDOUT)
    stdout.write(pc.communicate()[0])

    if pc.returncode != 0:
        print "%s...\t\033[91m\033[1m(compile error)\033[0m" % modname
        nfailed += 1
        continue

    pc = sp.Popen("./tests/%s" % modname, shell = True,
        stdout = sp.PIPE, stderr = sp.STDOUT)
    stdout.write(pc.communicate()[0])

    if pc.returncode != 0:
        remove("./tests/%s" % modname)
        print "%s...\t\033[91m\033[1m(runtime error)\033[0m" % modname
        nfailed += 1
        continue

    remove("./tests/%s" % modname)
    print "%s...\t\033[92m\033[1m(success)\033[0m" % modname
    nsuccess += 1

print "\n\033[94m\033[1mtesting done:\033[0m"
print "\033[92mSUCCESS\033[0m: %d" % nsuccess
print "\033[91mFAILURE\033[0m: %d" % nfailed