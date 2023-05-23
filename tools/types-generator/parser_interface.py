import sys, os
import xmlschema
from xml.etree import ElementTree
from pprint import pprint
import pdb

import logging
 
# Creating a logger object
logger = logging.getLogger(__name__.split('.')[0])
 
# Setting the threshold of logger
logger.setLevel(logging.INFO)

def get_file_basename_with_extension(filename):
    return os.path.basename(filename)

def get_file_basename_without_extension(filename):
    return os.path.splitext(get_file_basename_with_extension(filename))[0]

def get_typename_without_pointer(typename):
    return typename.split(" ")[0]

class XMLParser:
    def __init__(self, xmlFile, schemaFile):
        self.xml_file = xmlFile
        self.xml_schema = schemaFile
        # pdb.set_trace()
        logger.info("parsing schema {}".format(schemaFile))
        self.xml_schema_parsed = xmlschema.XMLSchema(schemaFile)

        logger.info('parsing xml {}'.format(xmlFile))
        self.xml_parsed = ElementTree.parse(xmlFile)
        self.xml_schema_parsed.validate(self.xml_parsed)
        if not self.xml_schema_parsed.is_valid(self.xml_parsed):
            logger.critical("Schema is not valid")
            sys.exit(-1)
        self.xml_as_dict = self.xml_schema_parsed.to_dict(self.xml_file, process_namespaces=False)
        pprint(self.xml_as_dict, depth=2, indent=1, compact=False, width=40)
        # logger.info("Able to parse {} using {} ============>".format(self.xml_file, self.xml_schema))
        # logger.info(self.xml_as_dict)
        logger.debug("Straight parsed xml {} is =====>".format(self.xml_parsed))
        logging.debug(ElementTree.tostring(self.xml_parsed.getroot()))

    def get_node(self, base_node=None, node_name=None):

        logger.debug("Lookup {} in base node {}".format(node_name, base_node))

        if None == base_node:
            base_node = self.xml_parsed.getroot()
            logger.debug("Setting base node to root {}".format(base_node))

        if None == node_name:
            logger.debug("Returning root-node, no lookup node specified")
            return base_node

        logger.debug(base_node)

        result = base_node.find(node_name)

        logger.debug("returning lookup result of {} => {}".format(node_name, result))
        return result

class FunctionReturnType:
    RETURN_TYPE_app = "App"
    RETURN_TYPE_peer = "Peer"

    attrib_return_type = "return_type"
    attrib_return_type_peer = "return_type_peer"
    attrib_return_type_app = "return_type_app"

    def __init__(self, fname, consumed_by, xmlElement):

        ret = xmlElement.get(FunctionReturnType.attrib_return_type)
        ret_peer = xmlElement.get(FunctionReturnType.attrib_return_type_peer)
        ret_app = xmlElement.get(FunctionReturnType.attrib_return_type_app)

        # logger.debug("For function name {}, got ret {}, ret_app {}, ret_peer {}, consumed_by {}".format(fname, ret, ret_app, ret_peer, consumed_by))

        if ret_peer == "":
            ret_peer = None

        if ret_app == "":
            ret_app = None

        if ret == "":
            ret = None

        if consumed_by == "" or consumed_by == None:
            consumed_by = Generator.attrib_consumed_by_ALL

        self.ret = None
        self.ret_app = None
        self.ret_peer = None

        if (ret_peer != None or ret_app != None):
            if (ret != None):
                logger.error("Ambiguous return type for {}. return {} is not None but ret_app {} /ret_peer {}  also specifed".format(fname, ret, ret_app, ret_peer))
                sys.exit(-1)

        if (consumed_by == Generator.attrib_consumed_by_ALL or consumed_by == Generator.attrib_consumed_by_App):
            if (ret == None and ret_app == None):
                logger.error("No app facing return type for app-facing {}. return {}, ret_app {}".format(fname, ret, ret_app))
                sys.exit(-1)
            else:
                if (ret_app != None):
                    self.ret_app = ret_app
                else:
                    self.ret_app = ret

        if (consumed_by == Generator.attrib_consumed_by_ALL or consumed_by == Generator.attrib_consumed_by_Peer):
            if (ret == None and ret_peer == None):
                logger.error("No peer facing return type for peer-facing {}. return {}, ret_peer {}".format(fname, ret, ret_peer))
                sys.exit(-1)
            else:
                if (ret_peer != None):
                    self.ret_peer = ret_peer
                else:
                    self.ret_peer = ret

        self.fname = fname
        self.consumed_by = consumed_by

        # logger.debug(self)

    def get_return_type(self, interface):
        if (interface == FunctionReturnType.RETURN_TYPE_app):
            return self.ret_app
        elif (interface == FunctionReturnType.RETURN_TYPE_peer):
            return self.ret_peer
        else:
            logger.error("Return-type cannot be uniquely determined for interface {}".format(interface))
            sys.exit(-1)

    def __str__(self):
        str = "ReturnType: name {}, consumed_by {} -> ret_app {}, ret_peer {}".format(self.fname, self.consumed_by, self.ret_app, self.ret_peer)
        return str

