global.setTimeout = function(func) {
    // We don't need it
};

var _tasks = [];

global.setImmediate = function(func, ...args) {
    _tasks.push(function() {
        return func.apply(undefined, args);
    });
    return _tasks.length;
};

global.clearImmediate = function(func) {
    // We don't need it
};

function flushTasks() {
    var i = 0;
    while (i < _tasks.length) {
        _tasks[i]();
        i++;
    }
    _tasks = [];
}

var graphql = require('graphql');

module.exports = {
    graphql,
    graphqlSync: function(...args) {
        var result;
        var error;
        graphql.graphql(...args).then(resolve => {
            result = resolve;
        }, reject => {
            error = reject;
        });
        flushTasks();
        if (error) {
            throw error;
        }
        return result;
    }
};
