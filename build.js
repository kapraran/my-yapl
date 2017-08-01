var execa = require('execa');
var chalk = require('chalk');
var fs = require('fs');
var program = require('commander');

program
    .version('0.2.0')
    .option('-n, --nostyles', 'Disable styles')
    .option('-r, --run', 'Run compiler after compilation')
    .option('-v, --visualize', 'Visualize Abstract Syntax Tree')
    .option('-i, --input <filename>', 'Set input filename')
    .option('-o, --output <filename>', 'Set output filename')
    .option('-b, --builder', 'Run mixal builder (requires -r)')
    .parse(process.argv);

if (program.nostyles)
    chalk.enabled = false;

var TAB = '  ';
var PREFIX = TAB + chalk.gray('>') + TAB.substr(1);

function getInputFilename() {
    var filename = program.input ? program.input: 'examples/example.yapl';
    return './' + filename;
}

function displayResult(name, started, res) {
    var ms = Date.now() - started;
    if (res.failed)
        console.log(chalk.red('$ ' + name + ' ') + chalk.black.bgRed(' failed ') + chalk.red(' to complete'));
    else
        console.log(chalk.green('$ ' + name + ' completed with ') + chalk.black.bgGreen(' success ') + chalk.green(' in ' + ms + 'ms'));

    if (res.stdout.length > 0) {
        console.log(TAB + '-> ' + chalk.black.bgWhite(' stdout '));
        name == 'gcc' ? beautifyGccOutput(res.stdout): console.log(res.stdout);
    }

    if (res.stderr.length > 0) {
        console.log(TAB + '-> ' + chalk.white.bgRed(' stderr '));
        name == 'gcc' ? beautifyGccOutput(res.stderr): console.log(res.stderr);
    }
}

function editYTABH() {
    var contents = fs.readFileSync('y.tab.h', 'utf8');

    contents = '#ifndef YTAB_H\n#define YTAB_H\n\n' + contents + '\n#endif\n';

    var index = contents.indexOf('typedef');
    var firstPart = contents.substr(0, index);
    var secondPart = contents.substr(index);
    
    contents = firstPart + '\ntypedef struct __ast_node ast_node;\n\n' + secondPart;

    fs.writeFileSync('y.tab.h', contents, 'utf8');
}

function onYaccCompleted(started, res) {
    displayResult('yacc', started, res);
    editYTABH();
    
    return execa('flex', ['yapl.compiler.l']);
}

function onFlexCompleted(started, res) {
    displayResult('flex', started, res);

    return execa('gcc', ['lex.yy.c', 'y.tab.c', '-o', 'yaplc']);
}

function find_constant_by_name(constants, name) {
    for (var i in constants) {
        if (constants[i][0] == name)
            return constants[i];
    }
    
    return null;
}

function find_constant_by_value(constants, value) {
    for (var i in constants) {
        if (constants[i][1] == value)
            return constants[i];
    }

    return null;
}

var __cache_constants = null;
function loadConstants() {
    if (__cache_constants != null)
        return __cache_constants;

    var files = ['y.tab.h', 'utils.compiler.h'];
    var constants = [];
    
    for (var i in files) {
        var pattern = /\#define\s+([a-zA-Z_]+)\s+(\d+|\'.\'|[a-zA-Z_]+)/g;
        var file = files[i];
        var content = fs.readFileSync(file, 'utf8');
        var skip = ['FALSE', 'TRUE', 'MAX_CHILDREN'];

        while ((found = pattern.exec(content))) {
            if (skip.indexOf(found[1]) > -1)
                continue;

            var value = found[2];

            if (value[0] == "'") {
                value = value.charCodeAt(1);
            } else if (isNaN(value)) {
                value = find_constant_by_name(constants, value)[1];
            } else {
                value = parseInt(value);
            }

            var constant = [
                found[1],
                value
            ];

            constants.push(constant);
        }
    }

    __cache_constants = constants;
    return constants;
}

function createVisualization(output) {
    var constants = loadConstants();

    var nodes = [];
    var edges = [];

    var null_id = 100000;

    output.replace(/\{viz\} (\d+)\:(\d+)\{(.*)\}\:(\d+)\:(.*)/g, function(line, _id, type, contents, children_len, children_str) {
        if (!parseInt(_id) == 0) {
            var node = {
                id: parseInt(_id),
                label: find_constant_by_value(constants, parseInt(type))[0]
            };

            if (node.label.match(/^SYM_/)) {
                if (node.label == 'SYM_TYPE_INTEGER') {
                    node.label = ': int';
                } else if (node.label == 'SYM_VARIABLE') {
                    node.label = '$ ' + contents.trim();
                } else if (node.label == 'SYM_METHOD') {
                    node.label = '() ' + contents.trim();
                } else if (node.label == 'SYM_CNST_INTEGER') {
                    node.label = contents.trim();
                } else if (contents.trim().length > 0)
                    node.label += '::' + contents.trim();

                node.color = {
                    // border: '#0a5a72'
                    background: '#caddf7'
                }
            }

            nodes.push(node);
        }

        var children = children_str.replace(/\s+/g, ' ').trim().split(' ');
        for (var i=0; i<children_len; i++) {
            edges.push({
                from: parseInt(_id),
                to: parseInt(children[i])
            });
        }

        return line;
    });

    var src = fs.readFileSync(getInputFilename());
    var tpl = fs.readFileSync('./ast.template.html', 'utf8');
    var rendered = tpl
        .replace('<!--__SOURCE_CODE__-->', src)                 // replace source code
        .replace('/**__NODES__**/0', JSON.stringify(nodes))     // replace nodes
        .replace('/**__EDGES__**/0', JSON.stringify(edges));    // replace edges

    fs.writeFileSync('./ast.html', rendered, 'utf8');
}

