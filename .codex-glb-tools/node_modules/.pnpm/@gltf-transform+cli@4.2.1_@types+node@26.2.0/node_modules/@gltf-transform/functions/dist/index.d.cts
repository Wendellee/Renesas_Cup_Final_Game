import { Document, GLTF, Material, Mesh, Node, Primitive, PrimitiveTarget, Property, PropertyResolver, Scene, Texture, TextureChannel, TextureInfo, Transform, TransformContext, TypedArray, bbox, mat4, vec2, vec3 } from "@gltf-transform/core";

//#region src/center.d.ts
/** Options for the {@link center} function. */
interface CenterOptions {
  /** Location on the model to be considered the pivot, and recentered at the origin. */
  pivot?: 'center' | 'above' | 'below' | vec3;
}
/**
 * Centers the {@link Scene} at the origin, or above/below it. Transformations from animation,
 * skinning, and morph targets are not taken into account.
 *
 * Example:
 *
 * ```ts
 * await document.transform(center({pivot: 'below'}));
 * ```
 *
 * @category Transforms
 */
declare function center(_options?: CenterOptions): Transform;
//#endregion
//#region src/clear-node-parent.d.ts
/**
 * Clears the parent of the given {@link Node}, leaving it attached
 * directly to its {@link Scene}. Inherited transforms will be applied
 * to the Node. This operation changes the Node's local transform,
 * but leaves its world transform unchanged.
 *
 * Example:
 *
 * ```typescript
 * import { clearNodeParent } from '@gltf-transform/functions';
 *
 * scene.traverse((node) => { ... }); // Scene → … → Node
 *
 * clearNodeParent(node);
 *
 * scene.traverse((node) => { ... }); // Scene → Node
 * ```
 *
 * To clear _all_ transforms of a Node, first clear its inherited transforms with
 * {@link clearNodeParent}, then clear the local transform with {@link clearNodeTransform}.
 */
declare function clearNodeParent(node: Node): Node;
//#endregion
//#region src/clear-node-transform.d.ts
/**
 * Clears local transform of the {@link Node}, applying the transform to children and meshes.
 *
 * - Applies transform to children
 * - Applies transform to {@link Mesh mesh}
 * - Resets {@link Light lights}, {@link Camera cameras}, and other attachments to the origin
 *
 * Example:
 *
 * ```typescript
 * import { clearNodeTransform } from '@gltf-transform/functions';
 *
 * node.getTranslation(); // → [ 5, 0, 0 ]
 * node.getMesh(); // → vertex data centered at origin
 *
 * clearNodeTransform(node);
 *
 * node.getTranslation(); // → [ 0, 0, 0 ]
 * node.getMesh(); // → vertex data centered at [ 5, 0, 0 ]
 * ```
 *
 * To clear _all_ transforms of a Node, first clear its inherited transforms with
 * {@link clearNodeParent}, then clear the local transform with {@link clearNodeTransform}.
 */
declare function clearNodeTransform(node: Node): Node;
//#endregion
//#region src/compact-primitive.d.ts
/**
 * Rewrites a {@link Primitive} such that all unused vertices in its vertex
 * attributes are removed. When multiple Primitives share vertex attributes,
 * each indexing only a few, compaction can be used to produce Primitives
 * each having smaller, independent vertex streams instead.
 *
 * Regardless of whether the Primitive is indexed or contains unused vertices,
 * compaction will clone every {@link Accessor}. The resulting Primitive will
 * share no Accessors with other Primitives, allowing later changes to
 * the vertex stream to be applied in isolation.
 *
 * Example:
 *
 * ```javascript
 * import { compactPrimitive, transformMesh } from '@gltf-transform/functions';
 * import { fromTranslation } from 'gl-matrix/mat4';
 *
 * const mesh = document.getRoot().listMeshes().find((mesh) => mesh.getName() === 'MyMesh');
 * const prim = mesh.listPrimitives().find((prim) => { ... });
 *
 * // Compact primitive, removing unused vertices and detaching shared vertex
 * // attributes. Without compaction, `transformPrimitive` might affect other
 * // primitives sharing the same vertex attributes.
 * compactPrimitive(prim);
 *
 * // Transform primitive vertices, y += 10.
 * transformPrimitive(prim, fromTranslation([], [0, 10, 0]));
 * ```
 *
 * Parameters 'remap' and 'dstVertexCount' are optional. When either is
 * provided, the other must be provided as well. If one or both are missing,
 * both will be computed from the mesh indices.
 *
 * @param remap - Mapping. Array index represents vertex index in the source
 *		attributes, array value represents index in the resulting compacted
 *		primitive. When omitted, calculated from indices.
 * @param dstVertexcount - Number of unique vertices in compacted primitive.
 *		When omitted, calculated from indices.
 */
declare function compactPrimitive(prim: Primitive, remap?: TypedArray, dstVertexCount?: number): Primitive;
//#endregion
//#region src/convert-primitive-mode.d.ts
/**
 * Converts a LINE_STRIP or LINE_LOOP {@link Primitive} to LINES, which is
 * more widely supported. Any other topology given as input (points or
 * triangles) will throw an error.
 *
 * Example:
 *
 * ```javascript
 * import { convertPrimitiveToLines } from '@gltf-transform/functions';
 *
 * console.log(prim.getMode()); // 2 (LINE_LOOP)
 * convertPrimitiveToLines(prim);
 * console.log(prim.getMode()); // 1 (LINES)
 * ```
 */
declare function convertPrimitiveToLines(prim: Primitive): void;
/**
 * Converts a TRIANGLE_STRIP or TRIANGLE_LOOP {@link Primitive} to TRIANGLES,
 * which is more widely supported. Any other topology given as input (points or
 * lines) will throw an error.
 *
 * Example:
 *
 * ```javascript
 * import { convertPrimitiveToTriangles } from '@gltf-transform/functions';
 *
 * console.log(prim.getMode()); // 5 (TRIANGLE_STRIP)
 * convertPrimitiveToTriangles(prim);
 * console.log(prim.getMode()); // 4 (TRIANGLES)
 * ```
 */
declare function convertPrimitiveToTriangles(prim: Primitive): void;
//#endregion
//#region src/dedup.d.ts
interface DedupOptions {
  /** Keep properties with unique names, even if they are duplicates. */
  keepUniqueNames?: boolean;
  /** List of {@link PropertyType} identifiers to be de-duplicated.*/
  propertyTypes?: string[];
}
/**
 * Removes duplicate {@link Accessor}, {@link Mesh}, {@link Texture}, and {@link Material}
 * properties. Partially based on a
 * [gist by mattdesl](https://gist.github.com/mattdesl/aea40285e2d73916b6b9101b36d84da8). Only
 * accessors in mesh primitives, morph targets, and animation samplers are processed.
 *
 * Example:
 *
 * ```ts
 * document.getRoot().listMeshes(); // → [Mesh, Mesh, Mesh]
 *
 * await document.transform(dedup({propertyTypes: [PropertyType.MESH]}));
 *
 * document.getRoot().listMeshes(); // → [Mesh]
 * ```
 *
 * @category Transforms
 */
declare function dedup(_options?: DedupOptions): Transform;
//#endregion
//#region src/dequantize.d.ts
/** Options for the {@link dequantize} function. */
interface DequantizeOptions {
  /**
   * Pattern (regex) used to filter vertex attribute semantics for quantization.
   * Default: `/^((?!JOINTS_).)*$/`.
   */
  pattern?: RegExp;
}
/**
 * Dequantize {@link Primitive Primitives}, removing {@link KHRMeshQuantization `KHR_mesh_quantization`}
 * if present. Dequantization will increase the size of the mesh on disk and in memory, but may be
 * necessary for compatibility with applications that don't support quantization.
 *
 * Example:
 *
 * ```javascript
 * import { dequantizePrimitive } from '@gltf-transform/functions';
 *
 * await document.transform(dequantize());
 * ```
 *
 * @category Transforms
 */
declare function dequantize(_options?: DequantizeOptions): Transform;
/**
 * Dequantize a single {@link Primitive}, converting all vertex attributes to float32. Dequantization
 * will increase the size of the mesh on disk and in memory, but may be necessary for compatibility
 * with applications that don't support quantization.
 *
 * Example:
 *
 * ```javascript
 * import { dequantizePrimitive } from '@gltf-transform/functions';
 *
 * const mesh = document.getRoot().listMeshes().find((mesh) => mesh.getName() === 'MyMesh');
 *
 * for (const prim of mesh.listPrimitives()) {
 * 	dequantizePrimitive(prim);
 * }
 * ```
 */
declare function dequantizePrimitive(prim: Primitive, _options?: DequantizeOptions): void;
//#endregion
//#region src/document-utils.d.ts
/**
 * Clones source {@link Document}, copying all properties and extensions within
 * it. Source document remains unchanged, and the two may be modified
 * independently after cloning.
 *
 * Example:
 *
 * ```javascript
 *	import { cloneDocument } from '@gltf-transform/functions';
 *
 *	const targetDocument = cloneDocument(sourceDocument);
 * ```
 */
