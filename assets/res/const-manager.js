const fs = require("fs");

class ConstManager {
  constructor(srcFiles) {
    this.srcFiles = srcFiles;
    this.constants = [];
    this.index = {};
  }

  add(name, value) {
    const newIndex = this.constants.push({ name, value }) - 1;
    this.index[name] = newIndex;
  }

  has(name) {
    return name in this.index;
  }

  load() {
    let pattern = /\#define\s+([a-zA-Z_]+)\s+(\d+|\'.\'|[a-zA-Z_]+)/g;
    let skip = ["FALSE", "TRUE", "MAX_CHILDREN"];

    for (let i in this.srcFiles) {
      let filename = this.srcFiles[i];
      let content = fs.readFileSync(filename, "utf8");
      let found;

      while ((found = pattern.exec(content))) {
        let [_, name, value] = found;

        if (skip.includes(name)) continue;

        if (value[0] === "'") {
          value = value.charCodeAt(1);
        } else if (isNaN(value)) {
          // value acts as a name here
          if (!this.has(value)) continue;

          value = this.findByName(value).value;
        } else {
          value = parseInt(value);
        }

        this.add(name, value);
      }
    }
  }

  findByName(name) {
    const index = this.index[name];
    return index ?? null;
  }

  findByValue(value) {
    return this.constants.find((constant) => constant.value === value) || null;
  }
}

module.exports = ConstManager;
