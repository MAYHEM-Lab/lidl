import {FloatClass, Layout, Type, TypedType, Uint16Class} from "./lib/BasicTypes";
import {Pointer, PtrClass} from "./lib/Pointer";
import {LidlObject} from "./lib/Object";
import {LidlStringClass} from "./lib/String";

export * from "./lib/MessageBuilder";
export * from "./lib/String";
export * from "./lib/BasicTypes";
export * from "./lib/Pointer";
export * from "./lib/Structure";

class Vec3Class implements TypedType<Vec3> {
    instantiate(data: Uint8Array): Vec3 {
        return new Vec3(data);
    }

    layout(): Layout {
        return {
            size: 12,
            alignment: 4
        };
    }

    members() {
        return {
            x: {
                type: new FloatClass(),
                offset: 0
            },
            y: {
                type: new FloatClass(),
                offset: 4
            },
            z: {
                type: new FloatClass(),
                offset: 8
            }
        };
    }
}

class Vec3 extends LidlObject {
    get_type(): Vec3Class {
        return new Vec3Class();
    }

    get value(): any {
        return {
            x: this.x,
            y: this.y,
            z: this.z
        };
    }

    get x(): number {
        const mem = this.get_type().members().x;
        return mem.type.instantiate(this.sliceBuffer(mem.offset, mem.type.layout().size)).value;
    }

    set x(val: number) {
        const mem = this.get_type().members().x;
        mem.type.instantiate(this.sliceBuffer(mem.offset, mem.type.layout().size)).value = val;
    }

    get y(): number {
        const mem = this.get_type().members().y;
        return mem.type.instantiate(this.sliceBuffer(mem.offset, mem.type.layout().size)).value;
    }

    set y(val: number) {
        const mem = this.get_type().members().y;
        mem.type.instantiate(this.sliceBuffer(mem.offset, mem.type.layout().size)).value = val;
    }

    get z(): number {
        const mem = this.get_type().members().z;
        return mem.type.instantiate(this.sliceBuffer(mem.offset, mem.type.layout().size)).value;
    }

    set z(val: number) {
        const mem = this.get_type().members().z;
        mem.type.instantiate(this.sliceBuffer(mem.offset, mem.type.layout().size)).value = val;
    }
}

let buf = new Uint8Array([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]);

let vec = new Vec3Class().instantiate(new Uint8Array(buf.buffer, 0, 12));

vec.x = 42;
vec.y = 50;
vec.z = 512;
console.log(vec);
console.log(buf);

let strbuf = new Uint8Array([4, 0, 65, 66, 67, 68]);
let str = new LidlStringClass().instantiate(strbuf);
console.log(str);

//
// let ptr_t = new PtrClass(new Uint16Class());
//
// const u16 = new Uint16Class().instantiate(buf);
//
// const ptr_slice = new Uint8Array(buf.buffer, 4, 2);
// const ptr_slice2 = new Uint8Array(buf.buffer, 6, 2);
//
// const ptr = ptr_t.instantiate(ptr_slice);
// const ptr2 = ptr_t.instantiate(ptr_slice2);
//
// console.log(u16.value);
// console.log(ptr.value);
// console.log(ptr2.value);