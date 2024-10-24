import os, logging, sys

import parser_interface as PARSER_INTF

import pdb

# Creating a logger object
logger = logging.getLogger(__name__.split('.')[0])

logger.setLevel(logging.INFO)

gCommentLine = "/"*60
gCommentStart = "//"
gProtocVersion='syntax = "proto3";'
gIndent = "    "

gProtoTypeString = "string"
gProtoTypeInt = "int32"
gProtoTypeLongInt = 'int64'
gProtoTypeFloat = "float"
gProtoTypeDouble = "double"
gXMLCompulsoryFunctionPrefix = "api_pa_pc_"
gProtoFunctionPrefix = "PC_"
gXMLCompulsoryFunctionPtrSuffix = "_Fptr"
gProtoFunctionPtrPrefix = "PA_"
gProtoServiceKeyword = "service"
gProtoMessageKeyword = "message"
gProtoNamepsaceSuffix = "_peer_to_peer"

def service_from_filename(filename):
    service_name = filename.replace("_", "") # eat all under-scores
    service_name = service_name.lower()
    char_list = list(service_name)
    char_list[0] = char_list[0].upper()
    service_name = "".join(char_list)

    return service_name

def remap_type(type, array):
    global gProtoTypeString
    global gProtoTypeInt
    global gProtoTypeLongInt
    global gProtoTypeFloat

    possible_string_types = ["INT8", "UINT8"]
    possible_int_types = ["INT8", "UINT8", "INT16", "UINT16", "INT32", "UINT32"]
    possible_long_int_types = ["INT64", "UINT64"]
    possible_float_types = ["FLOAT"]
    possible_double_types = ["DOUBLE"]
    not_an_array = ""

    if type in possible_string_types:
        # qualifies as a string only if it is also an array
        if array == "repeated":
            return gProtoTypeString, not_an_array

    if type in possible_int_types:
        return gProtoTypeInt, array
    
    if type in possible_long_int_types:
        return gProtoTypeLongInt, array

    if type in possible_float_types:
        return gProtoTypeFloat, array

    if type in possible_double_types:
        return gProtoTypeDouble, array
    
    return type, array # custom?

class InterfaceGen(PARSER_INTF.XMLToCode):
    def __init__(self, xmlFile, schemaFile, headerFile, sourceFile):
        super().__init__(xmlFile, schemaFile, headerFile, sourceFile)
        
        logger.info("Instantiating Proto InterfaceGen")

        meta_node = self.get_meta_node()

        self.meta_gen = ProtoMetadataGen(meta_node, meta_node)
        self.imports_gen = ProtoImportsGen(meta_node, self.get_imports_node())
        self.types_gen = ProtoTypesGen(meta_node, self.get_types_node())
        self.callbacks_gen = ProtoCallbacksGen(meta_node, self.get_callbacks_node())
        self.functions_gen = ProtoFunctionsGen(meta_node, self.get_functions_node())

class ProtoMetaField(PARSER_INTF.MetaField):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

    def declaration_file_start(self, targetFile):
        if self.suppress_key:
            field_string = "{}  {}\n\n".format(gCommentStart, self.value)
        else:
            field_string = "{}  {}: {}\n\n".format(gCommentStart, self.name, self.value)

        targetFile.write(field_string)

class ProtoMetadataGen(PARSER_INTF.MetadataGen):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

        for x in self.xml_meta_fields:
            self.meta_fields.append(ProtoMetaField(xmlMetaData, x))

        logger.debug("Collected meta-data fields {}".format(self.meta_fields))

        for m in self.meta_fields:
            logger.debug(m)

    def declaration_file_start(self, targetFile):
        targetFile.write("{}\n".format(gCommentLine))
        targetFile.write("{}\n".format(gCommentStart))
        for field in self.meta_fields:
            field.declaration_file_start(targetFile)
        targetFile.write("{}\n".format(gCommentStart))
        targetFile.write("{}\n\n\n".format(gCommentLine))

class ProtoImport(PARSER_INTF.Import):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

    def declaration_file_start(self, targetFile):
        if self.should_print("Proto", "Interface"):
            targetFile.write("import \"{}\";\n".format(self.name))

