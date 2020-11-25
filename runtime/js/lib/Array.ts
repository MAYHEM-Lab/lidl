import {Layout, LidlObject, Type} from "./Object";
import assert from "assert";

export class LidlArrayClass implements Type {
    constructor(public type: Type, public size: number) {
    }

    instantiate(data: Uint8Array): LidlObject {
        return new LidlArray(this, data);
    }

    layout(): Layout {
        return {
            size: this.type.layout().size * this.size,
            alignment: this.type.layout().alignment
        };
    }
}

export class LidlArray extends LidlObject {
    constructor(type: LidlArrayClass, buffer: Uint8Array) {
        super(buffer);
        this._type = type;
    }

    private readonly _type: LidlArrayClass;

    get_type(): Type {
        return this._type;
    }

    init(val: [LidlObject]) {
        assert(val.length == this._type.size);
        for (let i = 0; i < this._type.size; ++i) {
            this.at(i).init(val[i]);
        }
    }

    get value(): any {
        const arr = [];
        for (let i = 0; i < this._type.size; ++i) {
            arr.push(this.at(i).value);
        }
        return arr;
    }

    at(idx: number): any {
        return this._type.type.instantiate(this.buffer_for(idx));
    }

    private buffer_for(idx: number): Uint8Array {
        return super.sliceBuffer(idx * this._type.type.layout().size, this._type.type.layout().size);
    }
}