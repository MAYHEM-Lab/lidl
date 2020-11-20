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
        this.lidl_type = type;
    }

    private readonly lidl_type: LidlArrayClass;

    get_type(): Type {
        return this.lidl_type;
    }

    init(val: [LidlObject]) {
        assert(val.length == this.lidl_type.size);
        for (let i = 0; i < this.lidl_type.size; ++i) {
            this.at(i).init(val[i]);
        }
    }

    get value(): any {
        const arr = [];
        for (let i = 0; i < this.lidl_type.size; ++i) {
            arr.push(this.at(i).value);
        }
        return arr;
    }

    at(idx: number): any {
        return this.lidl_type.type.instantiate(this.buffer_for(idx));
    }

    private buffer_for(idx: number): Uint8Array {
        return super.sliceBuffer(idx * this.lidl_type.type.layout().size, this.lidl_type.type.layout().size);
    }
}