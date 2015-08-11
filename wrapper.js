global.setTimeout = function(func) {
    // We don't need it
};

var _tasks = [];

global.setImmediate = function(func) {
    var args = Array.prototype.slice.call(arguments)
    _tasks.push(function() {
        return func.apply(undefined, args);
    });
    return _tasks.length;
};

global.clearImmediate = function(func) {
    // We don't need it
}

module.exports = {
    flushTasks: function() {
        var i = 0;
        while (i < _tasks.length) {
            _tasks[i]();
            i++;
        }
        _tasks = [];
    }
};

var graphql = require('graphql');

graphql.graphql(new graphql.GraphQLSchema({
    query: new graphql.GraphQLObjectType({
        name: 'Query',
        fields: {
            hello: {
                type: graphql.GraphQLString,
                resolve: function() { return 'world' }
            }
        }
    })
}), '{ hello }').then(function(result) {
    module.exports.result = result;
})
