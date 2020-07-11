import { MessageBuilder } from "./MessageBuilder";

export class LidlString {
    private _data: Uint8Array;

    constructor(view: Uint8Array) {
        this._data = view;
    }

    private dataView() {
        return new DataView(this._data.buffer, this._data.byteOffset, this._data.byteLength);
    }

    length(): number {
        return this.dataView().getInt16(0, true);
    }

    private dataBuffer() {
        return new Uint8Array(this._data.subarray(2, 2 + this.length()));
    }

    get data(): string {
        return new TextDecoder("utf-8").decode(this.dataBuffer());
    }

    static create = (mb: MessageBuilder, data: string) : LidlString => {
        const sizeBuffer = mb.allocate(2 + data.length, 2);

        const sizeView = new DataView(sizeBuffer.buffer, sizeBuffer.byteOffset, sizeBuffer.byteLength);
        sizeView.setInt16(0, data.length, true);

        const dataArray = sizeBuffer.subarray(2);
        dataArray.set(new TextEncoder().encode(data));

        return new LidlString(sizeBuffer);
    }

    static _metadata = {
        layout: {
            size: 2,
            align: 2
        },
        reference: true
    }
}