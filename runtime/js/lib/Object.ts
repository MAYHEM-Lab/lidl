const inspect = Symbol.for('nodejs.util.inspect.custom');

import {MessageBuilder} from "./MessageBuilder";

export function CreateObject<T extends Type>(type: T, mb: MessageBuilder, init: any): LidlObject {
    const buf = mb.allocate(type.layout().size, type.layout().alignment);
    const res = type.instantiate(buf);
    res.init(init);
    return res;
}

export type Layout = { size: number; alignment: number; };

export interface Type {
    layout(): Layout;

    instantiate(data: Uint8Array): LidlObject;
}

export interface TypedType<T extends LidlObject> extends Type {
    instantiate(data: Uint8Array): T;
}

export abstract class LidlObject {
    private _buffer: Uint8Array;

    abstract get value(): any;
    abstract set value(val: any);

    init(val: any) {
        this.value = val;
    }

    constructor(data: Uint8Array) {
        this._buffer = data;
    }

    protected dataView(): DataView {
        return new DataView(this._buffer.buffer, this._buffer.byteOffset, this._buffer.byteLength);
    }

    protected sliceBuffer(offset: number, length: number) {
        return new Uint8Array(this.buffer().buffer, this.buffer().byteOffset + offset, length);
    }

    protected buffer(): Uint8Array {
        return this._buffer;
    }

    abstract get_type(): Type;

    layout(): Layout {
        return this.get_type().layout();
    }

    public [inspect](depth: number, opts: any) {
        return this.value;
    }
}
