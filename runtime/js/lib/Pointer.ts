import {LidlObject, Type, Layout, Int16} from "./BasicTypes"
import assert from "assert";
import {MessageBuilder} from "./MessageBuilder";
import {CreateObject} from "./Object";

export class Pointer extends LidlObject {
    private pointee_t: Type;

    constructor(pointee_type: Type, data: Uint8Array) {
        super(data);
        this.pointee_t = pointee_type;
    }

    set_offset(offset: number) {
        this.as_int16().value = offset;
    }

    get_offset(): number {
        return this.as_int16().value;
    }

    get_type(): Type {
        return new PointerClass(this.pointee_t);
    }

    deref(): LidlObject {
        const buf = this.pointee_buffer();
        return this.pointee_t.instantiate(buf);
    }

    init(val: LidlObject) {
        assert(val.get_type().layout().size === this.pointee_t.layout().size);
        const offset = this.buffer().byteOffset - val.buffer().byteOffset;
        this.set_offset(offset);
        // this.set_offset(val);
    }

    public get value(): any {
        return this.deref().value;
    }

    public set value(val: any) {
        this.deref().value = val;
    }

    private pointee_buffer() {
        let self = super.buffer();
        return new Uint8Array(self.buffer, self.byteOffset - this.get_offset(), this.pointee_t.layout().size);
    }

    private as_int16() {
        return new Int16(this.buffer());
    }
}

export class PointerClass implements Type {
    constructor(pointee_type: Type) {
        this.pointee_t = pointee_type;
    }

    private pointee_t: Type;

    instantiate(data: Uint8Array): LidlObject {
        return new Pointer(this.pointee_t, data);
    }

    layout(): Layout {
        return {
            size: 2,
            alignment: 2
        };
    }
}