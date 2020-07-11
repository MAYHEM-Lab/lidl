export class MessageBuilder {
    private data: Uint8Array;
    private ptr: number = 0;

    constructor() {
        this.data = new Uint8Array(128);
    }

    allocate(size: number, align: number): Uint8Array {
        while (this.ptr % align != 0) {
            this.ptr++;
        }
        const res = this.data.subarray(this.ptr, this.ptr + size);
        this.ptr += size;
        return res;
    }
}