declare function cloneDocument(source: Document): Document;
/**
 * Merges contents of source {@link Document} into target Document, without
 * modifying the source. Any extensions missing from the target will be added
 * {@link Scene Scenes} and {@link Buffer Buffers} are not combined —
 * the target Document may contain multiple Scenes and Buffers after this
 * operation. These may be cleaned up manually (see {@link unpartition}),
 * or document contents may be merged more granularly using
 * {@link copyToDocument}.
 *
 * Example:
 *
 * ```javascript
 *	import { mergeDocuments, unpartition } from '@gltf-transform/functions';
 *
 *	// Merge contents of sourceDocument into targetDocument.
 *	mergeDocuments(targetDocument, sourceDocument);
 *
 *	// (Optional) Remove all but one Buffer from the target Document.
 *	await targetDocument.transform(unpartition());
 * ```
 *
 * To merge several Scenes into one:
 *
 * ```javascript
 * import { mergeDocuments } from '@gltf-transform/functions';
 *
 * const map = mergeDocuments(targetDocument, sourceDocument);
 *
 * // Find original Scene.
 * const sceneA = targetDocument.getRoot().listScenes()[0];
 *
 * // Find counterpart of the source Scene in the target Document.
 * const sceneB = map.get(sourceDocument.getRoot().listScenes()[0]);
 *
 * // Create a Node, and append source Scene's direct children.
 * const rootNode = targetDocument.createNode()
 *		.setName('SceneB')
 *		.setPosition([10, 0, 0]);
 * for (const node of sceneB.listChildren()) {
 *		rootNode.addChild(node);
 * }
 *
 * // Append Node to original Scene, and dispose the empty Scene.
 * sceneA.addChild(rootNode);
 * sceneB.dispose();
 * ```
 */
declare function mergeDocuments(target: Document, source: Document, resolve?: PropertyResolver<Property>): Map<Property, Property>;
/**
 * Moves the specified {@link Property Properties} from the source
 * {@link Document} to the target Document, and removes them from the source.
 * Dependencies of the source properties will be copied into the
 * target, but not removed from the source. Returns a Map from source
 * properties to their counterparts in the target Document.
 *
 * Example:
 *
 * ```javascript
 *	import { moveToDocument, prune } from '@gltf-transform/functions';
 *
 *	// Move all materials from sourceDocument to targetDocument.
 *	const map = moveToDocument(targetDocument, sourceDocument, sourceDocument.listMaterials());
 *
 *	// Find the new counterpart of `sourceMaterial` in the target Document.
 *	const targetMaterial = map.get(sourceMaterial);
 *
 *	// (Optional) Remove any resources (like Textures) that may now be unused
 *	// in the source Document after their parent Materials have been moved.
 *	await sourceDocument.transform(prune());
 * ```
 *
 * Moving a {@link Mesh}, {@link Animation}, or another resource depending on
 * a {@link Buffer} will create a copy of the source Buffer in the target
 * Document. If the target Document should contain only one Buffer, call
 * {@link unpartition} after moving properties.
 *
 * Repeated use of `moveToDocument` may create multiple copies of some
 * resources, particularly shared dependencies like {@link Texture Textures} or
 * {@link Accessor Accessors}. While duplicates can be cleaned up with
 * {@link dedup}, it is also possible to prevent duplicates by creating and
 * reusing the same resolver for all calls to `moveToDocument`:
 *
 * ```javascript
 *	import { moveToDocument, createDefaultPropertyResolver } from '@gltf-transform/functions';
 *
 *	const resolve = createDefaultPropertyResolver(targetDocument, sourceDocument);
 *
 *	// Move materials individually, without creating duplicates of shared textures.
 *	moveToDocument(targetDocument, sourceDocument, materialA, resolve);
 *	moveToDocument(targetDocument, sourceDocument, materialB, resolve);
 *	moveToDocument(targetDocument, sourceDocument, materialC, resolve);
 * ```
 *
 * If the transferred properties include {@link ExtensionProperty ExtensionProperties},
 * the associated {@link Extension Extensions} must be added to the target
 * Document first:
 *
 * ```javascript
 *	for (const sourceExtension of source.getRoot().listExtensionsUsed()) {
 *		const targetExtension = target.createExtension(sourceExtension.constructor);
 *		if (sourceExtension.isRequired()) targetExtension.setRequired(true);
 *	}
 * ```
 *
 * {@link Root} properties cannot be moved.
 *
 * {@link TextureInfo} properties cannot be given in the property list, but
 * are handled automatically when moving a {@link Material}.
 *
 * To copy properties without removing them from the source Document, see
 * {@link copyToDocument}.
 *
 * @experimental
 */
declare function moveToDocument(target: Document, source: Document, sourceProperties: Property[], resolve?: PropertyResolver<Property>): Map<Property, Property>;
/**
 * Copies the specified {@link Property Properties} from the source
 * {@link Document} to the target Document, leaving originals in the source.
 * Dependencies of the source properties will also be copied into the
 * target. Returns a Map from source properties to their counterparts in the
 * target Document.
 *
 * Example:
 *
 * ```javascript
 *	import { copyToDocument } from '@gltf-transform/functions';
 *
 *	// Copy all materials from sourceDocument to targetDocument.
 *	const map = copyToDocument(targetDocument, sourceDocument, sourceDocument.listMaterials());
 *
 *	// Find the new counterpart of `sourceMaterial` in the target Document.
 *	const targetMaterial = map.get(sourceMaterial);
 * ```
 *
 * Copying a {@link Mesh}, {@link Animation}, or another resource depending on
 * a {@link Buffer} will create a copy of the source Buffer in the target
 * Document. If the target Document should contain only one Buffer, call
 * {@link unpartition} after copying properties.
 *
 * Repeated use of `copyToDocument` may create multiple copies of some
 * resources, particularly shared dependencies like {@link Texture Textures} or
 * {@link Accessor Accessors}. While duplicates can be cleaned up with
 * {@link dedup}, it is also possible to prevent duplicates by creating and
 * reusing the same resolver for all calls to `copyToDocument`:
 *
 * ```javascript
 *	import { copyToDocument, createDefaultPropertyResolver } from '@gltf-transform/functions';
 *
 *	const resolve = createDefaultPropertyResolver(targetDocument, sourceDocument);
 *
 *	// Copy materials individually, without creating duplicates of shared textures.
 *	copyToDocument(targetDocument, sourceDocument, materialA, resolve);
 *	copyToDocument(targetDocument, sourceDocument, materialB, resolve);
 *	copyToDocument(targetDocument, sourceDocument, materialC, resolve);
 * ```
 *
 * If the transferred properties include {@link ExtensionProperty ExtensionProperties},
 * the associated {@link Extension Extensions} must be added to the target
 * Document first:
 *
 * ```javascript
 *	for (const sourceExtension of source.getRoot().listExtensionsUsed()) {
 *		const targetExtension = target.createExtension(sourceExtension.constructor);
 *		if (sourceExtension.isRequired()) targetExtension.setRequired(true);
 *	}
 * ```
 *
 * {@link Root} properties cannot be copied.
 *
 * {@link TextureInfo} properties cannot be given in the property list, but
 * are handled automatically when copying a {@link Material}.
 *
 * To move properties to the target Document without leaving copies behind in
 * the source Document, use {@link moveToDocument} or dispose the properties
 * after copying.
 *
 * @experimental
 */
declare function copyToDocument(target: Document, source: Document, sourceProperties: Property[], resolve?: PropertyResolver<Property>): Map<Property, Property>;
/**
 * Creates a default `resolve` implementation. May be used when moving
 * properties between {@link Document Documents} with {@link mergeDocuments},
 * {@link copyToDocument}, and {@link moveToDocument}. When the same resolver
 * is passed to multiple invocations, these functions will reuse previously-
 * transferred resources.
 *
 * @experimental
 */
declare function createDefaultPropertyResolver(target: Document, source: Document): PropertyResolver<Property>;
//#endregion
//#region src/draco.d.ts
interface DracoOptions {
  method?: 'edgebreaker' | 'sequential';
  encodeSpeed?: number;
  decodeSpeed?: number;
  quantizePosition?: number;
  quantizeNormal?: number;
  quantizeColor?: number;
  quantizeTexcoord?: number;
  quantizeGeneric?: number;
  quantizationVolume?: 'mesh' | 'scene';
}
declare const DRACO_DEFAULTS: Required<DracoOptions>;
/**
 * Applies Draco compression using {@link KHRDracoMeshCompression KHR_draco_mesh_compression}.
 * Draco compression can reduce the size of triangle geometry.
 *
 * This function is a thin wrapper around the {@link KHRDracoMeshCompression} extension.
 *
 * ### Example
 *
 * ```typescript
 * import { NodeIO } from '@gltf-transform/core';
 * import { KHRDracoMeshCompression } from '@gltf-transform/extensions';
 * import { draco } from '@gltf-transform/functions';
 * import draco3d from 'draco3dgltf';
 *
 * const io = new NodeIO()
 * 	.registerExtensions([KHRDracoMeshCompression])
 * 	.registerDependencies({
 * 		'draco3d.encoder': await draco3d.createEncoderModule()
 * 	});
 *
 * await document.transform(
 *   draco({method: 'edgebreaker'})
 * );
 *
 * await io.write('compressed.glb', document);
 * ```
 *
 * Compression is deferred until generating output with an I/O class.
 *
 * @category Transforms
 */
