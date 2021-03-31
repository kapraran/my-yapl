#!/usr/bin/env node

const path = require("path");
const fs = require("fs");
const execa = require("execa");
const chalk = require("chalk");
// const u = require('./assets/res/utils');
const ConstManager = require("./assets/res/const-manager");
const app = require("commander");

const pr = function (filename) {
  return path.resolve(__dirname, filename);
};

const winBins = {
  flex: "./bin/win_flex.exe",
  yacc: "./bin/win_bison.exe",
  mixBuilder: "./bin/MIXBuilder.exe",
};

const cm = new ConstManager([pr("src/y.tab.h"), pr("src/utils.compiler.h")]);

app
  .version("0.3.0")
  .option("-n, --nostyles", "Disable styles")
  .option("-v, --visualize", "Visualize Abstract Syntax Tree")
  .option("-r, --run", "Run compiler after compilation")
  .option("-i, --input <filename>", "Set input filename")
  .option("-o, --output <filename>", "Set output filename")
  .option("-b, --builder", "Run mixal builder (requires -r)")
  .parse(process.argv);

if (app.nostyles) chalk.enabled = false;

const TAB = "  ";
const PREFIX = TAB + chalk.gray(">") + TAB.substr(1);

const main = function () {};

const getInputFilename = function () {
  let filename = app.input ? app.input : "examples/example.yapl";
  return pr(filename);
};

const getCommand = function (name) {
  if (isWindows()) return winBins[name];

  if (isLinux()) return name;

  return null;
};

const isWindows = function () {
  return process.platform === "win32";
};

const isLinux = function () {
  return process.platform === "linux";
};

const displayResult = function (name, started, res) {
  let ms = Date.now() - started;

  if (res.failed)
    console.log(
      chalk.red("$ " + name + " ") +
        chalk.black.bgRed(" failed ") +
        chalk.red(" to complete")
    );
  else
    console.log(
      chalk.green("$ " + name + " completed with ") +
        chalk.black.bgGreen(" success ") +
        chalk.green(" in " + ms + "ms")
    );

  if (res.stdout.length > 0) {
    console.log(TAB + "-> " + chalk.black.bgWhite(" stdout "));
    name == "gcc" ? beautifyGccOutput(res.stdout) : console.log(res.stdout);
  }

  if (res.stderr.length > 0) {
    console.log(TAB + "-> " + chalk.white.bgRed(" stderr "));
    name == "gcc" ? beautifyGccOutput(res.stderr) : console.log(res.stderr);
  }
};

const editYTABH = function () {
  // load files
  let template = fs.readFileSync(pr("./assets/res/y.tab.h.tmpl"), "utf8");
  let ytabh = fs.readFileSync(pr("./y.tab.h"), "utf8");

  // replace contents
  let contents = template.replace("/**__INSERT_CONTENT__**/", ytabh);

  // update y.tab.h
  fs.writeFileSync(pr("./y.tab.h"), contents, "utf8");
};

const onYaccCompleted = function (started, res) {
  displayResult("yacc", started, res);
  editYTABH();
  cm.load();

  return execa(getCommand("flex"), [pr("src/yapl.compiler.l")]);
};

const onFlexCompleted = function (started, res) {
  displayResult("flex", started, res);

  return execa("gcc", ["lex.yy.c", "y.tab.c", "-o", "build/yaplc"]);
};

function createVisualization(output) {
  var nodes = [];
  var edges = [];

  var null_id = 100000;

  output.replace(
    /\{viz\} (\d+)\:(\d+)\{(.*)\}\:(\d+)\:(.*)/g,
    function (line, _id, type, contents, children_len, children_str) {
      if (!parseInt(_id) == 0) {
        var node = {
          id: parseInt(_id),
          label: cm.findByValue(parseInt(type)).name,
        };

        if (node.label.match(/^SYM_/)) {
          if (node.label == "SYM_TYPE_INT") {
            node.label = ": int";
          } else if (node.label == "SYM_VARIABLE") {
            node.label = "$ " + contents.trim();
          } else if (node.label == "SYM_METHOD") {
            node.label = "() " + contents.trim();
          } else if (node.label == "SYM_CONSTANT_INT") {
            node.label = contents.trim();
          } else if (contents.trim().length > 0)
            node.label += "::" + contents.trim();

          node.color = {
            // border: '#0a5a72'
            background: "#caddf7",
          };
        }

        nodes.push(node);
      }

      var children = children_str.replace(/\s+/g, " ").trim().split(" ");
      for (var i = 0; i < children_len; i++) {
        edges.push({
          from: parseInt(_id),
          to: parseInt(children[i]),
        });
      }

      return line;
    }
  );

  var src = fs.readFileSync(getInputFilename());
  var tpl = fs.readFileSync("./assets/res/ast.html.tmpl", "utf8");
  var rendered = tpl
    .replace("<!--__SOURCE_CODE__-->", src) // replace source code
    .replace("/**__NODES__**/0", JSON.stringify(nodes)) // replace nodes
    .replace("/**__EDGES__**/0", JSON.stringify(edges)); // replace edges

  fs.writeFileSync("./build/ast.html", rendered, "utf8");
}