class Generator:
    attrib_consumed_by_ALL = "All"
    attrib_consumed_by_App = "Application"
    attrib_consumed_by_Peer = "Peer"

    def __init__(self, xmlMetaData, xmlElement):
        self.xml_meta = xmlMetaData
        self.xml_entity = xmlElement
        self.consumed_by = Generator.attrib_consumed_by_ALL

    def check_applicability(self, consumption_interface):
        logger.debug("Checking applicability this object's intended consumption {} for INTF={}".format(self.consumed_by, consumption_interface))
        logger.debug(self)
        if None == self.consumed_by or self.consumed_by == Generator.attrib_consumed_by_ALL or self.consumed_by == consumption_interface:
            logger.debug("Applicable")
            return True

        logger.debug("Not Applicable")
        return False

    def declaration_file_start(self, targetFile):
        pass

    def declaration_file_body(self, targetFile):
        pass

    def declaration_file_end(self, targetFile):
        pass

    def source_file_start(self, targetFile):
        pass

    def source_file_body(self, targetFile):
        pass

    def source_file_end(self, targetFile):
        pass

    def __str__(self):
        pass

class XMLToCode:
    interface_node_name="Interface"
    meta_node_name="MetaData"
    imports_node_name="Imports"
    types_node_name="Types"
    callbacks_node_name="Callbacks"
    functions_node_name="Functions"

    def __init__(self, xmlFile, schemaFile, headerFile, sourceFile):
        self.xml_file = xmlFile
        self.schema = schemaFile
        self.header = headerFile
        self.source = sourceFile
        self.parser_instance = XMLParser(xmlFile, schemaFile)

        self.meta_gen = None
        self.imports_gen = None
        self.types_gen = None
        self.callbacks_gen = None
        self.functions_gen = None

    def get_interface_node(self):
        return self.parser_instance.get_node(None, None)

    def get_meta_node(self):
        return self.parser_instance.get_node(self.get_interface_node(), XMLToCode.meta_node_name)

    def get_imports_node(self):
        return self.parser_instance.get_node(self.get_interface_node(), XMLToCode.imports_node_name)

    def get_types_node(self):
        return self.parser_instance.get_node(self.get_interface_node(), XMLToCode.types_node_name)

    def get_callbacks_node(self):
        return self.parser_instance.get_node(self.get_interface_node(), XMLToCode.callbacks_node_name)

    def get_functions_node(self):
        return self.parser_instance.get_node(self.get_interface_node(), XMLToCode.functions_node_name)

    def go(self):
        # generate header contents
        if None != self.header:
            if None != self.meta_gen:
                self.meta_gen.declaration_file_start(self.header)

            if None != self.imports_gen:
                self.imports_gen.declaration_file_start(self.header)

            if None != self.callbacks_gen:
                self.callbacks_gen.declaration_file_start(self.header)

            if None != self.types_gen:
                self.types_gen.declaration_file_start(self.header)

            if None != self.functions_gen:
                self.functions_gen.declaration_file_start(self.header)

            if None != self.meta_gen:
                self.meta_gen.declaration_file_body(self.header)

            if None != self.imports_gen:
                self.imports_gen.declaration_file_body(self.header)

            if None != self.callbacks_gen:
                self.callbacks_gen.declaration_file_body(self.header)

            if None != self.types_gen:
                self.types_gen.declaration_file_body(self.header)

            if None != self.functions_gen:
                self.functions_gen.declaration_file_body(self.header)

            if None != self.meta_gen:
                self.meta_gen.declaration_file_end(self.header)

            if None != self.imports_gen:
                self.imports_gen.declaration_file_end(self.header)

            if None != self.callbacks_gen:
                self.callbacks_gen.declaration_file_end(self.header)

            if None != self.types_gen:
                self.types_gen.declaration_file_end(self.header)

            if None != self.functions_gen:
                self.functions_gen.declaration_file_end(self.header)

        # generate source file content
        if None != self.source:
            if None != self.meta_gen:
                self.meta_gen.source_file_start(self.source)

            if None != self.imports_gen:
                self.imports_gen.source_file_start(self.source)

            if None != self.callbacks_gen:
                self.callbacks_gen.source_file_start(self.source)

            if None != self.types_gen:
                self.types_gen.source_file_start(self.source)

            if None != self.functions_gen:
                self.functions_gen.source_file_start(self.source)

            if None != self.meta_gen:
                self.meta_gen.source_file_body(self.source)

            if None != self.imports_gen:
                self.imports_gen.source_file_body(self.source)

            if None != self.callbacks_gen:
                self.callbacks_gen.source_file_body(self.source)

            if None != self.types_gen:
                self.types_gen.source_file_body(self.source)

            if None != self.functions_gen:
                self.functions_gen.source_file_body(self.source)

            if None != self.meta_gen:
                self.meta_gen.source_file_end(self.source)

            if None != self.imports_gen:
                self.imports_gen.source_file_end(self.source)

            if None != self.callbacks_gen:
                self.callbacks_gen.source_file_end(self.source)

            if None != self.types_gen:
                self.types_gen.source_file_end(self.source)

            if None != self.functions_gen:
                self.functions_gen.source_file_end(self.source)

