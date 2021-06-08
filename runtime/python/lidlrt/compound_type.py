from .basic_types import StoredObject, Memory
from .detail import self_or_val


def make_struct_ctor(type, description):
    def struct_ctor(instance, mem=None, from_memory_=None, **kwargs):
        if mem is not None:
            super(type, instance).__init__(mem)
        else:
            super(type, instance).__init__(Memory(bytearray(type.size)))
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

    return struct_ctor


def make_union_ctor(type, description):
    def union_ctor(instance, mem=None, from_memory_=None, **kwargs):
        if mem is not None:
            super(type, instance).__init__(mem)
        else:
            super(type, instance).__init__(Memory(bytearray(type.size)))

        if from_memory_ is not None:
            assert len(kwargs) == 0
            return

        assert len(kwargs) == 1

        for key, val in kwargs.items():
            memtype = description.members[key]["type"]
            alternative = getattr(type.alternatives, key)
            setattr(instance, "alternative", alternative)
            if (hasattr(memtype, "is_ptr")):
                memtype.from_memory(instance._mem.get_slice(description.members[key]["offset"])).point_to(val)
                continue
            setattr(instance, key, val)

    return union_ctor


def make_ctor(type, description, is_struct: bool):
    if is_struct:
        return make_struct_ctor(type, description)
    else:
        return make_union_ctor(type, description)


def struct_repr(description):
    def foo(instance):
        return repr({name: getattr(instance, name) for name in description.members.keys()})

    return foo


def union_repr(description):
    def foo(instance):
        for name, mem in description.members.items():
            if name == "alternative":
                continue
            if instance.alternative == getattr(instance.alternatives, name):
                return repr({name: getattr(instance, name)})
        return None

    return foo


def make_repr(is_struct: bool, description):
    if is_struct:
        return struct_repr(description)
    else:
        return union_repr(description)


def make_compound(is_struct, description):
    class CompoundType(description, StoredObject):
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

    CompoundType.__name__ = description.__name__
    CompoundType.__init__ = make_ctor(CompoundType, description, is_struct)
    CompoundType.__repr__ = make_repr(is_struct, description)

    return CompoundType


def Struct(description):
    return make_compound(True, description)


def Union(description):
    return make_compound(False, description)
