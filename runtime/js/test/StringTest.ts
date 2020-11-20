import * as lidl from "../lidl";
import { describe } from 'mocha';
import { expect } from 'chai';

describe("Create string works", () => {
    it('should be equal hello world', function () {
        const mb = new lidl.MessageBuilder;
        const str = (new lidl.LidlStringClass).create(mb, "hello world");
        expect(str.value).to.equal("hello world");
    });
}) ;