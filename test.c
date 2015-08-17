#include <JavaScriptCore/JavaScript.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    JSGlobalContextRef js;
    JSObjectRef mainFunc;
    struct {
        JSObjectRef schemaCtor;
        JSObjectRef objectTypeCtor;
        JSObjectRef interfaceTypeCtor;
        JSObjectRef unionTypeCtor;
        JSObjectRef enumTypeCtor;
        JSObjectRef inputObjectTypeCtor;
        JSObjectRef listCtor;
        JSObjectRef nonNullCtor;
    } ctors;
    struct {
        JSObjectRef intType;
        JSObjectRef floatType;
        JSObjectRef stringType;
        JSObjectRef booleanType;
        JSObjectRef idType;
    } scalarTypes;
} GraphQLContext;

typedef JSObjectRef GraphQLType;

typedef GraphQLContext* GraphQLContextRef;

typedef JSObjectRef GraphQLResultRef;

typedef JSObjectRef GraphQLSchemaRef;
typedef JSObjectRef GraphQLObjectTypeRef;
typedef JSObjectRef GraphQLFieldConfigMapRef;
typedef JSObjectRef GraphQLFieldConfigArgumentMapRef;
typedef JSObjectRef GraphQLInterfacesRef;
typedef JSObjectRef GraphQLObjectTypePredicateRef;
typedef void *(*GraphQLFieldResolveFunc)(void *value);

JSStringRef ReadScript(const char *name) {
    FILE *fp = fopen(name, "rb");
    assert(fp != NULL);

    fseek(fp, 0, SEEK_END);
    int size = (int)ftell(fp);
    rewind(fp);

    char *source = malloc(size + 1);
    source[size] = '\0';
    for (int i = 0; i < size;) {
        int read = (int)fread(&source[i], 1, size - 1, fp);
        i += read;
    }
    fclose(fp);

    JSStringRef script = JSStringCreateWithUTF8CString(source);
    free(source);
    return script;
}

static void _Eval(JSGlobalContextRef ctx, const char *script, JSObjectRef *out) {
    JSStringRef scriptStr = JSStringCreateWithUTF8CString(script);
    *out = JSValueToObject(ctx, JSEvaluateScript(ctx, scriptStr, NULL, NULL, 1, NULL), NULL);
    JSStringRelease(scriptStr);
}

GraphQLContextRef GraphQLContextCreate() {
    GraphQLContextRef ctx = malloc(sizeof(GraphQLContext));
    ctx->js = JSGlobalContextCreate(NULL);
    JSStringRef script = ReadScript("graphql.js");
    JSEvaluateScript(ctx->js, script, NULL, NULL, 1, NULL);
    JSStringRelease(script);

    _Eval(ctx->js, "lib.graphqlSync", &ctx->mainFunc);
    
    _Eval(ctx->js, "lib.graphql.GraphQLSchema", &ctx->ctors.schemaCtor);
    _Eval(ctx->js, "lib.graphql.GraphQLObjectType", &ctx->ctors.objectTypeCtor);
    _Eval(ctx->js, "lib.graphql.GraphQLInterfaceType", &ctx->ctors.interfaceTypeCtor);
    _Eval(ctx->js, "lib.graphql.GraphQLUnionType", &ctx->ctors.unionTypeCtor);
    _Eval(ctx->js, "lib.graphql.GraphQLEnumType", &ctx->ctors.enumTypeCtor);
    _Eval(ctx->js, "lib.graphql.GraphQLInputObjectType", &ctx->ctors.inputObjectTypeCtor);
    _Eval(ctx->js, "lib.graphql.GraphQLList", &ctx->ctors.listCtor);
    _Eval(ctx->js, "lib.graphql.GraphQLNonNull", &ctx->ctors.nonNullCtor);
    
    _Eval(ctx->js, "lib.graphql.GraphQLInt", &ctx->scalarTypes.intType);
    _Eval(ctx->js, "lib.graphql.GraphQLFloat", &ctx->scalarTypes.floatType);
    _Eval(ctx->js, "lib.graphql.GraphQLString", &ctx->scalarTypes.stringType);
    _Eval(ctx->js, "lib.graphql.GraphQLBoolean", &ctx->scalarTypes.booleanType);
    _Eval(ctx->js, "lib.graphql.GraphQLID", &ctx->scalarTypes.idType);
    return ctx;
}

void GraphQLContextRelease(GraphQLContextRef ctx) {
    JSGlobalContextRelease(ctx->js);
    ctx->js = NULL;
    
    ctx->mainFunc = NULL;
    
    ctx->ctors.schemaCtor = NULL;
    ctx->ctors.objectTypeCtor = NULL;
    
    ctx->scalarTypes.intType = NULL;
    ctx->scalarTypes.floatType = NULL;
    ctx->scalarTypes.stringType = NULL;
    ctx->scalarTypes.booleanType = NULL;
    ctx->scalarTypes.idType = NULL;
    
    free(ctx);
}

static void _SetProperty(JSGlobalContextRef ctx, JSObjectRef obj, const char *name, JSValueRef value) {
    JSStringRef nameStr = JSStringCreateWithUTF8CString(name);
    JSObjectSetProperty(ctx, obj, nameStr, value, kJSPropertyAttributeNone, NULL);
    JSStringRelease(nameStr);
}

