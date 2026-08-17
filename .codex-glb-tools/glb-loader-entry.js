import { GLTFLoader } from 'three/examples/jsm/loaders/GLTFLoader.js';

window.__parseEmbeddedGLB__ = (arrayBuffer) => new Promise((resolve, reject) => {
  new GLTFLoader().parse(arrayBuffer, '', (gltf) => resolve(gltf.scene), reject);
});
