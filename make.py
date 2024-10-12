import configparser
import subprocess
import os
import sys

# Read the config file
inifile  =  configparser.ConfigParser(interpolation = None)
inifile.read("config.ini")
conf = inifile["config"]

# Extract the items needed
cc = conf["compiler"]
link = conf["linker"]
qemu = conf["qemu"]
python = conf["python"]
userlinker = conf["userlinker"]
ar = conf["ar"]

# Create some flags for the compiler
cflags=[
    "-target", "i686-pc-win32", #target platform=Windows 32 bit
    "-m32",                     #force 32 bit
    "-Wall",                    #all warnings
    "-Werror",                  #make warnings into errors
    "-fno-builtin",             #don't use any builtin functions
    "-ffreestanding",           #don't use any libraries
    "-mno-sse",                 #don't use cpu's sse registers
    "-nostdinc",                #no standard library includes
    "-nostdlib",                #no standard library functions
    "-I.",                      #use current directory for includes
    "-c"                        #compile files
]

# Create options for the link phase
linkflags=[
    "/base:0x100000",       #where kernel gets loaded
    "/machine:x86",         #32 bit
    "/nodefaultlib",        #no standard libraries
    "/out:kernel.exe",      #output filename
    "/subsystem:console",   #console app
    "/fixed",               #not relocatable
    "/entry:start",         #function to be called on startup
    "/map:kernel.syms"      #kernel symbols
]

# Helper function
def run(*args):
    print(args)
    subprocess.check_call(*args)

# Take each .c file and compile it then link everything together
objectfiles=[]
for filename in os.listdir("."):
    if filename.endswith(".c"):
        obj=filename+".o"
        run(
            [cc] + cflags + ["-o",obj,filename]
        )
        objectfiles.append(obj)
run( [link] + linkflags + objectfiles )

import testsuite
run( [
    python, "fool.pyz", "hd.img",
    "create","64",
    "cp","kernel.exe","KERNEL.EXE"
])
testsuite.copy(python,"hd.img")

# Make a virtual 64MB disk and copy the kernal to it
# run( [
#     python, "fool.pyz", "hd.img",
#     "create","64",
#     "cp","kernel.exe","KERNEL.EXE",

#     #begin new stuff
#     "cp","article1.txt","ARTICLE1.TXT",
#     "cp","article2.txt","ARTICLE2.TXT",
#     "cp","article3.txt","ARTICLE3.TXT",
#     "cp","article4.txt","ARTICLE4.TXT",
#     "cp","article5.txt","ARTICLE5.TXT",
#     "cp","article6.txt","ARTICLE6.TXT",
#     "cp","article7.txt","ARTICLE7.TXT",
#     "cp","billofrights.txt","bill of rights.txt",
#     "cp","amendment1.txt","AMEND1.TXT",
#     "cp","amendment2.txt","amendment2.TXT",
#     "cp","amendment3.txt","amendment 3.TXT",
#     "cp","amendment4.txt","AMEND4.TXT",
#     "cp","amendment5.txt","AMEND5.TXT",
#     "cp","amendment6.txt","amend. 6.TXT",
#     "cp","amendment7.txt","a7.TXT",
#     "cp","amendment8.txt","8th amendment.txt",
#     "cp","amendment9.txt","AMEND 9.txt",
#     "cp","amendment10.txt","10.TXT"
#     #end new stuff
# ])

# Run Qemu
run( [ qemu,

    #virtual hard drive
    "-drive","format=raw,media=disk,file=hd.img,id=disk0",

    #redirect serial port to window
    "-serial","mon:stdio",

    #escape character for controlling qemu
    "-echr","96"
])