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

    [inspect](depth: number, opts: any) {
        return this.value;
    }
}