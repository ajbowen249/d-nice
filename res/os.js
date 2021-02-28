const OS = {
    readFile: function(path) {
        return new Promise(function(resolve, reject) {
            os_readFile_native(path, resolve, reject);
        });
    },
};
