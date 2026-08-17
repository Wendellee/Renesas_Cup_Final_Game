const fs = require('fs');
const path = require('path');

const projectRoot = path.resolve(__dirname, '..', '..');
const sourceRoot = path.join(projectRoot, 'report', 'work', 'hardware-obj');
const webRoot = path.join(projectRoot, 'report', 'web');
const outputPath = path.join(webRoot, 'assets', 'model', 'indexed-model-data.js');

function parseMaterials(text) {
  const materials = [];
  let current = null;
  text.split(/\r?\n/).forEach((line) => {
    const parts = line.trim().split(/\s+/);
    if (parts[0] === 'newmtl') {
      current = { name: parts[1], kd: [0.65, 0.65, 0.65], ks: [0.1, 0.1, 0.1], opacity: 1 };
      materials.push(current);
    } else if (current && parts[0] === 'Kd') {
      current.kd = parts.slice(1, 4).map(Number);
    } else if (current && parts[0] === 'Ks') {
      current.ks = parts.slice(1, 4).map(Number);
    } else if (current && parts[0] === 'd') {
      current.opacity = Number(parts[1]);
    }
  });
  return materials;
}

function toHex(values) {
  return `#${values.map((value) => Math.round(Math.max(0, Math.min(1, value)) * 255).toString(16).padStart(2, '0')).join('')}`;
}

function parseModel(key) {
  const modelRoot = path.join(sourceRoot, key);
  const sourceFiles = fs.readdirSync(modelRoot).map((name) => path.join(modelRoot, name));
  const objPath = sourceFiles.find((name) => name.toLowerCase().endsWith('.obj'));
  const mtlPath = sourceFiles.find((name) => name.toLowerCase().endsWith('.mtl'));
  const materials = parseMaterials(fs.readFileSync(mtlPath, 'utf8'));
  const materialIndex = new Map(materials.map((material, index) => [material.name, index]));
  const vertices = [];
  const indices = [];
  const groups = [];
  let activeMaterial = 0;

  function appendTriangle(a, b, c) {
    indices.push(a, b, c);
    const previous = groups[groups.length - 1];
    if (previous && previous.materialIndex === activeMaterial && previous.start + previous.count === indices.length - 3) {
      previous.count += 3;
    } else {
      groups.push({ start: indices.length - 3, count: 3, materialIndex: activeMaterial });
    }
  }

  fs.readFileSync(objPath, 'utf8').split(/\r?\n/).forEach((line) => {
    if (line.startsWith('v ')) {
      const [, x, y, z] = line.trim().split(/\s+/);
      vertices.push(Number(x), Number(y), Number(z));
      return;
    }
    if (line.startsWith('usemtl ')) {
      activeMaterial = materialIndex.get(line.trim().slice(7)) ?? 0;
      return;
    }
    if (!line.startsWith('f ')) return;
    const face = line.trim().slice(2).split(/\s+/).map((token) => {
      const rawIndex = Number(token.split('/')[0]);
      return rawIndex < 0 ? vertices.length / 3 + rawIndex : rawIndex - 1;
    });
    for (let index = 1; index < face.length - 1; index += 1) appendTriangle(face[0], face[index], face[index + 1]);
  });

  const model = {
    presentation: 'top',
    positions: Buffer.from(new Float32Array(vertices).buffer).toString('base64'),
    indices: Buffer.from(new Uint32Array(indices).buffer).toString('base64'),
    materials: materials.map((material) => ({
      name: material.name,
      color: toHex(material.kd),
      specular: Math.max(...material.ks),
      opacity: material.opacity,
    })),
    groups,
  };
  process.stdout.write(`${key}: ${indices.length / 3} triangles, ${vertices.length / 3} vertices, ${materials.length} materials.\n`);
  return model;
}

const keys = ['ra8p1-baseboard', 'motor-driver', 'sandwich-v3', 'lcd-v3-1', 'ov5640', 'switch-board'];
const entries = keys.map((key) => `  ${JSON.stringify(`indexed:${key}`)}: ${JSON.stringify(parseModel(key))}`);
const output = `/* Generated from the EasyEDA OBJ + MTL exports. */\nwindow.__INDEXED_MODELS__ = {\n${entries.join(',\n')}\n};\n`;
fs.writeFileSync(outputPath, output);
process.stdout.write(`Generated ${keys.length} models (${Buffer.byteLength(output)} bytes).\n`);
