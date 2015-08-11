#include <JavaScriptCore/JavaScript.h>
#include <stdio.h>
#include <stdlib.h>

JSStringRef ReadScript(const char *name) {
    FILE *fp = fopen(name, "rb");
    assert(fp != NULL);

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
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

int main(int argc, char *argv[]) {
    JSContextRef ctx = JSGlobalContextCreate(NULL);
    JSStringRef script = ReadScript("graphql.js");
    JSValueRef error;
    JSObjectRef module = JSValueToObject(ctx, JSEvaluateScript(ctx, script, NULL, NULL, 1, &error), NULL);
    JSEvaluateScript(ctx, JSStringCreateWithUTF8CString(
        "var graphql = lib.graphql, theresult;"
        "graphql.graphql(new graphql.GraphQLSchema({"
        "   query: new graphql.GraphQLObjectType({"
        "       name: 'Q',"
        "       fields: {hello: {type: graphql.GraphQLString, resolve: function(){return 'world'}}}"
        "   })"
        "}), '{ hello }').then(function(result) {"
        "   theresult = result;"
        "});"
        "lib.flushTasks();"
        "throw new Error(JSON.stringify(theresult))"
    ), NULL, NULL, 1, &error);
    if (error) {
        JSStringRef json = JSValueToStringCopy(ctx, error, NULL);
        char buf[1024];
        JSStringGetUTF8CString(json, buf, 1024);
        printf("%s", buf);
        // TODO: release
    }
    return 0;
}
