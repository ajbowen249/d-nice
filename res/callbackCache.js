const callbackCache = [];
const cacheIndex = 0;
const maxCacheSize = Number.MAX_SAFE_INTEGER;

function incrementCacheIndex() {
    // This probably isn't likely to happen, but just in case
    // we overflow, we need to find a new place to stick the
    // next callback.
    var loopedOnce = false;

    do {
        cacheIndex++;
        if (cacheIndex >= maxCacheSize) {
            if (!loopedOnce) {
                cacheIndex = 0;
                loopedOnce = true;
            } else {
                throw new Error('Out of room to cache callbacks.');
            }
        }
    } while (callbackCache[cacheIndex.toString()]);
}

function registerCallback(resolve, reject) {
    const callbackIndex = cacheIndex;
    incrementCacheIndex();
    callbackCache[callbackIndex.toString()] = {
        resolve,
        reject
    };

    return callbackIndex;
}

function resolveCallback(id, value) {
    const key = id.toString();
    callbackCache[key].resolve(value);
    delete callbackCache[key];
}

function rejectCallback(id, error) {
    const key = id.toString();

    if (typeof(error) === 'string') {
        error = new Error(error);
    }

    callbackCache[key].reject(error);
    delete callbackCache[key];
}
