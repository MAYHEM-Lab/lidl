const inspect = Symbol.for('nodejs.util.inspect.custom');

export class StructBase {
    private _type: any;
    constructor(type: any) {
        this._type = type;
    }

    [inspect](depth: number, opts: any) {
        const res: any = {};
        const mems = this._type._metadata.members;
        for (let key in mems) {
            res[key] = (this as any)[key];
        }
        return res;
    }
}