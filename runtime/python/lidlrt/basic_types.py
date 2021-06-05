from .buffer import Memory
import struct
import abc


class StoredObject:
    _mem: Memory

    def __init__(self, mem, from_memory_ = None):
        self._mem = mem

    @classmethod
    def from_memory(cls, mem: Memory):
        return cls(mem=mem, from_memory_ = True)


class CommonBasicType(StoredObject):
    is_basic = True

    def __repr__(self):
        return repr(f"{self.__class__.__name__}({repr(self.value)})")

    @property
    @abc.abstractmethod
    def value(self):
        pass

    @value.setter
    @abc.abstractmethod
    def value(self, val):
        pass

    def assign(self, val):
        self.value = val


def anon_init(instance, val=None, mem=None, from_memory_ = None):
    if mem is not None and from_memory_ is not None:
        assert val is None
        super(type(instance), instance).__init__(mem=mem)
        return

    super(type(instance), instance).__init__(Memory(bytearray(instance.size)))
    if val is not None:
        instance.assign(val)


def make_basic(t):
    class BoundT(t):
        @property
        def value(self):
            return t.read(self._mem)

        @value.setter
        def value(self, val):
            t.write(self._mem, val)

    BoundT.__init__ = anon_init
    BoundT.__name__ = t.__name__

    return BoundT


def make_int(sz: int, signed: bool):
    class Ret(CommonBasicType):
        @staticmethod
        def read(mem: Memory):
            buf = mem.get_slice(0, sz)
            return int.from_bytes(buf.raw_bytes(), byteorder='little', signed=signed)

        @staticmethod
        def write(mem: Memory, val: int):
            buf = mem.get_slice(0, sz)
            buf.raw_bytes()[:] = val.to_bytes(sz, byteorder='little', signed=signed)

        size = sz

    Ret.__name__ = f"{'I' if signed else 'U'}{sz * 8}"

    return make_basic(Ret)


def make_float(sz: int):
    @make_basic
    class FloatType(CommonBasicType):
        @staticmethod
        def read(mem: Memory):
            buf = mem.get_slice(0, sz)
            [val] = struct.unpack("f", buf.raw_bytes())
            return val

        @staticmethod
        def write(mem: Memory, val: float):
            buf = mem.get_slice(0, sz)
            buf.raw_bytes()[:] = struct.pack("f" if sz == 4 else "d", val)

        size = sz

    return FloatType


I8 = make_int(1, True)
I16 = make_int(2, True)
I32 = make_int(4, True)
I64 = make_int(8, True)

U8 = make_int(1, True)
U16 = make_int(2, True)
U32 = make_int(4, True)
U64 = make_int(8, True)

F32 = make_float(4)
F64 = make_float(8)


@make_basic
class Bool(CommonBasicType):
    @staticmethod
    def read(mem: Memory):
        buf = mem.get_slice(0, 1)
        return bool.from_bytes(buf.raw_bytes())

    size = 1
