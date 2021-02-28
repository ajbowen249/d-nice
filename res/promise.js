// This is only a rough approximation of the true Promise object.
// It is here for the convenience of making things feel more
// Nodejs-like.

const STATUS_PENDING = 'pending';
const STATUS_RESOLVED = 'resolved';
const STATUS_REJECTED = 'rejected';

function Promise(func) {
    this.thens = [];
    this.status = STATUS_PENDING;
    this.onCatch = undefined;
    this.onFinally = undefined;

    const thiz = this;
    this.then = function(callback) {
        if (thiz.status === STATUS_PENDING) {
            thiz.thens.push(callback);
        } else {
            callback(thiz.value);
        }

        return thiz;
    }

    this.catch = function(callback) {
        thiz.onCatch = callback;
        if (thiz.status === STATUS_REJECTED) {
            callback(thiz.value);
        }
        return thiz;
    }

    this.finally = function(callback) {
        thiz.onFinally = callback;
        if (thiz.status !== STATUS_PENDING) {
            callback(thiz.value);
        }
        return thiz;
    }

    const resolve = function(value) {
        thiz.status = STATUS_RESOLVED;
        thiz.value = value;
        thiz.thens.forEach(function(then) {
            then(thiz.value);
        });

        if (thiz.onFinally) {
            thiz.onFinally(thiz.value);
        }
    };

    const reject = function(value) {
        thiz.status = STATUS_REJECTED;
        thiz.value = value;
        if (thiz.onCatch) {
            thiz.onCatch(thiz.value);
        }

        if (thiz.onFinally) {
            thiz.onFinally(thiz.value);
        }
    };

    func(resolve, reject);
}

Promise.STATUS_PENDING = STATUS_PENDING;
Promise.STATUS_RESOLVED = STATUS_RESOLVED;
Promise.STATUS_REJECTED = STATUS_REJECTED;

Promise.resolve = function(value) {
    return new Promise(function(resolve) {
        resolve(value);
    });
}

Promise.reject = function(value) {
    return new Promise(function(resolve, reject) {
        reject(value);
    });
}

Promise.all = function(promises) {
    if (promises.length === 0) {
        return Promise.resolve([]);
    }

    return new Promise(function(resolve, reject) {
        const results = [];
        var complete = 0;
        var target = promises.length;
        promises.forEach(function(promise, index) {
            promise.then(function(value) {
                results[index] = value;
                complete++;
                if (complete === target) {
                    resolve(results);
                }
            })
            .catch(function(err) {
                reject(err);
            });
        });
    });
}
