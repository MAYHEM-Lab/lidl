import { LidlObject } from "./Object";

class Layout {
    size: number = 0;
    alignment: number = 0;
}

class BasicVector {
    view: Uint8Array;
    elemLayout: Layout;

    constructor(view: Uint8Array, layout: Layout) {
        this.view = view;
        this.elemLayout = layout;
    }

    length(): number {
        return new DataView(this.view.buffer).getInt16(0, true);
    }

    dataBuffer() {
        let offset = 2;
        while (offset % this.elemLayout.alignment != 0) {
            offset += 1;
        }
        return new Uint8Array(this.view.subarray(offset, offset + this.length() * this.elemLayout.size));
    }

    dataPerElement() {
        const buffer = this.dataBuffer();
        const res = [];
        const len = this.length()
        for (let i = 0; i < len; ++i) {
            res.push(buffer.subarray(i * this.elemLayout.size, (i + 1) * this.elemLayout.size));
        }
        return res;
    }
}