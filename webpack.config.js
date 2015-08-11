module.exports = {
    entry: "./wrapper.js",
    output: {
        path: __dirname,
        filename: 'graphql.js',
        library: 'graphql'
    }
};
