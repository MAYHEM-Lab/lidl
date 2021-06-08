from .basic_types import I16, CommonBasicType
from .builder import Builder


class RawPointer(CommonBasicType):
    @classmethod
    def create(cls, builder):
        return builder.create_raw(cls)

    @classmethod
    def create_with_offset(cls, builder, off: int):
        res = cls.create(builder)
        res.set_offset(off)
        return res

    def point_to(self, obj):
        assert self._mem.is_same_memory(obj._mem)
        my_base = self._mem.get_base()
        obj_base = obj._mem.get_base()
        self.offset = obj_base - my_base

    def deref_mem(self):
        buf = self._mem.get_slice(self.offset)
        return buf

    @property
    def offset(self):
        return I16.from_memory(self._mem).value

    @offset.setter
    def offset(self, off):
        I16.from_memory(self._mem).value = off

    size = 2
    is_ptr = True


def Pointer(point_to):
    class TypedPointer(RawPointer):
        def deref(self):
            return point_to.from_memory(self.deref_mem())

        @property
        def value(self):
            return self.deref()

        @value.setter
        def value(self, val):
            raise NotImplementedError()

        @staticmethod
        def create_with_object(builder: Builder, obj: point_to):
            res = TypedPointer.create(builder)
            res.point_to(obj)
            return res

    TypedPointer.__name__ = f"Pointer({point_to.__name__})"
    TypedPointer.pointee = point_to

    return TypedPointer
