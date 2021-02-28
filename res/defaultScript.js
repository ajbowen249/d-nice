println('lol');

OS.readFile('res\\defaultScript.js')
    .then(function(fc) {
        println(fc.toString());
    })
    .catch(function(err) {
        println('got error');
        println(err);
    });
