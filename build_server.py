import sys
import os
import shutil

from server_generator import generateServer

rootDirectory = os.getcwd()
protocol = sys.argv[1]

pathString = rootDirectory + "/protocols/" + protocol + "/" + protocol

generateServer(pathString + ".h", pathString + ".h.meta", pathString + "_server_custom.cpp", rootDirectory + "/server/gloip_server_generated.cpp")

if not os.path.exists(rootDirectory + "/server/build"):
    os.mkdir(rootDirectory + "/server/build")
os.chdir(rootDirectory + "/server/build")

os.system("cmake -D GLOIP_PROTOCOL=\"" + protocol + "\" ..")
os.system("make")
os.system("make install")

os.chdir(rootDirectory)

if not os.path.exists(rootDirectory + "/out"):
    os.mkdir(rootDirectory + "/out")

#shutil.copyfile(rootDirectory + "/server/build/libgloipserver.so", rootDirectory + "/out/libgloipserver.so")