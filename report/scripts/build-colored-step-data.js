const fs = require('fs');
const path = require('path');

const projectRoot = path.resolve(__dirname, '..', '..');
const occtFactory = require(path.join(projectRoot, '.codex-step-converter', 'node_modules', 'occt-import-js'));
const sourcePath = path.join(projectRoot, 'report', 'web', 'assets', 'model', 'vehicle-boards', 'source-step', 'sandwich-board-v3.step');
const outputPath = path.join(projectRoot, 'report', 'web', 'assets', 'model', 'colored-model-data.js');

const MODEL_KEY = 'colored:sandwich-board-v3';

function classifyColor(name, nativeColor) {
  if (nativeColor) {
    return `#${nativeColor.map((value) => Math.round(value * 255).toString(16).padStart(2, '0')).join('')}`;
  }
  const upper = name.toUpperCase();
  if (upper.startsWith('LED')) return '#e33b45';
  if (upper.startsWith('C0603') || upper.startsWith('C0805')) return '#c7a56b';
  if (upper.startsWith('R0603') || upper.startsWith('R0805')) return '#806441';
  if (upper.includes('RELAY')) return '#2965a8';
  if (upper.includes('FPC')) return '#e8dfc5';
  if (upper.includes('HDR')) return '#202735';
  if (upper.includes('CONN-TH')) return '#e8edf4';
  if (upper.includes('WIRELM')) return '#287c55';
  if (upper.includes('COMM-SMD')) return '#3780b8';
  if (upper.includes('SMA_')) return '#333946';
  if (upper.includes('QFN') || upper.includes('SOT-')) return '#171b22';
  return '#66758a';
}

function normal(a, b, c) {
  const ux = b[0] - a[0];
  const uy = b[1] - a[1];
  const uz = b[2] - a[2];
  const vx = c[0] - a[0];
  const vy = c[1] - a[1];
  const vz = c[2] - a[2];
  let x = uy * vz - uz * vy;
  let y = uz * vx - ux * vz;
  let z = ux * vy - uy * vx;
  const length = Math.hypot(x, y, z) || 1;
  x /= length;
  y /= length;
  z /= length;
  return [x, y, z];
}

function writeBinaryStl(triangles, label) {
  const buffer = Buffer.alloc(84 + triangles.length * 50);
  buffer.write(`EasyEDA colored model: ${label}`.slice(0, 80), 0, 'ascii');
  buffer.writeUInt32LE(triangles.length, 80);
  triangles.forEach((triangle, triangleIndex) => {
    const offset = 84 + triangleIndex * 50;
    const faceNormal = normal(triangle[0], triangle[1], triangle[2]);
    [...faceNormal, ...triangle[0], ...triangle[1], ...triangle[2]].forEach((value, valueIndex) => {
      buffer.writeFloatLE(value, offset + valueIndex * 4);
    });
    buffer.writeUInt16LE(0, offset + 48);
  });
  return buffer;
}

(async () => {
  const occt = await occtFactory();
  const result = occt.ReadStepFile(fs.readFileSync(sourcePath), null);
  if (!result.success) throw new Error('STEP import failed.');

  const groups = new Map();
  result.meshes.forEach((mesh) => {
    const color = classifyColor(mesh.name || '', mesh.color);
    if (!groups.has(color)) groups.set(color, []);
    const triangles = groups.get(color);
    const positions = mesh.attributes.position.array;
    const indices = mesh.index.array;
    for (let index = 0; index < indices.length; index += 3) {
      triangles.push([indices[index], indices[index + 1], indices[index + 2]].map((vertexIndex) => [
        positions[vertexIndex * 3],
        positions[vertexIndex * 3 + 1],
        positions[vertexIndex * 3 + 2],
      ]));
    }
  });

  const segments = [...groups.entries()].map(([color, triangles]) => ({
    color,
    data: writeBinaryStl(triangles, color).toString('base64'),
  }));
  const output = `/* Generated from the EasyEDA STEP assembly. */\nwindow.__COLORED_STL__ = {\n  ${JSON.stringify(MODEL_KEY)}: ${JSON.stringify(segments)}\n};\n`;
  fs.writeFileSync(outputPath, output);
  process.stdout.write(`Generated ${segments.length} color groups from ${result.meshes.length} solids (${Buffer.byteLength(output)} bytes).\n`);
})();
