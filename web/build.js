import { minify } from 'html-minifier';
import fs from 'fs';

const html = fs.readFileSync('src/index.html').toString();
const minified = minify(html, {
    collapseBooleanAttributes: true,
    collapseWhitespace: true,
    decodeEntities: true,
    html5: true,
    minifyCSS: true,
    minifyJS: true,
    removeComments: true,
    removeEmptyAttributes: true,
    removeOptionalTags: true,
    removeRedundantAttributes: true,
    removeScriptTypeAttributes: true,
    removeStyleLinkTypeAttributes: true,
    useShortDoctype: true
});
console.log(minified);

const cppCode = `#pragma once

#ifndef __WEBPAGE__
#define __WEBPAGE__

const char *page_html = \"${minified.replace(/\\/g,"\\\\").replace(/\n/g,/\\n/).replace(/\"/g,"\\\"")}\";
#endif
`;

console.log(cppCode);
// Write to file

fs.writeFileSync("../include/webpage.h",cppCode);
console.log("done. Writed to /include/webpage.h");
