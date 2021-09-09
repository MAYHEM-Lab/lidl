use crate::MessageBuilder;

#[repr(C)]
pub struct Pointer<T> {
    offset: i16,
    _: std::marker::PhantomData<T>
}

impl<T> Pointer<T> {
    fn deref<'a>(&'a self) -> &'a T {
        unsafe {
            let ptr_to_self = &self as *const Self;
            let ptr_to_bytes = ptr_to_self as *const u8;
            let ptr_to_pointee = ptr_to_bytes.sub(self.offset);
        }
    }
}