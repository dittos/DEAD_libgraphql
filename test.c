#include <JavaScriptCore/JavaScript.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    JSGlobalContextRef js;
    JSObjectRef mainFunc;
    JSObjectRef schemaCtor;
    JSObjectRef objectTypeCtor;
    JSObjectRef stringType;
} GraphQLContext;
typedef GraphQLContext* GraphQLContextRef;

typedef JSObjectRef GraphQLResultRef;

typedef JSObjectRef GraphQLSchemaRef;
typedef JSObjectRef GraphQLObjectTypeRef;
typedef JSObjectRef GraphQLFieldConfigMapRef;

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

GraphQLContextRef GraphQLContextCreate() {
    GraphQLContextRef ctx = malloc(sizeof(GraphQLContext));
    ctx->js = JSGlobalContextCreate(NULL);
    JSStringRef script = ReadScript("graphql.js");
    JSEvaluateScript(ctx->js, script, NULL, NULL, 1, NULL);
    JSStringRelease(script);

    script = JSStringCreateWithUTF8CString("lib.graphqlSync");
    ctx->mainFunc = JSValueToObject(ctx->js, JSEvaluateScript(ctx->js, script, NULL, NULL, 1, NULL), NULL);
    JSStringRelease(script);

    script = JSStringCreateWithUTF8CString("lib.graphql.GraphQLSchema");
    ctx->schemaCtor = JSValueToObject(ctx->js, JSEvaluateScript(ctx->js, script, NULL, NULL, 1, NULL), NULL);
    JSStringRelease(script);

    script = JSStringCreateWithUTF8CString("lib.graphql.GraphQLObjectType");
    ctx->objectTypeCtor = JSValueToObject(ctx->js, JSEvaluateScript(ctx->js, script, NULL, NULL, 1, NULL), NULL);
    JSStringRelease(script);
    
    script = JSStringCreateWithUTF8CString("lib.graphql.GraphQLString");
    ctx->stringType = JSValueToObject(ctx->js, JSEvaluateScript(ctx->js, script, NULL, NULL, 1, NULL), NULL);
    JSStringRelease(script);
    
    return ctx;
}

void GraphQLContextRelease(GraphQLContextRef ctx) {
    JSGlobalContextRelease(ctx->js);
    ctx->js = NULL;
    ctx->mainFunc = NULL;
    ctx->schemaCtor = NULL;
    ctx->objectTypeCtor = NULL;
    ctx->stringType = NULL;
}

GraphQLSchemaRef GraphQLSchemaCreate(GraphQLContextRef ctx, GraphQLObjectTypeRef query, GraphQLObjectTypeRef mutation) {
    JSObjectRef config = JSObjectMake(ctx->js, NULL, NULL);
    JSStringRef queryProp = JSStringCreateWithUTF8CString("query");
    JSObjectSetProperty(ctx->js, config, queryProp, query, kJSPropertyAttributeNone, NULL);
    JSStringRelease(queryProp);
    if (mutation) {
        JSStringRef mutationProp = JSStringCreateWithUTF8CString("mutation");
        JSObjectSetProperty(ctx->js, config, mutationProp, query, kJSPropertyAttributeNone, NULL);
        JSStringRelease(mutationProp);
    }
    JSValueRef args[] = { config };
    return JSObjectCallAsConstructor(ctx->js, ctx->schemaCtor, 1, args, NULL);
    // TODO protect from gc
}

GraphQLObjectTypeRef GraphQLObjectTypeCreate(GraphQLContextRef ctx, const char *name, GraphQLFieldConfigMapRef fields) {
    JSObjectRef config = JSObjectMake(ctx->js, NULL, NULL);
    JSStringRef nameProp = JSStringCreateWithUTF8CString("name");
    JSStringRef nameStr = JSStringCreateWithUTF8CString(name);
    JSValueRef nameValue = JSValueMakeString(ctx->js, nameStr);
    JSObjectSetProperty(ctx->js, config, nameProp, nameValue, kJSPropertyAttributeNone, NULL);
    JSStringRelease(nameStr);
    JSStringRelease(nameProp);
    JSStringRef fieldsProp = JSStringCreateWithUTF8CString("fields");
    JSObjectSetProperty(ctx->js, config, fieldsProp, fields, kJSPropertyAttributeNone, NULL);
    JSStringRelease(fieldsProp);
    JSValueRef args[] = { config };
    return JSObjectCallAsConstructor(ctx->js, ctx->objectTypeCtor, 1, args, NULL);
    // TODO protect from gc
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

void GraphQLFieldConfigMapSetField(GraphQLContextRef ctx, GraphQLFieldConfigMapRef fields, const char *name) {
    JSObjectRef fieldConfig = JSObjectMake(ctx->js, NULL, NULL);
    JSStringRef typeProp = JSStringCreateWithUTF8CString("type");
    JSObjectSetProperty(ctx->js, fieldConfig, typeProp, ctx->stringType, kJSPropertyAttributeNone, NULL);
    JSStringRelease(typeProp);
    
    JSObjectRef resolveFunc = JSObjectMakeFunctionWithCallback(ctx->js, NULL, f);
    
    JSStringRef resolveProp = JSStringCreateWithUTF8CString("resolve");
    JSObjectSetProperty(ctx->js, fieldConfig, resolveProp, resolveFunc, kJSPropertyAttributeNone, NULL);
    JSStringRelease(resolveProp);
    
    JSStringRef nameStr = JSStringCreateWithUTF8CString(name);
    JSObjectSetProperty(ctx->js, fields, nameStr, fieldConfig, kJSPropertyAttributeNone, NULL);
    JSStringRelease(nameStr);
}

GraphQLResultRef GraphQLMain(GraphQLContextRef ctx, GraphQLSchemaRef schema, const char *requestString) {
    JSStringRef requestStr = JSStringCreateWithUTF8CString(requestString);
    JSValueRef requestStrValue = JSValueMakeString(ctx->js, requestStr);
    JSValueRef args[] = { schema, requestStrValue };
    JSValueRef error;
    JSValueRef result = JSObjectCallAsFunction(ctx->js, ctx->mainFunc, NULL, 2, args, &error);
    JSStringRelease(requestStr);
    // TODO error check
    // TODO protect from gc
    return JSValueToObject(ctx->js, result, NULL);
}

int main(int argc, char *argv[]) {
    GraphQLContextRef ctx = GraphQLContextCreate();
    GraphQLFieldConfigMapRef fields = GraphQLFieldConfigMapCreate(ctx);
    GraphQLFieldConfigMapSetField(ctx, fields, "hello");
    GraphQLObjectTypeRef query = GraphQLObjectTypeCreate(ctx, "QueryRoot", fields);
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
