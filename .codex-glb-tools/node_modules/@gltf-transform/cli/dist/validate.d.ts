import type { ILogger } from '@gltf-transform/core';
import { TableFormat } from './util.js';
export interface ValidateOptions {
    limit: number;
    ignore: string[];
    format: TableFormat;
}
export declare function validate(input: string, options: ValidateOptions, logger: ILogger): Promise<void>;