class ProtoImportsGen(PARSER_INTF.ImportsGen):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

        logger.debug("Collected imports {}".format(self.xml_imports))

        for x in self.xml_imports:
            self.imports.append(ProtoImport(xmlMetaData, x))

        for i in self.imports:
            logger.debug(i)

    def declaration_file_start(self, targetFile):
        global gProtocVersion
        global gProtoNamepsaceSuffix

        base_file_name = PARSER_INTF.get_file_basename_without_extension(targetFile.name)
        targetFile.write("\n{}\n\n".format(gProtocVersion))

        for i in self.imports:
            i.declaration_file_start(targetFile)

        targetFile.write("\npackage {}{};\n\n".format(base_file_name, gProtoNamepsaceSuffix))

class ProtoEnumValue(PARSER_INTF.EnumValue):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

        for i in self.imports:
            i.declaration_file_start(targetFile)

class ProtoEnumValue(PARSER_INTF.EnumValue):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

    def declaration_file_body(self, targetFile):
         targetFile.write("    {:<32} = {}; // {}\n".format(self.name, self.value, self.description))

class ProtoEnum(PARSER_INTF.Enum):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

        for x in self.xml_enum_values:
            self.enum_values.append(ProtoEnumValue(xmlMetaData, x))

    def declaration_file_body(self, targetFile):
        targetFile.write("enum {} {}\n".format(self.name, "{"))

        for e in self.enum_values:
            e.declaration_file_body(targetFile)

        targetFile.write('}\n\n')

class ProtoField(PARSER_INTF.Field):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        self.array = ""

        if self.array_xml != None and self.array_xml != "0":
            self.array = "repeated"

        self.modified_type, self.modified_array = remap_type(self.type, self.array)

    def declaration_file_body(self, targetFile):
        global gIndent
        if len(self.modified_array) != 0:
            type_string = "{} {}".format(self.modified_array, self.modified_type)
        else:
            type_string = self.modified_type
        targetFile.write("{}{:<32}{:<16} = {}; // {}\n".format(gIndent, type_string, self.name, self.sequence, self.description))

class ProtoStruct(PARSER_INTF.Struct):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

        for x in self.xml_fields:
            field = ProtoField(xmlMetaData, x)
            if field.check_applicability(PARSER_INTF.Generator.attrib_consumed_by_Peer):
                self.fields.append(field)

    def declaration_file_body(self, targetFile):
        global gProtoMessageKeyword

        targetFile.write("{} {} {}\n".format(gProtoMessageKeyword, self.name, "{"))
        for f in self.fields:
            f.declaration_file_body(targetFile)
        targetFile.write("}\n\n")

class ProtoTypesGen(PARSER_INTF.TypesGen):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

        for e in self.xml_enums:
            an_enum = ProtoEnum(xmlMetaData, e)
            if an_enum.check_applicability(PARSER_INTF.Generator.attrib_consumed_by_Peer):
                self.enums.append(an_enum)

        for s in self.xml_structs:
            a_struct = ProtoStruct(xmlMetaData, s)
            if a_struct.check_applicability(PARSER_INTF.Generator.attrib_consumed_by_Peer):
                self.structs.append(a_struct)

    def declaration_file_body(self, targetFile):
        for e in self.enums:
            e.declaration_file_body(targetFile)

        for s in self.structs:
            s.declaration_file_body(targetFile)

class ProtoFuncParam(PARSER_INTF.FuncParam):
    def __init__(self, xmlMetaData, xmlElement, is_first, is_last):
        super().__init__(xmlMetaData, xmlElement)
        self.is_first = is_first
        self.is_last = is_last
        self.type_without_pointer = PARSER_INTF.get_typename_without_pointer(self.type)
        self.modified_type, self.modified_array = remap_type(self.type_without_pointer, None)

    def declaration_file_body(self, targetFile):
        if self.is_last:
            var_string = "{}".format(self.modified_type)
        else:
            var_string = "{}, ".format(self.modified_type)

        targetFile.write("{}".format(var_string))

