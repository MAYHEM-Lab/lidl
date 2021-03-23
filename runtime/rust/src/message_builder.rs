pub struct MessageBuilder<'a> {
    data: &'a mut [u8],
    ptr: u32,
}

impl MessageBuilder<'_> {
    pub fn new<'a>(buf: &'a mut [u8]) -> MessageBuilder {
        MessageBuilder { data: buf, ptr: 0 }
    }

    pub fn allocate<'a>(&'a self, size: i32, align: i32) -> &'a mut [u8] {
        unsafe {
            let const_self_ptr = self as *const MessageBuilder;
            let self_ptr = const_self_ptr as *mut MessageBuilder;
            let mut_self = &mut *self_ptr;

            while mut_self.ptr % align as u32 != 0 {
                mut_self.ptr += 1;
            }

            let res = &mut mut_self.data[mut_self.ptr as usize..mut_self.ptr as usize + size as usize];

            mut_self.ptr += size as u32;

            res
        }
    }
}