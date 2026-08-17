const fs = require('fs');
const path = require('path');

const projectRoot = path.resolve(__dirname, '..', '..');
const sourcePath = path.join(projectRoot, 'report', 'work', 'renesas-cup-optimized.glb');
const outputPath = path.join(projectRoot, 'report', 'web', 'assets', 'model', 'renesas-cup-data.js');
const encoded = fs.readFileSync(sourcePath).toString('base64');
const output = `/* Optimized embedded Renesas Cup vehicle assembly. */\nwindow.__EMBEDDED_GLB__ = {\n  "glb:renesas-cup": ${JSON.stringify(encoded)}\n};\n`;

fs.writeFileSync(outputPath, output);
process.stdout.write(`Embedded optimized GLB (${Buffer.byteLength(output)} bytes).\n`);
