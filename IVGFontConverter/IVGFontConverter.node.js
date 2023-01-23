(function(FuseBox){FuseBox.$fuse$=FuseBox;
FuseBox.target = "browser";
FuseBox.pkg("default", {}, function(___scope___){
___scope___.file("nodeCLI.js", function(exports, require, module, __filename, __dirname){

"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const fs = require("fs");
const typr = require("@fredli74/typr");
const IVGFontConverter = require("./IVGFontConverter");
const VERSION = "0.3.9";
if (process.argv.length < 3) {
    process.stderr.write("IVGFontConverter version " + VERSION + "\n\n" +
        "Usage: node IVGFontConverter.node.js <opentype filename> [ ? | - | <feature>[,<feature>...] ] [ <charset>[,<charset>] ]\n" +
        "\n" +
        "  ?          List all GSUB features\n" +
        "  -          No extra GSUB feature\n" +
        "  <feature>  Enable GSUB feature by [<script>.[<language>.]]<feature>\n" +
        "  <charset>  Convert unicode characters [<hex>[-<hex>]]\n" +
        "\n" +
        "Example: node IVGFontConverter.node.js font.otf\n" +
        "Example: node IVGFontConverter.node.js font.otf ss01\n" +
        "         node IVGFontConverter.node.js font.ttf latn.ROM.locl,latn.ss01\n" +
        "         node IVGFontConverter.node.js font.ttf - 0020-007f,a0-cf\n");
    process.exit(1);
}
let data = fs.readFileSync(process.argv[2]);
function toArrayBuffer(buf) {
    let ab = new ArrayBuffer(buf.length);
    let view = new Uint8Array(ab);
    for (let i = 0; i < buf.length; ++i) {
        view[i] = buf[i];
    }
    return ab;
}
const font = new typr.Font(toArrayBuffer(data));
const fontname = font.getFamilyName();
const subname = font.getSubFamilyName();
const fList = process.argv.length > 3 ? process.argv[3] : undefined;
const activeFeatures = [];
if (fList !== undefined && fList !== "-") {
    // script -> lang -> feature# -> lookup#
    const map = {};
    const nameMap = {};
    const enableList = fList.split(",");
    if (font.GSUB) {
        for (const script in font.GSUB.scriptList) {
            for (const lang in font.GSUB.scriptList[script]) {
                const slDot = script + "." + lang;
                const features = font.GSUB.scriptList[script][lang].features;
                for (let f = 0; f < features.length; ++f) {
                    const featureNum = features[f];
                    const feature = font.GSUB.featureList[featureNum];
                    if (!feature.tag)
                        continue;
                    let supported = true;
                    for (let t = 0; t < feature.tab.length; ++t) {
                        const lookup = font.GSUB.lookupList[feature.tab[t]];
                        if (lookup.ltype !== 1) {
                            supported = false;
                            break;
                        }
                    }
                    if (!supported)
                        continue;
                    for (let i = 0; i < enableList.length; ++i) {
                        const match = enableList[i].split(".");
                        const mtag = match.pop();
                        if (mtag !== feature.tag)
                            continue;
                        const mscript = match.shift();
                        if (mscript !== undefined && mscript !== script)
                            continue;
                        const mlang = match.pop();
                        if (mlang !== undefined && mlang !== lang)
                            continue;
                        font.enableGSUB(featureNum);
                        if (activeFeatures.indexOf(feature.tag) < 0) {
                            activeFeatures.push(feature.tag);
                        }
                    }
                    nameMap[feature.tag] = font.featureFriendlyName(feature) || nameMap[feature.tag] || feature.tag;
                    for (let t = 0; t < feature.tab.length; ++t) {
                        const lookupNum = feature.tab[t];
                        map[lookupNum] = map[lookupNum] || {};
                        map[lookupNum][feature.tag] = map[lookupNum][feature.tag] || [];
                        map[lookupNum][feature.tag].push(slDot);
                    }
                }
            }
        }
    }
    if (fList === "?") {
        let list = [];
        for (const m in map) {
            for (const f in map[m]) {
                let entry = "  " + nameMap[f] + "\n    " + (map[m][f].length > 1 ? "(" + map[m][f].join("|") + ")" : map[m][f][0]) + "." + f;
                if (list.indexOf(entry) < 0) {
                    list.push(entry);
                }
            }
        }
        if (list.length > 0) {
            process.stdout.write("Font has the following GSUB features.\n" + list.sort().join("\n"));
        }
        else {
            process.stdout.write("Font does not have any GSUB features.\n");
        }
        process.stdout.write("\n");
        process.exit(0);
    }
}
let charset;
let options = {
    charset: undefined,
    quality: 1,
};
if (process.argv.length > 4) {
    const cList = process.argv[4].split(",").map((v) => {
        const range = v.split("-");
        const from = parseInt(range[0], 16);
        const to = range.length > 1 ? parseInt(range[1], 16) : from;
        const r = [];
        for (let i = from; i <= to; ++i) {
            r.push(String.fromCharCode(i));
        }
        return r.join("");
    });
    options.charset = cList.join("");
}
let ivgfont = IVGFontConverter.fromTypr(font, options);
const outdata = ivgfont.toString();
const header = (fontname ? "\"" + fontname + (subname ? " - " + subname : "") + "\" font " : "Font ") +
    "converted with IVGFontConverter " + VERSION + ", " +
    (activeFeatures.length > 0 ? activeFeatures.join(", ") + ", " : "") + (options.charset ? "custom charset" : "ISO charset") + "\n\n" +
    (font.name.version ? font.name.version + "\n" : "") +
    (font.name.copyright ? font.name.copyright + "\n" : "") +
    (font.name.trademark ? font.name.trademark + "\n" : "") +
    (font.name.manufacturer ? font.name.manufacturer + (font.name.urlVendor ? " " + font.name.urlVendor : "") + "\n" : "") +
    (font.name.designer ? font.name.designer + (font.name.urlDesigner ? " " + font.name.urlDesigner : "") + "\n" : "") +
    (font.name.description ? font.name.description + "\n" : "") +
    (font.name.licence ? font.name.licence + "\n" : "") +
    (font.name.licenceURL ? font.name.licenceURL + "\n" : "");
process.stdout.write("/*\n\n" +
    header.replace(/\*\//g, "") +
    "\n*/\n" +
    "format ivgfont-1 requires:IMPD-1\n" +
    outdata);
process.exit(0);
//# sourceMappingURL=IVGFontConverter.node.js.map?tm=1654422719104
});
___scope___.file("IVGFontConverter.js", function(exports, require, module, __filename, __dirname){

"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.fromTypr = exports.Charset = void 0;
const assert = require("assert");
const ivgfont_1 = require("./ivgfont");
class Charset {
}
exports.Charset = Charset;
Charset.ISO = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~ ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ";
Charset.ASCII = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
function isStraightCurve(data) {
    const [a, b] = [-data[data.length - 1], data[data.length - 2]];
    // cache and range check part of the formula (see below)
    const d = 1 / Math.sqrt(a * a + b * b);
    if (!isFinite(d))
        return false;
    for (let i = 0; i < data.length - 2; i += 2) {
        // make sure we do not match control points outside the line ends
        if ((data[i] - 0) * (b - data[i]) < 0 || (data[i + 1] - 0) * (-a - data[i + 1]) < 0)
            return false;
        // line from 0,0 to a,b => general from equation (where c == 0) =>  ax + by = 0
        // Distance from point (x,y) => (| ax + by |) / sqrt(a^2 + b^2)
        if (Math.abs((a * data[i] + b * data[i + 1]) * d) >= 1)
            return false;
    }
    return true;
}
function trunc(v) {
    return (v - v % 1) || (!isFinite(v) || v === 0 ? v : v < 0 ? -0 : 0);
}
function TyprPathToSVGPath(path, quality) {
    // make TyprPath into an instruction list
    let instructions = [];
    {
        let startX = 0, prevX = 0;
        let startY = 0, prevY = 0;
        const lmap = { "M": 2, "L": 2, "Q": 4, "C": 6 };
        for (let i = 0, co = 0; i < path.cmds.length; ++i) {
            const cmd = path.cmds[i];
            const cn = co + (lmap[cmd] || 0);
            let inst = { type: cmd.toLowerCase(), data: [], coords: [] };
            while (co < cn) {
                const X = trunc(path.crds[co++] * quality);
                const Y = trunc(-path.crds[co++] * quality);
                inst.coords.push(X, Y);
                inst.data.push(X - prevX, Y - prevY);
            }
            switch (cmd) {
                case "M":
                case "L":
                case "Q":
                case "C":
                    prevX = inst.coords[inst.coords.length - 2];
                    prevY = inst.coords[inst.coords.length - 1];
                    if (cmd === "M") {
                        startX = prevX;
                        startY = prevY;
                    }
                    break;
                case "Z":
                    prevX = startX;
                    prevY = startY;
                    break;
                default:
                    throw (cmd + " not supported");
            }
            instructions.push(inst);
        }
    }
    // Optimize instructions
    {
        let prev;
        instructions = instructions.filter((inst, index) => {
            let next = instructions[index + 1];
            if (inst.data.length > 0) {
                // convert straight curves into lines segments
                if ((inst.type === "c" || inst.type === "q") && isStraightCurve(inst.data)) {
                    inst.type = 'l';
                    inst.data = inst.data.slice(-2);
                    inst.coords = inst.coords.slice(-2);
                }
                if (inst.type === "l") {
                    // horizontal and vertical line shorthands
                    if (inst.data[0] === 0) { // l 0 50 → v 50
                        inst.type = "v";
                        inst.data.shift();
                        inst.coords.shift();
                    }
                    else if (inst.data[1] === 0) { // l 50 0 → h 50
                        inst.type = "h";
                        inst.data.pop();
                        inst.coords.pop();
                    }
                }
                // collapse repeated commands
                // h 20 h 30 -> h 50
                if ('mhv'.indexOf(inst.type) > -1 && prev && inst.type === prev.type &&
                    (inst.type === "m" || (prev.data[0] >= 0) === (inst.data[0] >= 0))) {
                    prev.data[0] += inst.data[0];
                    if (inst.type === "m") { // m has two coords
                        prev.data[1] += inst.data[1];
                    }
                    prev.coords = inst.coords;
                    return false;
                }
                // convert cubic into smooth shorthands
                if (prev && inst.type === 'c') {
                    switch (prev.type) {
                        case "c": // c + c → c + s
                            if (inst.data[0] === -(prev.data[2] - prev.data[4]) &&
                                inst.data[1] === -(prev.data[3] - prev.data[5])) {
                                inst.type = 's';
                                inst.data = inst.data.slice(2);
                                inst.coords = inst.coords.slice(2);
                            }
                            break;
                        case "s": // s + c → s + s
                            if (inst.data[0] === -(prev.data[0] - prev.data[2]) &&
                                inst.data[1] === -(prev.data[1] - prev.data[3])) {
                                inst.type = 's';
                                inst.data = inst.data.slice(2);
                                inst.coords = inst.coords.slice(2);
                            }
                            break;
                        default: // [^cs] + c → [^cs] + s
                            if (inst.data[0] === 0 && inst.data[1] === 0) {
                                inst.type = 's';
                                inst.data = inst.data.slice(2);
                                inst.coords = inst.coords.slice(2);
                            }
                    }
                }
                // convert quadratic into smooth shorthands
                if (prev && inst.type === 'q') {
                    switch (prev.type) {
                        case "q": // q + q → q + t
                            if (inst.data[0] === (prev.data[2] - prev.data[0]) &&
                                inst.data[1] === (prev.data[3] - prev.data[1])) {
                                inst.type = 't';
                                inst.data = inst.data.slice(2);
                                inst.coords = inst.coords.slice(2);
                            }
                            break;
                        case "t": // t + q → t + t
                            if (inst.data[2] === prev.data[0] && inst.data[3] === prev.data[1]) {
                                inst.type = 't';
                                inst.data = inst.data.slice(2);
                                inst.coords = inst.coords.slice(2);
                            }
                    }
                }
                // remove useless non-first path segments
                if ('lhvqtcs'.indexOf(inst.type) > -1 && inst.data.every(i => i === 0)) {
                    // l 0,0 / h 0 / v 0 / q 0,0 0,0 / t 0,0 / c 0,0 0,0 0,0 / s 0,0 0,0
                    instructions[index] = instructions[index - 1]; // do we need it?
                    return false;
                }
                if (inst.type === "a" && inst.data[5] === 0 && inst.data[6] === 0) {
                    throw "we have A?";
                    // a 25,25 -30 0,1 0,0
                    instructions[index] = instructions[index - 1]; // do we need it?
                    return false;
                }
                prev = inst;
            }
            else {
                assert(inst.type === "Z" || inst.type === "z");
                return (prev === undefined || prev.type.toUpperCase() !== "Z");
            }
            return true;
        });
    }
    // Convert instructions to path string
    let out = "";
    {
        let prev;
        for (let i = 0; i < instructions.length; ++i) {
            const inst = instructions[i];
            const dataString = function (relative) {
                let c = relative ? inst.type : inst.type.toUpperCase();
                let s = (prev !== undefined && (c === prev.type || (c === "L" && prev.type === "M") || (c === "l" && prev.type === "m")) ? "," : c);
                s += (relative ? inst.data : inst.coords).join(",");
                return s.replace(/,-/g, "-");
            };
            let abs = dataString(false);
            let rel = dataString(true);
            if (abs.length < rel.length) {
                out += abs;
                inst.type = inst.type.toUpperCase();
            }
            else {
                out += rel;
            }
            prev = inst;
        }
    }
    return out;
}
function coverageIndex(cvg, val) {
    let tab = cvg.tab;
    if (cvg.fmt == 1)
        return tab.indexOf(val);
    for (let i = 0; i < tab.length; i += 3) {
        let start = tab[i], end = tab[i + 1], index = tab[i + 2];
        if (start <= val && val <= end)
            return index + (val - start);
    }
    return -1;
}
function GSUBlookup(table, index, gIndex) {
    for (let n = 0; n < table.featureList[index].tab.length; ++n) {
        let listIndex = table.featureList[index].tab[n];
        let lookup = table.lookupList[listIndex];
        if (lookup.ltype !== 1) {
            throw "unsupported GSUB lookup type " + lookup.ltype;
        }
        else {
            // coverage index 
            for (let t = 0; t < lookup.tabs.length; ++t) {
                let i = coverageIndex(lookup.tabs[t].coverage, gIndex);
                if (i >= 0) {
                    if (lookup.tabs[t].fmt === 1) {
                        return gIndex + lookup.tabs[t].delta;
                    }
                    else if (lookup.tabs[t].fmt === 2) {
                        return lookup.tabs[t].newg[i];
                    }
                    else {
                        throw "unsupported GSUB lookup tab format " + lookup.tabs[t].fmt;
                    }
                }
            }
        }
    }
    return gIndex;
}
function fromTypr(otfont, options) {
    const quality = options && options.quality || 1.0;
    const kerning = !(options && options.kerning === false);
    const ivgfont = new ivgfont_1.IVGfont(Math.round(otfont.head.unitsPerEm * quality), Math.round(otfont.hhea.ascender * quality), Math.round(otfont.hhea.descender * quality), Math.round(otfont.hhea.lineGap * quality));
    const charset = (options && options.charset || Charset.ISO).split("");
    charset.unshift("\0");
    let unicodeGlyphIndex = {};
    for (let i = 0; i < charset.length; ++i) {
        assert(charset[i].charCodeAt(0) !== undefined);
        let unicode = charset[i].charCodeAt(0);
        assert(unicode >= 0); // sanity check
        assert(unicode < 0xd800 || unicode > 0xdbff); // unicode surrogate range
        assert(unicode <= 0xffff); // don't allow more than 16bit
        let gIndex = otfont.codeToGlyph(unicode);
        if (gIndex === 0) { // .notdef
            unicode = 0;
        } /*else if (options.GSUB !== undefined && otfont.GSUB && otfont.GSUB.featureList) {
            for (let i = 0; i < otfont.GSUB.featureList.length; ++i) {
                if (options.GSUB.indexOf(otfont.GSUB.featureList[i].tag) >= 0) {
                    gIndex = GSUBlookup(otfont.GSUB, i, gIndex);
                }
            }
        }*/
        if (!ivgfont.hasGlyph(unicode)) {
            const path = otfont.glyphToPath(gIndex);
            const pathData = TyprPathToSVGPath(path, quality);
            ivgfont.setGlyph(new ivgfont_1.IVGglyph(unicode, otfont.hmtx.aWidth[gIndex], pathData));
            unicodeGlyphIndex[unicode] = gIndex;
        }
    }
    if (kerning) {
        for (let a in unicodeGlyphIndex) {
            for (let b in unicodeGlyphIndex) {
                let adjustment = otfont.getPairAdjustment(unicodeGlyphIndex[a], unicodeGlyphIndex[b]);
                if (adjustment !== 0) {
                    ivgfont.setKerning(Number(a), Number(b), adjustment);
                }
            }
        }
    }
    return ivgfont;
}
exports.fromTypr = fromTypr;
//# sourceMappingURL=IVGFontConverter.node.js.map?tm=1654422719104
});
___scope___.file("ivgfont.js", function(exports, require, module, __filename, __dirname){

"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.IVGfont = exports.IVGglyph = void 0;
const assert = require("assert");
function IMPDescape(val) {
    // Determine optimal escape. 
    // IMPD needs to escape $:;=[]{}"\ when outside of a string, and only "\ inside strings.
    let escapes = val.match(/[ $:;=[\]{}]/g);
    // two or more avoidable escapes makes "" worth it
    let stringEscape = (val === "" || (escapes && escapes.length > 1));
    let expression = stringEscape ?
        /[^\x20-\x7E]|["\\]/g :
        /[^\x20-\x7E]|[ $:;=[\]{}"\\]/g;
    let output = val.replace(expression, function (v) {
        assert(v.charCodeAt(0) !== undefined);
        assert(v.charCodeAt(0) >= 0); // sanity check
        assert(v.charCodeAt(0) < 0xd800 || v.charCodeAt(0) > 0xdbff); // unicode surrogate range
        assert(v.charCodeAt(0) <= 0xffff); // don't allow more than 16bit
        if (v >= "\x20" && v <= "\x7E") {
            return "\\" + v;
        }
        else {
            let hex = v.charCodeAt(0).toString(16);
            return (hex.length <= 2 ? "\\x" + ('00' + hex).slice(-2) : "\\u" + ('0000' + hex).slice(-4));
        }
    });
    if (stringEscape) {
        return "\"" + output + "\"";
    }
    else {
        return output;
    }
}
class KernCluster {
    constructor(left, right) {
        this.lList = left || [];
        this.rList = right || [];
    }
    Sort() {
        this.lList = this.lList.sort((a, b) => Number(a) - Number(b));
        this.rList = this.rList.sort((a, b) => Number(a) - Number(b));
    }
    AddLeft(unicode) {
        if (this.lList.indexOf(unicode) < 0) {
            this.lList.push(unicode);
        }
    }
    AddRight(unicode) {
        if (this.rList.indexOf(unicode) < 0) {
            this.rList.push(unicode);
        }
    }
    // convert internal list of unicode numbers to a string
    static listToString(list) {
        let s = "";
        for (let i = 0; i < list.length; ++i) {
            s += String.fromCharCode(list[i]);
        }
        return IMPDescape(s);
    }
    Left() {
        return KernCluster.listToString(this.lList);
    }
    Right() {
        return KernCluster.listToString(this.rList);
    }
    Pair() {
        return (this.Left() + " " + this.Right());
    }
    toString() {
        return this.Pair();
    }
    // Combines two clusters into a set with common pairs
    static Combine(A, B) {
        // Will test combine using both left focus (lf) and right focus (rf)
        //  in: [ABCD abfg, CDEF bcdefg]
        //  lf: [AB abfg, EF bcdefg, CD abcdefg]
        //  rf: [ABCD a, CDEF cde, ABCDEF bfg]
        let lf = [new KernCluster(), new KernCluster(), new KernCluster(),];
        let rf = [new KernCluster(), new KernCluster(), new KernCluster(),];
        // populate the lf and rf lists
        for (let i = 0; i < A.lList.length; ++i) {
            let v = A.lList[i];
            rf[0].AddLeft(v);
            rf[2].AddLeft(v);
            lf[B.lList.indexOf(v) < 0 ? 0 : 2].AddLeft(v);
        }
        for (let i = 0; i < B.lList.length; ++i) {
            let v = B.lList[i];
            rf[1].AddLeft(v);
            rf[2].AddLeft(v);
            lf[A.lList.indexOf(v) < 0 ? 1 : 2].AddLeft(B.lList[i]);
        }
        for (let i = 0; i < A.rList.length; ++i) {
            let v = A.rList[i];
            lf[0].AddRight(v);
            lf[2].AddRight(v);
            rf[B.rList.indexOf(v) < 0 ? 0 : 2].AddRight(v);
        }
        for (let i = 0; i < B.rList.length; ++i) {
            let v = B.rList[i];
            lf[1].AddRight(v);
            lf[2].AddRight(v);
            rf[A.rList.indexOf(v) < 0 ? 1 : 2].AddRight(B.rList[i]);
        }
        // Remove invalid pairs
        lf = lf.filter(v => {
            return v.lList.length > 0 && v.rList.length > 0;
        });
        rf = rf.filter(v => {
            return v.lList.length > 0 && v.rList.length > 0;
        });
        // Return the smallest combo
        if (lf.join(" ").length < rf.join(" ").length) {
            return lf;
        }
        else {
            return rf;
        }
    }
}
class IVGglyph {
    constructor(unicode, xAdvance, path) {
        this.unicode = unicode;
        this.xAdvance = xAdvance;
        this.path = path;
        this.kerning = {};
    }
}
exports.IVGglyph = IVGglyph;
class IVGfont {
    constructor(upm, ascent, descent, linegap) {
        this.upm = upm;
        this.ascent = ascent;
        this.descent = descent;
        this.linegap = linegap;
        this.glyphs = {};
    }
    hasGlyph(unicode) {
        return this.glyphs[unicode] !== undefined;
    }
    getGlyph(unicode) {
        let g = this.glyphs[unicode];
        if (!g) {
            g = this.glyphs[0];
        }
        assert(g);
        return g;
    }
    setGlyph(glyph) {
        this.glyphs[glyph.unicode] = glyph;
    }
    getKerning(first, second) {
        let a = this.glyphs[first];
        if (a) {
            let b = a.kerning[second];
            if (b) {
                return b;
            }
        }
        return 0;
    }
    setKerning(first, second, adjustment) {
        let a = this.glyphs[first];
        assert(a);
        if (a) {
            assert(a.kerning);
            a.kerning[second] = adjustment;
        }
    }
    toString() {
        const s = [
            `metrics upm:${this.upm} ascent:${this.ascent} descent:${this.descent} linegap:${this.linegap}`,
            `g=glyph;k=kern`
        ];
        for (let k in this.glyphs) {
            assert(this.glyphs.hasOwnProperty(k));
            let v = this.glyphs[k];
            assert(v.xAdvance !== undefined);
            assert(v.path !== undefined);
            if (v.xAdvance < 0) {
                throw `glyph ${IMPDescape(String.fromCharCode(Number(k)))} has an invalid x-advance (${v.xAdvance})`;
            }
            s.push(`$g ${IMPDescape(String.fromCharCode(Number(k)))} ${v.xAdvance} ${IMPDescape(v.path)}`);
        }
        // Cluster kernpairs            
        {
            // Build a kerncluster list of all kerning pairs
            let kernlist = {};
            for (let k in this.glyphs) {
                let v = this.glyphs[k];
                assert(v.kerning);
                for (let p in v.kerning) {
                    assert(v.kerning.hasOwnProperty(p));
                    let adjust = v.kerning[p];
                    assert(adjust);
                    if (kernlist[adjust] === undefined) {
                        kernlist[adjust] = [];
                    }
                    kernlist[adjust].push(new KernCluster([Number(k)], [Number(p)]));
                }
            }
            let sortedList = Object.keys(kernlist).sort((a, b) => Number(a) - Number(b));
            for (let adjust of sortedList) {
                assert(kernlist.hasOwnProperty(adjust));
                let list = kernlist[adjust];
                // Combine pairs
                let stale;
                do {
                    stale = true;
                    for (let i = 0; i < list.length; ++i) {
                        for (let d = i + 1; d < list.length; ++d) {
                            let oldSize = [list[i], list[d]].join(" ").length;
                            let combine = KernCluster.Combine(list[i], list[d]);
                            if (combine.join(" ").length < oldSize) {
                                list.splice(d, 1, ...combine);
                                list.splice(i, 1);
                                d = i; // rewind
                                stale = false;
                            }
                        }
                    }
                } while (!stale);
                // Sort the output, just to make the output pretty
                for (let i = 0; i < list.length; ++i) {
                    list[i].Sort();
                }
                s.push(`$k ${adjust} ${list.join(" ")}`);
            }
        }
        return s.join("\n");
    }
}
exports.IVGfont = IVGfont;
//# sourceMappingURL=IVGFontConverter.node.js.map?tm=1654422719104
});
return ___scope___.entry = "nodeCLI.js";
});
FuseBox.pkg("@fredli74/typr", {}, function(___scope___){
___scope___.file("dist/index.js", function(exports, require, module, __filename, __dirname){

"use strict";
/**
 *
 * TypeScript class wrapper for Typr.js library
 * Most types are only declared for code completion as the backing library has no idea what it actually parses or creates
 *
 * **/
exports.__esModule = true;
exports.Font = void 0;
var Typr_js_1 = require("./Typr.js");
var friendlyTags = { "aalt": "Access All Alternates", "abvf": "Above-base Forms", "abvm": "Above - base Mark Positioning", "abvs": "Above - base Substitutions", "afrc": "Alternative Fractions", "akhn": "Akhands", "blwf": "Below - base Forms", "blwm": "Below - base Mark Positioning", "blws": "Below - base Substitutions", "calt": "Contextual Alternates", "case": "Case - Sensitive Forms", "ccmp": "Glyph Composition / Decomposition", "cfar": "Conjunct Form After Ro", "cjct": "Conjunct Forms", "clig": "Contextual Ligatures", "cpct": "Centered CJK Punctuation", "cpsp": "Capital Spacing", "cswh": "Contextual Swash", "curs": "Cursive Positioning", "c2pc": "Petite Capitals From Capitals", "c2sc": "Small Capitals From Capitals", "dist": "Distances", "dlig": "Discretionary Ligatures", "dnom": "Denominators", "dtls": "Dotless Forms", "expt": "Expert Forms", "falt": "Final Glyph on Line Alternates", "fin2": "Terminal Forms #2", "fin3": "Terminal Forms #3", "fina": "Terminal Forms", "flac": "Flattened accent forms", "frac": "Fractions", "fwid": "Full Widths", "half": "Half Forms", "haln": "Halant Forms", "halt": "Alternate Half Widths", "hist": "Historical Forms", "hkna": "Horizontal Kana Alternates", "hlig": "Historical Ligatures", "hngl": "Hangul", "hojo": "Hojo Kanji Forms(JIS X 0212 - 1990 Kanji Forms)", "hwid": "Half Widths", "init": "Initial Forms", "isol": "Isolated Forms", "ital": "Italics", "jalt": "Justification Alternates", "jp78": "JIS78 Forms", "jp83": "JIS83 Forms", "jp90": "JIS90 Forms", "jp04": "JIS2004 Forms", "kern": "Kerning", "lfbd": "Left Bounds", "liga": "Standard Ligatures", "ljmo": "Leading Jamo Forms", "lnum": "Lining Figures", "locl": "Localized Forms", "ltra": "Left - to - right alternates", "ltrm": "Left - to - right mirrored forms", "mark": "Mark Positioning", "med2": "Medial Forms #2", "medi": "Medial Forms", "mgrk": "Mathematical Greek", "mkmk": "Mark to Mark Positioning", "mset": "Mark Positioning via Substitution", "nalt": "Alternate Annotation Forms", "nlck": "NLC Kanji Forms", "nukt": "Nukta Forms", "numr": "Numerators", "onum": "Oldstyle Figures", "opbd": "Optical Bounds", "ordn": "Ordinals", "ornm": "Ornaments", "palt": "Proportional Alternate Widths", "pcap": "Petite Capitals", "pkna": "Proportional Kana", "pnum": "Proportional Figures", "pref": "Pre - Base Forms", "pres": "Pre - base Substitutions", "pstf": "Post - base Forms", "psts": "Post - base Substitutions", "pwid": "Proportional Widths", "qwid": "Quarter Widths", "rand": "Randomize", "rclt": "Required Contextual Alternates", "rkrf": "Rakar Forms", "rlig": "Required Ligatures", "rphf": "Reph Forms", "rtbd": "Right Bounds", "rtla": "Right - to - left alternates", "rtlm": "Right - to - left mirrored forms", "ruby": "Ruby Notation Forms", "rvrn": "Required Variation Alternates", "salt": "Stylistic Alternates", "sinf": "Scientific Inferiors", "size": "Optical size", "smcp": "Small Capitals", "smpl": "Simplified Forms", "ssty": "Math script style alternates", "stch": "Stretching Glyph Decomposition", "subs": "Subscript", "sups": "Superscript", "swsh": "Swash", "titl": "Titling", "tjmo": "Trailing Jamo Forms", "tnam": "Traditional Name Forms", "tnum": "Tabular Figures", "trad": "Traditional Forms", "twid": "Third Widths", "unic": "Unicase", "valt": "Alternate Vertical Metrics", "vatu": "Vattu Variants", "vert": "Vertical Writing", "vhal": "Alternate Vertical Half Metrics", "vjmo": "Vowel Jamo Forms", "vkna": "Vertical Kana Alternates", "vkrn": "Vertical Kerning", "vpal": "Proportional Alternate Vertical Metrics", "vrt2": "Vertical Alternates and Rotation", "vrtr": "Vertical Alternates for Rotation", "zero": "Slashed Zero" };
var Font = /** @class */ (function () {
    function Font(data) {
        var obj = Typr_js_1.Typr.parse(data);
        // Only support for one font (obj[0])
        if (!obj.length || typeof obj[0] !== "object" || typeof obj[0].hasOwnProperty !== "function") {
            throw "unable to parse font";
        }
        for (var n in obj[0]) {
            this[n] = obj[0][n];
        }
        this.enabledGSUB = {};
    }
    Font.prototype.getFamilyName = function () {
        return this.name && (this.name.typoFamilyName || this.name.fontFamily) || "";
    };
    Font.prototype.getSubFamilyName = function () {
        return this.name && (this.name.typoSubfamilyName || this.name.fontSubfamily) || "";
    };
    Font.prototype.glyphToPath = function (gid) {
        return Typr_js_1.Typr.U.glyphToPath(this, gid);
    };
    Font.prototype.getPairAdjustment = function (gid1, gid2) {
        return Typr_js_1.Typr.U.getPairAdjustment(this, gid1, gid2);
    };
    Font.prototype.stringToGlyphs = function (str) {
        return Typr_js_1.Typr.U.stringToGlyphs(this, str);
    };
    Font.prototype.glyphsToPath = function (gls) {
        return Typr_js_1.Typr.U.glyphsToPath(this, gls);
    };
    Font.prototype.pathToSVG = function (path, prec) {
        return Typr_js_1.Typr.U.pathToSVG(path, prec);
    };
    Font.prototype.pathToContext = function (path, ctx) {
        return Typr_js_1.Typr.U.pathToContext(path, ctx);
    };
    /*** Additional features ***/
    Font.prototype.lookupFriendlyName = function (table, feature) {
        if (this[table] !== undefined) {
            var tbl = this[table];
            var feat = tbl.featureList[feature];
            return this.featureFriendlyName(feat);
        }
        return "";
    };
    Font.prototype.featureFriendlyName = function (feature) {
        if (friendlyTags[feature.tag]) {
            return friendlyTags[feature.tag];
        }
        if (feature.tag.match(/ss[0-2][0-9]/)) {
            var name_1 = "Stylistic Set " + Number(feature.tag.substr(2, 2)).toString();
            if (feature.featureParams) {
                var version = Typr_js_1.Typr._bin.readUshort(this._data, feature.featureParams);
                if (version === 0) {
                    var nameID = Typr_js_1.Typr._bin.readUshort(this._data, feature.featureParams + 2);
                    if (this.name && this.name[nameID] !== undefined) {
                        return name_1 + " - " + this.name[nameID];
                    }
                }
            }
            return name_1;
        }
        if (feature.tag.match(/cv[0-9][0-9]/)) {
            return "Character Variant " + Number(feature.tag.substr(2, 2)).toString();
        }
        return "";
    };
    Font.prototype.enableGSUB = function (featureNumber) {
        if (this.GSUB) {
            var feature = this.GSUB.featureList[featureNumber];
            if (feature) {
                for (var i = 0; i < feature.tab.length; ++i) {
                    this.enabledGSUB[feature.tab[i]] = (this.enabledGSUB[feature.tab[i]] || 0) + 1;
                }
            }
        }
    };
    Font.prototype.disableGSUB = function (featureNumber) {
        if (this.GSUB) {
            var feature = this.GSUB.featureList[featureNumber];
            if (feature) {
                for (var i = 0; i < feature.tab.length; ++i) {
                    if (this.enabledGSUB[feature.tab[i]] > 1) {
                        --this.enabledGSUB[feature.tab[i]];
                    }
                    else {
                        delete this.enabledGSUB[feature.tab[i]];
                    }
                }
            }
        }
    };
    Font.prototype.codeToGlyph = function (code) {
        var g = Typr_js_1.Typr.U.codeToGlyph(this, code);
        if (this.GSUB) {
            var gls = [g];
            for (var n in this.enabledGSUB) {
                var l = this.GSUB.lookupList[n];
                Typr_js_1.Typr.U._applySubs(gls, 0, l, this.GSUB.lookupList);
            }
            if (gls.length === 1)
                return gls[0];
        }
        return g;
    };
    ;
    return Font;
}());
exports.Font = Font;
//# sourceMappingURL=index.js.map
});
___scope___.file("dist/Typr.js", function(exports, require, module, __filename, __dirname){

"use strict";
var Typr = {};
Typr.parse = function (buff) {
    var bin = Typr._bin;
    var data = new Uint8Array(buff);
    var tag = bin.readASCII(data, 0, 4);
    if (tag == "ttcf") {
        var offset = 4;
        var majV = bin.readUshort(data, offset);
        offset += 2;
        var minV = bin.readUshort(data, offset);
        offset += 2;
        var numF = bin.readUint(data, offset);
        offset += 4;
        var fnts = [];
        for (var i = 0; i < numF; i++) {
            var foff = bin.readUint(data, offset);
            offset += 4;
            fnts.push(Typr._readFont(data, foff));
        }
        return fnts;
    }
    else
        return [Typr._readFont(data, 0)];
};
Typr._readFont = function (data, offset) {
    var bin = Typr._bin;
    var ooff = offset;
    var sfnt_version = bin.readFixed(data, offset);
    offset += 4;
    var numTables = bin.readUshort(data, offset);
    offset += 2;
    var searchRange = bin.readUshort(data, offset);
    offset += 2;
    var entrySelector = bin.readUshort(data, offset);
    offset += 2;
    var rangeShift = bin.readUshort(data, offset);
    offset += 2;
    var tags = [
        "cmap",
        "head",
        "hhea",
        "maxp",
        "hmtx",
        "name",
        "OS/2",
        "post",
        //"cvt",
        //"fpgm",
        "loca",
        "glyf",
        "kern",
        //"prep"
        //"gasp"
        "CFF ",
        "GPOS",
        "GSUB",
        "SVG "
        //"VORG",
    ];
    var obj = { _data: data, _offset: ooff };
    //console.warn(sfnt_version, numTables, searchRange, entrySelector, rangeShift);
    var tabs = {};
    for (var i = 0; i < numTables; i++) {
        var tag = bin.readASCII(data, offset, 4);
        offset += 4;
        var checkSum = bin.readUint(data, offset);
        offset += 4;
        var toffset = bin.readUint(data, offset);
        offset += 4;
        var length = bin.readUint(data, offset);
        offset += 4;
        tabs[tag] = { offset: toffset, length: length };
        //if(tags.indexOf(tag)==-1) console.warn("unknown tag", tag, length);
    }
    for (var i = 0; i < tags.length; i++) {
        var t = tags[i];
        //console.warn(t);
        //if(tabs[t]) console.warn(t, tabs[t].offset, tabs[t].length);
        if (tabs[t])
            obj[t.trim()] = Typr[t.trim()].parse(data, tabs[t].offset, tabs[t].length, obj);
    }
    return obj;
};
Typr._tabOffset = function (data, tab, foff) {
    var bin = Typr._bin;
    var numTables = bin.readUshort(data, foff + 4);
    var offset = foff + 12;
    for (var i = 0; i < numTables; i++) {
        var tag = bin.readASCII(data, offset, 4);
        offset += 4;
        var checkSum = bin.readUint(data, offset);
        offset += 4;
        var toffset = bin.readUint(data, offset);
        offset += 4;
        var length = bin.readUint(data, offset);
        offset += 4;
        if (tag == tab)
            return toffset;
    }
    return 0;
};
Typr._bin = {
    readFixed: function (data, o) {
        return ((data[o] << 8) | data[o + 1]) + (((data[o + 2] << 8) | data[o + 3]) / (256 * 256 + 4));
    },
    readF2dot14: function (data, o) {
        var num = Typr._bin.readShort(data, o);
        return num / 16384;
    },
    readInt: function (buff, p) {
        return Typr._bin._view(buff).getInt32(p);
    },
    readInt8: function (buff, p) {
        return Typr._bin._view(buff).getInt8(p);
    },
    readShort: function (buff, p) {
        return Typr._bin._view(buff).getInt16(p);
    },
    readUshort: function (buff, p) {
        return Typr._bin._view(buff).getUint16(p);
    },
    readUshorts: function (buff, p, len) {
        var arr = [];
        for (var i = 0; i < len; i++)
            arr.push(Typr._bin.readUshort(buff, p + i * 2));
        return arr;
    },
    readUint: function (buff, p) {
        return Typr._bin._view(buff).getUint32(p);
    },
    readUint64: function (buff, p) {
        //if(p>=buff.length) throw "error";
        return (Typr._bin.readUint(buff, p) * (0xffffffff + 1)) + Typr._bin.readUint(buff, p + 4);
    },
    readASCII: function (buff, p, l) {
        //if(p>=buff.length) throw "error";
        var s = "";
        for (var i = 0; i < l; i++)
            s += String.fromCharCode(buff[p + i]);
        return s;
    },
    readUnicode: function (buff, p, l) {
        //if(p>=buff.length) throw "error";
        var s = "";
        for (var i = 0; i < l; i++) {
            var c = (buff[p++] << 8) | buff[p++];
            s += String.fromCharCode(c);
        }
        return s;
    },
    _tdec: typeof window !== 'undefined' && window["TextDecoder"] ? new window["TextDecoder"]() : null,
    readUTF8: function (buff, p, l) {
        var tdec = Typr._bin._tdec;
        if (tdec && p == 0 && l == buff.length)
            return tdec["decode"](buff);
        return Typr._bin.readASCII(buff, p, l);
    },
    readBytes: function (buff, p, l) {
        //if(p>=buff.length) throw "error";
        var arr = [];
        for (var i = 0; i < l; i++)
            arr.push(buff[p + i]);
        return arr;
    },
    readASCIIArray: function (buff, p, l) {
        //if(p>=buff.length) throw "error";
        var s = [];
        for (var i = 0; i < l; i++)
            s.push(String.fromCharCode(buff[p + i]));
        return s;
    },
    _view: function (buff) {
        return buff._dataView || (buff._dataView = buff.buffer ?
            new DataView(buff.buffer, buff.byteOffset, buff.byteLength) :
            new DataView(new Uint8Array(buff).buffer));
    }
};
// OpenType Layout Common Table Formats
Typr._lctf = {};
Typr._lctf.parse = function (data, offset, length, font, subt) {
    var bin = Typr._bin;
    var obj = {};
    var offset0 = offset;
    var tableVersion = bin.readFixed(data, offset);
    offset += 4;
    var offScriptList = bin.readUshort(data, offset);
    offset += 2;
    var offFeatureList = bin.readUshort(data, offset);
    offset += 2;
    var offLookupList = bin.readUshort(data, offset);
    offset += 2;
    obj.scriptList = Typr._lctf.readScriptList(data, offset0 + offScriptList);
    obj.featureList = Typr._lctf.readFeatureList(data, offset0 + offFeatureList);
    obj.lookupList = Typr._lctf.readLookupList(data, offset0 + offLookupList, subt);
    return obj;
};
Typr._lctf.readLookupList = function (data, offset, subt) {
    var bin = Typr._bin;
    var offset0 = offset;
    var obj = [];
    var count = bin.readUshort(data, offset);
    offset += 2;
    for (var i = 0; i < count; i++) {
        var noff = bin.readUshort(data, offset);
        offset += 2;
        var lut = Typr._lctf.readLookupTable(data, offset0 + noff, subt);
        obj.push(lut);
    }
    return obj;
};
Typr._lctf.readLookupTable = function (data, offset, subt) {
    //console.warn("Parsing lookup table", offset);
    var bin = Typr._bin;
    var offset0 = offset;
    var obj = { tabs: [] };
    obj.ltype = bin.readUshort(data, offset);
    offset += 2;
    obj.flag = bin.readUshort(data, offset);
    offset += 2;
    var cnt = bin.readUshort(data, offset);
    offset += 2;
    var ltype = obj.ltype; // extension substitution can change this value
    for (var i = 0; i < cnt; i++) {
        var noff = bin.readUshort(data, offset);
        offset += 2;
        var tab = subt(data, ltype, offset0 + noff, obj);
        //console.warn(obj.type, tab);
        obj.tabs.push(tab);
    }
    return obj;
};
Typr._lctf.numOfOnes = function (n) {
    var num = 0;
    for (var i = 0; i < 32; i++)
        if (((n >>> i) & 1) != 0)
            num++;
    return num;
};
Typr._lctf.readClassDef = function (data, offset) {
    var bin = Typr._bin;
    var obj = [];
    var format = bin.readUshort(data, offset);
    offset += 2;
    if (format == 1) {
        var startGlyph = bin.readUshort(data, offset);
        offset += 2;
        var glyphCount = bin.readUshort(data, offset);
        offset += 2;
        for (var i = 0; i < glyphCount; i++) {
            obj.push(startGlyph + i);
            obj.push(startGlyph + i);
            obj.push(bin.readUshort(data, offset));
            offset += 2;
        }
    }
    if (format == 2) {
        var count = bin.readUshort(data, offset);
        offset += 2;
        for (var i = 0; i < count; i++) {
            obj.push(bin.readUshort(data, offset));
            offset += 2;
            obj.push(bin.readUshort(data, offset));
            offset += 2;
            obj.push(bin.readUshort(data, offset));
            offset += 2;
        }
    }
    return obj;
};
Typr._lctf.getInterval = function (tab, val) {
    for (var i = 0; i < tab.length; i += 3) {
        var start = tab[i], end = tab[i + 1], index = tab[i + 2];
        if (start <= val && val <= end)
            return i;
    }
    return -1;
};
Typr._lctf.readCoverage = function (data, offset) {
    var bin = Typr._bin;
    var cvg = {};
    cvg.fmt = bin.readUshort(data, offset);
    offset += 2;
    var count = bin.readUshort(data, offset);
    offset += 2;
    //console.warn("parsing coverage", offset-4, format, count);
    if (cvg.fmt == 1)
        cvg.tab = bin.readUshorts(data, offset, count);
    if (cvg.fmt == 2)
        cvg.tab = bin.readUshorts(data, offset, count * 3);
    return cvg;
};
Typr._lctf.coverageIndex = function (cvg, val) {
    var tab = cvg.tab;
    if (cvg.fmt == 1)
        return tab.indexOf(val);
    if (cvg.fmt == 2) {
        var ind = Typr._lctf.getInterval(tab, val);
        if (ind != -1)
            return tab[ind + 2] + (val - tab[ind]);
    }
    return -1;
};
Typr._lctf.readFeatureList = function (data, offset) {
    var bin = Typr._bin;
    var offset0 = offset;
    var obj = [];
    var count = bin.readUshort(data, offset);
    offset += 2;
    for (var i = 0; i < count; i++) {
        var tag = bin.readASCII(data, offset, 4);
        offset += 4;
        var noff = bin.readUshort(data, offset);
        offset += 2;
        var feat = Typr._lctf.readFeatureTable(data, offset0 + noff);
        feat.tag = tag.trim();
        obj.push(feat);
    }
    return obj;
};
Typr._lctf.readFeatureTable = function (data, offset) {
    var bin = Typr._bin;
    var offset0 = offset;
    var feat = {};
    var featureParams = bin.readUshort(data, offset);
    offset += 2;
    if (featureParams > 0) {
        feat.featureParams = offset0 + featureParams;
    }
    var lookupCount = bin.readUshort(data, offset);
    offset += 2;
    feat.tab = [];
    for (var i = 0; i < lookupCount; i++)
        feat.tab.push(bin.readUshort(data, offset + 2 * i));
    return feat;
};
Typr._lctf.readScriptList = function (data, offset) {
    var bin = Typr._bin;
    var offset0 = offset;
    var obj = {};
    var count = bin.readUshort(data, offset);
    offset += 2;
    for (var i = 0; i < count; i++) {
        var tag = bin.readASCII(data, offset, 4);
        offset += 4;
        var noff = bin.readUshort(data, offset);
        offset += 2;
        obj[tag.trim()] = Typr._lctf.readScriptTable(data, offset0 + noff);
    }
    return obj;
};
Typr._lctf.readScriptTable = function (data, offset) {
    var bin = Typr._bin;
    var offset0 = offset;
    var obj = {};
    var defLangSysOff = bin.readUshort(data, offset);
    offset += 2;
    obj["default"] = Typr._lctf.readLangSysTable(data, offset0 + defLangSysOff);
    var langSysCount = bin.readUshort(data, offset);
    offset += 2;
    for (var i = 0; i < langSysCount; i++) {
        var tag = bin.readASCII(data, offset, 4);
        offset += 4;
        var langSysOff = bin.readUshort(data, offset);
        offset += 2;
        obj[tag.trim()] = Typr._lctf.readLangSysTable(data, offset0 + langSysOff);
    }
    return obj;
};
Typr._lctf.readLangSysTable = function (data, offset) {
    var bin = Typr._bin;
    var obj = {};
    var lookupOrder = bin.readUshort(data, offset);
    offset += 2;
    //if(lookupOrder!=0)  throw "lookupOrder not 0";
    obj.reqFeature = bin.readUshort(data, offset);
    offset += 2;
    //if(obj.reqFeature != 0xffff) throw "reqFeatureIndex != 0xffff";
    //console.warn(lookupOrder, obj.reqFeature);
    var featureCount = bin.readUshort(data, offset);
    offset += 2;
    obj.features = bin.readUshorts(data, offset, featureCount);
    return obj;
};
Typr.CFF = {};
Typr.CFF.parse = function (data, offset, length) {
    var bin = Typr._bin;
    data = new Uint8Array(data.buffer, offset, length);
    offset = 0;
    // Header
    var major = data[offset];
    offset++;
    var minor = data[offset];
    offset++;
    var hdrSize = data[offset];
    offset++;
    var offsize = data[offset];
    offset++;
    //console.warn(major, minor, hdrSize, offsize);
    // Name INDEX
    var ninds = [];
    offset = Typr.CFF.readIndex(data, offset, ninds);
    var names = [];
    for (var i = 0; i < ninds.length - 1; i++)
        names.push(bin.readASCII(data, offset + ninds[i], ninds[i + 1] - ninds[i]));
    offset += ninds[ninds.length - 1];
    // Top DICT INDEX
    var tdinds = [];
    offset = Typr.CFF.readIndex(data, offset, tdinds); //console.warn(tdinds);
    // Top DICT Data
    var topDicts = [];
    for (var i = 0; i < tdinds.length - 1; i++)
        topDicts.push(Typr.CFF.readDict(data, offset + tdinds[i], offset + tdinds[i + 1]));
    offset += tdinds[tdinds.length - 1];
    var topdict = topDicts[0];
    //console.warn(topdict);
    // String INDEX
    var sinds = [];
    offset = Typr.CFF.readIndex(data, offset, sinds);
    // String Data
    var strings = [];
    for (var i = 0; i < sinds.length - 1; i++)
        strings.push(bin.readASCII(data, offset + sinds[i], sinds[i + 1] - sinds[i]));
    offset += sinds[sinds.length - 1];
    // Global Subr INDEX  (subroutines)		
    Typr.CFF.readSubrs(data, offset, topdict);
    // charstrings
    if (topdict.CharStrings) {
        offset = topdict.CharStrings;
        var sinds = [];
        offset = Typr.CFF.readIndex(data, offset, sinds);
        var cstr = [];
        for (var i = 0; i < sinds.length - 1; i++)
            cstr.push(bin.readBytes(data, offset + sinds[i], sinds[i + 1] - sinds[i]));
        //offset += sinds[sinds.length-1];
        topdict.CharStrings = cstr;
        //console.warn(topdict.CharStrings);
    }
    // CID font
    if (topdict.ROS) {
        offset = topdict.FDArray;
        var fdind = [];
        offset = Typr.CFF.readIndex(data, offset, fdind);
        topdict.FDArray = [];
        for (var i = 0; i < fdind.length - 1; i++) {
            var dict = Typr.CFF.readDict(data, offset + fdind[i], offset + fdind[i + 1]);
            Typr.CFF._readFDict(data, dict, strings);
            topdict.FDArray.push(dict);
        }
        offset += fdind[fdind.length - 1];
        offset = topdict.FDSelect;
        topdict.FDSelect = [];
        var fmt = data[offset];
        offset++;
        if (fmt == 3) {
            var rns = bin.readUshort(data, offset);
            offset += 2;
            for (var i = 0; i < rns + 1; i++) {
                topdict.FDSelect.push(bin.readUshort(data, offset), data[offset + 2]);
                offset += 3;
            }
        }
        else
            throw fmt;
    }
    // Encoding
    if (topdict.Encoding)
        topdict.Encoding = Typr.CFF.readEncoding(data, topdict.Encoding, topdict.CharStrings.length);
    // charset
    if (topdict.charset)
        topdict.charset = Typr.CFF.readCharset(data, topdict.charset, topdict.CharStrings.length);
    Typr.CFF._readFDict(data, topdict, strings);
    return topdict;
};
Typr.CFF._readFDict = function (data, dict, ss) {
    var offset;
    if (dict.Private) {
        offset = dict.Private[1];
        dict.Private = Typr.CFF.readDict(data, offset, offset + dict.Private[0]);
        if (dict.Private.Subrs)
            Typr.CFF.readSubrs(data, offset + dict.Private.Subrs, dict.Private);
    }
    for (var p in dict)
        if (["FamilyName", "FontName", "FullName", "Notice", "version", "Copyright"].indexOf(p) != -1)
            dict[p] = ss[dict[p] - 426 + 35];
};
Typr.CFF.readSubrs = function (data, offset, obj) {
    var bin = Typr._bin;
    var gsubinds = [];
    offset = Typr.CFF.readIndex(data, offset, gsubinds);
    var bias, nSubrs = gsubinds.length;
    if (false)
        bias = 0;
    else if (nSubrs < 1240)
        bias = 107;
    else if (nSubrs < 33900)
        bias = 1131;
    else
        bias = 32768;
    obj.Bias = bias;
    obj.Subrs = [];
    for (var i = 0; i < gsubinds.length - 1; i++)
        obj.Subrs.push(bin.readBytes(data, offset + gsubinds[i], gsubinds[i + 1] - gsubinds[i]));
    //offset += gsubinds[gsubinds.length-1];
};
Typr.CFF.tableSE = [
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 3, 4, 5, 6, 7, 8,
    9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56,
    57, 58, 59, 60, 61, 62, 63, 64,
    65, 66, 67, 68, 69, 70, 71, 72,
    73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 85, 86, 87, 88,
    89, 90, 91, 92, 93, 94, 95, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 96, 97, 98, 99, 100, 101, 102,
    103, 104, 105, 106, 107, 108, 109, 110,
    0, 111, 112, 113, 114, 0, 115, 116,
    117, 118, 119, 120, 121, 122, 0, 123,
    0, 124, 125, 126, 127, 128, 129, 130,
    131, 0, 132, 133, 0, 134, 135, 136,
    137, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 138, 0, 139, 0, 0, 0, 0,
    140, 141, 142, 143, 0, 0, 0, 0,
    0, 144, 0, 0, 0, 145, 0, 0,
    146, 147, 148, 149, 0, 0, 0, 0
];
Typr.CFF.glyphByUnicode = function (cff, code) {
    for (var i = 0; i < cff.charset.length; i++)
        if (cff.charset[i] == code)
            return i;
    return -1;
};
Typr.CFF.glyphBySE = function (cff, charcode) {
    if (charcode < 0 || charcode > 255)
        return -1;
    return Typr.CFF.glyphByUnicode(cff, Typr.CFF.tableSE[charcode]);
};
Typr.CFF.readEncoding = function (data, offset, num) {
    var bin = Typr._bin;
    var array = ['.notdef'];
    var format = data[offset];
    offset++;
    //console.warn("Encoding");
    //console.warn(format);
    if (format == 0) {
        var nCodes = data[offset];
        offset++;
        for (var i = 0; i < nCodes; i++)
            array.push(data[offset + i]);
    }
    /*
    else if(format==1 || format==2)
    {
        while(charset.length<num)
        {
            var first = bin.readUshort(data, offset);  offset+=2;
            var nLeft=0;
            if(format==1) {  nLeft = data[offset];  offset++;  }
            else          {  nLeft = bin.readUshort(data, offset);  offset+=2;  }
            for(var i=0; i<=nLeft; i++)  {  charset.push(first);  first++;  }
        }
    }
    */
    else
        throw "error: unknown encoding format: " + format;
    return array;
};
Typr.CFF.readCharset = function (data, offset, num) {
    var bin = Typr._bin;
    var charset = ['.notdef'];
    var format = data[offset];
    offset++;
    if (format == 0) {
        for (var i = 0; i < num; i++) {
            var first = bin.readUshort(data, offset);
            offset += 2;
            charset.push(first);
        }
    }
    else if (format == 1 || format == 2) {
        while (charset.length < num) {
            var first = bin.readUshort(data, offset);
            offset += 2;
            var nLeft = 0;
            if (format == 1) {
                nLeft = data[offset];
                offset++;
            }
            else {
                nLeft = bin.readUshort(data, offset);
                offset += 2;
            }
            for (var i = 0; i <= nLeft; i++) {
                charset.push(first);
                first++;
            }
        }
    }
    else
        throw "error: format: " + format;
    return charset;
};
Typr.CFF.readIndex = function (data, offset, inds) {
    var bin = Typr._bin;
    var count = bin.readUshort(data, offset) + 1;
    offset += 2;
    var offsize = data[offset];
    offset++;
    if (offsize == 1)
        for (var i = 0; i < count; i++)
            inds.push(data[offset + i]);
    else if (offsize == 2)
        for (var i = 0; i < count; i++)
            inds.push(bin.readUshort(data, offset + i * 2));
    else if (offsize == 3)
        for (var i = 0; i < count; i++)
            inds.push(bin.readUint(data, offset + i * 3 - 1) & 0x00ffffff);
    else if (count != 1)
        throw "unsupported offset size: " + offsize + ", count: " + count;
    offset += count * offsize;
    return offset - 1;
};
Typr.CFF.getCharString = function (data, offset, o) {
    var bin = Typr._bin;
    var b0 = data[offset], b1 = data[offset + 1], b2 = data[offset + 2], b3 = data[offset + 3], b4 = data[offset + 4];
    var vs = 1;
    var op = null, val = null;
    // operand
    if (b0 <= 20) {
        op = b0;
        vs = 1;
    }
    if (b0 == 12) {
        op = b0 * 100 + b1;
        vs = 2;
    }
    //if(b0==19 || b0==20) { op = b0/*+" "+b1*/;  vs=2; }
    if (21 <= b0 && b0 <= 27) {
        op = b0;
        vs = 1;
    }
    if (b0 == 28) {
        val = bin.readShort(data, offset + 1);
        vs = 3;
    }
    if (29 <= b0 && b0 <= 31) {
        op = b0;
        vs = 1;
    }
    if (32 <= b0 && b0 <= 246) {
        val = b0 - 139;
        vs = 1;
    }
    if (247 <= b0 && b0 <= 250) {
        val = (b0 - 247) * 256 + b1 + 108;
        vs = 2;
    }
    if (251 <= b0 && b0 <= 254) {
        val = -(b0 - 251) * 256 - b1 - 108;
        vs = 2;
    }
    if (b0 == 255) {
        val = bin.readInt(data, offset + 1) / 0xffff;
        vs = 5;
    }
    o.val = val != null ? val : "o" + op;
    o.size = vs;
};
Typr.CFF.readCharString = function (data, offset, length) {
    var end = offset + length;
    var bin = Typr._bin;
    var arr = [];
    while (offset < end) {
        var b0 = data[offset], b1 = data[offset + 1], b2 = data[offset + 2], b3 = data[offset + 3], b4 = data[offset + 4];
        var vs = 1;
        var op = null, val = null;
        // operand
        if (b0 <= 20) {
            op = b0;
            vs = 1;
        }
        if (b0 == 12) {
            op = b0 * 100 + b1;
            vs = 2;
        }
        if (b0 == 19 || b0 == 20) {
            op = b0 /*+" "+b1*/;
            vs = 2;
        }
        if (21 <= b0 && b0 <= 27) {
            op = b0;
            vs = 1;
        }
        if (b0 == 28) {
            val = bin.readShort(data, offset + 1);
            vs = 3;
        }
        if (29 <= b0 && b0 <= 31) {
            op = b0;
            vs = 1;
        }
        if (32 <= b0 && b0 <= 246) {
            val = b0 - 139;
            vs = 1;
        }
        if (247 <= b0 && b0 <= 250) {
            val = (b0 - 247) * 256 + b1 + 108;
            vs = 2;
        }
        if (251 <= b0 && b0 <= 254) {
            val = -(b0 - 251) * 256 - b1 - 108;
            vs = 2;
        }
        if (b0 == 255) {
            val = bin.readInt(data, offset + 1) / 0xffff;
            vs = 5;
        }
        arr.push(val != null ? val : "o" + op);
        offset += vs;
        //var cv = arr[arr.length-1];
        //if(cv==undefined) throw "error";
        //console.warn()
    }
    return arr;
};
Typr.CFF.readDict = function (data, offset, end) {
    var bin = Typr._bin;
    //var dict = [];
    var dict = {};
    var carr = [];
    while (offset < end) {
        var b0 = data[offset], b1 = data[offset + 1], b2 = data[offset + 2], b3 = data[offset + 3], b4 = data[offset + 4];
        var vs = 1;
        var key = null, val = null;
        // operand
        if (b0 == 28) {
            val = bin.readShort(data, offset + 1);
            vs = 3;
        }
        if (b0 == 29) {
            val = bin.readInt(data, offset + 1);
            vs = 5;
        }
        if (32 <= b0 && b0 <= 246) {
            val = b0 - 139;
            vs = 1;
        }
        if (247 <= b0 && b0 <= 250) {
            val = (b0 - 247) * 256 + b1 + 108;
            vs = 2;
        }
        if (251 <= b0 && b0 <= 254) {
            val = -(b0 - 251) * 256 - b1 - 108;
            vs = 2;
        }
        if (b0 == 255) {
            val = bin.readInt(data, offset + 1) / 0xffff;
            vs = 5;
            throw "unknown number";
        }
        if (b0 == 30) {
            var nibs = [];
            vs = 1;
            while (true) {
                var b = data[offset + vs];
                vs++;
                var nib0 = b >> 4, nib1 = b & 0xf;
                if (nib0 != 0xf)
                    nibs.push(nib0);
                if (nib1 != 0xf)
                    nibs.push(nib1);
                if (nib1 == 0xf)
                    break;
            }
            var s = "";
            var chars = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, ".", "e", "e-", "reserved", "-", "endOfNumber"];
            for (var i = 0; i < nibs.length; i++)
                s += chars[nibs[i]];
            //console.warn(nibs);
            val = parseFloat(s);
        }
        if (b0 <= 21) // operator
         {
            var keys = ["version", "Notice", "FullName", "FamilyName", "Weight", "FontBBox", "BlueValues", "OtherBlues", "FamilyBlues", "FamilyOtherBlues",
                "StdHW", "StdVW", "escape", "UniqueID", "XUID", "charset", "Encoding", "CharStrings", "Private", "Subrs",
                "defaultWidthX", "nominalWidthX"];
            key = keys[b0];
            vs = 1;
            if (b0 == 12) {
                var keys = ["Copyright", "isFixedPitch", "ItalicAngle", "UnderlinePosition", "UnderlineThickness", "PaintType", "CharstringType", "FontMatrix", "StrokeWidth", "BlueScale",
                    "BlueShift", "BlueFuzz", "StemSnapH", "StemSnapV", "ForceBold", 0, 0, "LanguageGroup", "ExpansionFactor", "initialRandomSeed",
                    "SyntheticBase", "PostScript", "BaseFontName", "BaseFontBlend", 0, 0, 0, 0, 0, 0,
                    "ROS", "CIDFontVersion", "CIDFontRevision", "CIDFontType", "CIDCount", "UIDBase", "FDArray", "FDSelect", "FontName"];
                key = keys[b1];
                vs = 2;
            }
        }
        if (key != null) {
            dict[key] = carr.length == 1 ? carr[0] : carr;
            carr = [];
        }
        else
            carr.push(val);
        offset += vs;
    }
    return dict;
};
Typr.cmap = {};
Typr.cmap.parse = function (data, offset, length) {
    data = new Uint8Array(data.buffer, offset, length);
    offset = 0;
    var offset0 = offset;
    var bin = Typr._bin;
    var obj = {};
    var version = bin.readUshort(data, offset);
    offset += 2;
    var numTables = bin.readUshort(data, offset);
    offset += 2;
    //console.warn(version, numTables);
    var offs = [];
    obj.tables = [];
    for (var i = 0; i < numTables; i++) {
        var platformID = bin.readUshort(data, offset);
        offset += 2;
        var encodingID = bin.readUshort(data, offset);
        offset += 2;
        var noffset = bin.readUint(data, offset);
        offset += 4;
        var id = "p" + platformID + "e" + encodingID;
        //console.warn("cmap subtable", platformID, encodingID, noffset);
        var tind = offs.indexOf(noffset);
        if (tind == -1) {
            tind = obj.tables.length;
            var subt;
            offs.push(noffset);
            var format = bin.readUshort(data, noffset);
            if (format == 0)
                subt = Typr.cmap.parse0(data, noffset);
            else if (format == 4)
                subt = Typr.cmap.parse4(data, noffset);
            else if (format == 6)
                subt = Typr.cmap.parse6(data, noffset);
            else if (format == 12)
                subt = Typr.cmap.parse12(data, noffset);
            else
                console.warn("unknown format: " + format, platformID, encodingID, noffset);
            obj.tables.push(subt);
        }
        if (obj[id] != null)
            throw "multiple tables for one platform+encoding";
        obj[id] = tind;
    }
    return obj;
};
Typr.cmap.parse0 = function (data, offset) {
    var bin = Typr._bin;
    var obj = {};
    obj.format = bin.readUshort(data, offset);
    offset += 2;
    var len = bin.readUshort(data, offset);
    offset += 2;
    var lang = bin.readUshort(data, offset);
    offset += 2;
    obj.map = [];
    for (var i = 0; i < len - 6; i++)
        obj.map.push(data[offset + i]);
    return obj;
};
Typr.cmap.parse4 = function (data, offset) {
    var bin = Typr._bin;
    var offset0 = offset;
    var obj = {};
    obj.format = bin.readUshort(data, offset);
    offset += 2;
    var length = bin.readUshort(data, offset);
    offset += 2;
    var language = bin.readUshort(data, offset);
    offset += 2;
    var segCountX2 = bin.readUshort(data, offset);
    offset += 2;
    var segCount = segCountX2 / 2;
    obj.searchRange = bin.readUshort(data, offset);
    offset += 2;
    obj.entrySelector = bin.readUshort(data, offset);
    offset += 2;
    obj.rangeShift = bin.readUshort(data, offset);
    offset += 2;
    obj.endCount = bin.readUshorts(data, offset, segCount);
    offset += segCount * 2;
    offset += 2;
    obj.startCount = bin.readUshorts(data, offset, segCount);
    offset += segCount * 2;
    obj.idDelta = [];
    for (var i = 0; i < segCount; i++) {
        obj.idDelta.push(bin.readShort(data, offset));
        offset += 2;
    }
    obj.idRangeOffset = bin.readUshorts(data, offset, segCount);
    offset += segCount * 2;
    obj.glyphIdArray = [];
    while (offset < offset0 + length) {
        obj.glyphIdArray.push(bin.readUshort(data, offset));
        offset += 2;
    }
    return obj;
};
Typr.cmap.parse6 = function (data, offset) {
    var bin = Typr._bin;
    var offset0 = offset;
    var obj = {};
    obj.format = bin.readUshort(data, offset);
    offset += 2;
    var length = bin.readUshort(data, offset);
    offset += 2;
    var language = bin.readUshort(data, offset);
    offset += 2;
    obj.firstCode = bin.readUshort(data, offset);
    offset += 2;
    var entryCount = bin.readUshort(data, offset);
    offset += 2;
    obj.glyphIdArray = [];
    for (var i = 0; i < entryCount; i++) {
        obj.glyphIdArray.push(bin.readUshort(data, offset));
        offset += 2;
    }
    return obj;
};
Typr.cmap.parse12 = function (data, offset) {
    var bin = Typr._bin;
    var offset0 = offset;
    var obj = {};
    obj.format = bin.readUshort(data, offset);
    offset += 2;
    offset += 2;
    var length = bin.readUint(data, offset);
    offset += 4;
    var lang = bin.readUint(data, offset);
    offset += 4;
    var nGroups = bin.readUint(data, offset);
    offset += 4;
    obj.groups = [];
    for (var i = 0; i < nGroups; i++) {
        var off = offset + i * 12;
        var startCharCode = bin.readUint(data, off + 0);
        var endCharCode = bin.readUint(data, off + 4);
        var startGlyphID = bin.readUint(data, off + 8);
        obj.groups.push([startCharCode, endCharCode, startGlyphID]);
    }
    return obj;
};
Typr.glyf = {};
Typr.glyf.parse = function (data, offset, length, font) {
    var obj = [];
    for (var g = 0; g < font.maxp.numGlyphs; g++)
        obj.push(null);
    return obj;
};
Typr.glyf._parseGlyf = function (font, g) {
    var bin = Typr._bin;
    var data = font._data;
    var offset = Typr._tabOffset(data, "glyf", font._offset) + font.loca[g];
    if (font.loca[g] == font.loca[g + 1])
        return null;
    var gl = {};
    gl.noc = bin.readShort(data, offset);
    offset += 2; // number of contours
    gl.xMin = bin.readShort(data, offset);
    offset += 2;
    gl.yMin = bin.readShort(data, offset);
    offset += 2;
    gl.xMax = bin.readShort(data, offset);
    offset += 2;
    gl.yMax = bin.readShort(data, offset);
    offset += 2;
    if (gl.xMin >= gl.xMax || gl.yMin >= gl.yMax)
        return null;
    if (gl.noc > 0) {
        gl.endPts = [];
        for (var i = 0; i < gl.noc; i++) {
            gl.endPts.push(bin.readUshort(data, offset));
            offset += 2;
        }
        var instructionLength = bin.readUshort(data, offset);
        offset += 2;
        if ((data.length - offset) < instructionLength)
            return null;
        gl.instructions = bin.readBytes(data, offset, instructionLength);
        offset += instructionLength;
        var crdnum = gl.endPts[gl.noc - 1] + 1;
        gl.flags = [];
        for (var i = 0; i < crdnum; i++) {
            var flag = data[offset];
            offset++;
            gl.flags.push(flag);
            if ((flag & 8) != 0) {
                var rep = data[offset];
                offset++;
                for (var j = 0; j < rep; j++) {
                    gl.flags.push(flag);
                    i++;
                }
            }
        }
        gl.xs = [];
        for (var i = 0; i < crdnum; i++) {
            var i8 = ((gl.flags[i] & 2) != 0), same = ((gl.flags[i] & 16) != 0);
            if (i8) {
                gl.xs.push(same ? data[offset] : -data[offset]);
                offset++;
            }
            else {
                if (same)
                    gl.xs.push(0);
                else {
                    gl.xs.push(bin.readShort(data, offset));
                    offset += 2;
                }
            }
        }
        gl.ys = [];
        for (var i = 0; i < crdnum; i++) {
            var i8 = ((gl.flags[i] & 4) != 0), same = ((gl.flags[i] & 32) != 0);
            if (i8) {
                gl.ys.push(same ? data[offset] : -data[offset]);
                offset++;
            }
            else {
                if (same)
                    gl.ys.push(0);
                else {
                    gl.ys.push(bin.readShort(data, offset));
                    offset += 2;
                }
            }
        }
        var x = 0, y = 0;
        for (var i = 0; i < crdnum; i++) {
            x += gl.xs[i];
            y += gl.ys[i];
            gl.xs[i] = x;
            gl.ys[i] = y;
        }
        //console.warn(endPtsOfContours, instructionLength, instructions, flags, xCoordinates, yCoordinates);
    }
    else {
        var ARG_1_AND_2_ARE_WORDS = 1 << 0;
        var ARGS_ARE_XY_VALUES = 1 << 1;
        var ROUND_XY_TO_GRID = 1 << 2;
        var WE_HAVE_A_SCALE = 1 << 3;
        var RESERVED = 1 << 4;
        var MORE_COMPONENTS = 1 << 5;
        var WE_HAVE_AN_X_AND_Y_SCALE = 1 << 6;
        var WE_HAVE_A_TWO_BY_TWO = 1 << 7;
        var WE_HAVE_INSTRUCTIONS = 1 << 8;
        var USE_MY_METRICS = 1 << 9;
        var OVERLAP_COMPOUND = 1 << 10;
        var SCALED_COMPONENT_OFFSET = 1 << 11;
        var UNSCALED_COMPONENT_OFFSET = 1 << 12;
        gl.parts = [];
        var flags;
        do {
            flags = bin.readUshort(data, offset);
            offset += 2;
            var part = { m: { a: 1, b: 0, c: 0, d: 1, tx: 0, ty: 0 }, p1: -1, p2: -1 };
            gl.parts.push(part);
            part.glyphIndex = bin.readUshort(data, offset);
            offset += 2;
            if (flags & ARG_1_AND_2_ARE_WORDS) {
                var arg1 = bin.readShort(data, offset);
                offset += 2;
                var arg2 = bin.readShort(data, offset);
                offset += 2;
            }
            else {
                var arg1 = bin.readInt8(data, offset);
                offset++;
                var arg2 = bin.readInt8(data, offset);
                offset++;
            }
            if (flags & ARGS_ARE_XY_VALUES) {
                part.m.tx = arg1;
                part.m.ty = arg2;
            }
            else {
                part.p1 = arg1;
                part.p2 = arg2;
            }
            //part.m.tx = arg1;  part.m.ty = arg2;
            //else { throw "params are not XY values"; }
            if (flags & WE_HAVE_A_SCALE) {
                part.m.a = part.m.d = bin.readF2dot14(data, offset);
                offset += 2;
            }
            else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) {
                part.m.a = bin.readF2dot14(data, offset);
                offset += 2;
                part.m.d = bin.readF2dot14(data, offset);
                offset += 2;
            }
            else if (flags & WE_HAVE_A_TWO_BY_TWO) {
                part.m.a = bin.readF2dot14(data, offset);
                offset += 2;
                part.m.b = bin.readF2dot14(data, offset);
                offset += 2;
                part.m.c = bin.readF2dot14(data, offset);
                offset += 2;
                part.m.d = bin.readF2dot14(data, offset);
                offset += 2;
            }
        } while (flags & MORE_COMPONENTS);
        if (flags & WE_HAVE_INSTRUCTIONS) {
            var numInstr = bin.readUshort(data, offset);
            offset += 2;
            gl.instr = [];
            for (var i = 0; i < numInstr; i++) {
                gl.instr.push(data[offset]);
                offset++;
            }
        }
    }
    return gl;
};
Typr.GPOS = {};
Typr.GPOS.parse = function (data, offset, length, font) { return Typr._lctf.parse(data, offset, length, font, Typr.GPOS.subt); };
Typr.GPOS.subt = function (data, ltype, offset, ltable) {
    var bin = Typr._bin, offset0 = offset, tab = {};
    tab.fmt = bin.readUshort(data, offset);
    offset += 2;
    //console.warn(ltype, tab.fmt);
    if (ltype == 1 || ltype == 2 || ltype == 3 || ltype == 7 || (ltype == 8 && tab.fmt <= 2)) {
        var covOff = bin.readUshort(data, offset);
        offset += 2;
        tab.coverage = Typr._lctf.readCoverage(data, covOff + offset0);
    }
    if (ltype == 1 && tab.fmt == 1) {
        var valFmt1 = bin.readUshort(data, offset);
        offset += 2;
        var ones1 = Typr._lctf.numOfOnes(valFmt1);
        if (valFmt1 != 0)
            tab.pos = Typr.GPOS.readValueRecord(data, offset, valFmt1);
    }
    else if (ltype == 2 && tab.fmt >= 1 && tab.fmt <= 2) {
        var valFmt1 = bin.readUshort(data, offset);
        offset += 2;
        var valFmt2 = bin.readUshort(data, offset);
        offset += 2;
        var ones1 = Typr._lctf.numOfOnes(valFmt1);
        var ones2 = Typr._lctf.numOfOnes(valFmt2);
        if (tab.fmt == 1) {
            tab.pairsets = [];
            var psc = bin.readUshort(data, offset);
            offset += 2; // PairSetCount
            for (var i = 0; i < psc; i++) {
                var psoff = offset0 + bin.readUshort(data, offset);
                offset += 2;
                var pvc = bin.readUshort(data, psoff);
                psoff += 2;
                var arr = [];
                for (var j = 0; j < pvc; j++) {
                    var gid2 = bin.readUshort(data, psoff);
                    psoff += 2;
                    var value1, value2;
                    if (valFmt1 != 0) {
                        value1 = Typr.GPOS.readValueRecord(data, psoff, valFmt1);
                        psoff += ones1 * 2;
                    }
                    if (valFmt2 != 0) {
                        value2 = Typr.GPOS.readValueRecord(data, psoff, valFmt2);
                        psoff += ones2 * 2;
                    }
                    //if(value1!=null) throw "e";
                    arr.push({ gid2: gid2, val1: value1, val2: value2 });
                }
                tab.pairsets.push(arr);
            }
        }
        if (tab.fmt == 2) {
            var classDef1 = bin.readUshort(data, offset);
            offset += 2;
            var classDef2 = bin.readUshort(data, offset);
            offset += 2;
            var class1Count = bin.readUshort(data, offset);
            offset += 2;
            var class2Count = bin.readUshort(data, offset);
            offset += 2;
            tab.classDef1 = Typr._lctf.readClassDef(data, offset0 + classDef1);
            tab.classDef2 = Typr._lctf.readClassDef(data, offset0 + classDef2);
            tab.matrix = [];
            for (var i = 0; i < class1Count; i++) {
                var row = [];
                for (var j = 0; j < class2Count; j++) {
                    var value1 = null, value2 = null;
                    if (valFmt1 != 0) {
                        value1 = Typr.GPOS.readValueRecord(data, offset, valFmt1);
                        offset += ones1 * 2;
                    }
                    if (valFmt2 != 0) {
                        value2 = Typr.GPOS.readValueRecord(data, offset, valFmt2);
                        offset += ones2 * 2;
                    }
                    row.push({ val1: value1, val2: value2 });
                }
                tab.matrix.push(row);
            }
        }
    }
    else if (ltype == 9 && tab.fmt == 1) {
        var extType = bin.readUshort(data, offset);
        offset += 2;
        var extOffset = bin.readUint(data, offset);
        offset += 4;
        if (ltable.ltype == 9) {
            ltable.ltype = extType;
        }
        else if (ltable.ltype != extType) {
            throw "invalid extension substitution"; // all subtables must be the same type
        }
        return Typr.GPOS.subt(data, ltable.ltype, offset0 + extOffset);
    }
    else
        console.warn("unsupported GPOS table LookupType", ltype, "format", tab.fmt);
    /*else if(ltype==4) {
        
    }*/
    return tab;
};
Typr.GPOS.readValueRecord = function (data, offset, valFmt) {
    var bin = Typr._bin;
    var arr = [];
    arr.push((valFmt & 1) ? bin.readShort(data, offset) : 0);
    offset += (valFmt & 1) ? 2 : 0; // X_PLACEMENT
    arr.push((valFmt & 2) ? bin.readShort(data, offset) : 0);
    offset += (valFmt & 2) ? 2 : 0; // Y_PLACEMENT
    arr.push((valFmt & 4) ? bin.readShort(data, offset) : 0);
    offset += (valFmt & 4) ? 2 : 0; // X_ADVANCE
    arr.push((valFmt & 8) ? bin.readShort(data, offset) : 0);
    offset += (valFmt & 8) ? 2 : 0; // Y_ADVANCE
    return arr;
};
Typr.GSUB = {};
Typr.GSUB.parse = function (data, offset, length, font) { return Typr._lctf.parse(data, offset, length, font, Typr.GSUB.subt); };
Typr.GSUB.subt = function (data, ltype, offset, ltable) {
    var bin = Typr._bin, offset0 = offset, tab = {};
    tab.fmt = bin.readUshort(data, offset);
    offset += 2;
    if (ltype != 1 && ltype != 4 && ltype != 5 && ltype != 6)
        return null;
    if (ltype == 1 || ltype == 4 || (ltype == 5 && tab.fmt <= 2) || (ltype == 6 && tab.fmt <= 2)) {
        var covOff = bin.readUshort(data, offset);
        offset += 2;
        tab.coverage = Typr._lctf.readCoverage(data, offset0 + covOff); // not always is coverage here
    }
    if (false) { }
    //  Single Substitution Subtable
    else if (ltype == 1 && tab.fmt >= 1 && tab.fmt <= 2) {
        if (tab.fmt == 1) {
            tab.delta = bin.readShort(data, offset);
            offset += 2;
        }
        else if (tab.fmt == 2) {
            var cnt = bin.readUshort(data, offset);
            offset += 2;
            tab.newg = bin.readUshorts(data, offset, cnt);
            offset += tab.newg.length * 2;
        }
    }
    //  Ligature Substitution Subtable
    else if (ltype == 4) {
        tab.vals = [];
        var cnt = bin.readUshort(data, offset);
        offset += 2;
        for (var i = 0; i < cnt; i++) {
            var loff = bin.readUshort(data, offset);
            offset += 2;
            tab.vals.push(Typr.GSUB.readLigatureSet(data, offset0 + loff));
        }
        //console.warn(tab.coverage);
        //console.warn(tab.vals);
    }
    //  Contextual Substitution Subtable
    else if (ltype == 5 && tab.fmt == 2) {
        if (tab.fmt == 2) {
            var cDefOffset = bin.readUshort(data, offset);
            offset += 2;
            tab.cDef = Typr._lctf.readClassDef(data, offset0 + cDefOffset);
            tab.scset = [];
            var subClassSetCount = bin.readUshort(data, offset);
            offset += 2;
            for (var i = 0; i < subClassSetCount; i++) {
                var scsOff = bin.readUshort(data, offset);
                offset += 2;
                tab.scset.push(scsOff == 0 ? null : Typr.GSUB.readSubClassSet(data, offset0 + scsOff));
            }
        }
        //else console.warn("unknown table format", tab.fmt);
    }
    //*
    else if (ltype == 6 && tab.fmt == 3) {
        /*
        if(tab.fmt==2) {
            var btDef = bin.readUshort(data, offset);  offset+=2;
            var inDef = bin.readUshort(data, offset);  offset+=2;
            var laDef = bin.readUshort(data, offset);  offset+=2;
            
            tab.btDef = Typr._lctf.readClassDef(data, offset0 + btDef);
            tab.inDef = Typr._lctf.readClassDef(data, offset0 + inDef);
            tab.laDef = Typr._lctf.readClassDef(data, offset0 + laDef);
            
            tab.scset = [];
            var cnt = bin.readUshort(data, offset);  offset+=2;
            for(var i=0; i<cnt; i++) {
                var loff = bin.readUshort(data, offset);  offset+=2;
                tab.scset.push(Typr.GSUB.readChainSubClassSet(data, offset0+loff));
            }
        }
        */
        if (tab.fmt == 3) {
            for (var i = 0; i < 3; i++) {
                var cnt = bin.readUshort(data, offset);
                offset += 2;
                var cvgs = [];
                for (var j = 0; j < cnt; j++)
                    cvgs.push(Typr._lctf.readCoverage(data, offset0 + bin.readUshort(data, offset + j * 2)));
                offset += cnt * 2;
                if (i == 0)
                    tab.backCvg = cvgs;
                if (i == 1)
                    tab.inptCvg = cvgs;
                if (i == 2)
                    tab.ahedCvg = cvgs;
            }
            var cnt = bin.readUshort(data, offset);
            offset += 2;
            tab.lookupRec = Typr.GSUB.readSubstLookupRecords(data, offset, cnt);
        }
        //console.warn(tab);
    } //*/
    else if (ltype == 7 && tab.fmt == 1) {
        var extType = bin.readUshort(data, offset);
        offset += 2;
        var extOffset = bin.readUint(data, offset);
        offset += 4;
        if (ltable.ltype == 9) {
            ltable.ltype = extType;
        }
        else if (ltable.ltype != extType) {
            throw "invalid extension substitution"; // all subtables must be the same type
        }
        return Typr.GSUB.subt(data, ltable.ltype, offset0 + extOffset);
    }
    else
        console.warn("unsupported GSUB table LookupType", ltype, "format", tab.fmt);
    //if(tab.coverage.indexOf(3)!=-1) console.warn(ltype, fmt, tab);
    return tab;
};
Typr.GSUB.readSubClassSet = function (data, offset) {
    var rUs = Typr._bin.readUshort, offset0 = offset, lset = [];
    var cnt = rUs(data, offset);
    offset += 2;
    for (var i = 0; i < cnt; i++) {
        var loff = rUs(data, offset);
        offset += 2;
        lset.push(Typr.GSUB.readSubClassRule(data, offset0 + loff));
    }
    return lset;
};
Typr.GSUB.readSubClassRule = function (data, offset) {
    var rUs = Typr._bin.readUshort, offset0 = offset, rule = {};
    var gcount = rUs(data, offset);
    offset += 2;
    var scount = rUs(data, offset);
    offset += 2;
    rule.input = [];
    for (var i = 0; i < gcount - 1; i++) {
        rule.input.push(rUs(data, offset));
        offset += 2;
    }
    rule.substLookupRecords = Typr.GSUB.readSubstLookupRecords(data, offset, scount);
    return rule;
};
Typr.GSUB.readSubstLookupRecords = function (data, offset, cnt) {
    var rUs = Typr._bin.readUshort;
    var out = [];
    for (var i = 0; i < cnt; i++) {
        out.push(rUs(data, offset), rUs(data, offset + 2));
        offset += 4;
    }
    return out;
};
Typr.GSUB.readChainSubClassSet = function (data, offset) {
    var bin = Typr._bin, offset0 = offset, lset = [];
    var cnt = bin.readUshort(data, offset);
    offset += 2;
    for (var i = 0; i < cnt; i++) {
        var loff = bin.readUshort(data, offset);
        offset += 2;
        lset.push(Typr.GSUB.readChainSubClassRule(data, offset0 + loff));
    }
    return lset;
};
Typr.GSUB.readChainSubClassRule = function (data, offset) {
    var bin = Typr._bin, offset0 = offset, rule = {};
    var pps = ["backtrack", "input", "lookahead"];
    for (var pi = 0; pi < pps.length; pi++) {
        var cnt = bin.readUshort(data, offset);
        offset += 2;
        if (pi == 1)
            cnt--;
        rule[pps[pi]] = bin.readUshorts(data, offset, cnt);
        offset += rule[pps[pi]].length * 2;
    }
    var cnt = bin.readUshort(data, offset);
    offset += 2;
    rule.subst = bin.readUshorts(data, offset, cnt * 2);
    offset += rule.subst.length * 2;
    return rule;
};
Typr.GSUB.readLigatureSet = function (data, offset) {
    var bin = Typr._bin, offset0 = offset, lset = [];
    var lcnt = bin.readUshort(data, offset);
    offset += 2;
    for (var j = 0; j < lcnt; j++) {
        var loff = bin.readUshort(data, offset);
        offset += 2;
        lset.push(Typr.GSUB.readLigature(data, offset0 + loff));
    }
    return lset;
};
Typr.GSUB.readLigature = function (data, offset) {
    var bin = Typr._bin, lig = { chain: [] };
    lig.nglyph = bin.readUshort(data, offset);
    offset += 2;
    var ccnt = bin.readUshort(data, offset);
    offset += 2;
    for (var k = 0; k < ccnt - 1; k++) {
        lig.chain.push(bin.readUshort(data, offset));
        offset += 2;
    }
    return lig;
};
Typr.head = {};
Typr.head.parse = function (data, offset, length) {
    var bin = Typr._bin;
    var obj = {};
    var tableVersion = bin.readFixed(data, offset);
    offset += 4;
    obj.fontRevision = bin.readFixed(data, offset);
    offset += 4;
    var checkSumAdjustment = bin.readUint(data, offset);
    offset += 4;
    var magicNumber = bin.readUint(data, offset);
    offset += 4;
    obj.flags = bin.readUshort(data, offset);
    offset += 2;
    obj.unitsPerEm = bin.readUshort(data, offset);
    offset += 2;
    obj.created = bin.readUint64(data, offset);
    offset += 8;
    obj.modified = bin.readUint64(data, offset);
    offset += 8;
    obj.xMin = bin.readShort(data, offset);
    offset += 2;
    obj.yMin = bin.readShort(data, offset);
    offset += 2;
    obj.xMax = bin.readShort(data, offset);
    offset += 2;
    obj.yMax = bin.readShort(data, offset);
    offset += 2;
    obj.macStyle = bin.readUshort(data, offset);
    offset += 2;
    obj.lowestRecPPEM = bin.readUshort(data, offset);
    offset += 2;
    obj.fontDirectionHint = bin.readShort(data, offset);
    offset += 2;
    obj.indexToLocFormat = bin.readShort(data, offset);
    offset += 2;
    obj.glyphDataFormat = bin.readShort(data, offset);
    offset += 2;
    return obj;
};
Typr.hhea = {};
Typr.hhea.parse = function (data, offset, length) {
    var bin = Typr._bin;
    var obj = {};
    var tableVersion = bin.readFixed(data, offset);
    offset += 4;
    obj.ascender = bin.readShort(data, offset);
    offset += 2;
    obj.descender = bin.readShort(data, offset);
    offset += 2;
    obj.lineGap = bin.readShort(data, offset);
    offset += 2;
    obj.advanceWidthMax = bin.readUshort(data, offset);
    offset += 2;
    obj.minLeftSideBearing = bin.readShort(data, offset);
    offset += 2;
    obj.minRightSideBearing = bin.readShort(data, offset);
    offset += 2;
    obj.xMaxExtent = bin.readShort(data, offset);
    offset += 2;
    obj.caretSlopeRise = bin.readShort(data, offset);
    offset += 2;
    obj.caretSlopeRun = bin.readShort(data, offset);
    offset += 2;
    obj.caretOffset = bin.readShort(data, offset);
    offset += 2;
    offset += 4 * 2;
    obj.metricDataFormat = bin.readShort(data, offset);
    offset += 2;
    obj.numberOfHMetrics = bin.readUshort(data, offset);
    offset += 2;
    return obj;
};
Typr.hmtx = {};
Typr.hmtx.parse = function (data, offset, length, font) {
    var bin = Typr._bin;
    var obj = {};
    obj.aWidth = [];
    obj.lsBearing = [];
    var aw = 0, lsb = 0;
    for (var i = 0; i < font.maxp.numGlyphs; i++) {
        if (i < font.hhea.numberOfHMetrics) {
            aw = bin.readUshort(data, offset);
            offset += 2;
            lsb = bin.readShort(data, offset);
            offset += 2;
        }
        obj.aWidth.push(aw);
        obj.lsBearing.push(lsb);
    }
    return obj;
};
Typr.kern = {};
Typr.kern.parse = function (data, offset, length, font) {
    var bin = Typr._bin;
    var version = bin.readUshort(data, offset);
    offset += 2;
    if (version == 1)
        return Typr.kern.parseV1(data, offset - 2, length, font);
    var nTables = bin.readUshort(data, offset);
    offset += 2;
    var map = { glyph1: [], rval: [] };
    for (var i = 0; i < nTables; i++) {
        offset += 2; // skip version
        var length = bin.readUshort(data, offset);
        offset += 2;
        var coverage = bin.readUshort(data, offset);
        offset += 2;
        var format = coverage >>> 8;
        /* I have seen format 128 once, that's why I do */ format &= 0xf;
        if (format == 0)
            offset = Typr.kern.readFormat0(data, offset, map);
        else
            throw "unknown kern table format: " + format;
    }
    return map;
};
Typr.kern.parseV1 = function (data, offset, length, font) {
    var bin = Typr._bin;
    var version = bin.readFixed(data, offset);
    offset += 4;
    var nTables = bin.readUint(data, offset);
    offset += 4;
    var map = { glyph1: [], rval: [] };
    for (var i = 0; i < nTables; i++) {
        var length = bin.readUint(data, offset);
        offset += 4;
        var coverage = bin.readUshort(data, offset);
        offset += 2;
        var tupleIndex = bin.readUshort(data, offset);
        offset += 2;
        var format = coverage >>> 8;
        /* I have seen format 128 once, that's why I do */ format &= 0xf;
        if (format == 0)
            offset = Typr.kern.readFormat0(data, offset, map);
        else
            throw "unknown kern table format: " + format;
    }
    return map;
};
Typr.kern.readFormat0 = function (data, offset, map) {
    var bin = Typr._bin;
    var pleft = -1;
    var nPairs = bin.readUshort(data, offset);
    offset += 2;
    var searchRange = bin.readUshort(data, offset);
    offset += 2;
    var entrySelector = bin.readUshort(data, offset);
    offset += 2;
    var rangeShift = bin.readUshort(data, offset);
    offset += 2;
    for (var j = 0; j < nPairs; j++) {
        var left = bin.readUshort(data, offset);
        offset += 2;
        var right = bin.readUshort(data, offset);
        offset += 2;
        var value = bin.readShort(data, offset);
        offset += 2;
        if (left != pleft) {
            map.glyph1.push(left);
            map.rval.push({ glyph2: [], vals: [] });
        }
        var rval = map.rval[map.rval.length - 1];
        rval.glyph2.push(right);
        rval.vals.push(value);
        pleft = left;
    }
    return offset;
};
Typr.loca = {};
Typr.loca.parse = function (data, offset, length, font) {
    var bin = Typr._bin;
    var obj = [];
    var ver = font.head.indexToLocFormat;
    //console.warn("loca", ver, length, 4*font.maxp.numGlyphs);
    var len = font.maxp.numGlyphs + 1;
    if (ver == 0)
        for (var i = 0; i < len; i++)
            obj.push(bin.readUshort(data, offset + (i << 1)) << 1);
    if (ver == 1)
        for (var i = 0; i < len; i++)
            obj.push(bin.readUint(data, offset + (i << 2)));
    return obj;
};
Typr.maxp = {};
Typr.maxp.parse = function (data, offset, length) {
    //console.warn(data.length, offset, length);
    var bin = Typr._bin;
    var obj = {};
    // both versions 0.5 and 1.0
    var ver = bin.readUint(data, offset);
    offset += 4;
    obj.numGlyphs = bin.readUshort(data, offset);
    offset += 2;
    // only 1.0
    if (ver == 0x00010000) {
        obj.maxPoints = bin.readUshort(data, offset);
        offset += 2;
        obj.maxContours = bin.readUshort(data, offset);
        offset += 2;
        obj.maxCompositePoints = bin.readUshort(data, offset);
        offset += 2;
        obj.maxCompositeContours = bin.readUshort(data, offset);
        offset += 2;
        obj.maxZones = bin.readUshort(data, offset);
        offset += 2;
        obj.maxTwilightPoints = bin.readUshort(data, offset);
        offset += 2;
        obj.maxStorage = bin.readUshort(data, offset);
        offset += 2;
        obj.maxFunctionDefs = bin.readUshort(data, offset);
        offset += 2;
        obj.maxInstructionDefs = bin.readUshort(data, offset);
        offset += 2;
        obj.maxStackElements = bin.readUshort(data, offset);
        offset += 2;
        obj.maxSizeOfInstructions = bin.readUshort(data, offset);
        offset += 2;
        obj.maxComponentElements = bin.readUshort(data, offset);
        offset += 2;
        obj.maxComponentDepth = bin.readUshort(data, offset);
        offset += 2;
    }
    return obj;
};
Typr.name = {};
Typr.name.parse = function (data, offset, length) {
    var bin = Typr._bin;
    var obj = {};
    var format = bin.readUshort(data, offset);
    offset += 2;
    var count = bin.readUshort(data, offset);
    offset += 2;
    var stringOffset = bin.readUshort(data, offset);
    offset += 2;
    //console.warn(format,count);
    var names = [
        "copyright",
        "fontFamily",
        "fontSubfamily",
        "ID",
        "fullName",
        "version",
        "postScriptName",
        "trademark",
        "manufacturer",
        "designer",
        "description",
        "urlVendor",
        "urlDesigner",
        "licence",
        "licenceURL",
        "---",
        "typoFamilyName",
        "typoSubfamilyName",
        "compatibleFull",
        "sampleText",
        "postScriptCID",
        "wwsFamilyName",
        "wwsSubfamilyName",
        "lightPalette",
        "darkPalette"
    ];
    var offset0 = offset;
    for (var i = 0; i < count; i++) {
        var platformID = bin.readUshort(data, offset);
        offset += 2;
        var encodingID = bin.readUshort(data, offset);
        offset += 2;
        var languageID = bin.readUshort(data, offset);
        offset += 2;
        var nameID = bin.readUshort(data, offset);
        offset += 2;
        var slen = bin.readUshort(data, offset);
        offset += 2;
        var noffset = bin.readUshort(data, offset);
        offset += 2;
        //console.warn(platformID, encodingID, languageID.toString(16), nameID, length, noffset);
        var cname = names[nameID];
        var soff = offset0 + count * 12 + noffset;
        var str;
        if (false) { }
        else if (platformID == 0)
            str = bin.readUnicode(data, soff, slen / 2);
        else if (platformID == 3 && encodingID == 0)
            str = bin.readUnicode(data, soff, slen / 2);
        else if (encodingID == 0)
            str = bin.readASCII(data, soff, slen);
        else if (encodingID == 1)
            str = bin.readUnicode(data, soff, slen / 2);
        else if (encodingID == 3)
            str = bin.readUnicode(data, soff, slen / 2);
        else if (platformID == 1) {
            str = bin.readASCII(data, soff, slen);
            console.warn("reading unknown MAC encoding " + encodingID + " as ASCII");
        }
        else
            throw "unknown encoding " + encodingID + ", platformID: " + platformID;
        var tid = "p" + platformID + "," + (languageID).toString(16); //Typr._platforms[platformID];
        if (obj[tid] == null)
            obj[tid] = {};
        obj[tid][cname !== undefined ? cname : nameID] = str;
        obj[tid]._lang = languageID;
        //console.warn(tid, obj[tid]);
    }
    /*
    if(format == 1)
    {
        var langTagCount = bin.readUshort(data, offset);  offset += 2;
        for(var i=0; i<langTagCount; i++)
        {
            var length  = bin.readUshort(data, offset);  offset += 2;
            var noffset = bin.readUshort(data, offset);  offset += 2;
        }
    }
    */
    //console.warn(obj);
    for (var p in obj)
        if (obj[p].postScriptName != null && obj[p]._lang == 0x0409)
            return obj[p]; // United States
    for (var p in obj)
        if (obj[p].postScriptName != null && obj[p]._lang == 0x0000)
            return obj[p]; // Universal
    for (var p in obj)
        if (obj[p].postScriptName != null && obj[p]._lang == 0x0c0c)
            return obj[p]; // Canada
    for (var p in obj)
        if (obj[p].postScriptName != null)
            return obj[p];
    var tname;
    for (var p in obj) {
        tname = p;
        break;
    }
    console.warn("returning name table with languageID " + obj[tname]._lang);
    return obj[tname];
};
Typr["OS/2"] = {};
Typr["OS/2"].parse = function (data, offset, length) {
    var bin = Typr._bin;
    var ver = bin.readUshort(data, offset);
    offset += 2;
    var obj = {};
    if (ver == 0)
        Typr["OS/2"].version0(data, offset, obj);
    else if (ver == 1)
        Typr["OS/2"].version1(data, offset, obj);
    else if (ver == 2 || ver == 3 || ver == 4)
        Typr["OS/2"].version2(data, offset, obj);
    else if (ver == 5)
        Typr["OS/2"].version5(data, offset, obj);
    else
        throw "unknown OS/2 table version: " + ver;
    return obj;
};
Typr["OS/2"].version0 = function (data, offset, obj) {
    var bin = Typr._bin;
    obj.xAvgCharWidth = bin.readShort(data, offset);
    offset += 2;
    obj.usWeightClass = bin.readUshort(data, offset);
    offset += 2;
    obj.usWidthClass = bin.readUshort(data, offset);
    offset += 2;
    obj.fsType = bin.readUshort(data, offset);
    offset += 2;
    obj.ySubscriptXSize = bin.readShort(data, offset);
    offset += 2;
    obj.ySubscriptYSize = bin.readShort(data, offset);
    offset += 2;
    obj.ySubscriptXOffset = bin.readShort(data, offset);
    offset += 2;
    obj.ySubscriptYOffset = bin.readShort(data, offset);
    offset += 2;
    obj.ySuperscriptXSize = bin.readShort(data, offset);
    offset += 2;
    obj.ySuperscriptYSize = bin.readShort(data, offset);
    offset += 2;
    obj.ySuperscriptXOffset = bin.readShort(data, offset);
    offset += 2;
    obj.ySuperscriptYOffset = bin.readShort(data, offset);
    offset += 2;
    obj.yStrikeoutSize = bin.readShort(data, offset);
    offset += 2;
    obj.yStrikeoutPosition = bin.readShort(data, offset);
    offset += 2;
    obj.sFamilyClass = bin.readShort(data, offset);
    offset += 2;
    obj.panose = bin.readBytes(data, offset, 10);
    offset += 10;
    obj.ulUnicodeRange1 = bin.readUint(data, offset);
    offset += 4;
    obj.ulUnicodeRange2 = bin.readUint(data, offset);
    offset += 4;
    obj.ulUnicodeRange3 = bin.readUint(data, offset);
    offset += 4;
    obj.ulUnicodeRange4 = bin.readUint(data, offset);
    offset += 4;
    obj.achVendID = [bin.readInt8(data, offset), bin.readInt8(data, offset + 1), bin.readInt8(data, offset + 2), bin.readInt8(data, offset + 3)];
    offset += 4;
    obj.fsSelection = bin.readUshort(data, offset);
    offset += 2;
    obj.usFirstCharIndex = bin.readUshort(data, offset);
    offset += 2;
    obj.usLastCharIndex = bin.readUshort(data, offset);
    offset += 2;
    obj.sTypoAscender = bin.readShort(data, offset);
    offset += 2;
    obj.sTypoDescender = bin.readShort(data, offset);
    offset += 2;
    obj.sTypoLineGap = bin.readShort(data, offset);
    offset += 2;
    obj.usWinAscent = bin.readUshort(data, offset);
    offset += 2;
    obj.usWinDescent = bin.readUshort(data, offset);
    offset += 2;
    return offset;
};
Typr["OS/2"].version1 = function (data, offset, obj) {
    var bin = Typr._bin;
    offset = Typr["OS/2"].version0(data, offset, obj);
    obj.ulCodePageRange1 = bin.readUint(data, offset);
    offset += 4;
    obj.ulCodePageRange2 = bin.readUint(data, offset);
    offset += 4;
    return offset;
};
Typr["OS/2"].version2 = function (data, offset, obj) {
    var bin = Typr._bin;
    offset = Typr["OS/2"].version1(data, offset, obj);
    obj.sxHeight = bin.readShort(data, offset);
    offset += 2;
    obj.sCapHeight = bin.readShort(data, offset);
    offset += 2;
    obj.usDefault = bin.readUshort(data, offset);
    offset += 2;
    obj.usBreak = bin.readUshort(data, offset);
    offset += 2;
    obj.usMaxContext = bin.readUshort(data, offset);
    offset += 2;
    return offset;
};
Typr["OS/2"].version5 = function (data, offset, obj) {
    var bin = Typr._bin;
    offset = Typr["OS/2"].version2(data, offset, obj);
    obj.usLowerOpticalPointSize = bin.readUshort(data, offset);
    offset += 2;
    obj.usUpperOpticalPointSize = bin.readUshort(data, offset);
    offset += 2;
    return offset;
};
Typr.post = {};
Typr.post.parse = function (data, offset, length) {
    var bin = Typr._bin;
    var obj = {};
    obj.version = bin.readFixed(data, offset);
    offset += 4;
    obj.italicAngle = bin.readFixed(data, offset);
    offset += 4;
    obj.underlinePosition = bin.readShort(data, offset);
    offset += 2;
    obj.underlineThickness = bin.readShort(data, offset);
    offset += 2;
    return obj;
};
Typr.SVG = {};
Typr.SVG.parse = function (data, offset, length) {
    var bin = Typr._bin;
    var obj = { entries: [] };
    var offset0 = offset;
    var tableVersion = bin.readUshort(data, offset);
    offset += 2;
    var svgDocIndexOffset = bin.readUint(data, offset);
    offset += 4;
    var reserved = bin.readUint(data, offset);
    offset += 4;
    offset = svgDocIndexOffset + offset0;
    var numEntries = bin.readUshort(data, offset);
    offset += 2;
    for (var i = 0; i < numEntries; i++) {
        var startGlyphID = bin.readUshort(data, offset);
        offset += 2;
        var endGlyphID = bin.readUshort(data, offset);
        offset += 2;
        var svgDocOffset = bin.readUint(data, offset);
        offset += 4;
        var svgDocLength = bin.readUint(data, offset);
        offset += 4;
        var sbuf = new Uint8Array(data.buffer, offset0 + svgDocOffset + svgDocIndexOffset, svgDocLength);
        var svg = bin.readUTF8(sbuf, 0, sbuf.length);
        for (var f = startGlyphID; f <= endGlyphID; f++) {
            obj.entries[f] = svg;
        }
    }
    return obj;
};
Typr.SVG.toPath = function (str) {
    var pth = { cmds: [], crds: [] };
    if (str == null)
        return pth;
    var prsr = new DOMParser();
    var doc = prsr["parseFromString"](str, "image/svg+xml");
    var svg = doc.firstChild;
    while (svg.tagName != "svg")
        svg = svg.nextSibling;
    var vb = svg.getAttribute("viewBox");
    if (vb)
        vb = vb.trim().split(" ").map(parseFloat);
    else
        vb = [0, 0, 1000, 1000];
    Typr.SVG._toPath(svg.children, pth);
    for (var i = 0; i < pth.crds.length; i += 2) {
        var x = pth.crds[i], y = pth.crds[i + 1];
        x -= vb[0];
        y -= vb[1];
        y = -y;
        pth.crds[i] = x;
        pth.crds[i + 1] = y;
    }
    return pth;
};
Typr.SVG._toPath = function (nds, pth, fill) {
    for (var ni = 0; ni < nds.length; ni++) {
        var nd = nds[ni], tn = nd.tagName;
        var cfl = nd.getAttribute("fill");
        if (cfl == null)
            cfl = fill;
        if (tn == "g")
            Typr.SVG._toPath(nd.children, pth, cfl);
        else if (tn == "path") {
            pth.cmds.push(cfl ? cfl : "#000000");
            var d = nd.getAttribute("d"); //console.warn(d);
            var toks = Typr.SVG._tokens(d); //console.warn(toks);
            Typr.SVG._toksToPath(toks, pth);
            pth.cmds.push("X");
        }
        else if (tn == "defs") { }
        else
            console.warn(tn, nd);
    }
};
Typr.SVG._tokens = function (d) {
    var ts = [], off = 0, rn = false, cn = ""; // reading number, current number
    while (off < d.length) {
        var cc = d.charCodeAt(off), ch = d.charAt(off);
        off++;
        var isNum = (48 <= cc && cc <= 57) || ch == "." || ch == "-";
        if (rn) {
            if (ch == "-") {
                ts.push(parseFloat(cn));
                cn = ch;
            }
            else if (isNum)
                cn += ch;
            else {
                ts.push(parseFloat(cn));
                if (ch != "," && ch != " ")
                    ts.push(ch);
                rn = false;
            }
        }
        else {
            if (isNum) {
                cn = ch;
                rn = true;
            }
            else if (ch != "," && ch != " ")
                ts.push(ch);
        }
    }
    if (rn)
        ts.push(parseFloat(cn));
    return ts;
};
Typr.SVG._toksToPath = function (ts, pth) {
    var i = 0, x = 0, y = 0, ox = 0, oy = 0;
    var pc = { "M": 2, "L": 2, "H": 1, "V": 1, "S": 4, "C": 6 };
    var cmds = pth.cmds, crds = pth.crds;
    while (i < ts.length) {
        var cmd = ts[i];
        i++;
        if (cmd == "z") {
            cmds.push("Z");
            x = ox;
            y = oy;
        }
        else {
            var cmu = cmd.toUpperCase();
            var ps = pc[cmu], reps = Typr.SVG._reps(ts, i, ps);
            for (var j = 0; j < reps; j++) {
                var xi = 0, yi = 0;
                if (cmd != cmu) {
                    xi = x;
                    yi = y;
                }
                if (false) { }
                else if (cmu == "M") {
                    x = xi + ts[i++];
                    y = yi + ts[i++];
                    cmds.push("M");
                    crds.push(x, y);
                    ox = x;
                    oy = y;
                }
                else if (cmu == "L") {
                    x = xi + ts[i++];
                    y = yi + ts[i++];
                    cmds.push("L");
                    crds.push(x, y);
                }
                else if (cmu == "H") {
                    x = xi + ts[i++];
                    cmds.push("L");
                    crds.push(x, y);
                }
                else if (cmu == "V") {
                    y = yi + ts[i++];
                    cmds.push("L");
                    crds.push(x, y);
                }
                else if (cmu == "C") {
                    var x1 = xi + ts[i++], y1 = yi + ts[i++], x2 = xi + ts[i++], y2 = yi + ts[i++], x3 = xi + ts[i++], y3 = yi + ts[i++];
                    cmds.push("C");
                    crds.push(x1, y1, x2, y2, x3, y3);
                    x = x3;
                    y = y3;
                }
                else if (cmu == "S") {
                    var co = Math.max(crds.length - 4, 0);
                    var x1 = x + x - crds[co], y1 = y + y - crds[co + 1];
                    var x2 = xi + ts[i++], y2 = yi + ts[i++], x3 = xi + ts[i++], y3 = yi + ts[i++];
                    cmds.push("C");
                    crds.push(x1, y1, x2, y2, x3, y3);
                    x = x3;
                    y = y3;
                }
                else
                    console.warn("Unknown SVG command " + cmd);
            }
        }
    }
};
Typr.SVG._reps = function (ts, off, ps) {
    var i = off;
    while (i < ts.length) {
        if ((typeof ts[i]) == "string")
            break;
        i += ps;
    }
    return (i - off) / ps;
};
if (Typr == null)
    Typr = {};
if (Typr.U == null)
    Typr.U = {};
Typr.U.codeToGlyph = function (font, code) {
    var cmap = font.cmap;
    var tind = -1;
    if (cmap.p0e4 != null)
        tind = cmap.p0e4;
    else if (cmap.p3e1 != null)
        tind = cmap.p3e1;
    else if (cmap.p1e0 != null)
        tind = cmap.p1e0;
    else if (cmap.p0e3 != null)
        tind = cmap.p0e3;
    if (tind == -1)
        throw "no familiar platform and encoding!";
    var tab = cmap.tables[tind];
    if (tab.format == 0) {
        if (code >= tab.map.length)
            return 0;
        return tab.map[code];
    }
    else if (tab.format == 4) {
        var sind = -1;
        for (var i = 0; i < tab.endCount.length; i++) {
            if (code <= tab.endCount[i]) {
                sind = i;
                break;
            }
        }
        if (sind == -1)
            return 0;
        if (tab.startCount[sind] > code)
            return 0;
        var gli = 0;
        if (tab.idRangeOffset[sind] != 0) {
            gli = tab.glyphIdArray[(code - tab.startCount[sind]) + (tab.idRangeOffset[sind] >> 1) - (tab.idRangeOffset.length - sind)];
        }
        else {
            gli = code + tab.idDelta[sind];
        }
        return gli & 0xFFFF;
    }
    else if (tab.format == 12) {
        if (code > tab.groups[tab.groups.length - 1][1])
            return 0;
        for (var i = 0; i < tab.groups.length; i++) {
            var grp = tab.groups[i];
            if (grp[0] <= code && code <= grp[1])
                return grp[2] + (code - grp[0]);
        }
        return 0;
    }
    else {
        throw "unknown cmap table format " + tab.format;
    }
};
Typr.U.glyphToPath = function (font, gid) {
    var path = { cmds: [], crds: [] };
    if (font.SVG && font.SVG.entries[gid]) {
        var p = font.SVG.entries[gid];
        if (p == null)
            return path;
        if (typeof p == "string") {
            p = Typr.SVG.toPath(p);
            font.SVG.entries[gid] = p;
        }
        return p;
    }
    else if (font.CFF) {
        var state = { x: 0, y: 0, stack: [], nStems: 0, haveWidth: false, width: font.CFF.Private ? font.CFF.Private.defaultWidthX : 0, open: false };
        var cff = font.CFF, pdct = font.CFF.Private;
        if (cff.ROS) {
            var gi = 0;
            while (cff.FDSelect[gi + 2] <= gid)
                gi += 2;
            pdct = cff.FDArray[cff.FDSelect[gi + 1]].Private;
        }
        Typr.U._drawCFF(font.CFF.CharStrings[gid], state, cff, pdct, path);
    }
    else if (font.glyf) {
        Typr.U._drawGlyf(gid, font, path);
    }
    return path;
};
Typr.U._drawGlyf = function (gid, font, path) {
    var gl = font.glyf[gid];
    if (gl == null)
        gl = font.glyf[gid] = Typr.glyf._parseGlyf(font, gid);
    if (gl != null) {
        if (gl.noc > -1) {
            Typr.U._simpleGlyph(gl, path);
        }
        else {
            Typr.U._compoGlyph(gl, font, path);
        }
    }
};
Typr.U._simpleGlyph = function (gl, p) {
    for (var c = 0; c < gl.noc; c++) {
        var i0 = (c == 0) ? 0 : (gl.endPts[c - 1] + 1);
        var il = gl.endPts[c];
        for (var i = i0; i <= il; i++) {
            var pr = (i == i0) ? il : (i - 1);
            var nx = (i == il) ? i0 : (i + 1);
            var onCurve = gl.flags[i] & 1;
            var prOnCurve = gl.flags[pr] & 1;
            var nxOnCurve = gl.flags[nx] & 1;
            var x = gl.xs[i], y = gl.ys[i];
            if (i == i0) {
                if (onCurve) {
                    if (prOnCurve) {
                        Typr.U.P.moveTo(p, gl.xs[pr], gl.ys[pr]);
                    }
                    else {
                        Typr.U.P.moveTo(p, x, y);
                        continue; /*  will do curveTo at il  */
                    }
                }
                else {
                    if (prOnCurve) {
                        Typr.U.P.moveTo(p, gl.xs[pr], gl.ys[pr]);
                    }
                    else {
                        Typr.U.P.moveTo(p, (gl.xs[pr] + x) / 2, (gl.ys[pr] + y) / 2);
                    }
                }
            }
            if (onCurve) {
                if (prOnCurve)
                    Typr.U.P.lineTo(p, x, y);
            }
            else {
                if (nxOnCurve) {
                    Typr.U.P.qcurveTo(p, x, y, gl.xs[nx], gl.ys[nx]);
                }
                else {
                    Typr.U.P.qcurveTo(p, x, y, (x + gl.xs[nx]) / 2, (y + gl.ys[nx]) / 2);
                }
            }
        }
        Typr.U.P.closePath(p);
    }
};
Typr.U._compoGlyph = function (gl, font, p) {
    for (var j = 0; j < gl.parts.length; j++) {
        var path = { cmds: [], crds: [] };
        var prt = gl.parts[j];
        Typr.U._drawGlyf(prt.glyphIndex, font, path);
        var m = prt.m;
        for (var i = 0; i < path.crds.length; i += 2) {
            var x = path.crds[i], y = path.crds[i + 1];
            p.crds.push(x * m.a + y * m.b + m.tx);
            p.crds.push(x * m.c + y * m.d + m.ty);
        }
        for (var i = 0; i < path.cmds.length; i++) {
            p.cmds.push(path.cmds[i]);
        }
    }
};
Typr.U._getGlyphClass = function (g, cd) {
    var intr = Typr._lctf.getInterval(cd, g);
    return intr == -1 ? 0 : cd[intr + 2];
    //for(var i=0; i<cd.start.length; i++) 
    //	if(cd.start[i]<=g && cd.end[i]>=g) return cd.class[i];
    //return 0;
};
Typr.U.getPairAdjustment = function (font, g1, g2) {
    var hasGPOSkern = false;
    if (font.GPOS) {
        var gpos = font["GPOS"];
        var llist = gpos.lookupList, flist = gpos.featureList;
        var tused = [];
        for (var i = 0; i < flist.length; i++) {
            var fl = flist[i]; //console.warn(fl);
            if (fl.tag != "kern")
                continue;
            hasGPOSkern = true;
            for (var ti = 0; ti < fl.tab.length; ti++) {
                if (tused[fl.tab[ti]])
                    continue;
                tused[fl.tab[ti]] = true;
                var tab = llist[fl.tab[ti]];
                //console.warn(tab);
                for (var j = 0; j < tab.tabs.length; j++) {
                    if (tab.tabs[j] == null)
                        continue;
                    var ltab = tab.tabs[j], ind;
                    if (ltab.coverage) {
                        ind = Typr._lctf.coverageIndex(ltab.coverage, g1);
                        if (ind == -1)
                            continue;
                    }
                    if (tab.ltype == 1) {
                        //console.warn(ltab);
                    }
                    else if (tab.ltype == 2) {
                        var adj = null;
                        if (ltab.fmt == 1) {
                            var right = ltab.pairsets[ind];
                            for (var i = 0; i < right.length; i++) {
                                if (right[i].gid2 == g2)
                                    adj = right[i];
                            }
                        }
                        else if (ltab.fmt == 2) {
                            var c1 = Typr.U._getGlyphClass(g1, ltab.classDef1);
                            var c2 = Typr.U._getGlyphClass(g2, ltab.classDef2);
                            adj = ltab.matrix[c1][c2];
                        }
                        if (adj) {
                            var offset = 0;
                            if (adj.val1 && adj.val1[2])
                                offset += adj.val1[2]; // xAdvance adjustment of first glyph
                            if (adj.val2 && adj.val2[0])
                                offset += adj.val2[0]; // xPlacement adjustment of second glyph
                            return offset;
                        }
                    }
                }
            }
        }
    }
    if (font.kern && !hasGPOSkern) {
        var ind1 = font.kern.glyph1.indexOf(g1);
        if (ind1 != -1) {
            var ind2 = font.kern.rval[ind1].glyph2.indexOf(g2);
            if (ind2 != -1)
                return font.kern.rval[ind1].vals[ind2];
        }
    }
    return 0;
};
Typr.U.stringToGlyphs = function (font, str) {
    var gls = [];
    for (var i = 0; i < str.length; i++) {
        var cc = str.codePointAt(i);
        if (cc > 0xffff)
            i++;
        gls.push(Typr.U.codeToGlyph(font, cc));
    }
    for (var i = 0; i < str.length; i++) {
        var cc = str.codePointAt(i); //
        if (cc == 2367) {
            var t = gls[i - 1];
            gls[i - 1] = gls[i];
            gls[i] = t;
        }
        //if(cc==2381) {  var t=gls[i+1];  gls[i+1]=gls[i];  gls[i]=t;  }
        if (cc > 0xffff)
            i++;
    }
    //console.warn(gls.slice(0));
    //console.warn(gls);  return gls;
    var gsub = font["GSUB"];
    if (gsub == null)
        return gls;
    var llist = gsub.lookupList, flist = gsub.featureList;
    var cligs = [
        "rlig",
        "liga",
        "mset",
        "isol",
        "init",
        "fina",
        "medi",
        "half",
        "pres",
        "blws" /* Tibetan fonts like Himalaya.ttf */
    ];
    //console.warn(gls.slice(0));
    var tused = [];
    for (var fi = 0; fi < flist.length; fi++) {
        var fl = flist[fi];
        if (cligs.indexOf(fl.tag) == -1)
            continue;
        //if(fl.tag=="blwf") continue;
        //console.warn(fl);
        //console.warn(fl.tag);
        for (var ti = 0; ti < fl.tab.length; ti++) {
            if (tused[fl.tab[ti]])
                continue;
            tused[fl.tab[ti]] = true;
            var tab = llist[fl.tab[ti]];
            //console.warn(fl.tab[ti], tab.ltype);
            //console.warn(fl.tag, tab);
            for (var ci = 0; ci < gls.length; ci++) {
                var feat = Typr.U._getWPfeature(str, ci);
                if ("isol,init,fina,medi".indexOf(fl.tag) != -1 && fl.tag != feat)
                    continue;
                Typr.U._applySubs(gls, ci, tab, llist);
            }
        }
    }
    return gls;
};
Typr.U._getWPfeature = function (str, ci) {
    var wsep = "\n\t\" ,.:;!?()  ،";
    var R = "آأؤإاةدذرزوٱٲٳٵٶٷڈډڊڋڌڍڎڏڐڑڒړڔڕږڗژڙۀۃۄۅۆۇۈۉۊۋۍۏےۓەۮۯܐܕܖܗܘܙܞܨܪܬܯݍݙݚݛݫݬݱݳݴݸݹࡀࡆࡇࡉࡔࡧࡩࡪࢪࢫࢬࢮࢱࢲࢹૅેૉ૊૎૏ૐ૑૒૝ૡ૤૯஁ஃ஄அஉ஌எஏ஑னப஫஬";
    var L = "ꡲ્૗";
    var slft = ci == 0 || wsep.indexOf(str[ci - 1]) != -1;
    var srgt = ci == str.length - 1 || wsep.indexOf(str[ci + 1]) != -1;
    if (!slft && R.indexOf(str[ci - 1]) != -1)
        slft = true;
    if (!srgt && R.indexOf(str[ci]) != -1)
        srgt = true;
    if (!srgt && L.indexOf(str[ci + 1]) != -1)
        srgt = true;
    if (!slft && L.indexOf(str[ci]) != -1)
        slft = true;
    var feat = null;
    if (slft) {
        feat = srgt ? "isol" : "init";
    }
    else {
        feat = srgt ? "fina" : "medi";
    }
    return feat;
};
Typr.U._applySubs = function (gls, ci, tab, llist) {
    //if(ci==0) console.warn("++++ ", tab.ltype);
    var rlim = gls.length - ci - 1;
    for (var j = 0; j < tab.tabs.length; j++) {
        if (tab.tabs[j] == null)
            continue;
        var ltab = tab.tabs[j], ind;
        if (ltab.coverage) {
            ind = Typr._lctf.coverageIndex(ltab.coverage, gls[ci]);
            if (ind == -1)
                continue;
        }
        //if(ci==0) console.warn(ind, ltab);
        if (tab.ltype == 1) {
            var gl = gls[ci];
            if (ltab.fmt == 1)
                gls[ci] = gls[ci] + ltab.delta;
            else
                gls[ci] = ltab.newg[ind];
            //console.warn("applying ... 1", ci, gl, gls[ci]);
        }
        else if (tab.ltype == 4) {
            var vals = ltab.vals[ind];
            for (var k = 0; k < vals.length; k++) {
                var lig = vals[k], rl = lig.chain.length;
                if (rl > rlim)
                    continue;
                var good = true, em1 = 0;
                for (var l = 0; l < rl; l++) {
                    while (gls[ci + em1 + (1 + l)] == -1)
                        em1++;
                    if (lig.chain[l] != gls[ci + em1 + (1 + l)])
                        good = false;
                }
                if (!good)
                    continue;
                gls[ci] = lig.nglyph;
                for (var l = 0; l < rl + em1; l++)
                    gls[ci + l + 1] = -1;
                break; // first character changed, other ligatures do not apply anymore
                //console.warn("lig", ci, lig.chain, lig.nglyph);
                //console.warn("applying ...");
            }
        }
        else if (tab.ltype == 5 && ltab.fmt == 2) {
            var cind = Typr._lctf.getInterval(ltab.cDef, gls[ci]);
            var cls = ltab.cDef[cind + 2], scs = ltab.scset[cls];
            for (var i = 0; i < scs.length; i++) {
                var sc = scs[i], inp = sc.input;
                if (inp.length > rlim)
                    continue;
                var good = true;
                for (var l = 0; l < inp.length; l++) {
                    var cind2 = Typr._lctf.getInterval(ltab.cDef, gls[ci + 1 + l]);
                    if (cind == -1 && ltab.cDef[cind2 + 2] != inp[l]) {
                        good = false;
                        break;
                    }
                }
                if (!good)
                    continue;
                //console.warn(ci, gl);
                var lrs = sc.substLookupRecords;
                for (var k = 0; k < lrs.length; k += 2) {
                    var gi = lrs[k], tabi = lrs[k + 1];
                    //Typr.U._applyType1(gls, ci+gi, llist[tabi]);
                    //console.warn(tabi, gls[ci+gi], llist[tabi]);
                }
            }
        }
        else if (tab.ltype == 6 && ltab.fmt == 3) {
            //if(ltab.backCvg.length==0) return;
            if (!Typr.U._glsCovered(gls, ltab.backCvg, ci - ltab.backCvg.length))
                continue;
            if (!Typr.U._glsCovered(gls, ltab.inptCvg, ci))
                continue;
            if (!Typr.U._glsCovered(gls, ltab.ahedCvg, ci + ltab.inptCvg.length))
                continue;
            //console.warn(ci, ltab);
            var lr = ltab.lookupRec; //console.warn(ci, gl, lr);
            for (var i = 0; i < lr.length; i += 2) {
                var cind = lr[i], tab2 = llist[lr[i + 1]];
                //console.warn("-", lr[i+1], tab2);
                Typr.U._applySubs(gls, ci + cind, tab2, llist);
            }
        } //else console.warn("Unknown table", tab.ltype, ltab.fmt);
    }
};
Typr.U._glsCovered = function (gls, cvgs, ci) {
    for (var i = 0; i < cvgs.length; i++) {
        var ind = Typr._lctf.coverageIndex(cvgs[i], gls[ci + i]);
        if (ind == -1)
            return false;
    }
    return true;
};
Typr.U.glyphsToPath = function (font, gls, clr) {
    //gls = gls.reverse();//gls.slice(0,12).concat(gls.slice(12).reverse());
    var tpath = { cmds: [], crds: [] };
    var x = 0;
    for (var i = 0; i < gls.length; i++) {
        var gid = gls[i];
        if (gid == -1)
            continue;
        var gid2 = (i < gls.length - 1 && gls[i + 1] != -1) ? gls[i + 1] : 0;
        var path = Typr.U.glyphToPath(font, gid);
        for (var j = 0; j < path.crds.length; j += 2) {
            tpath.crds.push(path.crds[j] + x);
            tpath.crds.push(path.crds[j + 1]);
        }
        if (clr)
            tpath.cmds.push(clr);
        for (var j = 0; j < path.cmds.length; j++)
            tpath.cmds.push(path.cmds[j]);
        if (clr)
            tpath.cmds.push("X");
        x += font.hmtx.aWidth[gid]; // - font.hmtx.lsBearing[gid];
        if (i < gls.length - 1)
            x += Typr.U.getPairAdjustment(font, gid, gid2);
    }
    return tpath;
};
Typr.U.pathToSVG = function (path, prec) {
    if (prec == null)
        prec = 5;
    var out = [], co = 0, lmap = { "M": 2, "L": 2, "Q": 4, "C": 6 };
    for (var i = 0; i < path.cmds.length; i++) {
        var cmd = path.cmds[i], cn = co + (lmap[cmd] ? lmap[cmd] : 0);
        out.push(cmd);
        while (co < cn) {
            var c = path.crds[co++];
            out.push(parseFloat(c.toFixed(prec)) + (co == cn ? "" : " "));
        }
    }
    return out.join("");
};
Typr.U.pathToContext = function (path, ctx) {
    var c = 0, crds = path.crds;
    for (var j = 0; j < path.cmds.length; j++) {
        var cmd = path.cmds[j];
        if (cmd == "M") {
            ctx.moveTo(crds[c], crds[c + 1]);
            c += 2;
        }
        else if (cmd == "L") {
            ctx.lineTo(crds[c], crds[c + 1]);
            c += 2;
        }
        else if (cmd == "C") {
            ctx.bezierCurveTo(crds[c], crds[c + 1], crds[c + 2], crds[c + 3], crds[c + 4], crds[c + 5]);
            c += 6;
        }
        else if (cmd == "Q") {
            ctx.quadraticCurveTo(crds[c], crds[c + 1], crds[c + 2], crds[c + 3]);
            c += 4;
        }
        else if (cmd.charAt(0) == "#") {
            ctx.beginPath();
            ctx.fillStyle = cmd;
        }
        else if (cmd == "Z") {
            ctx.closePath();
        }
        else if (cmd == "X") {
            ctx.fill();
        }
    }
};
Typr.U.P = {};
Typr.U.P.moveTo = function (p, x, y) {
    p.cmds.push("M");
    p.crds.push(x, y);
};
Typr.U.P.lineTo = function (p, x, y) {
    p.cmds.push("L");
    p.crds.push(x, y);
};
Typr.U.P.curveTo = function (p, a, b, c, d, e, f) {
    p.cmds.push("C");
    p.crds.push(a, b, c, d, e, f);
};
Typr.U.P.qcurveTo = function (p, a, b, c, d) {
    p.cmds.push("Q");
    p.crds.push(a, b, c, d);
};
Typr.U.P.closePath = function (p) { p.cmds.push("Z"); };
Typr.U._drawCFF = function (cmds, state, font, pdct, p) {
    var stack = state.stack;
    var nStems = state.nStems, haveWidth = state.haveWidth, width = state.width, open = state.open;
    var i = 0;
    var x = state.x, y = state.y, c1x = 0, c1y = 0, c2x = 0, c2y = 0, c3x = 0, c3y = 0, c4x = 0, c4y = 0, jpx = 0, jpy = 0;
    var o = { val: 0, size: 0 };
    //console.warn(cmds);
    while (i < cmds.length) {
        Typr.CFF.getCharString(cmds, i, o);
        var v = o.val;
        i += o.size;
        if (false) {
        }
        else if (v == "o1" || v == "o18") { //  hstem || hstemhm
            var hasWidthArg;
            // The number of stem operators on the stack is always even.
            // If the value is uneven, that means a width is specified.
            hasWidthArg = stack.length % 2 !== 0;
            if (hasWidthArg && !haveWidth) {
                width = stack.shift() + pdct.nominalWidthX;
            }
            nStems += stack.length >> 1;
            stack.length = 0;
            haveWidth = true;
        }
        else if (v == "o3" || v == "o23") { // vstem || vstemhm
            var hasWidthArg;
            // The number of stem operators on the stack is always even.
            // If the value is uneven, that means a width is specified.
            hasWidthArg = stack.length % 2 !== 0;
            if (hasWidthArg && !haveWidth) {
                width = stack.shift() + pdct.nominalWidthX;
            }
            nStems += stack.length >> 1;
            stack.length = 0;
            haveWidth = true;
        }
        else if (v == "o4") {
            if (stack.length > 1 && !haveWidth) {
                width = stack.shift() + pdct.nominalWidthX;
                haveWidth = true;
            }
            if (open)
                Typr.U.P.closePath(p);
            y += stack.pop();
            Typr.U.P.moveTo(p, x, y);
            open = true;
        }
        else if (v == "o5") {
            while (stack.length > 0) {
                x += stack.shift();
                y += stack.shift();
                Typr.U.P.lineTo(p, x, y);
            }
        }
        else if (v == "o6" || v == "o7") { // hlineto || vlineto
            var count = stack.length;
            var isX = (v == "o6");
            for (var j = 0; j < count; j++) {
                var sval = stack.shift();
                if (isX) {
                    x += sval;
                }
                else {
                    y += sval;
                }
                isX = !isX;
                Typr.U.P.lineTo(p, x, y);
            }
        }
        else if (v == "o8" || v == "o24") { // rrcurveto || rcurveline
            var count = stack.length;
            var index = 0;
            while (index + 6 <= count) {
                c1x = x + stack.shift();
                c1y = y + stack.shift();
                c2x = c1x + stack.shift();
                c2y = c1y + stack.shift();
                x = c2x + stack.shift();
                y = c2y + stack.shift();
                Typr.U.P.curveTo(p, c1x, c1y, c2x, c2y, x, y);
                index += 6;
            }
            if (v == "o24") {
                x += stack.shift();
                y += stack.shift();
                Typr.U.P.lineTo(p, x, y);
            }
        }
        else if (v == "o11") {
            break;
        }
        else if (v == "o1234" || v == "o1235" || v == "o1236" || v == "o1237") { //if((v+"").slice(0,3)=="o12")
            if (v == "o1234") {
                c1x = x + stack.shift(); // dx1
                c1y = y; // dy1
                c2x = c1x + stack.shift(); // dx2
                c2y = c1y + stack.shift(); // dy2
                jpx = c2x + stack.shift(); // dx3
                jpy = c2y; // dy3
                c3x = jpx + stack.shift(); // dx4
                c3y = c2y; // dy4
                c4x = c3x + stack.shift(); // dx5
                c4y = y; // dy5
                x = c4x + stack.shift(); // dx6
                Typr.U.P.curveTo(p, c1x, c1y, c2x, c2y, jpx, jpy);
                Typr.U.P.curveTo(p, c3x, c3y, c4x, c4y, x, y);
            }
            if (v == "o1235") {
                c1x = x + stack.shift(); // dx1
                c1y = y + stack.shift(); // dy1
                c2x = c1x + stack.shift(); // dx2
                c2y = c1y + stack.shift(); // dy2
                jpx = c2x + stack.shift(); // dx3
                jpy = c2y + stack.shift(); // dy3
                c3x = jpx + stack.shift(); // dx4
                c3y = jpy + stack.shift(); // dy4
                c4x = c3x + stack.shift(); // dx5
                c4y = c3y + stack.shift(); // dy5
                x = c4x + stack.shift(); // dx6
                y = c4y + stack.shift(); // dy6
                stack.shift(); // flex depth
                Typr.U.P.curveTo(p, c1x, c1y, c2x, c2y, jpx, jpy);
                Typr.U.P.curveTo(p, c3x, c3y, c4x, c4y, x, y);
            }
            if (v == "o1236") {
                c1x = x + stack.shift(); // dx1
                c1y = y + stack.shift(); // dy1
                c2x = c1x + stack.shift(); // dx2
                c2y = c1y + stack.shift(); // dy2
                jpx = c2x + stack.shift(); // dx3
                jpy = c2y; // dy3
                c3x = jpx + stack.shift(); // dx4
                c3y = c2y; // dy4
                c4x = c3x + stack.shift(); // dx5
                c4y = c3y + stack.shift(); // dy5
                x = c4x + stack.shift(); // dx6
                Typr.U.P.curveTo(p, c1x, c1y, c2x, c2y, jpx, jpy);
                Typr.U.P.curveTo(p, c3x, c3y, c4x, c4y, x, y);
            }
            if (v == "o1237") {
                c1x = x + stack.shift(); // dx1
                c1y = y + stack.shift(); // dy1
                c2x = c1x + stack.shift(); // dx2
                c2y = c1y + stack.shift(); // dy2
                jpx = c2x + stack.shift(); // dx3
                jpy = c2y + stack.shift(); // dy3
                c3x = jpx + stack.shift(); // dx4
                c3y = jpy + stack.shift(); // dy4
                c4x = c3x + stack.shift(); // dx5
                c4y = c3y + stack.shift(); // dy5
                if (Math.abs(c4x - x) > Math.abs(c4y - y)) {
                    x = c4x + stack.shift();
                }
                else {
                    y = c4y + stack.shift();
                }
                Typr.U.P.curveTo(p, c1x, c1y, c2x, c2y, jpx, jpy);
                Typr.U.P.curveTo(p, c3x, c3y, c4x, c4y, x, y);
            }
        }
        else if (v == "o14") {
            if (stack.length > 0 && !haveWidth) {
                width = stack.shift() + font.nominalWidthX;
                haveWidth = true;
            }
            if (stack.length == 4) { // seac = standard encoding accented character
                var asb = 0;
                var adx = stack.shift();
                var ady = stack.shift();
                var bchar = stack.shift();
                var achar = stack.shift();
                var bind = Typr.CFF.glyphBySE(font, bchar);
                var aind = Typr.CFF.glyphBySE(font, achar);
                //console.warn(bchar, bind);
                //console.warn(achar, aind);
                //state.x=x; state.y=y; state.nStems=nStems; state.haveWidth=haveWidth; state.width=width;  state.open=open;
                Typr.U._drawCFF(font.CharStrings[bind], state, font, pdct, p);
                state.x = adx;
                state.y = ady;
                Typr.U._drawCFF(font.CharStrings[aind], state, font, pdct, p);
                //x=state.x; y=state.y; nStems=state.nStems; haveWidth=state.haveWidth; width=state.width;  open=state.open;
            }
            if (open) {
                Typr.U.P.closePath(p);
                open = false;
            }
        }
        else if (v == "o19" || v == "o20") {
            var hasWidthArg;
            // The number of stem operators on the stack is always even.
            // If the value is uneven, that means a width is specified.
            hasWidthArg = stack.length % 2 !== 0;
            if (hasWidthArg && !haveWidth) {
                width = stack.shift() + pdct.nominalWidthX;
            }
            nStems += stack.length >> 1;
            stack.length = 0;
            haveWidth = true;
            i += (nStems + 7) >> 3;
        }
        else if (v == "o21") {
            if (stack.length > 2 && !haveWidth) {
                width = stack.shift() + pdct.nominalWidthX;
                haveWidth = true;
            }
            y += stack.pop();
            x += stack.pop();
            if (open)
                Typr.U.P.closePath(p);
            Typr.U.P.moveTo(p, x, y);
            open = true;
        }
        else if (v == "o22") {
            if (stack.length > 1 && !haveWidth) {
                width = stack.shift() + pdct.nominalWidthX;
                haveWidth = true;
            }
            x += stack.pop();
            if (open)
                Typr.U.P.closePath(p);
            Typr.U.P.moveTo(p, x, y);
            open = true;
        }
        else if (v == "o25") {
            while (stack.length > 6) {
                x += stack.shift();
                y += stack.shift();
                Typr.U.P.lineTo(p, x, y);
            }
            c1x = x + stack.shift();
            c1y = y + stack.shift();
            c2x = c1x + stack.shift();
            c2y = c1y + stack.shift();
            x = c2x + stack.shift();
            y = c2y + stack.shift();
            Typr.U.P.curveTo(p, c1x, c1y, c2x, c2y, x, y);
        }
        else if (v == "o26") {
            if (stack.length % 2) {
                x += stack.shift();
            }
            while (stack.length > 0) {
                c1x = x;
                c1y = y + stack.shift();
                c2x = c1x + stack.shift();
                c2y = c1y + stack.shift();
                x = c2x;
                y = c2y + stack.shift();
                Typr.U.P.curveTo(p, c1x, c1y, c2x, c2y, x, y);
            }
        }
        else if (v == "o27") {
            if (stack.length % 2) {
                y += stack.shift();
            }
            while (stack.length > 0) {
                c1x = x + stack.shift();
                c1y = y;
                c2x = c1x + stack.shift();
                c2y = c1y + stack.shift();
                x = c2x + stack.shift();
                y = c2y;
                Typr.U.P.curveTo(p, c1x, c1y, c2x, c2y, x, y);
            }
        }
        else if (v == "o10" || v == "o29") { // callsubr || callgsubr
            var obj = (v == "o10" ? pdct : font);
            if (stack.length == 0) {
                console.warn("error: empty stack");
            }
            else {
                var ind = stack.pop();
                var subr = obj.Subrs[ind + obj.Bias];
                state.x = x;
                state.y = y;
                state.nStems = nStems;
                state.haveWidth = haveWidth;
                state.width = width;
                state.open = open;
                Typr.U._drawCFF(subr, state, font, pdct, p);
                x = state.x;
                y = state.y;
                nStems = state.nStems;
                haveWidth = state.haveWidth;
                width = state.width;
                open = state.open;
            }
        }
        else if (v == "o30" || v == "o31") { // vhcurveto || hvcurveto
            var count, count1 = stack.length;
            var index = 0;
            var alternate = v == "o31";
            count = count1 & ~2;
            index += count1 - count;
            while (index < count) {
                if (alternate) {
                    c1x = x + stack.shift();
                    c1y = y;
                    c2x = c1x + stack.shift();
                    c2y = c1y + stack.shift();
                    y = c2y + stack.shift();
                    if (count - index == 5) {
                        x = c2x + stack.shift();
                        index++;
                    }
                    else {
                        x = c2x;
                    }
                    alternate = false;
                }
                else {
                    c1x = x;
                    c1y = y + stack.shift();
                    c2x = c1x + stack.shift();
                    c2y = c1y + stack.shift();
                    x = c2x + stack.shift();
                    if (count - index == 5) {
                        y = c2y + stack.shift();
                        index++;
                    }
                    else {
                        y = c2y;
                    }
                    alternate = true;
                }
                Typr.U.P.curveTo(p, c1x, c1y, c2x, c2y, x, y);
                index += 4;
            }
        }
        else if ((v + "").charAt(0) == "o") {
            console.warn("Unknown operation: " + v, cmds);
            throw v;
        }
        else
            stack.push(v);
    }
    //console.warn(cmds);
    state.x = x;
    state.y = y;
    state.nStems = nStems;
    state.haveWidth = haveWidth;
    state.width = width;
    state.open = open;
};
exports.Typr = Typr;

});
return ___scope___.entry = "dist/index.js";
});
FuseBox.pkg("assert", {}, function(___scope___){
___scope___.file("index.js", function(exports, require, module, __filename, __dirname){

/*
 * From https://github.com/defunctzombie/commonjs-assert/blob/master/assert.js
 */

// http://wiki.commonjs.org/wiki/Unit_Testing/1.0
//
// Originally from narwhal.js (http://narwhaljs.org)
// Copyright (c) 2009 Thomas Robinson <280north.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the 'Software'), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// when used in node, this will actually load the util module we depend on
// versus loading the builtin util module as happens otherwise
// this is a bug in node module loading as far as I am concerned
var util = require("util");

var pSlice = Array.prototype.slice;
var hasOwn = Object.prototype.hasOwnProperty;

// 1. The assert module provides functions that throw
// AssertionError's when particular conditions are not met. The
// assert module must conform to the following interface.

var assert = (module.exports = ok);

// 2. The AssertionError is defined in assert.
// new assert.AssertionError({ message: message,
//                             actual: actual,
//                             expected: expected })

assert.AssertionError = function AssertionError(options) {
	this.name = "AssertionError";
	this.actual = options.actual;
	this.expected = options.expected;
	this.operator = options.operator;
	if (options.message) {
		this.message = options.message;
		this.generatedMessage = false;
	} else {
		this.message = getMessage(this);
		this.generatedMessage = true;
	}
	var stackStartFunction = options.stackStartFunction || fail;

	if (Error.captureStackTrace) {
		Error.captureStackTrace(this, stackStartFunction);
	} else {
		// non v8 browsers so we can have a stacktrace
		var err = new Error();
		if (err.stack) {
			var out = err.stack;

			// try to strip useless frames
			var fn_name = stackStartFunction.name;
			var idx = out.indexOf("\n" + fn_name);
			if (idx >= 0) {
				// once we have located the function frame
				// we need to strip out everything before it (and its line)
				var next_line = out.indexOf("\n", idx + 1);
				out = out.substring(next_line + 1);
			}

			this.stack = out;
		}
	}
};

// assert.AssertionError instanceof Error
util.inherits(assert.AssertionError, Error);

function replacer(key, value) {
	if (util.isUndefined(value)) {
		return "" + value;
	}
	if (util.isNumber(value) && !isFinite(value)) {
		return value.toString();
	}
	if (util.isFunction(value) || util.isRegExp(value)) {
		return value.toString();
	}
	return value;
}

function truncate(s, n) {
	if (util.isString(s)) {
		return s.length < n ? s : s.slice(0, n);
	} else {
		return s;
	}
}

function getMessage(self) {
	return truncate(JSON.stringify(self.actual, replacer), 128) + " " + self.operator + " " + truncate(JSON.stringify(self.expected, replacer), 128);
}

// At present only the three keys mentioned above are used and
// understood by the spec. Implementations or sub modules can pass
// other keys to the AssertionError's constructor - they will be
// ignored.

// 3. All of the following functions must throw an AssertionError
// when a corresponding condition is not met, with a message that
// may be undefined if not provided.  All assertion methods provide
// both the actual and expected values to the assertion error for
// display purposes.

function fail(actual, expected, message, operator, stackStartFunction) {
	throw new assert.AssertionError({
		message: message,
		actual: actual,
		expected: expected,
		operator: operator,
		stackStartFunction: stackStartFunction
	});
}

// EXTENSION! allows for well behaved errors defined elsewhere.
assert.fail = fail;

// 4. Pure assertion tests whether a value is truthy, as determined
// by !!guard.
// assert.ok(guard, message_opt);
// This statement is equivalent to assert.equal(true, !!guard,
// message_opt);. To test strictly for the value true, use
// assert.strictEqual(true, guard, message_opt);.

function ok(value, message) {
	if (!value) fail(value, true, message, "==", assert.ok);
}
assert.ok = ok;

// 5. The equality assertion tests shallow, coercive equality with
// ==.
// assert.equal(actual, expected, message_opt);

assert.equal = function equal(actual, expected, message) {
	if (actual != expected) fail(actual, expected, message, "==", assert.equal);
};

// 6. The non-equality assertion tests for whether two objects are not equal
// with != assert.notEqual(actual, expected, message_opt);

assert.notEqual = function notEqual(actual, expected, message) {
	if (actual == expected) {
		fail(actual, expected, message, "!=", assert.notEqual);
	}
};

// 7. The equivalence assertion tests a deep equality relation.
// assert.deepEqual(actual, expected, message_opt);

assert.deepEqual = function deepEqual(actual, expected, message) {
	if (!_deepEqual(actual, expected)) {
		fail(actual, expected, message, "deepEqual", assert.deepEqual);
	}
};

function _deepEqual(actual, expected) {
	// 7.1. All identical values are equivalent, as determined by ===.
	if (actual === expected) {
		return true;
	} else if (util.isBuffer(actual) && util.isBuffer(expected)) {
		if (actual.length != expected.length) return false;

		for (var i = 0; i < actual.length; i++) {
			if (actual[i] !== expected[i]) return false;
		}

		return true;

		// 7.2. If the expected value is a Date object, the actual value is
		// equivalent if it is also a Date object that refers to the same time.
	} else if (util.isDate(actual) && util.isDate(expected)) {
		return actual.getTime() === expected.getTime();

		// 7.3 If the expected value is a RegExp object, the actual value is
		// equivalent if it is also a RegExp object with the same source and
		// properties (`global`, `multiline`, `lastIndex`, `ignoreCase`).
	} else if (util.isRegExp(actual) && util.isRegExp(expected)) {
		return (
			actual.source === expected.source &&
			actual.global === expected.global &&
			actual.multiline === expected.multiline &&
			actual.lastIndex === expected.lastIndex &&
			actual.ignoreCase === expected.ignoreCase
		);

		// 7.4. Other pairs that do not both pass typeof value == 'object',
		// equivalence is determined by ==.
	} else if (!util.isObject(actual) && !util.isObject(expected)) {
		return actual == expected;

		// 7.5 For all other Object pairs, including Array objects, equivalence is
		// determined by having the same number of owned properties (as verified
		// with Object.prototype.hasOwnProperty.call), the same set of keys
		// (although not necessarily the same order), equivalent values for every
		// corresponding key, and an identical 'prototype' property. Note: this
		// accounts for both named and indexed properties on Arrays.
	} else {
		return objEquiv(actual, expected);
	}
}

function isArguments(object) {
	return Object.prototype.toString.call(object) == "[object Arguments]";
}

function objEquiv(a, b) {
	if (util.isNullOrUndefined(a) || util.isNullOrUndefined(b)) return false;
	// an identical 'prototype' property.
	if (a.prototype !== b.prototype) return false;
	// if one is a primitive, the other must be same
	if (util.isPrimitive(a) || util.isPrimitive(b)) {
		return a === b;
	}
	var aIsArgs = isArguments(a),
		bIsArgs = isArguments(b);
	if ((aIsArgs && !bIsArgs) || (!aIsArgs && bIsArgs)) return false;
	if (aIsArgs) {
		a = pSlice.call(a);
		b = pSlice.call(b);
		return _deepEqual(a, b);
	}
	var ka = objectKeys(a),
		kb = objectKeys(b),
		key,
		i;
	// having the same number of owned properties (keys incorporates
	// hasOwnProperty)
	if (ka.length != kb.length) return false;
	//the same set of keys (although not necessarily the same order),
	ka.sort();
	kb.sort();
	//~~~cheap key test
	for (i = ka.length - 1; i >= 0; i--) {
		if (ka[i] != kb[i]) return false;
	}
	//equivalent values for every corresponding key, and
	//~~~possibly expensive deep test
	for (i = ka.length - 1; i >= 0; i--) {
		key = ka[i];
		if (!_deepEqual(a[key], b[key])) return false;
	}
	return true;
}

// 8. The non-equivalence assertion tests for any deep inequality.
// assert.notDeepEqual(actual, expected, message_opt);

assert.notDeepEqual = function notDeepEqual(actual, expected, message) {
	if (_deepEqual(actual, expected)) {
		fail(actual, expected, message, "notDeepEqual", assert.notDeepEqual);
	}
};

// 9. The strict equality assertion tests strict equality, as determined by ===.
// assert.strictEqual(actual, expected, message_opt);

assert.strictEqual = function strictEqual(actual, expected, message) {
	if (actual !== expected) {
		fail(actual, expected, message, "===", assert.strictEqual);
	}
};

// 10. The strict non-equality assertion tests for strict inequality, as
// determined by !==.  assert.notStrictEqual(actual, expected, message_opt);

assert.notStrictEqual = function notStrictEqual(actual, expected, message) {
	if (actual === expected) {
		fail(actual, expected, message, "!==", assert.notStrictEqual);
	}
};

function expectedException(actual, expected) {
	if (!actual || !expected) {
		return false;
	}

	if (Object.prototype.toString.call(expected) == "[object RegExp]") {
		return expected.test(actual);
	} else if (actual instanceof expected) {
		return true;
	} else if (expected.call({}, actual) === true) {
		return true;
	}

	return false;
}

function _throws(shouldThrow, block, expected, message) {
	var actual;

	if (util.isString(expected)) {
		message = expected;
		expected = null;
	}

	try {
		block();
	} catch (e) {
		actual = e;
	}

	message = (expected && expected.name ? " (" + expected.name + ")." : ".") + (message ? " " + message : ".");

	if (shouldThrow && !actual) {
		fail(actual, expected, "Missing expected exception" + message);
	}

	if (!shouldThrow && expectedException(actual, expected)) {
		fail(actual, expected, "Got unwanted exception" + message);
	}

	if ((shouldThrow && actual && expected && !expectedException(actual, expected)) || (!shouldThrow && actual)) {
		throw actual;
	}
}

// 11. Expected to throw an error:
// assert.throws(block, Error_opt, message_opt);

assert.throws = function(block, /*optional*/ error, /*optional*/ message) {
	_throws.apply(this, [true].concat(pSlice.call(arguments)));
};

// EXTENSION! This is annoying to write outside this module.
assert.doesNotThrow = function(block, /*optional*/ message) {
	_throws.apply(this, [false].concat(pSlice.call(arguments)));
};

assert.ifError = function(err) {
	if (err) {
		throw err;
	}
};

var objectKeys =
	Object.keys ||
	function(obj) {
		var keys = [];
		for (var key in obj) {
			if (hasOwn.call(obj, key)) keys.push(key);
		}
		return keys;
	};

});
return ___scope___.entry = "index.js";
});
FuseBox.pkg("fs", {}, function(___scope___){
___scope___.file("index.js", function(exports, require, module, __filename, __dirname){

if (FuseBox.isServer) {
	module.exports = global.require("fs");
} else {
	module.exports = {};
}

});
return ___scope___.entry = "index.js";
});
FuseBox.pkg("process", {}, function(___scope___){
___scope___.file("index.js", function(exports, require, module, __filename, __dirname){

// From https://github.com/defunctzombie/node-process/blob/master/browser.js
// shim for using process in browser
if (FuseBox.isServer) {
	if (typeof __process_env__ !== "undefined") {
		Object.assign(global.process.env, __process_env__);
	}
	module.exports = global.process;
} else {
	// Object assign polyfill
	if (typeof Object.assign != "function") {
		Object.assign = function(target, varArgs) {
			// .length of function is 2
			"use strict";
			if (target == null) {
				// TypeError if undefined or null
				throw new TypeError("Cannot convert undefined or null to object");
			}

			var to = Object(target);

			for (var index = 1; index < arguments.length; index++) {
				var nextSource = arguments[index];

				if (nextSource != null) {
					// Skip over if undefined or null
					for (var nextKey in nextSource) {
						// Avoid bugs when hasOwnProperty is shadowed
						if (Object.prototype.hasOwnProperty.call(nextSource, nextKey)) {
							to[nextKey] = nextSource[nextKey];
						}
					}
				}
			}
			return to;
		};
	}

	var productionEnv = false; //require('@system-env').production;

	var process = (module.exports = {});
	var queue = [];
	var draining = false;
	var currentQueue;
	var queueIndex = -1;

	function cleanUpNextTick() {
		draining = false;
		if (currentQueue.length) {
			queue = currentQueue.concat(queue);
		} else {
			queueIndex = -1;
		}
		if (queue.length) {
			drainQueue();
		}
	}

	function drainQueue() {
		if (draining) {
			return;
		}
		var timeout = setTimeout(cleanUpNextTick);
		draining = true;

		var len = queue.length;
		while (len) {
			currentQueue = queue;
			queue = [];
			while (++queueIndex < len) {
				if (currentQueue) {
					currentQueue[queueIndex].run();
				}
			}
			queueIndex = -1;
			len = queue.length;
		}
		currentQueue = null;
		draining = false;
		clearTimeout(timeout);
	}

	process.nextTick = function(fun) {
		var args = new Array(arguments.length - 1);
		if (arguments.length > 1) {
			for (var i = 1; i < arguments.length; i++) {
				args[i - 1] = arguments[i];
			}
		}
		queue.push(new Item(fun, args));
		if (queue.length === 1 && !draining) {
			setTimeout(drainQueue, 0);
		}
	};

	// v8 likes predictible objects
	function Item(fun, array) {
		this.fun = fun;
		this.array = array;
	}
	Item.prototype.run = function() {
		this.fun.apply(null, this.array);
	};
	process.title = "browser";
	process.browser = true;
	process.env = {
		NODE_ENV: productionEnv ? "production" : "development"
	};
	if (typeof __process_env__ !== "undefined") {
		Object.assign(process.env, __process_env__);
	}
	process.argv = [];
	process.version = ""; // empty string to avoid regexp issues
	process.versions = {};

	function noop() {}

	process.on = noop;
	process.addListener = noop;
	process.once = noop;
	process.off = noop;
	process.removeListener = noop;
	process.removeAllListeners = noop;
	process.emit = noop;

	process.binding = function(name) {
		throw new Error("process.binding is not supported");
	};

	process.cwd = function() {
		return "/";
	};
	process.chdir = function(dir) {
		throw new Error("process.chdir is not supported");
	};
	process.umask = function() {
		return 0;
	};
}

});
return ___scope___.entry = "index.js";
});
FuseBox.pkg("util", {}, function(___scope___){
___scope___.file("index.js", function(exports, require, module, __filename, __dirname){

/*
 * Fork of https://raw.githubusercontent.com/defunctzombie/node-util
 * inlining https://github.com/isaacs/inherits/blob/master/inherits_browser.js
 */

// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

var process = require("process");

var formatRegExp = /%[sdj%]/g;
exports.format = function(f) {
	if (!isString(f)) {
		var objects = [];
		for (var i = 0; i < arguments.length; i++) {
			objects.push(inspect(arguments[i]));
		}
		return objects.join(" ");
	}

	var i = 1;
	var args = arguments;
	var len = args.length;
	var str = String(f).replace(formatRegExp, function(x) {
		if (x === "%%") return "%";
		if (i >= len) return x;
		switch (x) {
			case "%s":
				return String(args[i++]);
			case "%d":
				return Number(args[i++]);
			case "%j":
				try {
					return JSON.stringify(args[i++]);
				} catch (_) {
					return "[Circular]";
				}
			default:
				return x;
		}
	});
	for (var x = args[i]; i < len; x = args[++i]) {
		if (isNull(x) || !isObject(x)) {
			str += " " + x;
		} else {
			str += " " + inspect(x);
		}
	}
	return str;
};

// Mark that a method should not be used.
// Returns a modified function which warns once by default.
// If --no-deprecation is set, then it is a no-op.
exports.deprecate = function(fn, msg) {
	// Allow for deprecating things in the process of starting up.
	if (isUndefined(global.process)) {
		return function() {
			return exports.deprecate(fn, msg).apply(this, arguments);
		};
	}

	if (process.noDeprecation === true) {
		return fn;
	}

	var warned = false;
	function deprecated() {
		if (!warned) {
			if (process.throwDeprecation) {
				throw new Error(msg);
			} else if (process.traceDeprecation) {
				console.trace(msg);
			} else {
				console.error(msg);
			}
			warned = true;
		}
		return fn.apply(this, arguments);
	}

	return deprecated;
};

var debugs = {};
var debugEnviron;
exports.debuglog = function(set) {
	if (isUndefined(debugEnviron)) debugEnviron = process.env.NODE_DEBUG || "";
	set = set.toUpperCase();
	if (!debugs[set]) {
		if (new RegExp("\\b" + set + "\\b", "i").test(debugEnviron)) {
			var pid = process.pid;
			debugs[set] = function() {
				var msg = exports.format.apply(exports, arguments);
				console.error("%s %d: %s", set, pid, msg);
			};
		} else {
			debugs[set] = function() {};
		}
	}
	return debugs[set];
};

/**
 * Echos the value of a value. Trys to print the value out
 * in the best way possible given the different types.
 *
 * @param {Object} obj The object to print out.
 * @param {Object} opts Optional options object that alters the output.
 */
/* legacy: obj, showHidden, depth, colors*/
function inspect(obj, opts) {
	// default options
	var ctx = {
		seen: [],
		stylize: stylizeNoColor
	};
	// legacy...
	if (arguments.length >= 3) ctx.depth = arguments[2];
	if (arguments.length >= 4) ctx.colors = arguments[3];
	if (isBoolean(opts)) {
		// legacy...
		ctx.showHidden = opts;
	} else if (opts) {
		// got an "options" object
		exports._extend(ctx, opts);
	}
	// set default options
	if (isUndefined(ctx.showHidden)) ctx.showHidden = false;
	if (isUndefined(ctx.depth)) ctx.depth = 2;
	if (isUndefined(ctx.colors)) ctx.colors = false;
	if (isUndefined(ctx.customInspect)) ctx.customInspect = true;
	if (ctx.colors) ctx.stylize = stylizeWithColor;
	return formatValue(ctx, obj, ctx.depth);
}
exports.inspect = inspect;

// http://en.wikipedia.org/wiki/ANSI_escape_code#graphics
inspect.colors = {
	bold: [1, 22],
	italic: [3, 23],
	underline: [4, 24],
	inverse: [7, 27],
	white: [37, 39],
	grey: [90, 39],
	black: [30, 39],
	blue: [34, 39],
	cyan: [36, 39],
	green: [32, 39],
	magenta: [35, 39],
	red: [31, 39],
	yellow: [33, 39]
};

// Don't use 'blue' not visible on cmd.exe
inspect.styles = {
	special: "cyan",
	number: "yellow",
	boolean: "yellow",
	undefined: "grey",
	null: "bold",
	string: "green",
	date: "magenta",
	// "name": intentionally not styling
	regexp: "red"
};

function stylizeWithColor(str, styleType) {
	var style = inspect.styles[styleType];

	if (style) {
		return "\u001b[" + inspect.colors[style][0] + "m" + str + "\u001b[" + inspect.colors[style][1] + "m";
	} else {
		return str;
	}
}

function stylizeNoColor(str, styleType) {
	return str;
}

function arrayToHash(array) {
	var hash = {};

	array.forEach(function(val, idx) {
		hash[val] = true;
	});

	return hash;
}

function formatValue(ctx, value, recurseTimes) {
	// Provide a hook for user-specified inspect functions.
	// Check that value is an object with an inspect function on it
	if (
		ctx.customInspect &&
		value &&
		isFunction(value.inspect) &&
		// Filter out the util module, it's inspect function is special
		value.inspect !== exports.inspect &&
		// Also filter out any prototype objects using the circular check.
		!(value.constructor && value.constructor.prototype === value)
	) {
		var ret = value.inspect(recurseTimes, ctx);
		if (!isString(ret)) {
			ret = formatValue(ctx, ret, recurseTimes);
		}
		return ret;
	}

	// Primitive types cannot have properties
	var primitive = formatPrimitive(ctx, value);
	if (primitive) {
		return primitive;
	}

	// Look up the keys of the object.
	var keys = Object.keys(value);
	var visibleKeys = arrayToHash(keys);

	if (ctx.showHidden) {
		keys = Object.getOwnPropertyNames(value);
	}

	// IE doesn't make error fields non-enumerable
	// http://msdn.microsoft.com/en-us/library/ie/dww52sbt(v=vs.94).aspx
	if (isError(value) && (keys.indexOf("message") >= 0 || keys.indexOf("description") >= 0)) {
		return formatError(value);
	}

	// Some type of object without properties can be shortcutted.
	if (keys.length === 0) {
		if (isFunction(value)) {
			var name = value.name ? ": " + value.name : "";
			return ctx.stylize("[Function" + name + "]", "special");
		}
		if (isRegExp(value)) {
			return ctx.stylize(RegExp.prototype.toString.call(value), "regexp");
		}
		if (isDate(value)) {
			return ctx.stylize(Date.prototype.toString.call(value), "date");
		}
		if (isError(value)) {
			return formatError(value);
		}
	}

	var base = "",
		array = false,
		braces = ["{", "}"];

	// Make Array say that they are Array
	if (isArray(value)) {
		array = true;
		braces = ["[", "]"];
	}

	// Make functions say that they are functions
	if (isFunction(value)) {
		var n = value.name ? ": " + value.name : "";
		base = " [Function" + n + "]";
	}

	// Make RegExps say that they are RegExps
	if (isRegExp(value)) {
		base = " " + RegExp.prototype.toString.call(value);
	}

	// Make dates with properties first say the date
	if (isDate(value)) {
		base = " " + Date.prototype.toUTCString.call(value);
	}

	// Make error with message first say the error
	if (isError(value)) {
		base = " " + formatError(value);
	}

	if (keys.length === 0 && (!array || value.length == 0)) {
		return braces[0] + base + braces[1];
	}

	if (recurseTimes < 0) {
		if (isRegExp(value)) {
			return ctx.stylize(RegExp.prototype.toString.call(value), "regexp");
		} else {
			return ctx.stylize("[Object]", "special");
		}
	}

	ctx.seen.push(value);

	var output;
	if (array) {
		output = formatArray(ctx, value, recurseTimes, visibleKeys, keys);
	} else {
		output = keys.map(function(key) {
			return formatProperty(ctx, value, recurseTimes, visibleKeys, key, array);
		});
	}

	ctx.seen.pop();

	return reduceToSingleString(output, base, braces);
}

function formatPrimitive(ctx, value) {
	if (isUndefined(value)) return ctx.stylize("undefined", "undefined");
	if (isString(value)) {
		var simple =
			"'" +
			JSON.stringify(value)
				.replace(/^"|"$/g, "")
				.replace(/'/g, "\\'")
				.replace(/\\"/g, '"') +
			"'";
		return ctx.stylize(simple, "string");
	}
	if (isNumber(value)) return ctx.stylize("" + value, "number");
	if (isBoolean(value)) return ctx.stylize("" + value, "boolean");
	// For some reason typeof null is "object", so special case here.
	if (isNull(value)) return ctx.stylize("null", "null");
}

function formatError(value) {
	return "[" + Error.prototype.toString.call(value) + "]";
}

function formatArray(ctx, value, recurseTimes, visibleKeys, keys) {
	var output = [];
	for (var i = 0, l = value.length; i < l; ++i) {
		if (hasOwnProperty(value, String(i))) {
			output.push(formatProperty(ctx, value, recurseTimes, visibleKeys, String(i), true));
		} else {
			output.push("");
		}
	}
	keys.forEach(function(key) {
		if (!key.match(/^\d+$/)) {
			output.push(formatProperty(ctx, value, recurseTimes, visibleKeys, key, true));
		}
	});
	return output;
}

function formatProperty(ctx, value, recurseTimes, visibleKeys, key, array) {
	var name, str, desc;
	desc = Object.getOwnPropertyDescriptor(value, key) || { value: value[key] };
	if (desc.get) {
		if (desc.set) {
			str = ctx.stylize("[Getter/Setter]", "special");
		} else {
			str = ctx.stylize("[Getter]", "special");
		}
	} else {
		if (desc.set) {
			str = ctx.stylize("[Setter]", "special");
		}
	}
	if (!hasOwnProperty(visibleKeys, key)) {
		name = "[" + key + "]";
	}
	if (!str) {
		if (ctx.seen.indexOf(desc.value) < 0) {
			if (isNull(recurseTimes)) {
				str = formatValue(ctx, desc.value, null);
			} else {
				str = formatValue(ctx, desc.value, recurseTimes - 1);
			}
			if (str.indexOf("\n") > -1) {
				if (array) {
					str = str
						.split("\n")
						.map(function(line) {
							return "  " + line;
						})
						.join("\n")
						.substr(2);
				} else {
					str =
						"\n" +
						str
							.split("\n")
							.map(function(line) {
								return "   " + line;
							})
							.join("\n");
				}
			}
		} else {
			str = ctx.stylize("[Circular]", "special");
		}
	}
	if (isUndefined(name)) {
		if (array && key.match(/^\d+$/)) {
			return str;
		}
		name = JSON.stringify("" + key);
		if (name.match(/^"([a-zA-Z_][a-zA-Z_0-9]*)"$/)) {
			name = name.substr(1, name.length - 2);
			name = ctx.stylize(name, "name");
		} else {
			name = name
				.replace(/'/g, "\\'")
				.replace(/\\"/g, '"')
				.replace(/(^"|"$)/g, "'");
			name = ctx.stylize(name, "string");
		}
	}

	return name + ": " + str;
}

function reduceToSingleString(output, base, braces) {
	var numLinesEst = 0;
	var length = output.reduce(function(prev, cur) {
		numLinesEst++;
		if (cur.indexOf("\n") >= 0) numLinesEst++;
		return prev + cur.replace(/\u001b\[\d\d?m/g, "").length + 1;
	}, 0);

	if (length > 60) {
		return braces[0] + (base === "" ? "" : base + "\n ") + " " + output.join(",\n  ") + " " + braces[1];
	}

	return braces[0] + base + " " + output.join(", ") + " " + braces[1];
}

// NOTE: These type checking functions intentionally don't use `instanceof`
// because it is fragile and can be easily faked with `Object.create()`.
function isArray(ar) {
	return Array.isArray(ar);
}
exports.isArray = isArray;

function isBoolean(arg) {
	return typeof arg === "boolean";
}
exports.isBoolean = isBoolean;

function isNull(arg) {
	return arg === null;
}
exports.isNull = isNull;

function isNullOrUndefined(arg) {
	return arg == null;
}
exports.isNullOrUndefined = isNullOrUndefined;

function isNumber(arg) {
	return typeof arg === "number";
}
exports.isNumber = isNumber;

function isString(arg) {
	return typeof arg === "string";
}
exports.isString = isString;

function isSymbol(arg) {
	return typeof arg === "symbol";
}
exports.isSymbol = isSymbol;

function isUndefined(arg) {
	return arg === void 0;
}
exports.isUndefined = isUndefined;

function isRegExp(re) {
	return isObject(re) && objectToString(re) === "[object RegExp]";
}
exports.isRegExp = isRegExp;

function isObject(arg) {
	return typeof arg === "object" && arg !== null;
}
exports.isObject = isObject;

function isDate(d) {
	return isObject(d) && objectToString(d) === "[object Date]";
}
exports.isDate = isDate;

function isError(e) {
	return isObject(e) && (objectToString(e) === "[object Error]" || e instanceof Error);
}
exports.isError = isError;

function isFunction(arg) {
	return typeof arg === "function";
}
exports.isFunction = isFunction;

function isPrimitive(arg) {
	return (
		arg === null ||
		typeof arg === "boolean" ||
		typeof arg === "number" ||
		typeof arg === "string" ||
		typeof arg === "symbol" || // ES6 symbol
		typeof arg === "undefined"
	);
}
exports.isPrimitive = isPrimitive;

exports.isBuffer = require("./isBuffer.js");

function objectToString(o) {
	return Object.prototype.toString.call(o);
}

function pad(n) {
	return n < 10 ? "0" + n.toString(10) : n.toString(10);
}

var months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];

// 26 Feb 16:19:34
function timestamp() {
	var d = new Date();
	var time = [pad(d.getHours()), pad(d.getMinutes()), pad(d.getSeconds())].join(":");
	return [d.getDate(), months[d.getMonth()], time].join(" ");
}

// log is just a thin wrapper to console.log that prepends a timestamp
exports.log = function() {
	console.log("%s - %s", timestamp(), exports.format.apply(exports, arguments));
};

/**
 * Inherit the prototype methods from one constructor into another.
 *
 * The Function.prototype.inherits from lang.js rewritten as a standalone
 * function (not on Function.prototype). NOTE: If this file is to be loaded
 * during bootstrapping this function needs to be rewritten using some native
 * functions as prototype setup using normal JavaScript does not work as
 * expected during bootstrapping (see mirror.js in r114903).
 *
 * @param {function} ctor Constructor function which needs to inherit the
 *     prototype.
 * @param {function} superCtor Constructor function to inherit prototype from.
 */
if (typeof Object.create === "function") {
	// implementation from standard node.js 'util' module
	exports.inherits = function inherits(ctor, superCtor) {
		ctor.super_ = superCtor;
		ctor.prototype = Object.create(superCtor.prototype, {
			constructor: {
				value: ctor,
				enumerable: false,
				writable: true,
				configurable: true
			}
		});
	};
} else {
	// old school shim for old browsers
	exports.inherits = function inherits(ctor, superCtor) {
		ctor.super_ = superCtor;
		var TempCtor = function() {};
		TempCtor.prototype = superCtor.prototype;
		ctor.prototype = new TempCtor();
		ctor.prototype.constructor = ctor;
	};
}

exports._extend = function(origin, add) {
	// Don't do anything if add isn't an object
	if (!add || !isObject(add)) return origin;

	var keys = Object.keys(add);
	var i = keys.length;
	while (i--) {
		origin[keys[i]] = add[keys[i]];
	}
	return origin;
};

function hasOwnProperty(obj, prop) {
	return Object.prototype.hasOwnProperty.call(obj, prop);
}

});
___scope___.file("isBuffer.js", function(exports, require, module, __filename, __dirname){

/*
 * From https://github.com/defunctzombie/node-util/blob/master/support/isBuffer.js
 */
module.exports = function isBuffer(arg) {
	if (typeof Buffer !== "undefined") {
		return arg instanceof Buffer;
	} else {
		return arg && typeof arg === "object" && typeof arg.copy === "function" && typeof arg.fill === "function" && typeof arg.readUInt8 === "function";
	}
};

});
return ___scope___.entry = "index.js";
});

FuseBox.import("default/nodeCLI.js");
FuseBox.main("default/nodeCLI.js");
})
(function(e){function r(e){var r=e.charCodeAt(0),n=e.charCodeAt(1);if((m||58!==n)&&(r>=97&&r<=122||64===r)){if(64===r){var t=e.split("/"),i=t.splice(2,t.length).join("/");return[t[0]+"/"+t[1],i||void 0]}var o=e.indexOf("/");if(o===-1)return[e];var a=e.substring(0,o),f=e.substring(o+1);return[a,f]}}function n(e){return e.substring(0,e.lastIndexOf("/"))||"./"}function t(){for(var e=[],r=0;r<arguments.length;r++)e[r]=arguments[r];for(var n=[],t=0,i=arguments.length;t<i;t++)n=n.concat(arguments[t].split("/"));for(var o=[],t=0,i=n.length;t<i;t++){var a=n[t];a&&"."!==a&&(".."===a?o.pop():o.push(a))}return""===n[0]&&o.unshift(""),o.join("/")||(o.length?"/":".")}function i(e){var r=e.match(/\.(\w{1,})$/);return r&&r[1]?e:e+".js"}function o(e){if(m){var r,n=document,t=n.getElementsByTagName("head")[0];/\.css$/.test(e)?(r=n.createElement("link"),r.rel="stylesheet",r.type="text/css",r.href=e):(r=n.createElement("script"),r.type="text/javascript",r.src=e,r.async=!0),t.insertBefore(r,t.firstChild)}}function a(e,r){for(var n in e)e.hasOwnProperty(n)&&r(n,e[n])}function f(e){return{server:require(e)}}function u(e,n){var o=n.path||"./",a=n.pkg||"default",u=r(e);if(u&&(o="./",a=u[0],n.v&&n.v[a]&&(a=a+"@"+n.v[a]),e=u[1]),e)if(126===e.charCodeAt(0))e=e.slice(2,e.length),o="./";else if(!m&&(47===e.charCodeAt(0)||58===e.charCodeAt(1)))return f(e);var s=x[a];if(!s){if(m&&"electron"!==_.target)throw"Package not found "+a;return f(a+(e?"/"+e:""))}e=e?e:"./"+s.s.entry;var l,d=t(o,e),c=i(d),p=s.f[c];return!p&&c.indexOf("*")>-1&&(l=c),p||l||(c=t(d,"/","index.js"),p=s.f[c],p||"."!==d||(c=s.s&&s.s.entry||"index.js",p=s.f[c]),p||(c=d+".js",p=s.f[c]),p||(p=s.f[d+".jsx"]),p||(c=d+"/index.jsx",p=s.f[c])),{file:p,wildcard:l,pkgName:a,versions:s.v,filePath:d,validPath:c}}function s(e,r,n){if(void 0===n&&(n={}),!m)return r(/\.(js|json)$/.test(e)?h.require(e):"");if(n&&n.ajaxed===e)return console.error(e,"does not provide a module");var i=new XMLHttpRequest;i.onreadystatechange=function(){if(4==i.readyState)if(200==i.status){var n=i.getResponseHeader("Content-Type"),o=i.responseText;/json/.test(n)?o="module.exports = "+o:/javascript/.test(n)||(o="module.exports = "+JSON.stringify(o));var a=t("./",e);_.dynamic(a,o),r(_.import(e,{ajaxed:e}))}else console.error(e,"not found on request"),r(void 0)},i.open("GET",e,!0),i.send()}function l(e,r){var n=y[e];if(n)for(var t in n){var i=n[t].apply(null,r);if(i===!1)return!1}}function d(e){if(null!==e&&["function","object","array"].indexOf(typeof e)!==-1&&!e.hasOwnProperty("default"))return Object.isFrozen(e)?void(e.default=e):void Object.defineProperty(e,"default",{value:e,writable:!0,enumerable:!1})}function c(e,r){if(void 0===r&&(r={}),58===e.charCodeAt(4)||58===e.charCodeAt(5))return o(e);var t=u(e,r);if(t.server)return t.server;var i=t.file;if(t.wildcard){var a=new RegExp(t.wildcard.replace(/\*/g,"@").replace(/[.?*+^$[\]\\(){}|-]/g,"\\$&").replace(/@@/g,".*").replace(/@/g,"[a-z0-9$_-]+"),"i"),f=x[t.pkgName];if(f){var p={};for(var v in f.f)a.test(v)&&(p[v]=c(t.pkgName+"/"+v));return p}}if(!i){var g="function"==typeof r,y=l("async",[e,r]);if(y===!1)return;return s(e,function(e){return g?r(e):null},r)}var w=t.pkgName;if(i.locals&&i.locals.module)return i.locals.module.exports;var b=i.locals={},j=n(t.validPath);b.exports={},b.module={exports:b.exports},b.require=function(e,r){var n=c(e,{pkg:w,path:j,v:t.versions});return _.sdep&&d(n),n},m||!h.require.main?b.require.main={filename:"./",paths:[]}:b.require.main=h.require.main;var k=[b.module.exports,b.require,b.module,t.validPath,j,w];return l("before-import",k),i.fn.apply(k[0],k),l("after-import",k),b.module.exports}if(e.FuseBox)return e.FuseBox;var p="undefined"!=typeof ServiceWorkerGlobalScope,v="undefined"!=typeof WorkerGlobalScope,m="undefined"!=typeof window&&"undefined"!=typeof window.navigator||v||p,h=m?v||p?{}:window:global;m&&(h.global=v||p?{}:window),e=m&&"undefined"==typeof __fbx__dnm__?e:module.exports;var g=m?v||p?{}:window.__fsbx__=window.__fsbx__||{}:h.$fsbx=h.$fsbx||{};m||(h.require=require);var x=g.p=g.p||{},y=g.e=g.e||{},_=function(){function r(){}return r.global=function(e,r){return void 0===r?h[e]:void(h[e]=r)},r.import=function(e,r){return c(e,r)},r.on=function(e,r){y[e]=y[e]||[],y[e].push(r)},r.exists=function(e){try{var r=u(e,{});return void 0!==r.file}catch(e){return!1}},r.remove=function(e){var r=u(e,{}),n=x[r.pkgName];n&&n.f[r.validPath]&&delete n.f[r.validPath]},r.main=function(e){return this.mainFile=e,r.import(e,{})},r.expose=function(r){var n=function(n){var t=r[n].alias,i=c(r[n].pkg);"*"===t?a(i,function(r,n){return e[r]=n}):"object"==typeof t?a(t,function(r,n){return e[n]=i[r]}):e[t]=i};for(var t in r)n(t)},r.dynamic=function(r,n,t){this.pkg(t&&t.pkg||"default",{},function(t){t.file(r,function(r,t,i,o,a){var f=new Function("__fbx__dnm__","exports","require","module","__filename","__dirname","__root__",n);f(!0,r,t,i,o,a,e)})})},r.flush=function(e){var r=x.default;for(var n in r.f)e&&!e(n)||delete r.f[n].locals},r.pkg=function(e,r,n){if(x[e])return n(x[e].s);var t=x[e]={};return t.f={},t.v=r,t.s={file:function(e,r){return t.f[e]={fn:r}}},n(t.s)},r.addPlugin=function(e){this.plugins.push(e)},r.packages=x,r.isBrowser=m,r.isServer=!m,r.plugins=[],r}();return m||(h.FuseBox=_),e.FuseBox=_}(this))
//# sourceMappingURL=IVGFontConverter.node.js.map?tm=1654422719104