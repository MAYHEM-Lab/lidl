import * as lidl from "../lidl";
import { describe } from 'mocha';
import { expect } from 'chai';

describe("Creating pointer to u8 works", () => {
    it('should be equal 42', function () {
        const u8type = new lidl.Uint8Class();
        const mb = new lidl.MessageBuilder;
        const u8 = lidl.CreateObject(u8type, mb, 42);
        const ptr = lidl.CreateObject(new lidl.PointerClass(u8type), mb, u8);
        expect(ptr.value).to.equal(42);
    });
});

describe("Creating pointer to string works", () => {
    it('should be equal hello world', function () {
        const strtype = new lidl.LidlStringClass();
        const mb = new lidl.MessageBuilder;
        const str = strtype.create(mb, "hello world");
        const ptr = lidl.CreateObject(new lidl.PointerClass(strtype), mb, str);
        expect(ptr.value).to.equal("hello world");
    });
});