class ProtoFunction(PARSER_INTF.Function):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        param_count = 0

        self.mutated_name = self.name

        is_first = None
        is_last = None

        temp_xml_param_list = []

        for index, param in enumerate(self.xml_parameters):
            temp_param = ProtoFuncParam(xmlMetaData, param, False, False)
            if temp_param.check_applicability(PARSER_INTF.Generator.attrib_consumed_by_Peer):
                logger.debug("For function {}, param {} of type {} is applicable to peer because consumed_by = {}".format(self.name, temp_param.variable_name, temp_param.type, temp_param.consumed_by))
                temp_xml_param_list.append(temp_param)

        list_max_idx = len(temp_xml_param_list) - 1

        for index, temp_xml_entry in enumerate(temp_xml_param_list):
            if index == 0:
                is_first = True
            else:
                is_first = False

            if index == list_max_idx:
                is_last = True
            else:
                is_last = False

            temp_xml_entry.is_first = is_first
            temp_xml_entry.is_last = is_last
            self.parameters.append(temp_xml_entry)

    def declaration_file_body(self, targetFile):
        global gIndent

        targetFile.write("{}rpc {}(".format(gIndent, self.mutated_name))
        for p in self.parameters:
            p.declaration_file_body(targetFile)

        targetFile.write(") returns ({}) {}{}\n".format(self.return_spec.get_return_type(PARSER_INTF.FunctionReturnType.RETURN_TYPE_peer), "{", "}"))

class ProtoCallbacksGen(PARSER_INTF.CallbacksGen):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

        global gXMLCompulsoryFunctionPtrSuffix
        global gProtoFunctionPtrPrefix

        for fptr in self.xml_fptrs:
            proto_func_ptr = ProtoFunction(xmlMetaData, fptr)
            if proto_func_ptr.check_applicability(PARSER_INTF.Generator.attrib_consumed_by_Peer):
                if proto_func_ptr.name.endswith(gXMLCompulsoryFunctionPtrSuffix):
                    proto_func_ptr.mutated_name = proto_func_ptr.name.replace(gXMLCompulsoryFunctionPtrSuffix, "")
                    proto_func_ptr.mutated_name = gProtoFunctionPtrPrefix + proto_func_ptr.mutated_name
                    self.fptrs.append(proto_func_ptr)
                else:
                    logger.critical('API function-ptr {} must end with prefix {}'.format(proto_func_ptr.name, gXMLCompulsoryFunctionPtrPrefix))
                    sys.exit(-1)

        logger.debug("Collected fptrs {}".format(self.fptrs))
        for f in self.fptrs:
            logger.debug(str(f))

    def declaration_file_end(self, targetFile):
        global gProtoServiceKeyword

        file_name = PARSER_INTF.get_file_basename_without_extension(targetFile.name)
        service_name = service_from_filename(file_name) + "ApplicationCallback" 

        targetFile.write("{} {} {}\n".format(gProtoServiceKeyword, service_name, "{"))

        for fptr in self.fptrs:
            fptr.declaration_file_body(targetFile)

        targetFile.write("{}\n\n".format("}"))

class ProtoFunctionsGen(PARSER_INTF.FunctionsGen):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

        global gXMLCompulsoryFunctionPrefix
        global gProtoFunctionPrefix

        for f in self.xml_funcs:
            proto_func = ProtoFunction(xmlMetaData, f)
            if proto_func.check_applicability(PARSER_INTF.Generator.attrib_consumed_by_Peer):
                if proto_func.name.startswith(gXMLCompulsoryFunctionPrefix):
                    proto_func.mutated_name = proto_func.name.replace(gXMLCompulsoryFunctionPrefix, gProtoFunctionPrefix)
                    self.functions.append(proto_func)
                else:
                    logger.critical('API function {} must start with prefix {}'.format(proto_func.name, gXMLCompulsoryFunctionPrefix))
                    sys.exit(-1)

        logger.debug("Collected functions {}".format(self.functions))
        for f in self.functions:
            logger.debug(str(f))

    def declaration_file_end(self, targetFile):
        global gProtoServiceKeyword

        file_name = PARSER_INTF.get_file_basename_without_extension(targetFile.name)
        service_name = service_from_filename(file_name) + "PayloadController" 

        targetFile.write("{} {} {}\n".format(gProtoServiceKeyword, service_name, "{"))

        for func in self.functions:
            func.declaration_file_body(targetFile)

        targetFile.write("{}\n\n".format("}"))