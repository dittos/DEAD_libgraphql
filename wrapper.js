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

module.exports = {
    flushTasks: function() {
        var i = 0;
        while (i < _tasks.length) {
            _tasks[i]();
            i++;
        }
        _tasks = [];
    },
    graphql: require('graphql'),
};
