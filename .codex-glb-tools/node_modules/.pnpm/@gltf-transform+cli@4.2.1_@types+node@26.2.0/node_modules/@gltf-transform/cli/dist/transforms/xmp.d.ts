import type { Transform } from '@gltf-transform/core';
export interface XMPOptions {
    packet?: string;
    reset?: boolean;
}
export declare const XMP_DEFAULTS: {
    packet: string;
    reset: boolean;
};
export declare const xmp: (_options?: XMPOptions) => Transform;
