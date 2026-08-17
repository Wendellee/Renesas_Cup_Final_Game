(self.webpackChunk_N_E = self.webpackChunk_N_E || []).push([
  [9912],
  {
    991200: (module, exports, require) => {
      const THREE = require(8409);
      const rendererModule = require(818);
      const orbitModule = require(1843);
      const loaderModule = require(8249);

      document.querySelectorAll('[data-model-viewer]').forEach((viewer) => {
        const canvasHost = viewer.querySelector('[data-model-canvas]');
        const loading = viewer.querySelector('[data-model-loading]');
        const progress = viewer.querySelector('[data-model-progress]');
        const error = viewer.querySelector('[data-model-error]');
        const empty = viewer.querySelector('[data-model-empty]');
        if (!canvasHost || !viewer.dataset.modelSrc) return;

        const scene = new THREE.Z58();
        scene.background = new THREE.Q1f(0x070d1c);
        const camera = new THREE.ubm(38, 1, 0.1, 10000);
        const renderer = new rendererModule.JeP({ antialias: true, alpha: false });
        renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
        renderer.outputColorSpace = THREE.er$;
        canvasHost.appendChild(renderer.domElement);

        const controls = new orbitModule.N(camera, renderer.domElement);
        controls.enableDamping = true;
        const allowAutoRotate = !window.matchMedia('(prefers-reduced-motion: reduce)').matches;
        controls.autoRotate = allowAutoRotate;
        controls.autoRotateSpeed = 1.1;
        controls.enablePan = false;

        scene.add(new THREE.dth(0xe4efff, 0x111b35, 2.6));
        const keyLight = new THREE.ZyN(0x4da3ff, 4);
        keyLight.position.set(3, 5, 4);
        scene.add(keyLight);

        let meshes = [];
        let loadToken = 0;
        const decodeBase64 = (encoded) => {
          if (!encoded) return null;
          const binary = atob(encoded);
          const bytes = new Uint8Array(binary.length);
          for (let index = 0; index < binary.length; index += 1) {
            bytes[index] = binary.charCodeAt(index);
          }
          return bytes.buffer;
        };
        const decodeEmbeddedModel = (source) => decodeBase64(window.__EMBEDDED_STL__?.[source]);
        const disposeMeshes = () => {
          meshes.forEach((root) => {
            scene.remove(root);
            root.traverse((mesh) => {
              if (mesh.geometry) mesh.geometry.dispose();
              if (!mesh.material) return;
              const materials = Array.isArray(mesh.material) ? mesh.material : [mesh.material];
              materials.forEach((material) => {
                if (material.map) material.map.dispose();
                material.dispose();
              });
            });
          });
          meshes = [];
        };

        const loadModel = (source, textureSource = '') => {
          const currentToken = ++loadToken;
          const indexedModel = window.__INDEXED_MODELS__?.[source];
          const topPresentation = Boolean(textureSource) || indexedModel?.presentation === 'top';
          controls.autoRotate = allowAutoRotate && !topPresentation;
          loading.hidden = false;
          error.hidden = true;
          if (empty) empty.hidden = true;
          progress.textContent = '0%';
          disposeMeshes();

          const handleGeometries = (items) => {
              if (currentToken !== loadToken) {
                items.forEach((item) => item.geometry.dispose());
                return;
              }
              meshes = items.map(({ geometry, color, materials }) => {
                geometry.computeVertexNormals();
                const material = materials || new THREE._4j({
                  color: color || 0x94acd1,
                  metalness: color ? 0.18 : 0.42,
                  roughness: color ? 0.58 : 0.5,
                });
                const mesh = new THREE.eaF(geometry, material);
                mesh.rotation.x = -Math.PI / 2;
                scene.add(mesh);
                return mesh;
              });

              const bounds = new THREE.NRn();
              bounds.makeEmpty();
              meshes.forEach((mesh) => bounds.expandByObject(mesh));
              const center = bounds.getCenter(new THREE.Pq0());
              meshes.forEach((mesh) => mesh.position.sub(center));
              bounds.makeEmpty();
              meshes.forEach((mesh) => bounds.expandByObject(mesh));
              const size = bounds.getSize(new THREE.Pq0());
              const extent = Math.max(size.x, size.y, size.z);
              camera.position.set(
                topPresentation ? 0.08 * extent : 1.25 * extent,
                topPresentation ? 1.48 * extent : 0.85 * extent,
                topPresentation ? 0.12 * extent : 1.25 * extent,
              );
              camera.near = Math.max(extent / 1000, 0.01);
              camera.far = 20 * extent;
              camera.updateProjectionMatrix();
              controls.target.set(0, 0, 0);
              controls.update();
              if (!textureSource) {
                loading.hidden = true;
                return;
              }

              const image = new Image();
              image.onload = () => {
                if (currentToken !== loadToken) return;
                const texture = new THREE.gPd(image);
                texture.colorSpace = THREE.er$;
                texture.needsUpdate = true;
                const imageWidth = size.x * (1520 / 1294);
                const imageDepth = size.z * (740 / 722);
                const topGeometry = new THREE.bdM(imageWidth, imageDepth);
                const topMaterial = new THREE.V9B({ map: texture, side: THREE.$EB });
                const topSurface = new THREE.eaF(topGeometry, topMaterial);
                topSurface.rotation.x = -Math.PI / 2;
                topSurface.position.set(
                  -21 * (size.x / 1294),
                  bounds.max.y + Math.max(extent * 0.001, 0.01),
                  0,
                );
                scene.add(topSurface);
                meshes.push(topSurface);
                loading.hidden = true;
              };
              image.onerror = () => {
                if (currentToken === loadToken) loading.hidden = true;
              };
              image.src = window.__EMBEDDED_TEXTURE__?.[textureSource] || textureSource;
          };
          const handleGeometry = (geometry) => handleGeometries([{ geometry }]);
          const handleError = () => {
              if (currentToken !== loadToken) return;
              loading.hidden = true;
              error.hidden = false;
          };

          const loader = new loaderModule.t();
          const embeddedGLB = window.__EMBEDDED_GLB__?.[source];
          if (embeddedGLB && window.__parseEmbeddedGLB__) {
            window.__parseEmbeddedGLB__(decodeBase64(embeddedGLB)).then((modelScene) => {
              if (currentToken !== loadToken) return;
              scene.add(modelScene);
              meshes = [modelScene];
              const bounds = new THREE.NRn().setFromObject(modelScene);
              const center = bounds.getCenter(new THREE.Pq0());
              modelScene.position.sub(center);
              bounds.setFromObject(modelScene);
              const size = bounds.getSize(new THREE.Pq0());
              const extent = Math.max(size.x, size.y, size.z);
              camera.position.set(1.2 * extent, 0.9 * extent, 1.3 * extent);
              camera.near = Math.max(extent / 1000, 0.01);
              camera.far = 20 * extent;
              camera.updateProjectionMatrix();
              controls.target.set(0, 0, 0);
              controls.update();
              progress.textContent = '100%';
              loading.hidden = true;
            }).catch(handleError);
            return;
          }
          if (indexedModel) {
            try {
              progress.textContent = '100%';
              const positionBuffer = decodeBase64(indexedModel.positions);
              const indexBuffer = decodeBase64(indexedModel.indices);
              const geometry = new THREE.LoY();
              geometry.setAttribute('position', new THREE.qtW(new Float32Array(positionBuffer), 3));
              geometry.setIndex(new THREE.MW4(new Uint32Array(indexBuffer), 1));
              indexedModel.groups.forEach((group) => geometry.addGroup(group.start, group.count, group.materialIndex));
              const materials = indexedModel.materials.map((material) => new THREE._4j({
                color: material.color,
                metalness: Math.min(0.5, material.specular * 0.42),
                roughness: Math.max(0.28, 0.82 - material.specular * 0.54),
                opacity: material.opacity,
                transparent: material.opacity < 1,
                side: THREE.$EB,
              }));
              handleGeometries([{ geometry, materials }]);
            } catch {
              handleError();
            }
            return;
          }
          const coloredSegments = window.__COLORED_STL__?.[source];
          if (coloredSegments) {
            try {
              progress.textContent = '100%';
              const visibleSegments = textureSource ? coloredSegments.slice(0, 1) : coloredSegments;
              handleGeometries(visibleSegments.map((segment) => ({
                geometry: loader.parse(decodeBase64(segment.data)),
                color: textureSource ? '#0b4fa3' : segment.color,
              })));
            } catch {
              handleError();
            }
            return;
          }
          const embeddedModel = decodeEmbeddedModel(source);
          if (embeddedModel) {
            try {
              progress.textContent = '100%';
              handleGeometry(loader.parse(embeddedModel));
            } catch {
              handleError();
            }
            return;
          }

          loader.load(
            source,
            handleGeometry,
            (event) => {
              if (event.total) progress.textContent = `${Math.round((event.loaded / event.total) * 100)}%`;
            },
            handleError,
          );
        };

        const showPlaceholder = (button) => {
          loadToken += 1;
          disposeMeshes();
          loading.hidden = true;
          error.hidden = true;
          if (!empty) return;
          empty.hidden = false;
          const title = empty.querySelector('[data-model-empty-title]');
          const meta = empty.querySelector('[data-model-empty-meta]');
          if (title) title.textContent = `${button.dataset.modelName || '硬件'}模型待导入`;
          if (meta) meta.textContent = button.dataset.modelMeta || '接入 STL 后即可在此旋转查看';
        };

        const updateCaption = (button) => {
          const stage = viewer.closest('[data-model-stage]');
          if (!stage) return;
          const system = stage.querySelector('[data-model-system]');
          const name = stage.querySelector('[data-model-name]');
          const meta = stage.querySelector('[data-model-meta]');
          if (system) system.textContent = button.dataset.modelSystem || '硬件系统';
          if (name) name.textContent = button.dataset.modelName || '硬件模型';
          if (meta) meta.textContent = button.dataset.modelMeta || 'STL';
        };

        loadModel(viewer.dataset.modelSrc);

        const targetId = viewer.id;
        const selectModel = (button) => {
          const catalog = button.closest('[data-model-group]');
          if (catalog) catalog.querySelectorAll('[data-model-select]').forEach((item) => item.classList.toggle('active', item === button));
          updateCaption(button);
          if (button.dataset.modelSrc) loadModel(button.dataset.modelSrc, button.dataset.modelTexture || '');
          else showPlaceholder(button);
        };

        document.querySelectorAll(`[data-model-select][data-model-target="${targetId}"]`).forEach((button) => {
          button.addEventListener('click', () => {
            selectModel(button);
          });
        });

        const modelSlide = viewer.closest('.model-slide');
        if (modelSlide) {
          modelSlide.querySelectorAll('[data-model-group-button]').forEach((groupButton) => {
            groupButton.addEventListener('click', () => {
              const groupName = groupButton.dataset.modelGroupButton;
              modelSlide.querySelectorAll('[data-model-group-button]').forEach((item) => {
                const active = item === groupButton;
                item.classList.toggle('active', active);
                item.setAttribute('aria-selected', String(active));
              });
              modelSlide.querySelectorAll('[data-model-group]').forEach((group) => {
                group.hidden = group.dataset.modelGroup !== groupName;
              });
              const activeGroup = modelSlide.querySelector(`[data-model-group="${groupName}"]`);
              const firstModel = activeGroup?.querySelector('[data-model-select].active') || activeGroup?.querySelector('[data-model-select]');
              if (firstModel) selectModel(firstModel);
            });
          });
        }

        const resize = () => {
          const width = canvasHost.clientWidth;
          const height = canvasHost.clientHeight;
          if (!width || !height) return;
          renderer.setSize(width, height, false);
          camera.aspect = width / height;
          camera.updateProjectionMatrix();
        };
        resize();
        const resizeObserver = new ResizeObserver(resize);
        resizeObserver.observe(canvasHost);

        let frameId = 0;
        const render = () => {
          controls.update();
          renderer.render(scene, camera);
          frameId = requestAnimationFrame(render);
        };
        render();

        window.addEventListener('pagehide', () => {
          cancelAnimationFrame(frameId);
          resizeObserver.disconnect();
          controls.dispose();
          disposeMeshes();
          renderer.dispose();
        }, { once: true });
      });
    },
  },
  (require) => require(991200),
]);
