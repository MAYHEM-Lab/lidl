from .buffer import Memory
from .builder import Builder
from .basic_types import I16, CommonBasicType


class String(CommonBasicType):
    @staticmethod
    def create(builder: Builder, data: str):
        encoded = data.encode("utf-8")
        mem = builder.allocate(2 + len(encoded), 2)
        I16.from_memory(mem.get_slice(0, 2)).value = len(encoded)
        mem.get_slice(2).raw_bytes()[:] = encoded
        return String.from_memory(mem)

    def to_string(self):
        return bytes(self.string_memory().raw_bytes()).decode("utf-8")

    def string_memory(self):
        return self._mem.get_slice(2, len(self))

    def __len__(self):
        return I16.read(self._mem)

    @property
    def value(self):
        return self.to_string()

    is_ref = True
