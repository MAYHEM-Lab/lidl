use crate::MessageBuilder;

#[repr(C)]
pub struct String {
    len: i16
}

impl String {
    pub fn size(&self) -> i16 {
        self.len
    }

    pub fn get<'a>(&'a self) -> &'a str {
        use core::slice;
        use core::str;

        unsafe {
            // The string contents lie right after the length.
            let ptr = (&self.len as *const i16).offset(1) as *const u8;

            let slice = slice::from_raw_parts(ptr, self.size() as usize);

            str::from_utf8(slice).unwrap()
        }
    }

    pub fn from_buffer<'a>(buf: &'a[u8]) -> &'a String {
        unsafe {
            let buf_ptr :*const u8 = &buf[0];
            let ptr = buf_ptr as *const String;
            &*ptr
        }
    }

    pub fn new<'a>(mb: &'a mut MessageBuilder<'a>, str: &str) -> &'a String {
        let buffer = mb.allocate(2 + str.bytes().len() as i32, 1);

        unsafe {
            let ptr = (&mut buffer[0] as *mut u8) as *mut i16;
            *ptr = str.as_bytes().len() as i16
        }
        buffer[core::mem::size_of::<String>() ..].copy_from_slice(str.as_bytes());
        String::from_buffer(buffer)
    }
}