declare function draco(_options?: DracoOptions): Transform;
//#endregion
//#region src/flatten.d.ts
/** Options for the {@link flatten} function. */
interface FlattenOptions {
  /**
   * Whether to perform cleanup steps after completing the operation. Recommended, and enabled by
   * default. Cleanup removes temporary resources created during the operation, but may also remove
   * pre-existing unused or duplicate resources in the {@link Document}. Applications that require
   * keeping these resources may need to disable cleanup, instead calling {@link dedup} and
   * {@link prune} manually (with customized options) later in the processing pipeline.
   * @experimental
   */
  cleanup?: boolean;
}
declare const FLATTEN_DEFAULTS: Required<FlattenOptions>;
/**
 * Flattens the scene graph, leaving {@link Node Nodes} with
 * {@link Mesh Meshes}, {@link Camera Cameras}, and other attachments
 * as direct children of the {@link Scene}. Skeletons and their
 * descendants are left in their original Node structure.
 *
 * {@link Animation} targeting a Node or its parents will
 * prevent that Node from being moved.
 *
 * Example:
 *
 * ```ts
 * import { flatten } from '@gltf-transform/functions';
 *
 * await document.transform(flatten());
 * ```
 *
 * @category Transforms
 */
declare function flatten(_options?: FlattenOptions): Transform;
//#endregion
//#region src/get-bounds.d.ts
/**
 * Computes bounding box (AABB) in world space for the given {@link Node} or {@link Scene}.
 *
 * Example:
 *
 * ```ts
 * import { getBounds } from '@gltf-transform/functions';
 *
 * const {min, max} = getBounds(scene);
 * ```
 */
declare function getBounds(node: Node | Scene): bbox;
//#endregion
//#region src/get-texture-color-space.d.ts
/**
 * Returns the color space (if any) implied by the {@link Material} slots to
 * which a texture is assigned, or null for non-color textures. If the texture
 * is not connected to any {@link Material}, this function will also return
 * null — any metadata in the image file will be ignored.
 *
 * Under current glTF specifications, only 'srgb' and non-color (null) textures
 * are used.
 *
 * Example:
 *
 * ```typescript
 * import { getTextureColorSpace } from '@gltf-transform/functions';
 *
 * const baseColorTexture = material.getBaseColorTexture();
 * const normalTexture = material.getNormalTexture();
 *
 * getTextureColorSpace(baseColorTexture); // → 'srgb'
 * getTextureColorSpace(normalTexture); // → null
 * ```
 */
declare function getTextureColorSpace(texture: Texture): 'srgb' | null;
//#endregion
//#region src/get-vertex-count.d.ts
/**
 * Various methods of estimating a vertex count. For some background on why
 * multiple definitions of a vertex count should exist, see [_Vertex Count
 * Higher in Engine than in 3D Software_](https://shahriyarshahrabi.medium.com/vertex-count-higher-in-engine-than-in-3d-software-badc348ada66).
 * Totals for a {@link Scene}, {@link Node}, or {@link Mesh} will not
 * necessarily match the sum of the totals for each {@link Primitive}. Choose
 * the appropriate method for a relevant total or estimate:
 *
 * - {@link getSceneVertexCount}
 * - {@link getNodeVertexCount}
 * - {@link getMeshVertexCount}
 * - {@link getPrimitiveVertexCount}
 *
 * Many rendering features, such as volumetric transmission, may lead
 * to additional passes over some or all vertices. These tradeoffs are
 * implementation-dependent, and not considered here.
 */
declare enum VertexCountMethod {
  /**
   * Expected number of vertices processed by the vertex shader for one render
   * pass, without considering the vertex cache.
   */
  RENDER = "render",
  /**
   * Expected number of vertices processed by the vertex shader for one render
   * pass, assuming an Average Transform to Vertex Ratio (ATVR) of 1. Approaching
   * this result requires optimizing for locality of vertex references (see
   * {@link reorder}).
   *
   * References:
   * - [ACMR and ATVR](https://www.realtimerendering.com/blog/acmr-and-atvr/), Real-Time Rendering
   */
  RENDER_CACHED = "render-cached",
  /**
   * Expected number of vertices uploaded to the GPU, assuming that a client
   * uploads each unique {@link Accessor} only once. Unless glTF vertex
   * attributes are pre-processed to a known buffer layout, and the client is
   * optimized for that buffer layout, this total will be optimistic.
   */
  UPLOAD = "upload",
  /**
   * Expected number of vertices uploaded to the GPU, assuming that a client
   * uploads each unique {@link Primitive} individually, duplicating vertex
   * attribute {@link Accessor Accessors} shared by multiple primitives, but
   * never uploading the same Mesh or Primitive to GPU memory more than once.
   */
  UPLOAD_NAIVE = "upload-naive",
  /**
   * Number of vertex positions never used by any {@link Primitive}. If all
   * vertices are unused, this total will match `UPLOAD`.
   */
  UNUSED = "unused"
}
/**
 * Computes total number of vertices in a {@link Scene}, by the
 * specified method. Totals for the Scene will not necessarily match the sum
 * of the totals for each {@link Mesh} or {@link Primitive} within it. See
 * {@link VertexCountMethod} for available methods.
 */
declare function getSceneVertexCount(scene: Scene, method: VertexCountMethod): number;
/**
 * Computes total number of vertices in a {@link Node}, by the
 * specified method. Totals for the node will not necessarily match the sum
 * of the totals for each {@link Mesh} or {@link Primitive} within it. See
 * {@link VertexCountMethod} for available methods.
 */
declare function getNodeVertexCount(node: Node | Scene, method: VertexCountMethod): number;
/**
 * Computes total number of vertices in a {@link Mesh}, by the
 * specified method. Totals for the Mesh will not necessarily match the sum
 * of the totals for each {@link Primitive} within it. See
 * {@link VertexCountMethod} for available methods.
 */
declare function getMeshVertexCount(mesh: Mesh, method: VertexCountMethod): number;
/**
 * Computes total number of vertices in a {@link Primitive}, by the
 * specified method. See {@link VertexCountMethod} for available methods.
 */
declare function getPrimitiveVertexCount(prim: Primitive, method: VertexCountMethod): number;
//#endregion
//#region src/inspect.d.ts
/** Inspects the contents of a glTF file and returns a JSON report. */
declare function inspect(doc: Document): InspectReport;
interface InspectReport {
  scenes: InspectPropertyReport<InspectSceneReport>;
  meshes: InspectPropertyReport<InspectMeshReport>;
  materials: InspectPropertyReport<InspectMaterialReport>;
  textures: InspectPropertyReport<InspectTextureReport>;
  animations: InspectPropertyReport<InspectAnimationReport>;
}
interface InspectPropertyReport<T> {
  properties: T[];
  errors?: string[];
  warnings?: string[];
}
interface InspectSceneReport {
  name: string;
  rootName: string;
  bboxMin: number[];
  bboxMax: number[];
  renderVertexCount: number;
  uploadVertexCount: number;
  uploadNaiveVertexCount: number;
}
interface InspectMeshReport {
  name: string;
  meshPrimitives: number;
  mode: string[];
  vertices: number;
  glPrimitives: number;
  indices: string[];
  attributes: string[];
  instances: number;
  size: number;
}
interface InspectMaterialReport {
  name: string;
  instances: number;
  textures: string[];
  alphaMode: GLTF.MaterialAlphaMode;
  doubleSided: boolean;
}
interface InspectTextureReport {
  name: string;
  uri: string;
  slots: string[];
  instances: number;
  mimeType: string;
  resolution: string;
  compression: string;
  size: number;
  gpuSize: number | null;
}
interface InspectAnimationReport {
  name: string;
  channels: number;
  samplers: number;
  keyframes: number;
  duration: number;
  size: number;
}
//#endregion
//#region src/instance.d.ts
interface InstanceOptions {
  /** Minimum number of meshes considered eligible for instancing. Default: 5. */
  min?: number;
}
declare const INSTANCE_DEFAULTS: Required<InstanceOptions>;
/**
 * Creates GPU instances (with {@link EXTMeshGPUInstancing}) for shared {@link Mesh} references. In
 * engines supporting the extension, reused Meshes will be drawn with GPU instancing, greatly
 * reducing draw calls and improving performance in many cases. If you're not sure that identical
 * Meshes share vertex data and materials ("linked duplicates"), run {@link dedup} first to link them.
 *
 * Example:
 *
 * ```javascript
 * import { dedup, instance } from '@gltf-transform/functions';
 *
 * await document.transform(
 * 	dedup(),
 * 	instance({min: 5}),
 * );
 * ```
 *
 * @category Transforms
 */
declare function instance(_options?: InstanceOptions): Transform;
//#endregion
//#region src/join.d.ts
/** Options for the {@link join} function. */
interface JoinOptions {
  /**
   * Prevents joining distinct {@link Mesh Meshes} and {@link Node Nodes}.
   * Joins only Primitives found within the same parent Mesh. To preserve
   * only _named_ Nodes and Meshes, use
   * {@link JoinOptions.keepNamed keepNamed} instead. Default: false.
   */
  keepMeshes?: boolean;
  /**
   * Prevents joining _named_ {@link Mesh Meshes} and {@link Node Nodes}.
   * If {@link JoinOptions.keepMeshes keepMeshes} is enabled, keepNamed will
   * have no effect. Default: false.
   */
  keepNamed?: boolean;
  /**
   * Whether to perform cleanup steps after completing the operation. Recommended, and enabled by
   * default. Cleanup removes temporary resources created during the operation, but may also remove
   * pre-existing unused or duplicate resources in the {@link Document}. Applications that require
   * keeping these resources may need to disable cleanup, instead calling {@link dedup} and
   * {@link prune} manually (with customized options) later in the processing pipeline.
   * @experimental
   */
  cleanup?: boolean;
  /**
   * A filter function used to evaluate a condition on a given {@link Node Node}.
   * This function should return a boolean indicating whether the node
   * satisfies the provided condition.
   *
   * @param {Node} node - The node instance to be evaluated.
   * @returns {boolean} - The result of the evaluation; `true` if the condition is met, otherwise `false`.
   */
  filter?: (node: Node) => boolean;
}
declare const JOIN_DEFAULTS: Required<JoinOptions>;
/**
 * Joins compatible {@link Primitive Primitives} and reduces draw calls.
 * Primitives are eligible for joining if they are members of the same
 * {@link Mesh} or, optionally, attached to sibling {@link Node Nodes}
 * in the scene hierarchy. For best results, apply {@link dedup} and
 * {@link flatten} first to maximize the number of Primitives that
 * can be joined.
 *
 * NOTE: In a Scene that heavily reuses the same Mesh data, joining may
 * increase vertex count. Consider alternatives, like
 * {@link instance instancing} with {@link EXTMeshGPUInstancing}.
 *
 * Example:
 *
 * ```ts
 * import { PropertyType } from '@gltf-transform/core';
 * import { join, flatten, dedup } from '@gltf-transform/functions';
 *
 * await document.transform(
 * 	dedup({ propertyTypes: [PropertyType.MATERIAL] }),
 * 	flatten(),
 * 	join({ keepNamed: false }),
 * );
 * ```
 *
 * @category Transforms
 */