static void _SetStringProperty(JSGlobalContextRef ctx, JSObjectRef obj, const char *name, const char *value) {
    JSStringRef valueStr = JSStringCreateWithUTF8CString(value);
    JSValueRef jsValue = JSValueMakeString(ctx, valueStr);
    _SetProperty(ctx, obj, name, jsValue);
    JSStringRelease(valueStr);
}

GraphQLSchemaRef GraphQLSchemaCreate(GraphQLContextRef ctx, GraphQLObjectTypeRef query, GraphQLObjectTypeRef mutation) {
    JSObjectRef config = JSObjectMake(ctx->js, NULL, NULL);
    _SetProperty(ctx->js, config, "query", query);
    if (mutation) {
        _SetProperty(ctx->js, config, "mutation", query);
    }
    JSValueRef args[] = { config };
    // TODO error check
    return JSObjectCallAsConstructor(ctx->js, ctx->ctors.schemaCtor, 1, args, NULL);
}

GraphQLObjectTypeRef GraphQLObjectTypeCreate(GraphQLContextRef ctx, const char *name, /* nullable */ GraphQLInterfacesRef interfaces, GraphQLFieldConfigMapRef fields, /* nullable */ GraphQLObjectTypePredicateRef isTypeOf, /* nullable */ const char *description) {
    JSObjectRef config = JSObjectMake(ctx->js, NULL, NULL);
    _SetStringProperty(ctx->js, config, "name", name);
    _SetProperty(ctx->js, config, "fields", fields);
    if (interfaces) {
        _SetProperty(ctx->js, config, "interfaces", interfaces);
    }
    if (isTypeOf) {
        _SetProperty(ctx->js, config, "isTypeOf", isTypeOf);
    }
    if (description) {
        _SetStringProperty(ctx->js, config, "description", description);
    }
    JSValueRef args[] = { config };
    // TODO error check
    return JSObjectCallAsConstructor(ctx->js, ctx->ctors.objectTypeCtor, 1, args, NULL);
}

GraphQLFieldConfigMapRef GraphQLFieldConfigMapCreate(GraphQLContextRef ctx) {
    return JSObjectMake(ctx->js, NULL, NULL);
}

static JSValueRef f( JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception) {
    JSStringRef str = JSStringCreateWithUTF8CString("world");
    JSValueRef strValue = JSValueMakeString(ctx, str);
    JSStringRelease(str);
    return strValue;
}

void GraphQLFieldConfigMapSetField(GraphQLContextRef ctx, GraphQLFieldConfigMapRef fields, const char *name, GraphQLType type, /* nullable */ GraphQLFieldConfigArgumentMapRef args, GraphQLFieldResolveFunc resolve) {
    JSObjectRef fieldConfig = JSObjectMake(ctx->js, NULL, NULL);
    _SetProperty(ctx->js, fieldConfig, "type", type);
    if (args) {
        _SetProperty(ctx->js, fieldConfig, "args", args);
    }
    
    JSObjectRef resolveFunc = JSObjectMakeFunctionWithCallback(ctx->js, NULL, f);
    
    _SetProperty(ctx->js, fieldConfig, "resolve", resolveFunc);
    _SetProperty(ctx->js, fields, name, fieldConfig);
}

GraphQLFieldConfigArgumentMapRef GraphQLFieldConfigArgumentMapCreate(GraphQLContextRef ctx) {
    return JSObjectMake(ctx->js, NULL, NULL);
}

GraphQLResultRef GraphQLMain(GraphQLContextRef ctx, GraphQLSchemaRef schema, const char *requestString) {
    JSStringRef requestStr = JSStringCreateWithUTF8CString(requestString);
    JSValueRef requestStrValue = JSValueMakeString(ctx->js, requestStr);
    JSValueRef args[] = { schema, requestStrValue };
    JSValueRef error;
    JSValueRef result = JSObjectCallAsFunction(ctx->js, ctx->mainFunc, NULL, 2, args, &error);
    JSStringRelease(requestStr);
    // TODO error check
    return JSValueToObject(ctx->js, result, NULL);
}

int main(int argc, char *argv[]) {
    GraphQLContextRef ctx = GraphQLContextCreate();
    GraphQLFieldConfigMapRef fields = GraphQLFieldConfigMapCreate(ctx);
    GraphQLFieldConfigMapSetField(ctx, fields, "hello", ctx->scalarTypes.stringType, NULL, NULL);
    GraphQLObjectTypeRef query = GraphQLObjectTypeCreate(ctx, "QueryRoot", NULL, fields, NULL, NULL);
    GraphQLSchemaRef schema = GraphQLSchemaCreate(ctx, query, NULL);

    GraphQLResultRef result = GraphQLMain(ctx, schema, "{ hello }");

    JSStringRef json = JSValueCreateJSONString(ctx->js, result, 2, NULL);
    char str[1024];
    JSStringGetUTF8CString(json, str, 1024);
    printf("%s\n", str);
    JSStringRelease(json);

    GraphQLContextRelease(ctx);
    return 0;
}
