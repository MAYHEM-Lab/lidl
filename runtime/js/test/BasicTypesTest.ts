import * as lidl from "../lidl";
import { describe } from 'mocha';
import { expect } from 'chai';

describe("Creating u8 works", () => {
    it('should be equal 42', function () {
        const u8type = new lidl.Uint8Class();
        const mb = new lidl.MessageBuilder;
        const u8 = lidl.CreateObject(u8type, mb, 42);
        expect(u8.value).to.eq(42);
    });
});