declare function join(_options?: JoinOptions): Transform;
//#endregion
//#region src/join-primitives.d.ts
interface JoinPrimitiveOptions {
  skipValidation?: boolean;
}
/**
 * Given a list of compatible Mesh {@link Primitive Primitives}, returns new Primitive
 * containing their vertex data. Compatibility requires that all Primitives share the
 * same {@link Material Materials}, draw mode, and vertex attribute types. Primitives
 * using morph targets cannot currently be joined.
 *
 * Example:
 *
 * ```javascript
 * import { joinPrimitives } from '@gltf-transform/functions';
 *
 * // Succeeds if Primitives are compatible, or throws an error.
 * const result = joinPrimitives(mesh.listPrimitives());
 *
 * for (const prim of mesh.listPrimitives()) {
 * 	prim.dispose();
 * }
 *
 * mesh.addPrimitive(result);
 * ```
 */
declare function joinPrimitives(prims: Primitive[], _options?: JoinPrimitiveOptions): Primitive;
//#endregion
//#region src/list-node-scenes.d.ts
/**
 * Finds the parent {@link Scene Scenes} associated with the given {@link Node}.
 * In most cases a Node is associated with only one Scene, but it is possible
 * for a Node to be located in two or more Scenes, or none at all.
 *
 * Example:
 *
 * ```typescript
 * import { listNodeScenes } from '@gltf-transform/functions';
 *
 * const node = document.getRoot().listNodes()
 *  .find((node) => node.getName() === 'MyNode');
 *
 * const scenes = listNodeScenes(node);
 * ```
 */
declare function listNodeScenes(node: Node): Scene[];
//#endregion
//#region src/list-texture-channels.d.ts
/**
 * Returns a list of {@link TextureChannel TextureChannels} used by the given
 * texture. Determination is based only on the _role_ of the textures, e.g.
 * a texture used for the `occlusionTexture` will have (at least) a red channel
 * in use. See {@link getTextureChannelMask} for bitmask alternative.
 *
 * Example:
 *
 * ```js
 * const channels = listTextureChannels(texture);
 * if (channels.includes(TextureChannel.R)) {
 *   console.log('texture red channel used');
 * }
 * ```
 */
declare function listTextureChannels(texture: Texture): TextureChannel[];
/**
 * Returns bitmask of all {@link TextureChannel TextureChannels} used by the
 * given texture. Determination is based only on the _role_ of the textures, e.g.
 * a texture used for the `occlusionTexture` will have (at least) a red channel.
 * See {@link listTextureChannels} for an array alternative.
 *
 * Example:
 *
 * ```js
 * const mask = getTextureChannelMask(texture);
 * if (mask & TextureChannel.R) {
 *   console.log('texture red channel used');
 * }
 * ```
 */
declare function getTextureChannelMask(texture: Texture): number;
//#endregion
//#region src/list-texture-info.d.ts
/**
 * Lists all {@link TextureInfo} definitions associated with a given
 * {@link Texture}. May be used to determine which UV transforms
 * and texCoord indices are applied to the material, without explicitly
 * checking the material properties and extensions.
 *
 * Example:
 *
 * ```typescript
 * // Find TextureInfo instances associated with the texture.
 * const results = listTextureInfo(texture);
 *
 * // Find which UV sets (TEXCOORD_0, TEXCOORD_1, ...) are required.
 * const texCoords = results.map((info) => info.getTexCoord());
 * // → [0, 1]
 * ```
 */
declare function listTextureInfo(texture: Texture): TextureInfo[];
/**
 * Lists all {@link TextureInfo} definitions associated with any {@link Texture}
 * on the given {@link Material}. May be used to determine which UV transforms
 * and texCoord indices are applied to the material, without explicitly
 * checking the material properties and extensions.
 *
 * Example:
 *
 * ```typescript
 * const results = listTextureInfoByMaterial(material);
 *
 * const texCoords = results.map((info) => info.getTexCoord());
 * // → [0, 1]
 * ```
 */
declare function listTextureInfoByMaterial(material: Material): TextureInfo[];
//#endregion
//#region src/list-texture-slots.d.ts
/**
 * Returns names of all texture slots using the given texture.
 *
 * Example:
 *
 * ```js
 * const slots = listTextureSlots(texture);
 * // → ['occlusionTexture', 'metallicRoughnessTexture']
 * ```
 */
declare function listTextureSlots(texture: Texture): string[];
//#endregion
//#region src/quantize.d.ts
/** Options for the {@link quantize} function. */
interface QuantizeOptions {
  /** Pattern (regex) used to filter vertex attribute semantics for quantization. Default: all. */
  pattern?: RegExp;
  /** Pattern (regex) used to filter morph target semantics for quantization. Default: `options.pattern`. */
  patternTargets?: RegExp;
  /** Bounds for quantization grid. */
  quantizationVolume?: 'mesh' | 'scene';
  /** Quantization bits for `POSITION` attributes. */
  quantizePosition?: number;
  /** Quantization bits for `NORMAL` attributes. */
  quantizeNormal?: number;
  /** Quantization bits for `TEXCOORD_*` attributes. */
  quantizeTexcoord?: number;
  /** Quantization bits for `COLOR_*` attributes. */
  quantizeColor?: number;
  /** Quantization bits for `WEIGHT_*` attributes. */
  quantizeWeight?: number;
  /** Quantization bits for application-specific (`_*`) attributes. */
  quantizeGeneric?: number;
  /** Normalize weight attributes. */
  normalizeWeights?: boolean;
  /**
   * Whether to perform cleanup steps after completing the operation. Recommended, and enabled by
   * default. Cleanup removes temporary resources created during the operation, but may also remove
   * pre-existing unused or duplicate resources in the {@link Document}. Applications that require
   * keeping these resources may need to disable cleanup, instead calling {@link dedup} and
   * {@link prune} manually (with customized options) later in the processing pipeline.
   * @experimental
   */
  cleanup?: boolean;
}
declare const QUANTIZE_DEFAULTS: Required<Omit<QuantizeOptions, 'patternTargets'>>;
/**
 * References:
 * - https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_mesh_quantization
 * - http://www.aclockworkberry.com/normal-unpacking-quantization-errors/
 * - https://www.mathworks.com/help/dsp/ref/uniformencoder.html
 * - https://oroboro.com/compressed-unit-vectors/
 */
/**
 * Quantizes vertex attributes with `KHR_mesh_quantization`, reducing the size and memory footprint
 * of the file. Conceptually, quantization refers to snapping values to regular intervals; vertex
 * positions are snapped to a 3D grid, UVs to a 2D grid, and so on. When quantized to <= 16 bits,
 * larger component types may be more compactly stored as 16-bit or 8-bit attributes.
 *
 * Often, it can be useful to quantize to precision lower than the maximum allowed by the component
 * type. Positions quantized to 14 bits in a 16-bit accessor will occupy 16 bits in VRAM, but they
 * can be compressed further for network compression with lossless encodings such as ZSTD.
 *
 * Vertex positions are shifted into [-1,1] or [0,1] range before quantization. Compensating for
 * that shift, a transform is applied to the parent {@link Node}, or inverse bind matrices for a
 * {@link Skin} if applicable. Materials using {@link KHRMaterialsVolume} are adjusted to maintain
 * appearance. In future releases, UVs may also be transformed with {@link KHRTextureTransform}.
 * Currently UVs outside of [0,1] range are not quantized.
 *
 * In most cases, quantization requires {@link KHRMeshQuantization}; the extension will be added
 * automatically when `quantize()` is applied. When applying meshopt compression with
 * {@link EXTMeshoptCompression}, quantization is usually applied before compression.
 *
 * Example:
 *
 * ```javascript
 * import { quantize } from '@gltf-transform/functions';
 *
 * await document.transform(
 *   quantize({
 *		quantizePosition: 14,
 *		quantizeNormal: 10,
 *   }),
 * );
 * ```
 *
 * For the inverse operation, see {@link dequantize}.
 *
 * @category Transforms
 */
