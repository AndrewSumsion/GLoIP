from generator_common import parseHeaderFile, parseMetaFile, writeHeader

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

    writeHeader(sourceFile, meta, "gloip_server.h")

    with open(customPath, "r") as custom:
        sourceFile.writelines(custom.readlines())
        sourceFile.write("\n")

    for functionKey in functions:
        writeFunction(sourceFile, functionKey, functions[functionKey], meta)

    writeFunctionArray(sourceFile, functions)

    sourceFile.close()