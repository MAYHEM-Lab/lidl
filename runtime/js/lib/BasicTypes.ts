import {MessageBuilder} from "./MessageBuilder";
import * as util from "util";
const inspect = Symbol.for('nodejs.util.inspect.custom');

const partial = (fn: any, firstArg: any) => {  return (...lastArgs: any) => {    return fn(firstArg, ...lastArgs);  }}

function CreateBasic(type: any, mb: MessageBuilder, data: number) {
    const buf = mb.allocate(type._metadata.layout.size, type._metadata.layout.alignment);
    const res = new type(buf);
    res.data = data;
    return res;
}

class BasicType {
    private _data : Uint8Array;

    get data(): any { throw "Not implemented"; }
    set data(val: any) { throw "Not implemented"; }

    constructor(data: Uint8Array) {
        this._data = data;
    }

    protected dataView(): DataView {
        return new DataView(this._data.buffer, this._data.byteOffset, this._data.byteLength);
    }

    public [inspect](depth: number, opts: any) {
        return this.data;
    }
}

export class Float extends BasicType {
    get data(): number {
        return super.dataView().getFloat32(0, true);
    }

    set data(val: number) {
        super.dataView().setFloat32(0, val, true);
    }

    static _metadata = {
        layout: {
            size: 4,
            alignment: 4
        }
    }

    static create = partial(CreateBasic, Float);
}

export class Double extends BasicType {
    get data(): number {
        return super.dataView().getFloat64(0, true);
    }

    set data(val: number) {
        super.dataView().setFloat64(0, val, true);
    }

    static _metadata = {
        layout: {
            size: 8,
            alignment: 8
        }
    }

    static create = partial(CreateBasic, Double);
}

export class Uint8 {
    constructor(public _data: Uint8Array) {
    }

    get data(): number {
        return new DataView(this._data.buffer).getUint8(0);
    }

    set data(val: number) {
        new DataView(this._data.buffer).setUint8(0, val);
    }

    static _metadata = {
        layout: {
            size: 1,
            alignment: 1
        }
    }

}

export class Int8 {
    constructor(public _data: Uint8Array) {
    }

    get data(): number {
        return new DataView(this._data.buffer).getInt8(0);
    }

    set data(val: number) {
        new DataView(this._data.buffer).setInt8(0, val);
    }

    static _metadata = {
        layout: {
            size: 1,
            alignment: 1
        }
    }
}

export class Uint16 {
    constructor(public _data: Uint8Array) {
    }

    get data(): number {
        return new DataView(this._data.buffer).getUint16(0, true);
    }

    set data(val: number) {
        new DataView(this._data.buffer).setUint16(0, val, true);
    }

    static _metadata = {
        layout: {
            size: 2,
            alignment: 2
        }
    }

}

export class Int16 {
    constructor(public _data: Uint8Array) {
    }

    get data(): number {
        return new DataView(this._data.buffer).getInt16(0, true);
    }

    set data(val: number) {
        new DataView(this._data.buffer).setInt16(0, val, true);
    }

    static _metadata = {
        layout: {
            size: 2,
            alignment: 2
        }
    }
}

export class Uint32 {
    constructor(public _data: Uint8Array) {
    }

    get data(): number {
        return new DataView(this._data.buffer).getUint32(0, true);
    }

    set data(val: number) {
        new DataView(this._data.buffer).setUint32(0, val, true);
    }

    static _metadata = {
        layout: {
            size: 4,
            alignment: 4
        }
    }
}

export class Int32 {
    constructor(public _data: Uint8Array) {
    }

    get data(): number {
        return new DataView(this._data.buffer).getInt32(0, true);
    }

    set data(val: number) {
        new DataView(this._data.buffer).setInt32(0, val, true);
    }

    static _metadata = {
        layout: {
            size: 4,
            alignment: 4
        }
    }
}

export class Uint64 {
    constructor(public _data: Uint8Array) {
    }

    get data(): bigint {
        return new DataView(this._data.buffer).getBigUint64(0, true);
    }

    set data(val: bigint) {
        new DataView(this._data.buffer).setBigUint64(0, val, true);
    }

    static _metadata = {
        layout: {
            size: 8,
            alignment: 8
        }
    }
}

export class Int64 {
    constructor(public _data: Uint8Array) {
    }

    get data(): bigint {
        return new DataView(this._data.buffer).getBigInt64(0, true);
    }

    set data(val: bigint) {
        new DataView(this._data.buffer).setBigInt64(0, val, true);
    }

    static _metadata = {
        layout: {
            size: 8,
            alignment: 8
        }
    }
}
