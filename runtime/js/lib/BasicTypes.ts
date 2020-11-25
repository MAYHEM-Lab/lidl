import {Layout, Type, LidlObject, TypedType} from "./Object";

export {Layout, Type, LidlObject, TypedType} from "./Object"

export class BoolClass implements TypedType<Bool> {
    instantiate(data: Uint8Array): Bool {
        return new Bool(data);
    }

    layout(): Layout {
        return {
            size: 1,
            alignment: 1
        };
    }
}

export class Bool extends LidlObject {
    get_type(): Type {
        return new BoolClass();
    }

    get value(): boolean {
        return super.dataView().getUint8(0) != 0;
    }

    set value(val: boolean) {
        super.dataView().setUint8(0, val ? 1 : 0);
    }
}

export class FloatClass implements TypedType<Float> {
    instantiate(buffer: Uint8Array): Float {
        return new Float(buffer);
    }

    layout(): Layout {
        return {
            size: 4,
            alignment: 4
        };
    }
}

export class Float extends LidlObject {
    get value(): number {
        return super.dataView().getFloat32(0, true);
    }

    set value(val: number) {
        super.dataView().setFloat32(0, val, true);
    }

    get_type(): Type {
        return new FloatClass();
    }
}

export class DoubleClass implements Type {
    instantiate(data: Uint8Array): LidlObject {
        return new Double(data);
    }

    layout(): Layout {
        return {
            size: 8,
            alignment: 8
        };
    }
}

export class Double extends LidlObject {
    get value(): number {
        return super.dataView().getFloat64(0, true);
    }

    set value(val: number) {
        super.dataView().setFloat64(0, val, true);
    }

    get_type(): Type {
        return new DoubleClass();
    }
}

export class Uint8Class implements Type {
    instantiate(data: Uint8Array): LidlObject {
        return new Uint8(data);
    }

    layout(): Layout {
        return {
            size: 1,
            alignment: 1
        };
    }
}

export class Uint8 extends LidlObject {
    get value(): number {
        return super.dataView().getUint8(0);
    }

    set value(val: number) {
        super.dataView().setUint8(0, val);
    }

    get_type(): Type {
        return new Uint8Class();
    }
}

export class Int8Class implements Type {
    instantiate(data: Uint8Array): LidlObject {
        return new Int8(data);
    }

    layout(): Layout {
        return {
            size: 1,
            alignment: 1
        };
    }
}

export class Int8 extends LidlObject {
    get value(): number {
        return super.dataView().getInt8(0);
    }

    set value(val: number) {
        super.dataView().setInt8(0, val);
    }

    get_type(): Type {
        return new Int8Class();
    }
}

export class Uint16Class implements Type {
    instantiate(data: Uint8Array): LidlObject {
        return new Uint16(data);
    }

    layout(): Layout {
        return {
            size: 2,
            alignment: 2
        };
    }
}

export class Uint16 extends LidlObject {
    get value(): number {
        return super.dataView().getUint16(0, true);
    }

    set value(val: number) {
        super.dataView().setUint16(0, val, true);
    }

    get_type(): Type {
        return new Uint16Class();
    }
}

export class Int16Class implements Type {
    instantiate(data: Uint8Array): LidlObject {
        return new Int16(data);
    }

    layout(): Layout {
        return {
            size: 2,
            alignment: 2
        };
    }
}

export class Int16 extends LidlObject {
    get value(): number {
        return super.dataView().getInt16(0, true);
    }

    set value(val: number) {
        super.dataView().setInt16(0, val, true);
    }

    get_type(): Type {
        return new Int16Class();
    }
}

export class Uint32Class implements Type {
    instantiate(data: Uint8Array): LidlObject {
        return new Uint32(data);
    }

    layout(): Layout {
        return {
            size: 4,
            alignment: 4
        };
    }
}

export class Uint32 extends LidlObject {
    get value(): number {
        return super.dataView().getUint32(0, true);
    }

    set value(val: number) {
        super.dataView().setUint32(0, val, true);
    }

    get_type(): Type {
        return new Uint32Class();
    }
}

export class Int32Class implements Type {
    instantiate(data: Uint8Array): LidlObject {
        return new Int32(data);
    }

    layout(): Layout {
        return {
            size: 4,
            alignment: 4
        };
    }
}

export class Int32 extends LidlObject {
    get value(): number {
        return super.dataView().getInt32(0, true);
    }

    set value(val: number) {
        super.dataView().setInt32(0, val, true);
    }

    get_type(): Type {
        return new Int32Class();
    }
}

export class Uint64Class implements Type {
    instantiate(data: Uint8Array): LidlObject {
        return new Uint64(data);
    }

    layout(): Layout {
        return {
            size: 8,
            alignment: 8
        };
    }
}

export class Uint64 extends LidlObject {
    get value(): bigint {
        return super.dataView().getBigUint64(0, true);
    }

    set value(val: bigint) {
        super.dataView().setBigUint64(0, val, true);
    }

    get_type(): Type {
        return new Uint64Class();
    }
}

export class Int64Class implements Type {
    instantiate(data: Uint8Array): LidlObject {
        return new Int64(data);
    }

    layout(): Layout {
        return {
            size: 8,
            alignment: 8
        };
    }
}

export class Int64 extends LidlObject {
    get value(): bigint {
        return super.dataView().getBigInt64(0, true);
    }

    set value(val: bigint) {
        super.dataView().setBigInt64(0, val, true);
    }

    get_type(): Type {
        return new Int64Class();
    }
}