class MetaField(Generator):
    tag_field = "MetaField"
    attrib_name = "name"
    attrib_value = "value"
    attrib_suppress_key = "suppress_key"
    attrib_description = "description"

    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        self.name = xmlElement.get(MetaField.attrib_name)
        self.value = xmlElement.get(MetaField.attrib_value)
        self.suppress_key = xmlElement.get(MetaField.attrib_suppress_key)
        self.description = xmlElement.get(MetaField.attrib_description)

    def __str__(self):
        representation = "name = {}\nvalue = {}\nsuppress_key = {}\ndescription = {}\n".format(self.name, self.value, self.suppress_key, self.description)
        return representation

class MetadataGen(Generator):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        self.meta_fields = []
        self.xml_meta_fields = xmlElement.findall(MetaField.tag_field)

class Import(Generator):
    tag_field = "Import"
    attrib_name = "name"
    attrib_include_location = "include_in"
    attrib_language = "language"

    LANGUAGE_CPP = "Cpp"
    LANGUAGE_PYTHON = "Python"
    LANGUAGE_PROTO = "Proto"
    LANGUAGE_GO = "Golang"
    LANGUAGE_ALL = "All"

    INCLUDE_IN_LIB = "Lib"
    INCLUDE_IN_DECLARATIONS = "Interface"
    INCLUDE_IN_ALL = "All"

    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        self.name = xmlElement.get(Import.attrib_name)
        self.include_in = xmlElement.get(Import.attrib_include_location)
        self.language = xmlElement.get(Import.attrib_language)

    def __str__(self):
        representation = "name = {}\ninclude_in = {}\nlanguage = {}\n".format(self.name, self.include_in, self.language)
        return representation

    def should_print(self, lang, intf):
        logger.debug("Checking print-ness for LANG={}, INTF={}".format(lang, intf))
        logger.debug(self)
        if self.language != lang and self.language != Import.LANGUAGE_ALL:
            return False

        if self.include_in != intf and self.include_in != Import.INCLUDE_IN_ALL:
            return False

        return True

class ImportsGen(Generator):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        self.imports = []
        self.xml_imports = xmlElement.findall(Import.tag_field)

class EnumValue(Generator):
    tag_field = "EnumValue"
    attrib_name = "name"
    attrib_description = "description"
    attrib_value = "value"

    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        self.name = xmlElement.get(EnumValue.attrib_name)
        self.description = xmlElement.get(EnumValue.attrib_description)
        self.value = xmlElement.get(EnumValue.attrib_value)

    def __str__(self):
        representation = "name = {} = {},  ==> {}\n".format(self.name, self.value, self.description)

        return representation

class Enum(Generator):
    tag_field = "Enum"
    attrib_name = "name"
    attrib_description = "description"
    attrib_consumed_by = "consumed_by"

    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        self.name = xmlElement.get(Enum.attrib_name)
        self.description = xmlElement.get(Enum.attrib_description)
        self.consumed_by = xmlElement.get(Enum.attrib_consumed_by)
        self.enum_values = []
        self.xml_enum_values = xmlElement.findall(EnumValue.tag_field)

        logger.debug(str(self))
        logger.debug("{} -> consumed by {}".format(self.name, self.consumed_by))

    def __str__(self):
        representation = "name = {}, description = {}, consumed_by = {}\n\n".format(self.name, self.description, self.consumed_by)

        for e in self.enum_values:
            representation = representation + "  {}\n".format(str(e))

        return representation