declare function quantize(_options?: QuantizeOptions): Transform;
//#endregion
//#region src/meshopt.d.ts
interface MeshoptOptions extends Omit<QuantizeOptions, 'pattern' | 'patternTargets'> {
  encoder: unknown;
  level?: 'medium' | 'high';
}
declare const MESHOPT_DEFAULTS: Required<Omit<MeshoptOptions, 'encoder'>>;
/**
 * Applies Meshopt compression using {@link EXTMeshoptCompression EXT_meshopt_compression}.
 * This type of compression can reduce the size of point, line, and triangle geometry,
 * morph targets, and animation data.
 *
 * This function is a thin wrapper around {@link reorder}, {@link quantize}, and
 * {@link EXTMeshoptCompression}, and exposes relatively few configuration options.
 * To access more options (like quantization bits) direct use of the underlying
 * functions is recommended.
 *
 * Example:
 *
 * ```javascript
 * import { MeshoptEncoder } from 'meshoptimizer';
 * import { meshopt } from '@gltf-transform/functions';
 *
 * await MeshoptEncoder.ready;
 *
 * await document.transform(
 *   meshopt({encoder: MeshoptEncoder, level: 'medium'})
 * );
 * ```
 *
 * Compression is deferred until generating output with an I/O class.
 *
 * @category Transforms
 */
declare function meshopt(_options: MeshoptOptions): Transform;
//#endregion
//#region src/metal-rough.d.ts
interface MetalRoughOptions {}
/**
 * Convert {@link Material}s from spec/gloss PBR workflow to metal/rough PBR workflow,
 * removing `KHR_materials_pbrSpecularGlossiness` and adding `KHR_materials_ior` and
 * `KHR_materials_specular`. The metal/rough PBR workflow is preferred for most use cases,
 * and is a prerequisite for other advanced PBR extensions provided by glTF.
 *
 * No options are currently implemented for this function.
 *
 * @category Transforms
 */
declare function metalRough(_options?: MetalRoughOptions): Transform;
//#endregion
//#region src/normals.d.ts
/** Options for the {@link normals} function. */
interface NormalsOptions {
  /** Whether to overwrite existing `NORMAL` attributes. */
  overwrite?: boolean;
}
/**
 * Generates flat vertex normals for mesh primitives.
 *
 * Example:
 *
 * ```ts
 * import { normals } from '@gltf-transform/functions';
 *
 * await document.transform(normals({overwrite: true}));
 * ```
 *
 * @category Transforms
 */
declare function normals(_options?: NormalsOptions): Transform;
//#endregion
//#region src/palette.d.ts
interface PaletteOptions {
  /** Size (in pixels) of a single block within each palette texture. Default: 4. */
  blockSize?: number;
  /**
   * Minimum number of blocks in the palette texture. If fewer unique
   * material values are found, no palettes will be generated. Default: 5.
   */
  min?: number;
  /**
   * Whether to keep unused vertex attributes, such as UVs without an assigned
   * texture. If kept, unused UV coordinates may prevent palette texture
   * creation. Default: false.
   */
  keepAttributes?: boolean;
  /**
   * Whether to perform cleanup steps after completing the operation. Recommended, and enabled by
   * default. Cleanup removes temporary resources created during the operation, but may also remove
   * pre-existing unused or duplicate resources in the {@link Document}. Applications that require
   * keeping these resources may need to disable cleanup, instead calling {@link dedup} and
   * {@link prune} manually (with customized options) later in the processing pipeline.
   * @experimental
   */
  cleanup?: boolean;
}
declare const PALETTE_DEFAULTS: Required<PaletteOptions>;
/**
 * Creates palette textures containing all unique values of scalar
 * {@link Material} properties within the scene, then merges materials. For
 * scenes with many solid-colored materials (often found in CAD, architectural,
 * or low-poly styles), texture palettes can reduce the number of materials
 * used, and significantly increase the number of {@link Mesh} objects eligible
 * for {@link join} operations.
 *
 * Materials already containing texture coordinates (UVs) are not eligible for
 * texture palette optimizations. Currently only a material's base color,
 * alpha, emissive factor, metallic factor, and roughness factor are converted
 * to palette textures.
 *
 * Example:
 *
 * ```typescript
 * import { palette, flatten, dequantize, join } from '@gltf-transform/functions';
 *
 * await document.transform(
 * 	palette({ min: 5 }),
 * 	flatten(),
 * 	dequantize(),
 * 	join()
 * );
 * ```
 *
 * The illustration below shows a typical base color palette texture:
 *
 * <img
 * 	src="/media/functions/palette.png"
 * 	alt="Row of colored blocks"
 * 	style="width: 100%; max-width: 320px; image-rendering: pixelated;">
 *
 * @category Transforms
 */
declare function palette(_options?: PaletteOptions): Transform;
//#endregion
//#region src/partition.d.ts
interface PartitionOptions {
  animations?: boolean | Array<string>;
  meshes?: boolean | Array<string>;
}
/**
 * Partitions the binary payload of a glTF file so separate mesh or animation data is in separate
 * `.bin` {@link Buffer}s. This technique may be useful for engines that support lazy-loading
 * specific binary resources as needed over the application lifecycle.
 *
 * Example:
 *
 * ```ts
 * document.getRoot().listBuffers(); // → [Buffer]
 *
 * await document.transform(partition({meshes: true}));
 *
 * document.getRoot().listBuffers(); // → [Buffer, Buffer, ...]
 * ```
 *
 * @category Transforms
 */
declare function partition(_options?: PartitionOptions): Transform;
//#endregion
//#region src/prune.d.ts
interface PruneOptions {
  /** List of {@link PropertyType} identifiers to be de-duplicated.*/
  propertyTypes?: string[];
  /** Whether to keep empty leaf nodes. */
  keepLeaves?: boolean;
  /** Whether to keep unused vertex attributes, such as UVs without an assigned texture. */
  keepAttributes?: boolean;
  /**
   * Whether to keep redundant mesh indices, where vertex count equals index count.
   * @deprecated Disabled. To remove indices, use {@link unweld} or other APIs.
   * @privateRemarks TODO(v5): Remove this option.
   */
  keepIndices?: boolean;
  /** Whether to keep single-color textures that can be converted to material factors. */
  keepSolidTextures?: boolean;
  /** Whether custom extras should prevent pruning a property. */
  keepExtras?: boolean;
}
declare const PRUNE_DEFAULTS: Required<PruneOptions>;
/**
 * Removes properties from the file if they are not referenced by a {@link Scene}. Commonly helpful
 * for cleaning up after other operations, e.g. allowing a node to be detached and any unused
 * meshes, materials, or other resources to be removed automatically.
 *
 * Example:
 *
 * ```javascript
 * import { PropertyType } from '@gltf-transform/core';
 * import { prune } from '@gltf-transform/functions';
 *
 * document.getRoot().listMaterials(); // → [Material, Material]
 *
 * await document.transform(
 * 	prune({
 * 		propertyTypes: [PropertyType.MATERIAL],
 * 		keepExtras: true
 * 	})
 * );
 *
 * document.getRoot().listMaterials(); // → [Material]
 * ```
 *
 * By default, pruning will aggressively remove most unused resources. Use
 * {@link PruneOptions} to limit what is considered for pruning.
 *
 * @category Transforms
 */
declare function prune(_options?: PruneOptions): Transform;
//#endregion
//#region src/reorder.d.ts
/** Options for the {@link reorder} function. */
interface ReorderOptions {
  /** MeshoptEncoder instance. */
  encoder: unknown;
  /**
   * Whether the order should be optimal for transmission size (recommended for Web)
   * or for GPU rendering performance. Default is 'size'.
   */
  target?: 'size' | 'performance';
  /**
   * Whether to perform cleanup steps after completing the operation. Recommended, and enabled by
   * default. Cleanup removes temporary resources created during the operation, but may also remove
   * pre-existing unused or duplicate resources in the {@link Document}. Applications that require
   * keeping these resources may need to disable cleanup, instead calling {@link dedup} and
   * {@link prune} manually (with customized options) later in the processing pipeline.
   * @experimental
   */
  cleanup?: boolean;
}
/**
 * Optimizes {@link Mesh} {@link Primitive Primitives} for locality of reference. Choose whether
 * the order should be optimal for transmission size (recommended for Web) or for GPU rendering
 * performance. Requires a MeshoptEncoder instance from the Meshoptimizer library.
 *
 * Example:
 *
 * ```ts
 * import { MeshoptEncoder } from 'meshoptimizer';
 * import { reorder } from '@gltf-transform/functions';
 *
 * await MeshoptEncoder.ready;
 *
 * await document.transform(
 * 	reorder({encoder: MeshoptEncoder})
 * );
 * ```
 *
 * @category Transforms
 */
declare function reorder(_options: ReorderOptions): Transform;
//#endregion
//#region src/resample.d.ts
interface ResampleOptions {
  ready?: Promise<void>;
  resample?: unknown;
  tolerance?: number;
  /**
   * Whether to perform cleanup steps after completing the operation. Recommended, and enabled by
   * default. Cleanup removes temporary resources created during the operation, but may also remove
   * pre-existing unused or duplicate resources in the {@link Document}. Applications that require
   * keeping these resources may need to disable cleanup, instead calling {@link dedup} and
   * {@link prune} manually (with customized options) later in the processing pipeline.
   * @experimental
   */
  cleanup?: boolean;
}
/**
 * Resample {@link AnimationChannel AnimationChannels}, losslessly deduplicating keyframes to
 * reduce file size. Duplicate keyframes are commonly present in animation 'baked' by the
 * authoring software to apply IK constraints or other software-specific features.
 *
 * Optionally, a WebAssembly implementation from the
 * [`keyframe-resample`](https://github.com/donmccurdy/keyframe-resample-wasm) library may be
 * provided. The WebAssembly version is usually much faster at processing large animation
 * sequences, but may not be compatible with all runtimes and JavaScript build tools.
 *
 * Result: (0,0,0,0,1,1,1,0,0,0,0,0,0,0) → (0,0,1,1,0,0)
 *
 * Example:
 *
 * ```
 * import { resample } from '@gltf-transform/functions';
 * import { ready, resample as resampleWASM } from 'keyframe-resample';
 *
 * // JavaScript (slower)
 * await document.transform(resample());
 *
 * // WebAssembly (faster)
 * await document.transform(resample({ ready, resample: resampleWASM }));
 * ```
 *
 * @privateRemarks Implementation based on THREE.KeyframeTrack#optimize().
 * @category Transforms
 */
