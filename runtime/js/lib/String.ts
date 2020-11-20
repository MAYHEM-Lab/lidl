import { MessageBuilder } from "./MessageBuilder";
import {Layout, LidlObject, Type, TypedType} from "./Object";

export class LidlStringClass implements TypedType<LidlString> {
    instantiate(data: Uint8Array): LidlString {
        return new LidlString(data);
    }

    create(mb: MessageBuilder, data: string) {
        const sizeBuffer = mb.allocate(2 + data.length, 2);

        const sizeView = new DataView(sizeBuffer.buffer, sizeBuffer.byteOffset, sizeBuffer.byteLength);
        sizeView.setInt16(0, data.length, true);

        const dataArray = sizeBuffer.subarray(2);
        dataArray.set(new TextEncoder().encode(data));

        return new LidlString(sizeBuffer);
    }

    layout(): Layout {
        return {
            size: 2,
            alignment: 2
        };
    }
}

export class LidlString extends LidlObject {
    length(): number {
        return this.dataView().getInt16(0, true);
    }

    private dataBuffer() {
        return super.sliceBuffer(2, this.length());
    }

    get value(): string {
        return new TextDecoder("utf-8").decode(this.dataBuffer());
    }

    get_type(): Type {
        return new LidlStringClass();
    }
}