class Field(Generator):
    tag_field = "Field"
    attrib_name = "name"
    attrib_type = "type"
    attrib_description = "description"
    attrib_sequence = "sequence"
    attrib_array = "array"
    attrib_consumed_by = "consumed_by"

    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        self.name = xmlElement.get(Field.attrib_name)
        self.type = xmlElement.get(Field.attrib_type)
        self.description = xmlElement.get(Field.attrib_description)
        self.sequence = xmlElement.get(Field.attrib_sequence)
        self.consumed_by = xmlElement.get(Field.attrib_consumed_by)
        self.array_xml = xmlElement.get(Field.attrib_array)

        logger.debug(str(self))
        logger.debug("{} -> consumed by {}".format(self.name, self.consumed_by))

        logger.debug(str(self))
        logger.debug("{} -> consumed by {}".format(self.name, self.consumed_by))

    def __str__(self):
        representation = "name = {}, type = {}, sequence = {}, array = {}\ndescription = {}\nconsumed_by = {}\n\n".format(self.name, self.type, self.sequence, self.array_xml, self.description, self.consumed_by)
        return representation

class Struct(Generator):
    tag_field = "Struct"
    attrib_name = "name"
    attrib_description = "description"
    attrib_consumed_by = "consumed_by"

    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        self.name = xmlElement.get(Struct.attrib_name)
        self.fields = []
        self.description = xmlElement.get(Struct.attrib_description)
        self.consumed_by = xmlElement.get(Struct.attrib_consumed_by)
        self.xml_fields = xmlElement.findall(Field.tag_field)

        logger.debug(str(self))
        logger.debug("{} -> consumed by {}".format(self.name, self.consumed_by))

    def __str__(self):
        representation = "Struct name = {}\n{}, consumed_by {} ====>\n".format(self.name, self.description, self.consumed_by)
        
        for f in self.fields:
            representation = representation + "  {}\n".format(str(f))

        return representation

class TypesGen(Generator):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        self.structs = []
        self.enums = []
        self.xml_structs = xmlElement.findall(Struct.tag_field)
        self.xml_enums = xmlElement.findall(Enum.tag_field)

class FuncParam(Generator):
    tag_field = "Parameter"
    attrib_variable_name = "variable_name"
    attrib_type = "type"
    attrib_description = "description"
    attrib_consumed_by = "consumed_by"

    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        self.variable_name = xmlElement.get(FuncParam.attrib_variable_name)
        self.type = xmlElement.get(FuncParam.attrib_type)
        self.description = xmlElement.get(FuncParam.attrib_description)
        self.consumed_by = xmlElement.get(FuncParam.attrib_consumed_by)

    def __str__(self):
        representation = "name = {}, type = {}\ndescription = {}\n\n".format(self.variable_name, self.type, self.description)
        return representation

class FunctionPtr(Generator):
    tag_field = "FunctionPtr"
    attrib_name = "name"
    attrib_description = "description"
    attrib_consumed_by = "consumed_by"

    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        self.name = xmlElement.get(FunctionPtr.attrib_name)
        self.description = xmlElement.get(FunctionPtr.attrib_description)
        self.consumed_by = xmlElement.get(FunctionPtr.attrib_consumed_by)
        self.parameters = []
        self.xml_parameters = xmlElement.findall(FuncParam.tag_field)
        self.return_spec = FunctionReturnType(self.name, self.consumed_by, xmlElement)

    def __str__(self):
        representation = "Fptr {}/{} {}  ==> {}\n".format(self.return_spec.get_return_type(FunctionReturnType.RETURN_TYPE_app), self.return_spec.get_return_type(FunctionReturnType.RETURN_TYPE_peer), self.name, self.description)
        
        for p in self.parameters:
            representation = representation + "  {}\n".format(str(p))

        return representation

class CallbacksGen(Generator):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        self.fptrs = []
        self.xml_fptrs = xmlElement.findall(FunctionPtr.tag_field)

class Function(Generator):
    tag_field = "Function"
    attrib_name = "name"
    attrib_description = "description"
    attrib_success_return = "success_return"
    attrib_failure_return = "failure_return"
    attrib_consumed_by = "consumed_by"

    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        self.name = xmlElement.get(Function.attrib_name)
        self.description = xmlElement.get(Function.attrib_description)
        self.success_return = xmlElement.get(Function.attrib_success_return)
        self.failure_return = xmlElement.get(Function.attrib_failure_return)
        self.consumed_by = xmlElement.get(Function.attrib_consumed_by)
        self.parameters = []
        self.xml_parameters = xmlElement.findall(FuncParam.tag_field)
        self.return_spec = FunctionReturnType(self.name, self.consumed_by, xmlElement)

    def __str__(self):
        representation = "Function {}/{} {}  ==> {}\n".format(self.return_spec.get_return_type(FunctionReturnType.RETURN_TYPE_app), self.return_spec.get_return_type(FunctionReturnType.RETURN_TYPE_peer), self.name, self.description)
        
        for p in self.parameters:
            representation = representation + "  {}\n".format(str(p))

        return representation

class FunctionsGen(Generator):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        self.functions = []
        self.xml_funcs = xmlElement.findall(Function.tag_field)
