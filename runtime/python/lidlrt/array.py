from .basic_types import StoredObject, anon_init
from .pointer import Pointer
from .detail import self_or_val


def Array(base_type, arr_size: int):
    effective_base_type = base_type
    if hasattr(base_type, "is_ref") and not hasattr(base_type, "is_ptr"):
        # Each element here is actually a Ptr
        effective_base_type = Pointer(base_type)
        pass

    class ArrayType(StoredObject):
        def __len__(self):
            return arr_size

        def raw_items(self):
            elem_size = effective_base_type.size
            buffers = (self._mem.get_slice(i * elem_size, elem_size) for i in range(arr_size))
            return [effective_base_type.from_memory(buffer) for buffer in buffers]

        def __getitem__(self, idx: int):
            return self_or_val(self.raw_items()[idx])

        def __setitem__(self, key, value):
            self.raw_items()[key].assign(value)

        def __repr__(self):
            return repr(self.raw_items())

        def __iter__(self):
            for elem in self.raw_items():
                yield self_or_val(elem)

        def assign(self, vals):
            for el, val in zip(self.raw_items(), vals):
                el.assign(val)

        @staticmethod
        def create(builder, arr):
            assert len(arr) == arr_size
            res = builder.create_raw(ArrayType)
            for to, fr in zip(res.raw_items(), arr):
                to.assign(fr)
            return res

        size = effective_base_type.size * arr_size

    if effective_base_type == base_type:
        ArrayType.__init__ = anon_init

    return ArrayType