declare function resample(_options?: ResampleOptions): Transform;
//#endregion
//#region src/sequence.d.ts
interface SequenceOptions {
  /** Frames per second, where one node is shown each frame. Default 10. */
  fps?: number;
  /** Pattern (regex) used to filter nodes for the sequence. Required. */
  pattern: RegExp;
  /** Name of the new animation. */
  name?: string;
  /** Whether to sort the nodes by name, or use original order. Default true. */
  sort?: boolean;
}
/**
 * Creates an {@link Animation} displaying each of the specified {@link Node}s sequentially.
 *
 * @category Transforms
 */
declare function sequence(_options?: SequenceOptions): Transform;
//#endregion
//#region src/simplify.d.ts
/** Options for the {@link simplify} function. */
interface SimplifyOptions {
  /** MeshoptSimplifier instance. */
  simplifier: unknown;
  /** Target ratio (0–1) of vertices to keep. Default: 0.0 (0%). */
  ratio?: number;
  /** Limit on error, as a fraction of mesh radius. Default: 0.0001 (0.01%). */
  error?: number;
  /**
   * Whether to lock topological borders of the mesh. May be necessary when
   * adjacent 'chunks' of a large mesh (e.g. terrain) share a border, helping
   * to ensure no seams appear.
   */
  lockBorder?: boolean;
}
declare const SIMPLIFY_DEFAULTS: Required<Omit<SimplifyOptions, 'simplifier'>>;
/**
 * Simplification algorithm, based on meshoptimizer, producing meshes with fewer
 * triangles and vertices. Simplification is lossy, but the algorithm aims to
 * preserve visual quality as much as possible for given parameters.
 *
 * The algorithm aims to reach the target 'ratio', while minimizing error. If
 * error exceeds the specified 'error' threshold, the algorithm will quit
 * before reaching the target ratio. Examples:
 *
 * - ratio=0.0, error=0.0001: Aims for maximum simplification, constrained to 0.01% error.
 * - ratio=0.5, error=0.0001: Aims for 50% simplification, constrained to 0.01% error.
 * - ratio=0.5, error=1: Aims for 50% simplification, unconstrained by error.
 *
 * Topology, particularly split vertices, will also limit the simplifier. For
 * best results, apply a {@link weld} operation before simplification.
 *
 * Example:
 *
 * ```javascript
 * import { simplify, weld } from '@gltf-transform/functions';
 * import { MeshoptSimplifier } from 'meshoptimizer';
 *
 * await document.transform(
 *   weld({}),
 *   simplify({ simplifier: MeshoptSimplifier, ratio: 0.75, error: 0.001 })
 * );
 * ```
 *
 * References:
 * - https://github.com/zeux/meshoptimizer/blob/master/js/README.md#simplifier
 *
 * @category Transforms
 */
declare function simplify(_options: SimplifyOptions): Transform;
/** @hidden */
declare function simplifyPrimitive(prim: Primitive, _options: SimplifyOptions): Primitive;
//#endregion
//#region src/sort-primitive-weights.d.ts
/**
 * Sorts skinning weights from high to low, for each vertex of the input
 * {@link Primitive} or {@link PrimitiveTarget}, and normalizes the weights.
 * Optionally, uses the given 'limit' to remove least-significant joint
 * influences such that no vertex has more than 'limit' influences.
 *
 * Most realtime engines support a limited number of joint influences per vertex,
 * often 4 or 8. Sorting and removing the additional influences can reduce file
 * size and improve compatibility.
 *
 * Example:
 *
 * ```javascript
 * import { sortPrimitiveWeights } from '@gltf-transform/functions';
 *
 * const limit = 4;
 * for (const mesh of document.getRoot().listMeshes()) {
 * 	for (const prim of mesh.listPrimitives()) {
 * 		sortPrimitiveWeights(prim, limit);
 * 	}
 * }
 * ```
 *
 * @param prim Input, to be modified in place.
 * @param limit Maximum number of joint influences per vertex. Must be a multiple of four.
 */
declare function sortPrimitiveWeights(prim: Primitive | PrimitiveTarget, limit?: number): void;
//#endregion
//#region src/sparse.d.ts
/** Options for the {@link sparse} function. */
interface SparseOptions {
  /**
   * Threshold ratio used to determine when an accessor should be sparse.
   * Default: 1 / 3.
   */
  ratio: number;
}
/**
 * Scans all {@link Accessor Accessors} in the Document, detecting whether each Accessor
 * would benefit from sparse data storage. Currently, sparse data storage is used only
 * when many values (>= ratio) are zeroes. Particularly for assets using morph target
 * ("shape key") animation, sparse data storage may significantly reduce file sizes.
 *
 * Example:
 *
 * ```ts
 * import { sparse } from '@gltf-transform/functions';
 *
 * accessor.getArray(); // → [ 0, 0, 0, 0, 0, 25.0, 0, 0, ... ]
 * accessor.getSparse(); // → false
 *
 * await document.transform(sparse({ratio: 1 / 10}));
 *
 * accessor.getSparse(); // → true
 * ```
 *
 * @experimental
 * @category Transforms
 */
declare function sparse(_options?: SparseOptions): Transform;
//#endregion
//#region src/tangents.d.ts
/** Options for the {@link tangents} function. */
interface TangentsOptions {
  /**
   * Callback function to generate tangents from position, uv, and normal attributes.
   * Generally, users will want to provide the `generateTangents` from the
   * [mikktspace](https://github.com/donmccurdy/mikktspace-wasm) library, which is not
   * included by default.
   */
  generateTangents?: (pos: Float32Array, norm: Float32Array, uv: Float32Array) => Float32Array;
  /** Whether to overwrite existing `TANGENT` attributes. */
  overwrite?: boolean;
}
/**
 * Generates MikkTSpace vertex tangents for mesh primitives, which may fix rendering issues
 * occurring with some baked normal maps. Requires access to the [mikktspace](https://github.com/donmccurdy/mikktspace-wasm)
 * WASM package, or equivalent.
 *
 * Example:
 *
 * ```ts
 * import { generateTangents } from 'mikktspace';
 * import { tangents } from '@gltf-transform/functions';
 *
 * await document.transform(
 * 	tangents({generateTangents})
 * );
 * ```
 *
 * @category Transforms
 */
declare function tangents(_options?: TangentsOptions): Transform;
//#endregion
//#region src/texture-compress.d.ts
type Format = (typeof TEXTURE_COMPRESS_SUPPORTED_FORMATS)[number];
declare const TEXTURE_COMPRESS_SUPPORTED_FORMATS: readonly ["jpeg", "png", "webp", "avif"];
/** Resampling filter methods. LANCZOS3 is sharper, LANCZOS2 is smoother. */
declare enum TextureResizeFilter {
  /** Lanczos3 (sharp) */
  LANCZOS3 = "lanczos3",
  /** Lanczos2 (smooth) */
  LANCZOS2 = "lanczos2"
}
interface TextureCompressOptions {
  /** Instance of the Sharp encoder, which must be installed from the
   * 'sharp' package and provided by the caller. When not provided, a
   * platform-specific fallback implementation will be used, and most
   * quality- and compression-related options are ignored.
   */
  encoder?: unknown;
  /**
   * Target image format. If specified, included textures in other formats
   * will be converted. Default: original format.
   */
  targetFormat?: Format;
  /**
   * Resizes textures to given maximum width/height, preserving aspect ratio.
   * For example, a 4096x8192 texture, resized with limit [2048, 2048] will
   * be reduced to 1024x2048.
   *
   * Presets "nearest-pot", "ceil-pot", and "floor-pot" resize textures to
   * power-of-two dimensions, for older graphics APIs including WebGL 1.0.
   */
  resize?: vec2 | 'nearest-pot' | 'ceil-pot' | 'floor-pot';
  /** Interpolation used if resizing. Default: TextureResizeFilter.LANCZOS3. */
  resizeFilter?: TextureResizeFilter;
  /** Pattern identifying textures to compress, matched to name or URI. */
  pattern?: RegExp | null;
  /**
   * Pattern matching the format(s) to be compressed or converted. Some examples
   * of formats include "jpeg" and "png".
   */
  formats?: RegExp | null;
  /**
   * Pattern matching the material texture slot(s) to be compressed or converted.
   * Some examples of slot names include "baseColorTexture", "occlusionTexture",
   * "metallicRoughnessTexture", and "normalTexture".
   */
  slots?: RegExp | null;
  /** Quality, 1-100. Default: auto. */
  quality?: number | null;
  /**
   * Level of CPU effort to reduce file size, 0-100. PNG, WebP, and AVIF
   * only. Supported only when a Sharp encoder is provided. Default: auto.
   */
  effort?: number | null;
  /**
   * Use lossless compression mode. WebP and AVIF only. Supported only when a
   * Sharp encoder is provided. Default: false.
   */
  lossless?: boolean;
  /**
   * Use near lossless compression mode. WebP only. Supported only when a
   * Sharp encoder is provided. Default: false.
   */
  nearLossless?: boolean;
  /**
   * Allows lower resolution for chroma than for luma, reducing file size. For
   * non-color textures such as normal maps, the chroma/luma distinction does
   * not apply, and chroma subsampling should be disabled. Options are '4:4:4'
   * (off) and '4:2:0' (on). JPEG and AVIF only. Default: auto.
   */
  chromaSubsampling?: '4:2:0' | '4:4:4';
  /**
   * Attempts to avoid processing images that could exceed memory or other other
   * limits, throwing an error instead. Default: true.
   * @experimental
   */
  limitInputPixels?: boolean;
}
type CompressTextureOptions = Omit<TextureCompressOptions, 'pattern' | 'formats' | 'slots'>;
declare const TEXTURE_COMPRESS_DEFAULTS: Omit<TextureCompressOptions, 'resize' | 'targetFormat' | 'encoder'>;
/**
 * Optimizes images, optionally resizing or converting to JPEG, PNG, WebP, or AVIF formats.
 *
 * For best results use a Node.js environment, install the `sharp` module, and
 * provide an encoder. When the encoder is omitted — `sharp` works only in Node.js —
 * the implementation will use a platform-specific fallback encoder, and most
 * quality- and compression-related options are ignored.
 *
 * Example:
 *
 * ```javascript
 * import { textureCompress } from '@gltf-transform/functions';
 * import sharp from 'sharp';
 *
 * // (A) Optimize without conversion.
 * await document.transform(
 * 	textureCompress({encoder: sharp})
 * );
 *
 * // (B) Optimize and convert images to WebP.
 * await document.transform(
 * 	textureCompress({
 * 		encoder: sharp,
 * 		targetFormat: 'webp',
 * 		slots: /^(?!normalTexture).*$/ // exclude normal maps
 * 	})
 * );
 *
 * // (C) Resize and convert images to WebP in a browser, without a Sharp
 * // encoder. Most quality- and compression-related options are ignored.
 * await document.transform(
 * 	textureCompress({ targetFormat: 'webp', resize: [1024, 1024] })
 * );
 * ```
 *
 * @category Transforms
 */
