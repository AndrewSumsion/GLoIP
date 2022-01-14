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

        if argsString != "void" and argsString != "":
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

    file.write("#include \"gloip_client.h\"\n")
    file.write("\n")
    file.write("#include <cstdint>\n")
    file.write("using std::uint8_t;\n")
    file.write("using std::uint32_t;\n")
    file.write("\n")
    file.write("#include <cstring>\n")
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
        # don't generate code for a custom function, just provide its hash for the implementation
        file.write("#define HASH_" + name + " (" + hashExpression + ")\n")
        return

    declarationString = "GL_APICALL " + retType + " GL_APIENTRY " + name + "(" + argsString + ") {\n"
    file.write(declarationString)

    file.write("    uint32_t functionHash = " + hashExpression + ";\n\n")

    returnPrimitive = retType != "void"
    waitForReturn = returnPrimitive

    if not waitForReturn and metaArgs != None:
        # check if there are any blob return arguments
        for argName, argType in args:
            if argName in metaArgs and metaArgs[argName]["type"] == "return":
                waitForReturn = True
                break
    
    file.write("    bool waitForReturn = " + ("true" if waitForReturn else "false") + ";\n")
    
    if returnPrimitive:
        file.write("    size_t returnSize = sizeof(" + retType + ");\n")
        file.write("    " + retType + " ret;\n\n")
    else:
        file.write("    size_t returnSize = 0;\n\n")
    
    argIndex = 1
    for argName, argType in args:
        argumentType = "PrimitiveArgument"
        if metaArgs != None and argName in metaArgs:
            if metaArgs[argName]["type"] == "blob":
                argumentType = "BlobArgument"
            if metaArgs[argName]["type"] == "return":
                argumentType = "BlobReturnArgument"
        
        argLine = "    " + argumentType + " arg" + str(argIndex) + " = " + argumentType

        if argumentType == "PrimitiveArgument":
            argLine += "(sizeof(" + argType + "), &" + argName + ");\n"
        if argumentType == "BlobArgument" or argumentType == "BlobReturnArgument":
            expression = metaArgs[argName]["expression"]
            if expression == "/string/":
                expression = "strlen(" + argName + ") + 1" # +1 to include null terminator
            argLine += "((" + expression + "), " + argName + ");\n"
        
        file.write(argLine)

        argIndex += 1
    
    file.write("\n")

    argumentsLine = "    Argument* args[] = {"
    for i in range(len(args)):
        argumentsLine += "&arg" + str(i + 1) + ", "
    argumentsLine += "};\n\n"

    file.write(argumentsLine)

    executeLine = "    gloip_execute(functionHash, waitForReturn, returnSize, " \
               + ("&ret" if returnPrimitive else "nullptr") + ", " + str(len(args)) + ", args);\n"
    
    file.write(executeLine)

    if returnPrimitive:
        file.write("\n    return ret;\n")

    file.write("}\n\n")


def generateClient(headerPath, metaPath, customPath, sourcePath):
    metaFile = open(metaPath, "r")
    meta = parseMetaFile(metaFile)
    metaFile.close()

    headerFile = open(headerPath, "r")
    functions = parseHeaderFile(headerFile)
    headerFile.close()

    sourceFile = open(sourcePath, "w")

    writeHeader(sourceFile, meta)

    for function in functions:
        writeFunction(sourceFile, function, functions[function], meta)
    
    with open(customPath, "r") as custom:
        sourceFile.writelines(custom.readlines())
        sourceFile.write("\n")

    sourceFile.close()