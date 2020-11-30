const fs = require('fs');

class ConstManager {
    constructor(srcFiles) {
        this.srcFiles = srcFiles;
        this.constants = [];
        this.index = {};
    }

    add(name, value) {
        let i = this.constants.push({name, value});
        this.index[name] = i-1;
    }

    has(name) {
        return name in this.index;
    }

    load() {
        let pattern = /\#define\s+([a-zA-Z_]+)\s+(\d+|\'.\'|[a-zA-Z_]+)/g;
        let skip = ['FALSE', 'TRUE', 'MAX_CHILDREN'];

        for (let i in this.srcFiles) {
            let filename = this.srcFiles[i];
            let content = fs.readFileSync(filename, 'utf8');
            let found;

            while ((found = pattern.exec(content))) {
                let [_, name, value] = found;

                if (skip.includes(name))
                    continue;

                if (value[0] === '\'') {
                    value = value.charCodeAt(1);
                } else if (isNaN(value)) {
                    // value acts as a name here
                    if (!this.has(value))
                        continue;

                    value = this.findByName(value).value;
                } else {
                    value = parseInt(value);
                }

                this.add(name, value);
            }
        }
    }

    findByName(name) {
        if (!(name in this.index))
            return null;

        let i = this.index[name];
        return this.constants[i];
    }

    findByValue(value) {
        for (let i in this.constants)
            if (this.constants[i].value === value)
                return this.constants[i];

        return null;
    }
}

/** test **/
// let cm = new ConstManager(['../y.tab.h', '../utils.compiler.h']);
// console.log(cm.constants);
// console.log(cm.findByName('EL_SUB'))
// console.log(cm.findByValue(1017))

module.exports = ConstManager;