declare function textureCompress(_options: TextureCompressOptions): Transform;
/**
 * Optimizes a single {@link Texture}, optionally resizing or converting to JPEG, PNG, WebP, or AVIF formats.
 *
 * For best results use a Node.js environment, install the `sharp` module, and
 * provide an encoder. When the encoder is omitted — `sharp` works only in Node.js —
 * the implementation will use a platform-specific fallback encoder, and most
 * quality- and compression-related options are ignored.
 *
 * Example:
 *
 * ```javascript
 * import { compressTexture } from '@gltf-transform/functions';
 * import sharp from 'sharp';
 *
 * const texture = document.getRoot().listTextures()
 * 	.find((texture) => texture.getName() === 'MyTexture');
 *
 * // (A) Node.js.
 * await compressTexture(texture, {
 * 	encoder: sharp,
 * 	targetFormat: 'webp',
 * 	resize: [1024, 1024]
 * });
 *
 * // (B) Web.
 * await compressTexture(texture, {
 * 	targetFormat: 'webp',
 * 	resize: [1024, 1024]
 * });
 * ```
 */
declare function compressTexture(texture: Texture, _options: CompressTextureOptions): Promise<void>;
//#endregion
//#region src/transform-mesh.d.ts
/**
 * Applies a transform matrix to every {@link Primitive} in the given {@link Mesh}.
 *
 * For every Primitive in the Mesh, the operation first applies
 * {@link compactPrimitive} to isolate vertex streams, then calls
 * {@link transformPrimitive}. Transformed Mesh will no longer share vertex
 * attributes with any other Meshes — attributes are cloned before
 * transformation.
 *
 * Example:
 *
 * ```javascript
 * import { fromTranslation } from 'gl-matrix/mat4';
 * import { transformMesh } from '@gltf-transform/functions';
 *
 * // offset vertices, y += 10.
 * transformMesh(mesh, fromTranslation([], [0, 10, 0]));
 * ```
 *
 * @param mesh
 * @param matrix
 */
declare function transformMesh(mesh: Mesh, matrix: mat4): void;
//#endregion
//#region src/transform-primitive.d.ts
/**
 * Applies a transform matrix to a {@link Primitive}.
 *
 * All vertex attributes on the Primitive and its
 * {@link PrimitiveTarget PrimitiveTargets} are modified in place. If vertex
 * streams are shared with other Primitives, and overwriting the shared vertex
 * attributes is not desired, use {@link compactPrimitive} to pre-process
 * the Primitive or call {@link transformMesh} instead.
 *
 * Example:
 *
 * ```javascript
 * import { fromTranslation } from 'gl-matrix/mat4';
 * import { transformPrimitive } from '@gltf-transform/functions';
 *
 * // offset vertices, y += 10.
 * transformPrimitive(prim, fromTranslation([], [0, 10, 0]));
 * ```
 *
 * @param prim
 * @param matrix
 */
declare function transformPrimitive(prim: Primitive, matrix: mat4): void;
//#endregion
//#region src/uninstance.d.ts
interface UninstanceOptions {}
/**
 * Removes extension {@link EXTMeshGPUInstancing}, reversing the effects of the
 * {@link instance} transform or similar instancing operations. For each {@link Node}
 * associated with an {@link InstancedMesh}, the Node's {@link Mesh} and InstancedMesh will
 * be detached. In their place, one Node per instance will be attached to the original
 * Node as children, associated with the same Mesh. The extension, `EXT_mesh_gpu_instancing`,
 * will be removed from the {@link Document}.
 *
 * In applications that support `EXT_mesh_gpu_instancing`, removing the extension
 * is likely to substantially increase draw calls and reduce performance. Removing
 * the extension may be helpful for compatibility in applications without such support.
 *
 * Example:
 *
 * ```ts
 * import { uninstance } from '@gltf-transform/functions';
 *
 * document.getRoot().listNodes(); // → [ Node x 10 ]
 *
 * await document.transform(uninstance());
 *
 * document.getRoot().listNodes(); // → [ Node x 1000 ]
 * ```
 *
 * @category Transforms
 */
declare function uninstance(_options?: UninstanceOptions): Transform;
/**
 * Given a {@link Node} with an {@link InstancedMesh} extension, returns a list
 * containing one Node per instance in the InstancedMesh. Each Node will have
 * the transform (translation/rotation/scale) of the corresponding instance,
 * and will be assigned to the same {@link Mesh}.
 *
 * May be used to unpack instancing previously applied with {@link instance}
 * and {@link EXTMeshGPUInstancing}. For a transform that applies this operation
 * to the entire {@link Document}, see {@link uninstance}.
 *
 * Example:
 * ```javascript
 * import { createInstanceNodes } from '@gltf-transform/functions';
 *
 * for (const instanceNode of createInstanceNodes(batchNode)) {
 *  batchNode.addChild(instanceNode);
 * }
 *
 * batchNode.setMesh(null).setExtension('EXTMeshGPUInstancing', null);
 * ```
 */
declare function createInstanceNodes(batchNode: Node): Node[];
//#endregion
//#region src/unlit.d.ts
/**
 * @category Transforms
 */
declare function unlit(): Transform;
//#endregion
//#region src/unpartition.d.ts
interface UnpartitionOptions {}
/**
 * Removes partitions from the binary payload of a glTF file, so that the asset
 * contains at most one (1) `.bin` {@link Buffer}. This process reverses the
 * changes from a {@link partition} transform.
 *
 * Example:
 *
 * ```ts
 * document.getRoot().listBuffers(); // → [Buffer, Buffer, ...]
 *
 * await document.transform(unpartition());
 *
 * document.getRoot().listBuffers(); // → [Buffer]
 * ```
 *
 * @category Transforms
 */
declare function unpartition(_options?: UnpartitionOptions): Transform;
//#endregion
//#region src/unweld.d.ts
/** Options for the {@link unweld} function. */
interface UnweldOptions {}
/**
 * De-index {@link Primitive}s, disconnecting any shared vertices. This operation will generally
 * increase the number of vertices in a mesh, but may be helpful for some geometry operations or
 * for creating hard edges.
 *
 * No options are currently implemented for this function.
 *
 * @category Transforms
 */