const beautifyYaplOutput = function (output) {
  output = output.replace(/\[lex\]/g, `[${chalk.magenta("lex")}]`);
  output = output.replace(/\[yacc\]/g, `[${chalk.blue("yacc")}]`);

  output = output.replace(
    /\((epsilon|symbol|element)\)/g,
    function (match, str) {
      // if (str == 'epsilon') str = ' 💩 ';
      if (str == "epsilon") str = " ε ";
      else if (str == "element") str = " 💲 ";
      else str = " 🗽 ";
      return " " + chalk.yellow(str) + "   >";
    }
  );

  output = output.replace(/\$(\d+)/g, function (match, num) {
    let inum = parseInt(num);
    // let constant = findConstantByValue(constants, inum);
    let constant = cm.findByValue(inum);

    // return match + ' {' + constant[0] + '}';
    return match + " {" + constant.name + "}";
  });

  console.log(output);
};

function onGccCompleted(started, res) {
  displayResult("gcc", started, res);

  console.log("\n  " + chalk.black.bgGreen(" ✔ build completed \n"));

  if (app.run) {
    process.stdout.write(chalk.black.bgWhite("\n   running yaplc "));

    var fstream = fs.createReadStream(getInputFilename());
    fstream.on("open", function () {
      var args = [];

      if (app.visualize) args.push("-v");

      var proc = execa("./build/yaplc", args, {
        // stdout: process.stdout,
        // stderr: process.stderr
      });

      proc
        .then(function (res) {
          console.log(chalk.black.bgGreen(" ✔ "));

          if (app.visualize) createVisualization(res.stdout);

          beautifyYaplOutput(res.stdout);

          // run mix builder
          if (app.builder) execa(winBins.mixBuilder, ["./compiled.mix"]);
        })
        .catch(function (err) {
          console.log(chalk.black.bgRed(" ✖ "));

          console.error(err);

          if (err.stdout) beautifyYaplOutput(err.stdout);

          if (err.stderr) beautifyYaplOutput(err.stderr);

          console.log(chalk.red("$ command yapl failed"));
        });

      fstream.pipe(proc.stdin);
    });
  }

  return true;
}

function getReplacements() {
  return [
    [
      " error:",
      function () {
        return chalk.red(" (error)");
      },
    ],
    [
      " warning:",
      function () {
        return chalk.yellow(" (warning)");
      },
    ],
    [
      /[^\s]*\.(y|h|c|l):/i,
      function (b, a) {
        var found = a.match(/[^\s]*\.(y|h|c|l):/i);
        return chalk.cyan(found[0].substr(0, found[0].length - 1)) + ":";
      },
    ],
    [
      /(\d+):(\d+):/i,
      function (b, a) {
        var found = a.match(/(\d+):(\d+):/i);
        return (
          " { line: " +
          chalk.underline(found[1]) +
          ", col: " +
          chalk.underline(found[2]) +
          " } ->"
        );
      },
    ],
    [
      "\n",
      function () {
        return "\n" + PREFIX;
      },
    ],
  ];
}

function hasBeautifyFinished(output) {
  var replacements = getReplacements();

  var indexes = [];
  for (var i in replacements) indexes.push(output.search(replacements[i][0]));

  return Math.max.apply(this, indexes) < 0;
}

function beautifyGccOutput(output) {
  let contents = output;

  process.stdout.write(PREFIX);
  while (!hasBeautifyFinished(contents)) {
    let replacements = getReplacements();
    let min = [contents.length + 1, null];

    for (let i in replacements) {
      let index = contents.search(replacements[i][0]);

      if (index < min[0] && index > -1) min = [index, replacements[i]];
    }

    let index = contents.search(min[1][0]);
    let before = contents.substr(0, index);
    let after = contents.substr(index);
    let contents = after.replace(min[1][0], "");

    process.stdout.write(before + min[1][1](before, after));
  }

  process.stdout.write(contents + "\n");
}

function onError(err) {
  console.log(
    chalk.red('$ command "' + err.cmd + '" ') + chalk.black.bgRed(" failed ")
  );

  beautifyGccOutput(err.stderr);
}

var t = Date.now();
execa(getCommand("yacc"), ["-d", "./src/yapl.compiler.y"])
  .then(function (r) {
    let s = t;
    t = Date.now();
    return onYaccCompleted(s, r);
  })
  .then(function (r) {
    let s = t;
    t = Date.now();
    return onFlexCompleted(s, r);
  })
  .then(function (r) {
    let s = t;
    t = Date.now();
    return onGccCompleted(s, r);
  })
  .catch(function (e) {
    onError(e);
    console.log(chalk.black.bgRed("\n ✖ build failed "));
  });