function beautifyYaplOutput(output) {
    var constants = loadConstants();

    output = output.replace(/\[lex\]/g, '[' + chalk.magenta('lex') + ']');
    output = output.replace(/\[yacc\]/g, '[' + chalk.blue('yacc') + ']');
    
    output = output.replace(/\((epsilon|symbol|element)\)/g, function(match, str) {
        if (str == 'epsilon') str = ' 💩 ';
        else if (str == 'element') str = ' 💲 ';
        else str = ' 🗽 ';
        return ' ' + chalk.yellow(str) + '   >';
    });
    
    output = output.replace(/\$(\d+)/g, function(match, num) {
        var inum = parseInt(num);
        var constant = find_constant_by_value(constants, inum);

        return match + ' {' + constant[0] + '}';
    });
    
    console.log(output);
}

function onGccCompleted(started, res) {
    displayResult('gcc', started, res);
    
    console.log(chalk.black.bgGreen('\n ✔ build completed '));

    if (program.run) {
        process.stdout.write(chalk.black.bgWhite('\n   running yapl '));
        
        var fstream = fs.createReadStream(getInputFilename());
        fstream.on('open', function() {
            
            var proc = execa('yaplc', [], {
                // stdout: process.stdout,
                // stderr: process.stderr
            });

            proc
                .then(function(res) {
                    console.log(chalk.black.bgGreen(' ✔ '));

                    if (program.visualize)
                        createVisualization(res.stdout);

                    beautifyYaplOutput(res.stdout);

                    // run mix builder
                    if (program.builder)
                        execa('./bin/MIXBuilder.exe', ['./compiled.mix']);
                })
                .catch(function(err) {
                    console.log(chalk.black.bgRed(' ✖ '));

                    if (err.stdout)
                        beautifyYaplOutput(err.stdout);
                    
                    if (err.stderr)
                        beautifyYaplOutput(err.stderr);

                    console.log(chalk.red('$ command yapl failed'));
                });

            fstream.pipe(proc.stdin);
        });
    }

    return true;
}

function getReplacements() {
    return [
        [' error:', function() {
            return chalk.red(' (error)');
        }],
        [' warning:', function() {
            return chalk.yellow(' (warning)');
        }],
        [/[^\s]*\.(y|h|c|l):/i, function(b, a) {
            var found = a.match(/[^\s]*\.(y|h|c|l):/i);
            return chalk.cyan(found[0].substr(0, found[0].length - 1)) + ':';
        }],
        [/(\d+):(\d+):/i, function(b, a) {
            var found = a.match(/(\d+):(\d+):/i);
            return ' { line: ' + chalk.underline(found[1]) + ', col: ' + chalk.underline(found[2]) + ' } ->';
        }],
        ['\n', function() {
            return '\n' + PREFIX;
        }]
    ];
}

function hasBeautifyFinished(output) {
    var replacements = getReplacements();
    
    var indexes = [];
    for (var i in replacements)
        indexes.push(output.search(replacements[i][0]));

    return Math.max.apply(this, indexes) < 0;
}

function beautifyGccOutput(output) {
    var contents = output;

    process.stdout.write(PREFIX);
    while (!hasBeautifyFinished(contents)) {
        var replacements = getReplacements();
        var min = [contents.length + 1, null];

        for (var i in replacements) {
            var index = contents.search(replacements[i][0]);

            if (index < min[0] && index > -1)
                min = [index, replacements[i]];
        }

        var index = contents.search(min[1][0]);
        var before = contents.substr(0, index);
        var after = contents.substr(index);
        var contents = after.replace(min[1][0], '');

        process.stdout.write(before + min[1][1](before, after));
    }

    process.stdout.write(contents + '\n');
}

function onError(err) {
    console.log(chalk.red('$ command "' + err.cmd + '" ') + chalk.black.bgRed(' failed '));

    beautifyGccOutput(err.stderr);
}

var t = Date.now();
execa('yacc', ['-d', 'yapl.compiler.y'])
    .then(function(r) {
        var s=t;t=Date.now();
        return onYaccCompleted(s, r);
    })
    .then(function(r) {
        var s=t;t=Date.now();
        return onFlexCompleted(s, r);
    })
    .then(function(r) {
        var s=t;t=Date.now();
        return onGccCompleted(s, r);
    })
    .catch(function(e) {
        onError(e);
        console.log(chalk.black.bgRed('\n ✖ build failed '));
    });