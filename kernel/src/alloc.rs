use {
    core::{
        alloc::{GlobalAlloc, Layout},
        mem::{self, MaybeUninit},
    },
    linked_list_allocator::LockedHeap,
};

#[global_allocator]
pub static mut ALLOC: StaticAlloc<{ 8 * 1024 * 1024 }> = unsafe { StaticAlloc::new() };

pub struct StaticAlloc<const N: usize> {
    heap: [MaybeUninit<u8>; N],
    alloc: LockedHeap,
}

impl<const N: usize> StaticAlloc<N> {
    pub const unsafe fn new() -> Self {
        Self { heap: [MaybeUninit::uninit(); N], alloc: LockedHeap::empty() }
    }

    pub fn init(&'static mut self) {
        // static values cannot change your place
        self.alloc.lock().init_from_slice(&mut self.heap);
    }
}

unsafe impl<const N: usize> GlobalAlloc for StaticAlloc<N> {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        self.alloc.alloc(layout)
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        self.alloc.dealloc(ptr, layout)
    }
}

#[no_mangle]
unsafe extern "C" fn malloc(size: usize) -> *mut u8 {
    ALLOC.alloc(Layout::from_size_align(size, mem::align_of::<usize>()).unwrap())
}

#[no_mangle]
unsafe extern "C" fn free(_ptr: *mut u8) {
    // it is bump allocator - NOT MEMORY LEAKING
}
