import {Layout, LidlObject, Type} from "./Object";

export class VectorClass implements Type {
    constructor(public type: Type) {
    }

    instantiate(data: Uint8Array): LidlObject {
        return new Vector(this, data);
    }

    layout(): Layout {
        return {
            size: 2,
            alignment: this.type.layout().alignment
        };
    }
}

export class Vector extends LidlObject {
    private readonly _type : VectorClass;

    constructor(type: VectorClass, buffer: Uint8Array) {
        super(buffer);
        this._type = type;
    }

    length(): number {
        return super.dataView().getInt16(0, true);
    }

    dataBuffer() {
        let offset = 2;
        while (offset % this._type.layout().alignment != 0) {
            offset += 1;
        }
        return this.sliceBuffer(offset, this.length() * this._type.layout().size);
    }

    get_type(): Type {
        return this._type;
    }

    get value(): any {
        const arr = [];
        for (let i = 0; i < this.length(); ++i) {
            arr.push(this.at(i).value);
        }
        return arr;
    }

    at(idx: number): any {
        return this._type.type.instantiate(this.buffer_for(idx));
    }

    private buffer_for(idx: number): Uint8Array {
        const baseBuffer = this.dataBuffer();
        return baseBuffer.subarray(idx * this._type.type.layout().size, (idx + 1) * this._type.type.layout().size);
    }
}