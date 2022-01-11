def readHeader(file, meta):
    line = file.readline()
    while line and not line.strip().startswith("endheader"):
        meta["header"] += line
        line = file.readline()


def parseMetaFile(file):
    meta = {
        "header": "",
        "functions": {}
    }

    """
    structure of meta dictionary:
    {
        "header": "#include ...",
        "functions": {
            "exampleFunction": {
                "arg1": {"type": "blob", "expression": "10 * sizeof(GLint)"}
            }
        }
    }
    """

    currentFunction = ""
    metaLine = "."
    while metaLine:
        metaLine = file.readline()

        args = metaLine.strip().split(" ")

        if args[0] == "header":
            readHeader(file, meta)
            continue

        if args[0] == "func":
            currentFunction = args[1]
            meta["functions"][args[1]] = {}
            continue

        if args[0] == "custom":
            meta["functions"][currentFunction]["custom"] = True
            currentFunction = ""
            continue

        if args[0] == "bsize":
            startIndex = metaLine.index("bsize")
            startIndex += len("bsize " + args[1]) + 1
            meta["functions"][currentFunction][args[1]] = {
                "type": "blob",
                "expression": metaLine[startIndex:].strip()
            }
            continue

        if args[0] == "rsize":
            startIndex = metaLine.index("rsize")
            startIndex += len("rsize " + args[1]) + 1
            meta["functions"][currentFunction][args[1]] = {
                "type": "return",
                "expression": metaLine[startIndex:].strip()
            }
            continue
    
    return meta

def parseHeaderFile(file):
    functions = {}

    hashCounter = 0
    line = "."
    while line:
        line = file.readline()
        if not line.startswith("GL_APICALL"):
            continue

        retTypeLeft = len("GL_APICALL ")
        retTypeRight = line.index("GL_APIENTRY")

        retType = line[retTypeLeft:retTypeRight].strip()

        nameLeft = line.index("GL_APIENTRY ") + len("GL_APIENTRY ")
        nameRight = line.index("(")

        name = line[nameLeft:nameRight].strip()

        functionHash = hashCounter
        hashCounter += 1

        argsString = line[line.index("(") + 1 : line.index(")")].strip()

        args = []

        if argsString != "void":
            for s in argsString.split(","):
                kv = s.strip().split(" ")
                if kv[-1].startswith("*"):
                    kv[-1] = kv[-1][1:]
                    kv[-2] = kv[-2] + "*"
                args.append((kv[-1], kv[-2]))

        functions[name] = {
            "hashExpression": str(functionHash),
            "argsString": argsString,
            "args": args,
            "retType": retType
        }
    
    return functions

def writeHeader(file, meta):
    file.write(meta["header"])

    file.write("#include \"gloip_server.h\"\n")
    file.write("\n")
    file.write("#include <cstdint>\n")
    file.write("using std::uint8_t;\n")
    file.write("using std::uint32_t;\n")
    file.write("\n")
    file.write("#include <cstring>\n")
    file.write("using std::memcpy;\n")
    file.write("\n")

def writeFunction(file, name, function, meta):
    hashExpression = function["hashExpression"]
    argsString = function["argsString"]
    args = function["args"]
    retType = function["retType"]

    metaArgs = None
    if name in meta["functions"]:
        metaArgs = meta["functions"][name]
    
    if metaArgs != None and "custom" in metaArgs and metaArgs["custom"]:
        # don't generate code for a custom function
        return

    declarationString = "void gloipRedirect_" + name + "(bool* returnedSomething, uint8_t* returnSize, void* returnLocation, uint8_t numArgs, Argument** args) {\n"
    file.write(declarationString)

    if retType != "void":
        file.write("    *returnedSomething = true;\n")
        file.write("    *returnSize = sizeof(" + retType + ");\n")
        file.write("    " + retType + " ret;\n\n")
    else:
        file.write("    *returnedSomething = false;\n")
        file.write("    *returnSize = 0;\n\n")
    
    argIndex = 0
    for argName, argType in args:
        argumentType = "PrimitiveArgument"
        if metaArgs != None and argName in metaArgs:
            if metaArgs[argName]["type"] == "blob":
                argumentType = "BlobArgument"
            if metaArgs[argName]["type"] == "return":
                argumentType = "BlobReturnArgument"

        typeWithModifiers = argType
        if argumentType == "BlobArgument":
            typeWithModifiers = "const " + typeWithModifiers
        
        argLine = "    " + typeWithModifiers + " arg" + str(argIndex + 1) + " = (" + typeWithModifiers + ")((("

        argLine += argumentType + "*)args[" + str(argIndex) + "])->"

        if argumentType == "PrimitiveArgument":
            if argType == "float" or argType == "GLfloat":
                argLine += "dataFloat"
            elif argType == "double" or argType == "GLdouble":
                argLine += "dataDouble"
            else:
                argLine += "dataInteger"
        
        if argumentType == "BlobArgument":
            argLine += "data"
        
        if argumentType == "BlobReturnArgument":
            argLine += "destination"
        
        argLine += ");\n"
        file.write(argLine)

        argIndex += 1
    
    file.write("\n")
    
    invocationLine = "    ret = " if retType != "void" else "    "
    invocationLine += name + "("

    if len(args) > 0:
        for i in range(len(args) - 1):
            invocationLine += "arg" + str(i + 1) + ", "
        invocationLine += "arg" + str(len(args))
    invocationLine += ");\n"

    file.write(invocationLine)

    if retType != "void":
        file.write("\n    memcpy(returnLocation, &ret, *returnSize);\n")

    file.write("}\n\n")

def writeFunctionArray(file, functions):
    file.write("static GloipFunction functionArray[] = {\n")

    counter = 0
    for f in functions:
        if functions[f]["hashExpression"] != str(counter):
            # make sure function hashes are in order starting at 0
            raise Exception("Hashes are not in order!")
        
        file.write("    gloipRedirect_" + f + ",\n")

        counter += 1
    
    file.write("};\n\n")

    file.write("GloipFunction* gloip_getGloipFunctionArray() {\n")
    file.write("    return functionArray;\n")
    file.write("}\n\n")

def generateServer(headerPath, metaPath, customPath, sourcePath):
    metaFile = open(metaPath, "r")
    meta = parseMetaFile(metaFile)
    metaFile.close()

    headerFile = open(headerPath, "r")
    functions = parseHeaderFile(headerFile)
    headerFile.close()

    sourceFile = open(sourcePath, "w")

    writeHeader(sourceFile, meta)

    with open(customPath, "r") as custom:
        sourceFile.writelines(custom.readlines())
        sourceFile.write("\n")

    for functionKey in functions:
        writeFunction(sourceFile, functionKey, functions[functionKey], meta)

    writeFunctionArray(sourceFile, functions)

    sourceFile.close()