declare function unweld(_options?: UnweldOptions): Transform;
//#endregion
//#region src/unwrap.d.ts
/** Options for the {@link unwrap} transform. */
interface UnwrapOptions {
  /** watlas instance. */
  watlas: unknown;
  /**
   * Target texture coordinate index (0, 1, 2, ...) for generated unwrapping.
   * Default: 0.
   */
  texcoord?: number;
  /**
   * Whether to overwrite existing attributes at the target texCoord index, if
   * any. Default: false.
   */
  overwrite?: boolean;
  /**
   * Methods of grouping texcoords with the {@link unwrap} function.
   * Default: 'mesh'.
   */
  groupBy?: 'primitive' | 'mesh' | 'scene';
}
/** Options for the {@link unwrapPrimitives} function. */
interface UnwrapPrimitivesOptions {
  /** watlas instance. */
  watlas: unknown;
  /**
   * Target texture coordinate index (0, 1, 2, ...) for generated unwrapping.
   * Default: 0.
   */
  texcoord?: number;
  /**
   * Whether to overwrite existing attributes at the target texCoord index, if
   * any. Default: false.
   */
  overwrite?: boolean;
  /**
   * Per-primitive texel density weights. Texel space in the atlas is allocated
   * proportionally with geometry dimensions in local space. If specified,
   * weights scale the allocation. Default: [1, 1, 1, ...].
   */
  weights?: number[];
}
declare const UNWRAP_DEFAULTS: Required<Omit<UnwrapOptions, 'watlas'>>;
/**
 * Generate new texture coordinates (“UV mappings”) for {@link Primitive Primitives}.
 * Useful for adding texture coordinates in scenes without existing UVs, or for
 * creating a second set of texture coordinates for baked textures such as ambient
 * occlusion maps and lightmaps. Operation may increase vertex count to
 * accommodate UV seams.
 *
 * UV layouts may be grouped, reducing the number of textures required. Available
 * groupings:
 *
 * - `"primitive"`: Each primitive is given it's own texcoord atlas.
 * - `"mesh"`: All primitives in a mesh share a texcoord atlas. (default)
 * - `"scene"`: All primitives in the scene share a texcoord atlas.
 *
 * Example:
 *
 * ```ts
 * import * as watlas from 'watlas';
 * import { unwrap } from '@gltf-transform/functions';
 *
 * // Generate a TEXCOORD_1 attribute for all primitives.
 * await document.transform(
 *   unwrap({ watlas, texcoord: 1, overwrite: true, groupBy: 'scene' })
 * );
 * ```
 *
 * For more control and customization, see {@link unwrapPrimitives}.
 *
 * @experimental
 * @category Transforms
 */
declare function unwrap(_options: UnwrapOptions): Transform;
/**
 * Generate new texture coordinates (“UV mappings”) for {@link Primitive Primitives}.
 * Useful for adding texture coordinates in scenes without existing UVs, or for
 * creating a second set of texture coordinates for baked textures such as ambient
 * occlusion maps and lightmaps. Operation may increase vertex count to
 * accommodate UV seams.
 *
 * UV layouts may be grouped, reducing the number of textures required. Available
 * groupings:
 *
 * - `"primitive"`: Each primitive is given it's own texcoord atlas.
 * - `"mesh"`: All primitives in a mesh share a texcoord atlas. (default)
 * - `"scene"`: All primitives in the scene share a texcoord atlas.
 *
 * watlas must be initialized before calling this function.
 *
 * Example:
 *
 * ```ts
 * import * as watlas from 'watlas';
 * import { unwrapPrimitives } from '@gltf-transform/functions';
 *
 * // Initialize watlas.
 * await watlas.Initialize();
 *
 * // Generate a TEXCOORD_1 attribute for the specified primitives.
 * unwrapPrimitives(mesh.listPrimitives(), {
 *   watlas,
 *   texcoord: 1,
 *   overwrite: true
 * });
 * ```
 *
 * To create texture coordinates for an entire Document, see {@link unwrap}.
 *
 * @experimental
 */
declare function unwrapPrimitives(primitives: Primitive[], options: UnwrapPrimitivesOptions): void;
//#endregion
//#region src/utils.d.ts
/**
 * Prepares a function used in an {@link Document#transform} pipeline. Use of this wrapper is
 * optional, and plain functions may be used in transform pipelines just as well. The wrapper is
 * used internally so earlier pipeline stages can detect and optimize based on later stages.
 * @hidden
 */
declare function createTransform(name: string, fn: Transform): Transform;
/** @hidden */
declare function isTransformPending(context: TransformContext | undefined, initial: string, pending: string): boolean;
/**
 * Performs a shallow merge on an 'options' object and a 'defaults' object.
 * Equivalent to `{...defaults, ...options}` _except_ that `undefined` values
 * in the 'options' object are ignored.
 *
 * @hidden
 */
declare function assignDefaults<Defaults, Options>(defaults: Defaults, options: Options): Defaults & Options;
/** @hidden */
declare function getGLPrimitiveCount(prim: Primitive): number;
/**
 * Scales `size` NxN dimensions to fit within `limit` NxN dimensions, without
 * changing aspect ratio. If `size` <= `limit` in all dimensions, returns `size`.
 * @hidden
 */
declare function fitWithin(size: vec2, limit: vec2): vec2;
type ResizePreset = 'nearest-pot' | 'ceil-pot' | 'floor-pot';
/**
 * Scales `size` NxN dimensions to the specified power of two.
 * @hidden
 */
declare function fitPowerOfTwo(size: vec2, method: ResizePreset): vec2;
//#endregion
//#region src/vertex-color-space.d.ts
/** Options for the {@link vertexColorSpace} function. */
interface ColorSpaceOptions {
  /** Input color space of vertex colors, to be converted to "srgb-linear". Required. */
  inputColorSpace: 'srgb' | 'srgb-linear';
}
/**
 * Vertex color color space correction. The glTF format requires vertex colors to be stored
 * in Linear Rec. 709 D65 color space, and this function provides a way to correct vertex
 * colors that are (incorrectly) stored in sRGB.
 *
 * Example:
 *
 * ```typescript
 * import { vertexColorSpace } from '@gltf-transform/functions';
 *
 * await document.transform(
 *   vertexColorSpace({ inputColorSpace: 'srgb' })
 * );
 * ```
 *
 * @category Transforms
 */
declare function vertexColorSpace(options: ColorSpaceOptions): Transform;
//#endregion
//#region src/weld.d.ts
/** Options for the {@link weld} function. */
interface WeldOptions {
  /** Whether to overwrite existing indices. */
  overwrite?: boolean;
}
declare const WELD_DEFAULTS: Required<WeldOptions>;
/**
 * Welds {@link Primitive Primitives}, merging bitwise identical vertices. When
 * merged and indexed, data is shared more efficiently between vertices. File size
 * can be reduced, and the GPU uses the vertex cache more efficiently.
 *
 * Example:
 *
 * ```javascript
 * import { weld, getSceneVertexCount, VertexCountMethod } from '@gltf-transform/functions';
 *
 * const scene = document.getDefaultScene();
 * const srcVertexCount = getSceneVertexCount(scene, VertexCountMethod.UPLOAD);
 * await document.transform(weld());
 * const dstVertexCount = getSceneVertexCount(scene, VertexCountMethod.UPLOAD);
 * ```
 *
 * @category Transforms
 */
declare function weld(_options?: WeldOptions): Transform;
/**
 * Welds a {@link Primitive}, merging bitwise identical vertices. When merged
 * and indexed, data is shared more efficiently between vertices. File size can
 * be reduced, and the GPU uses the vertex cache more efficiently.
 *
 * Example:
 *
 * ```javascript
 * import { weldPrimitive, getMeshVertexCount, VertexCountMethod } from '@gltf-transform/functions';
 *
 * const mesh = document.getRoot().listMeshes()
 * 	.find((mesh) => mesh.getName() === 'Gizmo');
 *
 * const srcVertexCount = getMeshVertexCount(mesh, VertexCountMethod.UPLOAD);
 *
 * for (const prim of mesh.listPrimitives()) {
 *   weldPrimitive(prim);
 * }
 *
 * const dstVertexCount = getMeshVertexCount(mesh, VertexCountMethod.UPLOAD);
 * ```
 */
declare function weldPrimitive(prim: Primitive, _options?: WeldOptions): void;
//#endregion
export { CenterOptions, ColorSpaceOptions, CompressTextureOptions, DRACO_DEFAULTS, DedupOptions, type DequantizeOptions, DracoOptions, FLATTEN_DEFAULTS, FlattenOptions, INSTANCE_DEFAULTS, InspectAnimationReport, InspectMaterialReport, InspectMeshReport, InspectPropertyReport, InspectReport, InspectSceneReport, InspectTextureReport, InstanceOptions, JOIN_DEFAULTS, JoinOptions, MESHOPT_DEFAULTS, MeshoptOptions, MetalRoughOptions, NormalsOptions, PALETTE_DEFAULTS, PRUNE_DEFAULTS, PaletteOptions, PartitionOptions, PruneOptions, QUANTIZE_DEFAULTS, QuantizeOptions, ReorderOptions, ResampleOptions, SIMPLIFY_DEFAULTS, SequenceOptions, SimplifyOptions, SparseOptions, TEXTURE_COMPRESS_DEFAULTS, TEXTURE_COMPRESS_SUPPORTED_FORMATS, TangentsOptions, TextureCompressOptions, TextureResizeFilter, UNWRAP_DEFAULTS, UninstanceOptions, UnpartitionOptions, UnweldOptions, UnwrapOptions, UnwrapPrimitivesOptions, VertexCountMethod, WELD_DEFAULTS, WeldOptions, assignDefaults, center, clearNodeParent, clearNodeTransform, cloneDocument, compactPrimitive, compressTexture, convertPrimitiveToLines, convertPrimitiveToTriangles, copyToDocument, createDefaultPropertyResolver, createInstanceNodes, createTransform, dedup, dequantize, dequantizePrimitive, draco, fitPowerOfTwo, fitWithin, flatten, getBounds, getGLPrimitiveCount, getMeshVertexCount, getNodeVertexCount, getPrimitiveVertexCount, getSceneVertexCount, getTextureChannelMask, getTextureColorSpace, inspect, instance, isTransformPending, join, joinPrimitives, listNodeScenes, listTextureChannels, listTextureInfo, listTextureInfoByMaterial, listTextureSlots, mergeDocuments, meshopt, metalRough, moveToDocument, normals, palette, partition, prune, quantize, reorder, resample, sequence, simplify, simplifyPrimitive, sortPrimitiveWeights, sparse, tangents, textureCompress, transformMesh, transformPrimitive, uninstance, unlit, unpartition, unweld, unwrap, unwrapPrimitives, vertexColorSpace, weld, weldPrimitive };