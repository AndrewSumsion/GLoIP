import sys
import os
import shutil

from client_generator import generateClient

rootDirectory = os.getcwd()
protocol = sys.argv[1]

pathString = rootDirectory + "/protocols/" + protocol + "/" + protocol

generateClient(pathString + ".h", pathString + ".h.meta", pathString + "_client_custom.cpp", rootDirectory + "/client/gloip_client_generated.cpp")

if not os.path.exists(rootDirectory + "/client/build"):
    os.mkdir(rootDirectory + "/client/build")
os.chdir(rootDirectory + "/client/build")

os.system("cmake -D GLOIP_PROTOCOL=\"" + protocol + "\" ..")
os.system("make")

os.chdir(rootDirectory)

if not os.path.exists(rootDirectory + "/out"):
    os.mkdir(rootDirectory + "/out")

shutil.copyfile(rootDirectory + "/client/build/libgloipclient.so", rootDirectory + "/out/libgloipclient.so")