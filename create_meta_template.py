import sys
import os

docComment = """// this file is parsed by reading the first space-delimited string of each line
// preceding spaces are ignored
// // is a valid comment
// an empty line is ignored

// Commands:
//   header/endheader:
//     every line coming after header and before endheader will be inserted into the
//     source file at the beginning. any macros should be defined here
//   func <funcName>:
//     specifies the name of the function for which following metadata is specified
//   bsize <arg> <expression>:
//     specifies the size in bytes of an argument that is a blob of data
//     arg is the name of the argument as defined in the header
//     expression is a C++ expression that determines the length of the data
//     other arguments of the function can and should be used in this expression
//     /string/ is a special case, which assumes arg is a null-terminated string
//   rsize <arg> <expression>:
//     specifies that the given argument is a pointer that will be written to
//     expression is a C++ expression that determines the size of data that is allocated
//     and can be validly written to
//     other arguments of the function can and should be used in this expression
//   custom:
//     specifies that this function should not be generated, and has custom code defined
//     in the accompanying code file
//   blocking:
//     denotes that this function should always wait for a response from the server,
//     regardless of whether anything gets returned

"""

def createMetaTemplate(headerName, headerIn, metaOut):
    metaOut.write(docComment)
    metaOut.write("header\n#include \"" + headerName + "\"\nendheader\n\n")

    hashCounter = 0
    line = "."
    while line:
        line = headerIn.readline()
        if not line.startswith("GL_APICALL"):
            continue

        if not "*" in line:
            continue

        nameLeft = line.index("GL_APIENTRY ") + len("GL_APIENTRY ")
        nameRight = line.index("(")

        name = line[nameLeft:nameRight].strip()

        metaOut.write("func " + name + "\n    \n\n")

headerPath = sys.argv[1]
metaPath = sys.argv[2]

headerName = os.path.basename(headerPath)

headerIn = open(headerPath, "r")
metaOut = open(metaPath, "w")

createMetaTemplate(headerName, headerIn, metaOut)

headerIn.close()
metaOut.close()