from sys import stdout, exit
from os import listdir, remove, name as osname
from os.path import splitext, join as joinp
import subprocess as sp

COMPILER = "c++"
CXXFLAGS = [
    "-std=c++11",
    "-Wall", "-Wextra",
    "-Wno-missing-braces", # clang false positive
    "-I."
]
COLORS = (osname != "nt")

nsuccess = 0
nfailed  = 0

def print_result(modname, fmsg = None):
    global nsuccess, nfailed
    if fmsg:
        if COLORS:
            print "%s...\t\033[91m\033[1m(%s)\033[0m" % (modname, fmsg)
        else:
            print "%s...\t(%s)" % (modname, fmsg)
        nfailed += 1
    else:
        if COLORS:
            print "%s...\t\033[92m\033[1m(success)\033[0m" % modname
        else:
            print "%s...\t(success)" % modname
        nsuccess += 1

for fname in listdir("tests"):
    (modname, modext) = splitext(fname)

    if modext != ".cpp":
        continue

    srcpath = joinp("tests", fname)
    exepath = joinp("tests", modname)

    pc = sp.Popen([ COMPILER, srcpath, "-o", exepath ] + CXXFLAGS,
        stdout = sp.PIPE, stderr = sp.STDOUT)
    stdout.write(pc.communicate()[0])

    if pc.returncode != 0:
        print_result(modname, "compile error")
        continue

    pc = sp.Popen(exepath, stdout = sp.PIPE, stderr = sp.STDOUT)
    stdout.write(pc.communicate()[0])

    if pc.returncode != 0:
        remove(exepath)
        print_result(modname, "runtime error")
        continue

    remove(exepath)
    print_result(modname)

if COLORS:
    print "\n\033[94m\033[1mtesting done:\033[0m"
    print "\033[92mSUCCESS\033[0m: \033[1m%d\033[0m" % nsuccess
    print "\033[91mFAILURE\033[0m: \033[1m%d\033[0m" % nfailed
else:
    print "\ntesting done:"
    print "SUCCESS: %d" % nsuccess
    print "FAILURE: %d" % nfailed