import {LidlObject, Type, TypedType} from "./Object";

const inspect = Symbol.for('nodejs.util.inspect.custom');

export type Member = {
    type: Type;
    offset: number;
};

export interface StructType extends Type {
    members(): { [key: string]: Member };
}

export abstract class StructObject extends LidlObject {
    abstract get_type(): StructType;

    get value(): any {
        const res: any = {};
        const mems = this.get_type().members();
        for (let key in mems) {
            res[key] = (this as any)[key];
        }
        return res;
    }

    init(val: any) {
        const mems = this.get_type().members();
        for (let key in mems) {
            this.member_by_name(key).init(val[key]);
        }
    }

    [inspect](depth: number, opts: any) {
        return this.value;
    }

    protected member_by_name(member: string) : LidlObject {
        const mem = this.get_type().members()[member];
        return mem.type.instantiate(this.sliceBuffer(mem.offset, mem.type.layout().size));
    }
}

export abstract class ValueStructObject extends StructObject {
    set value(val: any) {
        const mems = this.get_type().members();
        for (let key in mems) {
            (this as any)[key] = val[key];
        }
    }

    get value() : any {
        const res: any = {};
        const mems = this.get_type().members();
        for (let key in mems) {
            res[key] = (this as any)[key];
        }
        return res;
    }
}