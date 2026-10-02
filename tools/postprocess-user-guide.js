#!/usr/bin/env node

const fs = require("fs");
const path = require("path");

function usage() {
  console.error("usage: postprocess-user-guide.js <guide-dir> <markdown-file> <docling-json-file>");
  process.exit(1);
}

function escapeRegExp(text) {
  return text.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

function readText(file) {
  return fs.readFileSync(file, "utf8");
}

function writeText(file, text) {
  fs.writeFileSync(file, text, "utf8");
}

function trim(text) {
  return text.replace(/^\s+/, "").replace(/\s+$/, "");
}

function cleanToc(block) {
  const items = [];
  const lines = block.split(/\n/);

  for (let i = 0; i < lines.length; ++i) {
    let line = lines[i];
    if (!/^\|/.test(line)) {
      continue;
    }
    if (/^\|[-\s|]+\|?$/.test(line)) {
      continue;
    }

    line = line.replace(/^\|/, "").replace(/\|$/, "");

    const cols = line.split("|").map(trim);
    let entry = trim(cols.filter((col) => col.length > 0).join(" "));
    if (entry.length === 0) {
      continue;
    }

    let page = "";
    const pageMatch = entry.match(/\.{3,}\s*(\d+)\s*$/);
    if (pageMatch) {
      page = pageMatch[1];
      entry = trim(entry.replace(/\.{3,}\s*\d+\s*$/, ""));
    }
    if (entry.length === 0) {
      continue;
    }

    items.push(page.length > 0 ? "- " + entry + ", p. " + page : "- " + entry);
  }

  return items.length > 0 ? items.join("\n") + "\n" : null;
}

function stripPrefixFromFile(file, prefix) {
  const text = readText(file);
  writeText(file, text.replace(new RegExp(escapeRegExp(prefix), "g"), ""));
}

function postprocessMarkdown(file) {
  let text = readText(file);
  const guideBase = path.basename(file, ".md");
  const artifactDir = guideBase + "_artifacts";
  const encodedArtifactDir = artifactDir.replace(/ /g, "%20");

  text = text.replace(new RegExp(escapeRegExp(artifactDir + "/"), "g"), encodedArtifactDir + "/");

  text = text.replace(
    /(## Table of Contents\n\n)([\s\S]*?)(\n## [^\n]+)/,
    function (match, before, tocBlock, after) {
      const cleaned = cleanToc(tocBlock);
      return cleaned === null ? match : before + cleaned + after;
    }
  );

  writeText(file, text);
}

function prunePageImages(guideDir, jsonFile) {
  const data = JSON.parse(readText(jsonFile));

  if (data.pages) {
    for (const page of Object.values(data.pages)) {
      delete page.image;
    }
  }

  const artifactDir = path.join(guideDir, path.basename(jsonFile, ".docling.json") + "_artifacts");
  if (fs.existsSync(artifactDir)) {
    for (const name of fs.readdirSync(artifactDir)) {
      if (/^page_\d+_.*\.png$/.test(name)) {
        fs.unlinkSync(path.join(artifactDir, name));
      }
    }
  }

  writeText(jsonFile, JSON.stringify(data, null, 2) + "\n");
}

if (process.argv.length !== 5) {
  usage();
}

const guideDir = path.resolve(process.argv[2]);
const markdownFile = process.argv[3];
const jsonFile = process.argv[4];
const guidePrefix = guideDir + path.sep;

stripPrefixFromFile(markdownFile, guidePrefix);
stripPrefixFromFile(jsonFile, guidePrefix);
postprocessMarkdown(markdownFile);
prunePageImages(guideDir, jsonFile);
