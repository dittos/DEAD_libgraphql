module.exports = {
    entry: "./wrapper.js",
    output: {
        path: __dirname,
        filename: 'graphql.js',
        library: 'lib'
    },
    module: {
        loaders: [
            {
                test: /\.js$/,
                exclude: /node_modules/,
                loader: 'babel-loader'
            }
        ]
    }
};
