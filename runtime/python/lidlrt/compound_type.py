from .basic_types import StoredObject, Memory
from .detail import self_or_val


def make_compound(description):
    class CompoundType(description, StoredObject):
        def __repr__(self):
            return repr({name: getattr(self, name) for name in description.members.keys()})

        def assign(self, val):
            if hasattr(self, "is_ref"):
                raise NotImplementedError("cannot assign to reference types!")

            if isinstance(val, dict):
                for name, mem in description.members.items():
                    setattr(self, name, val[name])
                return

            for name, mem in description.members.items():
                setattr(self, name, getattr(val, name))

    for name, mem in description.members.items():
        def getter(instance, off=mem["offset"], typ=mem["type"]):
            buf = instance._mem.get_slice(off, typ.size)
            return self_or_val(typ.from_memory(buf))

        def setter(instance, val, off=mem["offset"], typ=mem["type"]):
            buf = instance._mem.get_slice(off, typ.size)
            typ.from_memory(buf).assign(val)

        setattr(CompoundType, name, property(getter, setter))

    def ctor(instance, mem=None, from_memory_=None, **kwargs):
        if mem is not None:
            super(CompoundType, instance).__init__(mem)
        else:
            super(CompoundType, instance).__init__(Memory(bytearray(CompoundType.size)))
        if from_memory_ is not None:
            assert len(kwargs) == 0
            return
        assert len(kwargs) == len(description.members)
        for key, val in kwargs.items():
            memtype = description.members[key]["type"]
            if (hasattr(memtype, "is_ptr")):
                memtype.from_memory(instance._mem.get_slice(description.members[key]["offset"])).point_to(val)
                continue
            setattr(instance, key, val)

    CompoundType.__name__ = description.__name__
    CompoundType.__init__ = ctor

    return CompoundType
