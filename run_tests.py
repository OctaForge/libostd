from sys import stdout, exit
from os import listdir, remove
from os.path import splitext
import subprocess as sp

COMPILER = "c++"
CXXFLAGS="-std=c++11 -Wall -Wextra -I."

for fname in listdir("./tests"):
    if fname.endswith(".cpp"):
        modname = splitext(fname)[0]
        stdout.write("%s...\t" % modname)
        pc = sp.Popen("%s tests/%s -o tests/%s %s"
            % (COMPILER, fname, modname, CXXFLAGS), shell = True,
                stdout = sp.PIPE, stderr = sp.STDOUT)
        pcdata = pc.communicate()[0]
        if pc.returncode != 0:
            print "\033[91m(compile error)\033[0m"
            stdout.write(pcdata)
            exit(1)
        pc = sp.Popen("./tests/%s" % modname, shell = True,
            stdout = sp.PIPE, stderr = sp.STDOUT)
        pcdata = pc.communicate()[0]
        if pc.returncode != 0:
            remove("./tests/%s" % modname)
            print "\033[91m(runtime error)\033[0m"
            stdout.write(pcdata)
            exit(1)
        remove("./tests/%s" % modname)
        print "\033[92m(success)\033[0m"

print "testing successful"